#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/uaccess.h>

#define NO_OF_DEVICES 4

#define DEV_MEM_SIZE_PCD_DEV1  1024
#define DEV_MEM_SIZE_PCD_DEV2  512
#define DEV_MEM_SIZE_PCD_DEV3 1024
#define DEV_MEM_SIZE_PCD_DEV4 512

// all memory operations will be done on this buffer
char device_buffer_pcdev1[DEV_MEM_SIZE_PCD_DEV1];
char device_buffer_pcdev2[DEV_MEM_SIZE_PCD_DEV2];
char device_buffer_pcdev3[DEV_MEM_SIZE_PCD_DEV3];
char device_buffer_pcdev4[DEV_MEM_SIZE_PCD_DEV4];

// device's private data
struct pcdev_private_data {
	char *buffer;
	int size;
	const char* serial_data;
	int perm;
	struct cdev cdev;
};
// Driver's private data
struct pcdrv_private_data {
	int total_devices;
        // this holds the device number
        dev_t device_number;
        struct class *pcd_class;
        struct device *pcd_device;

	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

struct pcdrv_private_data pcdrv_data = {
	.total_devices = NO_OF_DEVICES,
	.pcdev_data = {
		[0] = {
			.buffer = device_buffer_pcdev1,
			.size = DEV_MEM_SIZE_PCD_DEV1,
			.serial_data = "PCDDEV1XYZ123",
			.perm = 0x01 /* RDONLY */
			},
		[1] = {
			.buffer = device_buffer_pcdev2,
                        .size = DEV_MEM_SIZE_PCD_DEV2,
                        .serial_data = "PCDDEV2XYZ123",
                        .perm = 0x10 /* WRONLY */
			},
                [2] = {
                        .buffer = device_buffer_pcdev3,
                        .size = DEV_MEM_SIZE_PCD_DEV3,
                        .serial_data = "PCDDEV3XYZ123",
                        .perm = 0x11 /* RDWRONLY */
                        },
                [3] = {
                        .buffer = device_buffer_pcdev4,
                        .size = DEV_MEM_SIZE_PCD_DEV4,
                        .serial_data = "PCDDEV4XYZ123",
                        .perm = 0x11 /* RDWRONLY */
                        },

	}
};

loff_t pcd_llseek(struct file *filp, loff_t off, int whence);
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *off);
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *off);
int checkPermission(int accessMode, int perm);
int pcd_open(struct inode *pinode, struct file *filp);
int pcd_release(struct inode *pinode, struct file *filp);

// file operations variable
loff_t pcd_llseek(struct file *filp, loff_t off, int whence)
{

        struct pcdev_private_data* pcdev_prv_data = filp->private_data;
        int size = pcdev_prv_data->size;
        //char* device_buffer = pcdev_prv_data->buffer;

	loff_t temp = 0;
	switch(whence)
	{
		case SEEK_SET:
			if((off > size) || (off < 0))
				return -EINVAL;
			filp->f_pos = off;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + off;
			if((temp > size) || (temp < 0 ))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = off + size;
			if((temp > size) || (temp < 0))
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
	
	struct pcdev_private_data* pcdev_prv_data = filp->private_data;
	int size = pcdev_prv_data->size;
	char* device_buffer = pcdev_prv_data->buffer;
	
	if((*off + count) > size)
	{
		count = size - *off;
	}
	if(copy_to_user(buff, &device_buffer[*off], count))
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
        struct pcdev_private_data* pcdev_prv_data = filp->private_data;
        int size = pcdev_prv_data->size;
        char* device_buffer = pcdev_prv_data->buffer;

        pr_info("write requested for %zu bytes\n ", count);
        pr_info("Current file position = %lld\n", *off);
        if((*off + count) > size)
        {
                count = size - *off;
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

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11

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
	int minor;
	minor = MINOR(pinode->i_rdev);
	pr_info("minor access = %d\n ", minor);

	struct pcdev_private_data* pcdev_data;
	pcdev_data = container_of(pinode->i_cdev, struct pcdev_private_data, cdev);
	
	filp->private_data = pcdev_data;
	
	if(checkPermission(filp->f_mode, pcdev_data->perm))
	{
		return 1;
	}
	pr_info("Open call was successful");
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

static int __init pcd_init(void)
{
	int ret;
	// 1. dynamically allocate device numer 
	ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "PCD Multiple Drivers");
	if(ret < 0 )
		goto out;

        // 2. Create device class under /sys/class folder
        pcdrv_data.pcd_class = class_create("PCD_CLASS_MULTIPLE_DEVICES");
        if(IS_ERR(pcdrv_data.pcd_class))
        {
                ret = PTR_ERR(pcdrv_data.pcd_class);
                goto unregister_chr_dev;
        }
	int i=0;
	for(i=0;i<NO_OF_DEVICES;i++) {
		pr_info("Device Number <major>:<minor> = %d %d \n", MAJOR(pcdrv_data.device_number + i), MINOR(pcdrv_data.device_number + i));
		
		// 3. init cdev structure
		cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops); 

		// 4. register device with the VFS
       		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number + i, 1);
		if(ret  < 0)
			goto cdev_del;
		
	        // 5. Create device file inside /sys/class/dev folder
	        pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, NULL, pcdrv_data.device_number + i, NULL, "PCD-%d", i+1);
        
       		if(IS_ERR(pcdrv_data.pcd_device))
        	{
                	ret = PTR_ERR(pcdrv_data.pcd_device);
                	goto class_del;
        	}
	}

	pr_info("All devices are initialized");
	goto out;
cdev_del:
class_del:
	for(;i>=0;i--) {
		device_destroy(pcdrv_data.pcd_class, pcdrv_data.device_number + i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
class_destroy(pcdrv_data.pcd_class);
unregister_chr_dev:
	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
out: 
	return ret;
}

static void __exit pcd_cleanup(void)
{
        for(int i=0;i<NO_OF_DEVICES;i++) {
                device_destroy(pcdrv_data.pcd_class, pcdrv_data.device_number + i);
                cdev_del(&pcdrv_data.pcdev_data[i].cdev);
        }
        class_destroy(pcdrv_data.pcd_class);
        unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
	pr_info("All devices unloaded successfully");
}

module_init(pcd_init);
module_exit(pcd_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Diksha Kumari");
MODULE_DESCRIPTION("Pseudo character driver");
MODULE_VERSION("1.0");

