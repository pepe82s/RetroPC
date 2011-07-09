#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#define RPCDRV_PROCDIR "rpc"
#define RPCDRV_PROC_T_WATER "t_water"
#define RPCDRV_PROC_V_WATER "v_water"
#define RPCDRV_PROC_V_FAN   "v_fan"
#endif

#define RPCDRV "rpc" /* Name des Moduls */

/* USB IDs
 * TESTING ONLY!!!
 * Provided by LUFA
 */
#define VID 0x03EB
#define PID 0x2040

typedef struct
{
	u16 v_water;
	s16 t_water;
	u16 v_fan;
	u8  p_fan;
} rpc_status_raw_t;

static rpc_status_raw_t sensor_status = {
	.v_water = 280,
	.t_water = 27,
	.v_fan = 800,
	.p_fan = 60,
};

static dev_t rpcdrv_dev_number;
static struct cdev *driver_object;
struct class *rpcdrv_class;
static atomic_t access_count = ATOMIC_INIT(-1);

static int driver_open( struct inode *devicefile, struct file *instance )
{
	if( instance->f_flags&O_RDWR || instance->f_flags&O_WRONLY ) {
		if( atomic_inc_and_test( &access_count ) ) {
			return 0; /* Schreibender Zugriff erlaubt */
		}
		/* Ist bereits schreibend geoeffnet */
		atomic_dec( &access_count );
		return -EBUSY;
	}
	return 0; /* Lesender Zugriff ist immer erlaubt */
}

static int driver_close( struct inode *devicefile, struct file *instance )
{
	if( instance->f_flags&O_RDWR || instance->f_flags&O_WRONLY )
		atomic_dec( &access_count );
	return 0;
}

static ssize_t driver_read( struct file *instance, char __user *user, size_t count, loff_t *offset )
{
	unsigned long not_copied, to_copy;
	char buffer[25];
	sprintf(buffer,"%u %d %u %u\n",sensor_status.v_water,sensor_status.t_water,sensor_status.v_fan,sensor_status.p_fan);
	to_copy = min( count, strlen(buffer)+1 );
	not_copied = copy_to_user(user, buffer, to_copy);
	return to_copy-not_copied;
}

static struct file_operations fops = {
	.owner	= THIS_MODULE,
	.open	= driver_open,
	.release= driver_close,
	.read	= driver_read,
};

#ifdef CONFIG_PROC_FS
static struct proc_dir_entry *proc_dir, *proc_v_water, *proc_t_water, *proc_v_fan;

static int proc_v_water_read( char *buf, char **start, off_t offset, int size, int *peof, void *data )
{
	int bytes_written=0, ret;

	ret = snprintf( buf, size, "%u", sensor_status.v_water );
	bytes_written += (ret>(size-bytes_written)) ? (size-bytes_written) : ret ;
	*peof = 1;
	return bytes_written;
}
static int proc_t_water_read( char *buf, char **start, off_t offset, int size, int *peof, void *data )
{
	int bytes_written=0, ret;

	ret = snprintf( buf, size, "%u", sensor_status.t_water );
	bytes_written += (ret>(size-bytes_written)) ? (size-bytes_written) : ret ;
	*peof = 1;
	return bytes_written;
}
static int proc_v_fan_read( char *buf, char **start, off_t offset, int size, int *peof, void *data )
{
	int bytes_written=0, ret;

	ret = snprintf( buf, size, "%u", sensor_status.v_fan );
	bytes_written += (ret>(size-bytes_written)) ? (size-bytes_written) : ret ;
	*peof = 1;
	return bytes_written;
}
#endif


static int __init mod_init(void)
{
	if( alloc_chrdev_region(&rpcdrv_dev_number,0,1,RPCDRV) < 0 )
		return -EIO;
	driver_object = cdev_alloc();
	if( driver_object == NULL )
		goto free_device_number;
	driver_object->owner = THIS_MODULE;
	driver_object->ops = &fops;
	if( cdev_add(driver_object,rpcdrv_dev_number,1) )
		goto free_cdev;
	/* Eintrag ins sys fs */
	rpcdrv_class = class_create( THIS_MODULE, RPCDRV );
	if( IS_ERR( rpcdrv_class) ) {
		pr_err("rpcdrv: no udev support\n");
		goto free_cdev;
	}
	device_create( rpcdrv_class, NULL, rpcdrv_dev_number, NULL, "%s", RPCDRV );

#ifdef CONFIG_PROC_FS
	/* Proc-Files erstellen */
	proc_dir = proc_mkdir( RPCDRV_PROCDIR, NULL );
	proc_t_water = create_proc_entry( RPCDRV_PROC_T_WATER, S_IRUGO, proc_dir );
	if( proc_t_water ) {
		proc_t_water->read_proc = proc_t_water_read;
		proc_t_water->data = NULL;
	}
	proc_v_water = create_proc_entry( RPCDRV_PROC_V_WATER, S_IRUGO, proc_dir );
	if( proc_v_water ) {
		proc_v_water->read_proc = proc_v_water_read;
		proc_v_water->data = NULL;
	}
	proc_v_fan = create_proc_entry( RPCDRV_PROC_V_FAN, S_IRUGO, proc_dir );
	if( proc_v_fan ) {
		proc_v_fan->read_proc = proc_v_fan_read;
		proc_v_fan->data = NULL;
	}
#endif

	return 0;
free_cdev:
	kobject_put( &driver_object->kobj );
free_device_number:
	unregister_chrdev_region( rpcdrv_dev_number, 1 );
	return -EIO;
}

static void __exit mod_exit(void)
{
#ifdef CONFIG_PROC_FS
	/* Proc Files entfernen */
	if(proc_v_fan) remove_proc_entry( RPCDRV_PROC_V_FAN, proc_dir );
	if(proc_v_water) remove_proc_entry( RPCDRV_PROC_T_WATER, proc_dir );
	if(proc_t_water) remove_proc_entry( RPCDRV_PROC_T_WATER, proc_dir );
	if(proc_dir) remove_proc_entry( RPCDRV_PROCDIR, NULL );
#endif

	/* Eintrag im sys fs loeschen, damit geraetedatei weg */
	device_destroy( rpcdrv_class, rpcdrv_dev_number );
	class_destroy( rpcdrv_class );
	/* Treiber abmelden */
	cdev_del( driver_object );
	unregister_chrdev_region( rpcdrv_dev_number, 1 );
	return;
}

module_init( mod_init );
module_exit( mod_exit );

// META
MODULE_AUTHOR("Michael Pape");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No functionality yet");
MODULE_SUPPORTED_DEVICE("none");
