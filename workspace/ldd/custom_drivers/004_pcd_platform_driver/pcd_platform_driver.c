#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/uaccess.h>
#include "platform.h"
#include <linux/platform_device.h>
#include <linux/slab.h>


#define MAX_DEVICES 2

struct pcdev_private_data {
	struct pcdev_platform_data pdata;
	char* buffer;
	dev_t dev_num;
	struct cdev dev;
};

struct pcdrv_private_data {
	int total_devices;
	dev_t device_num_base;
	struct class *class_pcd;
	struct device *device_pcd;
};

struct pcdrv_private_data pcdrv_data;

int checkPermission(int accessMode, int perm)
{
	if(perm == RDWR)
		return 0;
	if(perm == RDONLY)
		if((accessMode & FMODE_READ) && !(accessMode & FMODE_WRITE) )
			return 0;
	if(perm == WRONLY)
		if((accessMode & FMODE_WRITE) && !(accessMode & FMODE_READ) )
			return 0;
	return 1;
}

int pcd_open(struct inode *pinode, struct file *filp)
{
	return 0;
}

int pcd_release(struct inode *pinode, struct file *filp)
{
	return 0;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *off)
{
	return 0;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *off)
{
	return 0;
}

loff_t pcd_llseek(struct file *filp, loff_t off, int whence)
{
	return 0;
}

struct file_operations pcd_fops = {
	.read = pcd_read,
	.write = pcd_write,
	.open = pcd_open,
	.llseek = pcd_llseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};

int pcd_platform_driver_probe(struct platform_device *device)
{
	int ret;
 	pr_info(" A device is detected\n");	
	struct pcdev_private_data* dev_data;
	struct pcdev_platform_data *pdata;

	// get platform data
	pdata = (struct pcdev_platform_data*) dev_get_platdata(&device->dev);
	if(!pdata)
	{
		pr_info("no platform data \n");
		ret = -EINVAL;
		goto out;
	}

        // dynamically allocate memory for device private data
	//dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
	dev_data = devm_kzalloc(&device->dev, sizeof(*dev_data), GFP_KERNEL);
	if(!dev_data) {
		pr_info("kzalloc 1 failed\n");	
		ret = -ENOMEM;
		goto out;
	}

	dev_set_drvdata(&device->dev, dev_data);

	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

        // dynamically allocate memory for device private data -> buffer
//	dev_data->buffer = kzalloc(dev_data->pdata.size, GFP_KERNEL);
	dev_data->buffer = devm_kzalloc(&device->dev, dev_data->pdata.size, GFP_KERNEL);
	if(!dev_data->buffer) {
		pr_info("no memory\n");
		ret = -ENOMEM;
		goto dev_data_free;
	}
	
	// get the device number
	dev_data->dev_num = pcdrv_data.device_num_base + device->id;

	// do cdev init and cdev add
	cdev_init(&dev_data->dev, &pcd_fops); 
	
	// 4. register device with the VFS
    	dev_data->dev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->dev, dev_data->dev_num, 1);
	if(ret  < 0) {
		pr_info("c dev add failed\n");
		goto buffer_free;
	}

	// 5. Create device file inside /sys/class/dev folder
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num, NULL, "PCD-%d", device->id);
        
       	if(IS_ERR(pcdrv_data.device_pcd))
        {
		pr_info("Device create failed\n");
               	ret = PTR_ERR(pcdrv_data.device_pcd);
               	goto cdev_del;
        }
	pcdrv_data.total_devices++;
	pr_info("Device probe is successful \n");
	return 0;
cdev_del:
	cdev_del(&dev_data->dev);
buffer_free:
//	kfree(dev_data->buffer);
	devm_kfree(&device->dev, dev_data->buffer);
dev_data_free:
//	kfree(dev_data);
        devm_kfree(&device->dev, dev_data);
	return ret;
out:	
	pr_info("Probe Failed\n");
	return ret;
}

int pcd_platform_driver_remove(struct platform_device *device)
{
	pr_info("Driver remove is invoked\n");
	struct pcdev_private_data* dev_data = dev_get_drvdata(&device->dev);
	// device destroy
	device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);
	// remove cdev entry from system 
	cdev_del(&dev_data->dev);

//	kfree(dev_data->buffer);
//	kfree(dev_data);
	pcdrv_data.total_devices--;
	pr_info("Device is removed successfully \n");
	return 0;
}

struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .driver = {
	.name = "pseudo-char-device"
	}
};

static int __init pcd_platform_driver_init(void)
{	
        //allocate device number
	int ret = alloc_chrdev_region(&pcdrv_data.device_num_base, 0, MAX_DEVICES, "PCD Platform Driver");
	if(ret < 0 )
	{
		pr_info("Alloc chrdev failed \n");
		return ret;
	}
	// create device class under /sys/class
	pcdrv_data.class_pcd = class_create("PCD_CLASS_PLATFORM_DEVICE");
        if(IS_ERR(pcdrv_data.class_pcd))
        {
		pr_info("class create failed \n");
                ret = PTR_ERR(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
                return ret;
        }

	platform_driver_register(&pcd_platform_driver);
	pr_info("pcd platform driver loaded \n");
	return 0;
}

static void __exit pcd_platform_driver_cleanup(void)
{
	platform_driver_unregister(&pcd_platform_driver);
	class_destroy(pcdrv_data.class_pcd);
	unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
	pr_info("pcd Platform driver unloaded \n");
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Diksha Kumari");
MODULE_DESCRIPTION("Pseudo Platform driver");
MODULE_VERSION("1.0");

