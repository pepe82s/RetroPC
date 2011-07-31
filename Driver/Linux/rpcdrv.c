#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/usb.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>


#if 0
#include <linux/stat.h>
#include <linux/proc_fs.h>
#define RPCDRV_PROCDIR "rpc"
#define RPCDRV_PROC_T_WATER "t_water"
#define RPCDRV_PROC_V_WATER "v_water"
#define RPCDRV_PROC_V_FAN   "v_fan"
#endif

#define RPCDRV "rpc" /* Name des Moduls */
#define WRITES_IN_FLIGHT 8

/* USB IDs
 * TESTING ONLY!!!
 * Provided by LUFA
 */
#define VENDOR_ID 0x03EB
#define PRODUCT_ID 0x2040

#define USB_RPC_MINOR_BASE	16
#define STR_OUT_BUFFER_SIZE	25


/* table of devices that work with this driver */
static const struct usb_device_id rpc_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID), USB_INTERFACE_INFO(0xFF,0xFF,0xFF), }, 
	{ }					/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, rpc_table);

struct rpc_status_raw
{
	u16 v_water;
	s16 t_water;
	u16 v_fan;
	u8  p_fan;
} __attribute__((packed));

/* Structure to hold all of our device specific stuff */
struct usb_rpc {
	struct usb_device	*udev;			/* the usb device for this device */
	struct usb_interface	*interface;		/* the interface for this device */
	struct semaphore	limit_sem;		/* limiting the number of writes in progress */
	struct usb_anchor	submitted;		/* in case we need to retract our submissions */
	struct urb		*int_in_urb;		/* the urb to read data with */
	struct rpc_status_raw *rpc_status;
	unsigned char           *int_in_buffer;	/* the buffer to receive data */
	size_t			int_in_size;		/* the size of the receive buffer */
	size_t			int_in_filled;		/* number of bytes in the buffer */
	//size_t			int_in_copied;		/* already copied to user space */
	__u8			int_in_endpointAddr;	/* the address of the bulk in endpoint */
	//__u8			bulk_out_endpointAddr;	/* the address of the bulk out endpoint */
	int			errors;			/* the last request tanked */
	int			open_count;		/* count the number of openers */
	bool			ongoing_read;		/* a read is going on */
	//bool			processed_urb;		/* indicates we haven't processed the urb */
	spinlock_t		err_lock;		/* lock for errors */
	struct kref		kref;
	struct mutex		io_mutex;		/* synchronize I/O with disconnect */
#if 0
	struct proc_dir_entry *proc_dir;
	struct proc_dir_entry	*proc_v_water;
	struct proc_dir_entry		*proc_t_water;
	struct proc_dir_entry *proc_v_fan;
#endif
	struct completion	int_in_completion;	/* to wait for an ongoing read */
};
#define to_rpc_dev(d) container_of(d, struct usb_rpc, kref)

/*static struct rpc_status_raw sensor_status_test = {
	.v_water = 280,
	.t_water = 27,
	.v_fan = 800,
	.p_fan = 60,
};*/

static struct usb_driver rpc_driver;

//static dev_t rpcdrv_dev_number;
//static struct cdev *driver_object;
//struct class *rpcdrv_class;
//static atomic_t access_count = ATOMIC_INIT(-1);

static void rpc_delete(struct kref *kref)
{
	struct usb_rpc *dev = to_rpc_dev(kref);

	usb_free_urb(dev->int_in_urb);
	usb_put_dev(dev->udev);
	kfree(dev->int_in_buffer);
	kfree(dev->rpc_status);
	kfree(dev);
}

static int driver_open( struct inode *devicefile, struct file *instance )
{
	struct usb_rpc *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	subminor = iminor(devicefile);
	
	interface = usb_find_interface(&rpc_driver, subminor);
	if(!interface) {
		err("%s - error, cant find device for minor %d", __func__, subminor);
		retval = -ENODEV;
		goto exit;
	}
	
	dev = usb_get_intfdata(interface);
	if(!dev) {
		retval = -ENODEV;
		goto exit;
	}
	
	/* increment usage count for device */
	kref_get(&dev->kref);
	
	/* lock device to allow correctly handling errors in resumption */
	mutex_lock(&dev->io_mutex);
	
	if(!dev->open_count++) {
		retval = usb_autopm_get_interface(interface);
		if(retval) {
			dev->open_count--;
			mutex_unlock(&dev->io_mutex);
			kref_put(&dev->kref, rpc_delete);
			goto exit;
		}
	}
	
	/* save our object in the files private structure */
	instance->private_data = dev;
	mutex_unlock(&dev->io_mutex);
exit:
	return retval;
}

