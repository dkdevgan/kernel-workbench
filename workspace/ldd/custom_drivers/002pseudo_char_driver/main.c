#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/uaccess.h>

#define DEV_MEM_SIZE 512

// all memory operations will be done on this buffer
char device_buffer[DEV_MEM_SIZE];

// this holds the device number
dev_t device_number;

// cdev variable 
struct cdev pcd_cdev;

// file operations variable
loff_t pcd_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t temp = 0;
	switch(whence)
	{
		case SEEK_SET:
			if((off > DEV_MEM_SIZE) || (off < 0))
				return -EINVAL;
			filp->f_pos = off;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + off;
			if((temp > DEV_MEM_SIZE) || (temp < 0 ))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = off + DEV_MEM_SIZE;
			if((temp > DEV_MEM_SIZE) || (temp < 0))
				return -EINVAL; 
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL;
	}
	return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *off)
{
	pr_info("read requested for %zu bytes\n ", count);
	pr_info("Current file position = %lld\n", *off);
	if((*off + count) > DEV_MEM_SIZE)
	{
		count = DEV_MEM_SIZE - *off;
	}
	if(copy_to_user(buff, &device_buffer[*off],count))
	{
		return -EFAULT;
	}
	*off += count;
	pr_info("Number of bytes successfully read %zu\n", count);
	pr_info("Updated file offset = %lld\n", *off);
	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *off)
{
        pr_info("write requested for %zu bytes\n ", count);
        pr_info("Current file position = %lld\n", *off);
        if((*off + count) > DEV_MEM_SIZE)
        {
                count = DEV_MEM_SIZE - *off;
        }
	if(count == 0)
	{
		return -ENOMEM;
	}	
        if(copy_from_user(&device_buffer[*off], buff, count))
        {
                return -EFAULT;
        }
        *off += count;
        pr_info("Number of bytes successfully written %zu\n", count);
        pr_info("Updated file offset = %lld\n", *off);
        return count;

}

int pcd_open(struct inode *pinode, struct file *filp)
{
	return 0;
}

int pcd_release(struct inode *pinode, struct file *filp)
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

struct class *pcd_class;
struct device *pcd_device;

static int __init pcd_init(void)
{
	int ret;
	// 1. dynamically allocate device numer 
	ret = alloc_chrdev_region(&device_number, 0, 1, "PCD");
	if(ret < 0 )
		goto out;
	
	// 2. init cdev structure
	cdev_init(&pcd_cdev, &pcd_fops); 
	
	// 3. register device with the VFS
        pcd_cdev.owner = THIS_MODULE;
	ret = cdev_add(&pcd_cdev, device_number, 1);
	if(ret  < 0)
		goto unregister_chr_dev;
	
	// 4. Create device class under /sys/class folder
	pcd_class = class_create("PCD_CLASS");
	if(IS_ERR(pcd_class))
	{
		ret = ERR_PTR(pcd_class);
		goto cdev_del;
	}

	pr_info("class create is success");
	// 5. Create device file inside /sys/class/dev folder
	pcd_device = device_create(pcd_class, NULL, device_number, NULL, "PCD");
	
	if(IS_ERR(pcd_device))
	{
		ret = ERR_PTR(pcd_device);
		goto destroy_class;
	}

	pr_info("Device init is successful");
	return 0;

destroy_class:
        class_destroy(pcd_class);
cdev_del:
	cdev_del(&pcd_cdev);
unregister_chr_dev:
	unregister_chrdev_region(device_number, 1);
out: 
	return ret;
}

static void __exit pcd_cleanup(void)
{
	device_destroy(pcd_class, device_number);
	class_destroy(pcd_class);
	cdev_del(&pcd_cdev);	
	unregister_chrdev_region(device_number, 1);
	pr_info("Device exit is successful");
	return;
}

module_init(pcd_init);
module_exit(pcd_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Diksha Kumari");
MODULE_DESCRIPTION("Pseudo character driver");
MODULE_VERSION("1.0");

