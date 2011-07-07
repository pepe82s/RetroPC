#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define MODNAME "rpc" /* Name des Moduls */

static dev_t module_dev_number;
static struct cdev *driver_object;
struct class *module_class;

static struct file_operations fops = {
	/* Adressen der jeweiligen
	 * Treiberfunktionen
	 */
};

static int __init mod_init(void)
{
	if( alloc_chrdev_region(&module_dev_number,0,1,MODNAME) < 0 )
		return -EIO;
	driver_object = cdev_alloc();
	if( driver_object == NULL )
		goto free_device_number;
	driver_object->owner = THIS_MODULE;
	driver_object->ops = &fops;
	if( cdev_add(driver_object,module_dev_number,1) )
		goto free_cdev;
	/* Eintrag ins sys fs */
	module_class = class_create( THIS_MODULE, MODNAME );
	device_create( module_class, NULL, module_dev_number, NULL, "%s", MODNAME );
	return 0;
free_cdev:
	kobject_put( &driver_object->kobj );
free_device_number:
	unregister_chrdev_region( module_dev_number, 1 );
	return -EIO;
}

static void __exit mod_exit(void)
{
	/* Eintrag im sys fs loeschen, damit geraetedatei weg */
	device_destroy( module_class, module_dev_number );
	class_destroy( module_class );
	/* Treiber abmelden */
	cdev_del( driver_object );
	unregister_chrdev_region( module_dev_number, 1 );
	return;
}

module_init( mod_init );
module_exit( mod_exit );

// META
MODULE_AUTHOR("Michael Pape");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No functionality yet");
MODULE_SUPPORTED_DEVICE("none");