static int driver_close( struct inode *devicefile, struct file *instance )
{
	struct usb_rpc *dev;
		
	dev = (struct usb_rpc *)instance->private_data;
	if(dev == NULL)
		return -ENODEV;
	
	/* allow the device to be autosuspended */
	mutex_lock(&dev->io_mutex);
	if(!--dev->open_count && dev->interface)
		usb_autopm_put_interface(dev->interface);
	mutex_unlock(&dev->io_mutex);
	
	/* decrement the count on our device */
	kref_put(&dev->kref, rpc_delete);
	return 0;
}

static void rpc_read_int_callback(struct urb *urb)
{
	struct usb_rpc *dev;
		
	dev = urb->context;
	
	spin_lock(&dev->err_lock);
	/* sync/async unlink faults aren't errors */
	if(urb->status) {
		if(!(urb->status == -ENOENT ||
			urb->status == -ECONNRESET ||
			urb->status == -ESHUTDOWN))
			err("%s - nonzero write int status received: %d", __func__,urb->status);
		dev->errors = urb->status;
	}
	else if( urb->actual_length < sizeof(struct rpc_status_raw) ) {
		err("%s - actual length (%d) shorter than data type (%d)", __func__, urb->actual_length, sizeof(struct rpc_status_raw));
		dev->errors = -EIO;
	}
	else {
		dev->int_in_filled = urb->actual_length;
		memcpy(dev->rpc_status, dev->int_in_buffer, sizeof(struct rpc_status_raw));
	}
	dev->ongoing_read = 0;
	spin_unlock(&dev->err_lock);
	
	complete(&dev->int_in_completion);
}

static int rpc_do_read_io(struct usb_rpc *dev, size_t count)
{
	int rv;
	/*prepare a read */
	usb_fill_int_urb(dev->int_in_urb,
			dev->udev,
			usb_rcvintpipe(dev->udev,dev->int_in_endpointAddr),
			dev->int_in_buffer,
			min(dev->int_in_size, count),
			rpc_read_int_callback,
			dev,
			10 /*TODO put interval from device here */);
	/* tell everybody to leave the urb alone */
	spin_lock_irq(&dev->err_lock);
	dev->ongoing_read = 1;
	spin_unlock_irq(&dev->err_lock);
	
	/* do it */
	rv = usb_submit_urb(dev->int_in_urb, GFP_KERNEL);
	if(rv < 0) {
		err("%s - failed submitting read urb, error %d", __func__, rv);
		dev->int_in_filled = 0;
		rv = (rv == -ENOMEM) ? rv : -EIO;
		spin_lock_irq(&dev->err_lock);
		dev->ongoing_read = 0;
		spin_unlock_irq(&dev->err_lock);
	}
	
	return rv;
}

static ssize_t driver_read( struct file *instance, char __user *user, size_t count, loff_t *offset )
{
	struct usb_rpc *dev;
	int rv;
	bool ongoing_io;
	unsigned long not_copied, to_copy;
	char buffer[25];
	
	dev = (struct usb_rpc *)instance->private_data;
	/* if we cannot read at all, return EOF */
	if(!dev->int_in_urb || ! count)
	{
		return 0;
	}
	
	/* no concurrent readers */
	rv = mutex_lock_interruptible(&dev->io_mutex);
	if( rv < 0 )
		return rv;
	
	if( !dev->interface ) { /* disconnect() was called */
		rv = -ENODEV;
		goto exit;
	}
	
	/* if IO is under way, we must not touch things */
retry:
	spin_lock_irq(&dev->err_lock);
	ongoing_io = dev->ongoing_read;
	spin_unlock_irq(&dev->err_lock);
	
	if(ongoing_io) {
		/* nonblocking IO shall not wait */
		if(instance->f_flags & O_NONBLOCK) {
			rv = -EAGAIN;
			goto exit;
		}
		
		/* IO may take forever
		 * hence wait in an interruptable state
		 */
		rv = wait_for_completion_interruptible(&dev->int_in_completion);
		if( rv < 0 )
			goto exit;
		/*
		 * by waiting we also semiprocessed the urb
		 * we must finish now
		 */
		//dev->int_in_copied = 0;
		//dev->processed_urb = 1;
	}
	
	/*if(!dev->processed_urb) {
		/*
		 * the URB hasn't been processed
		 * do it now
		 *//*
		wait_for_completion(&dev->int_in_completion);
		dev->int_in_copied = 0;
		dev->processed_urb = 1;
	}*/
	
	/* errors must be reported */
	rv = dev->errors;
	if( rv < 0 ) {
		/* any error is reported once */
		dev->errors = 0;
		/* to preserve notifications about reset */
		rv = (rv == -EPIPE) ? rv : -EIO;
		/* no data to deliver */
		dev->int_in_filled = 0;
		/* report it */
		goto exit;
	}
	
	/*
	 * if the buffer is filled we may satisfy the read
	 * else we need to start IO
	 */
	if(dev->int_in_filled) {
		/* we had read data */
		sprintf(buffer,"%u %d %u %u\n",dev->rpc_status->v_water,dev->rpc_status->t_water,dev->rpc_status->v_fan,dev->rpc_status->p_fan);
		to_copy = min( count, strlen(buffer)+1 );
		not_copied = copy_to_user(user, buffer, to_copy);
		
		if( not_copied )
			rv = -EFAULT;
		else
			rv = to_copy - not_copied;
		dev->int_in_filled = 0;
	}
	else {
		/* no data in the buffer */
		rv = rpc_do_read_io(dev, count);
		if( rv < 0 )
			goto exit;
		else if( !(instance->f_flags & O_NONBLOCK) )
			goto retry;
		rv = -EAGAIN;
	}
exit:
	mutex_unlock(&dev->io_mutex);
	return rv;
}

static struct file_operations fops = {
	.owner	= THIS_MODULE,
	.open	= driver_open,
	.release= driver_close,
	.read	= driver_read,
};

#if 0
static struct proc_dir_entry *proc_dir, *proc_v_water, *proc_t_water, *proc_v_fan;

static int proc_v_water_read( char *buf, char **start, off_t offset, int size, int *peof, void *data )
{
	int bytes_written=0, ret;

	ret = snprintf( buf, size, "%u", sensor_status_test.v_water );
	bytes_written += (ret>(size-bytes_written)) ? (size-bytes_written) : ret ;
	*peof = 1;
	return bytes_written;
}
static int proc_t_water_read( char *buf, char **start, off_t offset, int size, int *peof, void *data )
{
	int bytes_written=0, ret;

	ret = snprintf( buf, size, "%u", sensor_status_test.t_water );
	bytes_written += (ret>(size-bytes_written)) ? (size-bytes_written) : ret ;
	*peof = 1;
	return bytes_written;
}
static int proc_v_fan_read( char *buf, char **start, off_t offset, int size, int *peof, void *data )
{
	int bytes_written=0, ret;

	ret = snprintf( buf, size, "%u", sensor_status_test.v_fan );
	bytes_written += (ret>(size-bytes_written)) ? (size-bytes_written) : ret ;
	*peof = 1;
	return bytes_written;
}
#endif

static struct usb_class_driver rpc_class = {
	.name =		RPCDRV,
	.fops =		&fops,
	.minor_base =	USB_RPC_MINOR_BASE,
};

static int rpc_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	struct usb_rpc *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	size_t buffer_size;
	int retval = -ENOMEM;

	/* check if this is the interface we want to handle */
	iface_desc = interface->cur_altsetting;
	if( iface_desc->desc.bNumEndpoints != 1 )
		return -ENODEV;
	endpoint = &iface_desc->endpoint[0].desc;
	if( !usb_endpoint_is_int_in(endpoint) )
		return -ENODEV;

	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		err("Out of memory");
		goto error;
	}
	kref_init(&dev->kref);
	sema_init(&dev->limit_sem, WRITES_IN_FLIGHT);
	mutex_init(&dev->io_mutex);
	spin_lock_init(&dev->err_lock);
	init_usb_anchor(&dev->submitted);
	init_completion(&dev->int_in_completion);

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	/* set up the endpoint information */
	buffer_size = le16_to_cpu(endpoint->wMaxPacketSize);
	dev->int_in_size = buffer_size;
	dev->int_in_endpointAddr = endpoint->bEndpointAddress;
	dev->int_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
	dev->rpc_status = kmalloc(sizeof(struct rpc_status_raw), GFP_KERNEL);
	if (!dev->int_in_buffer) {
		err("Could not allocate int_in_buffer");
		goto error;
	}
	if (!dev->rpc_status) {
		err("Could not allocate rpc_status");
		goto error;
	}
	dev->int_in_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->int_in_urb) {
		err("Could not allocate int_in_urb");
		goto error;
	}

	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);

	/* we can register the device now, as it is ready */
	retval = usb_register_dev(interface, &rpc_class);
	if (retval) {
		/* something prevented us from registering this driver */
		err("Not able to get a minor for this device.");
		usb_set_intfdata(interface, NULL);
		goto error;
	}
	
#if 0
	/* Proc-Files erstellen */
	dev->proc_dir = proc_mkdir( RPCDRV_PROCDIR, NULL );
	dev->proc_t_water = create_proc_entry( RPCDRV_PROC_T_WATER, S_IRUGO, dev->proc_dir );
	if( dev->proc_t_water ) {
		dev->proc_t_water->read_proc = proc_t_water_read;
		dev->proc_t_water->data = NULL;
	}
	dev->proc_v_water = create_proc_entry( RPCDRV_PROC_V_WATER, S_IRUGO, dev->proc_dir );
	if( dev->proc_v_water ) {
		dev->proc_v_water->read_proc = proc_v_water_read;
		dev->proc_v_water->data = NULL;
	}
	dev->proc_v_fan = create_proc_entry( RPCDRV_PROC_V_FAN, S_IRUGO, dev->proc_dir );
	if( proc_v_fan ) {
		dev->proc_v_fan->read_proc = proc_v_fan_read;
		dev->proc_v_fan->data = NULL;
	}
#endif

	/* let the user know what node this device is now attached to */
	dev_info(&interface->dev,
		 "RPC device now attached to Minor %d",
		 interface->minor);
	return 0;

error:
	if (dev)
		/* this frees allocated memory */
		kref_put(&dev->kref, rpc_delete);
	return retval;
}

static void rpc_disconnect(struct usb_interface *interface)
{
	struct usb_rpc *dev;
	int minor = interface->minor;

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);
	
#if 0
	/* Proc Files entfernen */
	if(dev->proc_v_fan) remove_proc_entry( RPCDRV_PROC_V_FAN, dev->proc_dir );
	if(dev->proc_v_water) remove_proc_entry( RPCDRV_PROC_T_WATER, dev->proc_dir );
	if(dev->proc_t_water) remove_proc_entry( RPCDRV_PROC_T_WATER, dev->proc_dir );
	if(dev->proc_dir) remove_proc_entry( RPCDRV_PROCDIR, NULL );
#endif

	/* give back our minor */
	usb_deregister_dev(interface, &rpc_class);

	/* prevent more I/O from starting */
	mutex_lock(&dev->io_mutex);
	dev->interface = NULL;
	mutex_unlock(&dev->io_mutex);

	usb_kill_anchored_urbs(&dev->submitted);

	/* decrement our usage count */
	kref_put(&dev->kref, rpc_delete);

	dev_info(&interface->dev, "RPC #%d now disconnected", minor);
}

static void rpc_draw_down(struct usb_rpc *dev)
{
	int time;

	time = usb_wait_anchor_empty_timeout( &dev->submitted , 1000 );
	if( !time )
		usb_kill_anchored_urbs( &dev->submitted );
	usb_kill_urb( dev->int_in_urb );
}

static int rpc_suspend( struct usb_interface *intf, pm_message_t message)
{
	struct usb_rpc *dev = usb_get_intfdata(intf);

	if( !dev )
		return 0;
	rpc_draw_down(dev);
	return 0;
}

static int rpc_resume( struct usb_interface *intf )
{
	return 0;
}

static int rpc_pre_reset(struct usb_interface *intf)
{
	struct usb_rpc *dev = usb_get_intfdata(intf);

	mutex_lock(&dev->io_mutex);
	rpc_draw_down(dev);

	return 0;
}

static int rpc_post_reset(struct usb_interface *intf)
{
	struct usb_rpc *dev = usb_get_intfdata(intf);

	/* we are sure no URBs are active - no locking needed */
	dev->errors = -EPIPE;
	mutex_unlock(&dev->io_mutex);

	return 0;
}

static struct usb_driver rpc_driver = {
	.name =		RPCDRV,
	.probe =	rpc_probe,
	.disconnect =	rpc_disconnect,
	.suspend =	rpc_suspend,
	.resume =	rpc_resume,
	.pre_reset =	rpc_pre_reset,
	.post_reset =	rpc_post_reset,
	.id_table =	rpc_table,
	.supports_autosuspend = 1,
};

static int __init mod_init(void)
{
	int result;

	/* register this driver with the USB subsystem */
	result = usb_register(&rpc_driver);
	if (result)
		err("rpcdrv: usb_register failed. Error number %d", result);

	return result;
}

static void __exit mod_exit(void)
{
		/* deregister this driver with the USB subsystem */
	usb_deregister(&rpc_driver);
}

module_init( mod_init );
module_exit( mod_exit );

// META
MODULE_AUTHOR("Michael Pape");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No functionality yet");
MODULE_SUPPORTED_DEVICE("none");
