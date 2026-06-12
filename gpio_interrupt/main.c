#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include<linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include<linux/kdev_t.h>
#include<linux/cdev.h>

#define MAX_DEVICES 10

/*Driver private data structure */
struct pcdrv_private_data
{
	int total_devices;
	dev_t device_num_base;
	struct class *class_gpio;
	struct device *device_gpio;
		/* Cdev variable */
	struct cdev gpio_cdev;
};

enum pcdev_names
{
	GPIO_INTERRUPT
};

struct gpiodev_private_data
{

	char label[20];
	struct gpio_desc *desc;
	atomic_t irq_count;
	int irq;
	struct work_struct work;
	ktime_t last_press_time;
	struct mutex lock;
};

static irqreturn_t gpio_test_irq_handler(
        int irq,
        void *dev_id)
{
    pr_info("Button pressed\n");
	atomic_inc(&((struct gpiodev_private_data *)dev_id)->irq_count);

	schedule_work(&((struct gpiodev_private_data *)dev_id)->work);
	pr_info("Work scheduled \n");

    return IRQ_HANDLED;
}

static void schedule_work_queue(struct work_struct* work)
{
	struct gpiodev_private_data *data;
	data = container_of(work,
                            struct gpiodev_private_data,
                            work);

	pr_info("count from workqueue = %d\n",atomic_read(&data->irq_count));
	mutex_lock(&data->lock);
	data->last_press_time = ktime_get();
	mutex_unlock(&data->lock);
	pr_info("Updated time stamp in workqueue\n");

	pr_info("Workqueue executed\n");
}

/*Driver's private data */
struct pcdrv_private_data gpio_drv_data;
ssize_t value_show(struct device *dev, struct device_attribute *attr,char *buf);
ssize_t irq_count_show(struct device *dev, struct device_attribute *attr,char *buf);
ssize_t last_press_time_show(struct device *dev, struct device_attribute *attr,char *buf);
int gpio_interrupt_platform_driver_probe(struct platform_device *pdev);
void gpio_interrupt_platform_driver_remove(struct platform_device *pdev);

ssize_t value_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	int value;

	if (!dev_data) {
		dev_err(dev, "No driver data available in value_show\n");
		return -ENODEV;
	}

	if (!dev_data->desc) {
		dev_err(dev, "No gpio descriptor in value_show\n");
		return -ENODEV;
	}

	value = gpiod_get_value(dev_data->desc);
	if (value < 0)
		return value;
	return sprintf(buf, "%d\n", value);
}

ssize_t last_press_time_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	
	if (!dev_data) {
		dev_err(dev, "No driver data available in value_show\n");
		return -ENODEV;
	}
	mutex_lock(&dev_data->lock);
	long long int ts = ktime_to_ns(dev_data->last_press_time);
	mutex_unlock(&dev_data->lock);

	return sprintf(buf, "%lld\n", ts);
	
}

ssize_t irq_count_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	
	if (!dev_data) {
		dev_err(dev, "No driver data available in value_show\n");
		return -ENODEV;
	}
	return sprintf(buf,
                       "%lld\n",
                       dev_data->irq_count);
}

static DEVICE_ATTR_RO(value);
static DEVICE_ATTR_RO(last_press_time);
static DEVICE_ATTR_RO(irq_count);

struct attribute *pcd_attrs[] = 
{
	&dev_attr_value.attr,
	&dev_attr_last_press_time.attr,
	&dev_attr_irq_count.attr,
	NULL
};

struct attribute_group pcd_attr_group =
{
	.attrs = pcd_attrs
};



void gpio_interrupt_platform_driver_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	sysfs_remove_group(&dev->kobj, &pcd_attr_group);
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	cancel_work_sync(&dev_data->work);
	free_irq(dev_data->irq, dev_data);
	pr_info("GPIO Interrupt Platform Device Removed\n");
}



int gpio_interrupt_platform_driver_probe(struct platform_device *pdev)
{
    pr_info("GPIO Interrupt Platform Device Detected\n");

	struct device *dev = &pdev->dev;

	//int i = 0;

	int ret;

	//const char *name;


	/*parent device node */
	//struct device_node *parent = pdev->dev.of_node;
	//struct device_node *child = NULL;

	struct gpiodev_private_data *dev_data;

	dev_data = devm_kzalloc(dev,sizeof(*dev_data), GFP_KERNEL);
	if(!dev_data){
		dev_err(dev,"Cannot allocate memory\n");
		return -ENOMEM;
	}

	dev_data->desc = devm_gpiod_get(&pdev->dev,
                        "button",
                        GPIOD_IN);
	
	atomic_set(&dev_data->irq_count, 0);

	int value = gpiod_get_value(dev_data->desc);
		if (value < 0) {
			dev_err(dev, "Failed to read GPIO value\n");
			return value;
		}
		pr_info("GPIO value=%d\n", value);

	ret = sysfs_create_group(&dev->kobj, &pcd_attr_group);
	
	if(ret){
		dev_err(dev,"Error in creating sysfs group\n");
		return ret;
	}
	dev_info(dev,"sysfs created\n");

	dev_data->irq = gpiod_to_irq(dev_data->desc);
	if(dev_data->irq < 0){
		sysfs_remove_group(&dev->kobj, &pcd_attr_group);
		dev_err(dev,"Failed to get IRQ number for the GPIO\n");
		return dev_data->irq;
	}
	pr_info("IRQ=%d\n", dev_data->irq);

	ret = request_irq(
        dev_data->irq,
        gpio_test_irq_handler,
        IRQF_TRIGGER_FALLING,
        "gpio_test",
        dev_data);
	
	if (ret) {
		free_irq(dev_data->irq, dev_data);
		sysfs_remove_group(&dev->kobj, &pcd_attr_group);
		dev_err(dev, "Failed to request IRQ: %d\n", ret);
		return ret;
	}
	platform_set_drvdata(pdev, dev_data);
	pr_info("Requested IRQ successfully\n");

	INIT_WORK(&dev_data->work,
          schedule_work_queue);

	mutex_init(&dev_data->lock);

	// int childCount = of_get_child_count(parent);
	// if(!childCount){
	// 	dev_err(dev,"No devices found\n");
	// 	return -EINVAL;
	// }

	// dev_info(dev,"Total devices found = %d\n",childCount);

	// gpio_drv_data.device_gpio = devm_kzalloc(dev, sizeof(struct device *) * childCount, GFP_KERNEL);

// 	for_each_available_child_of_node(parent,child)
// 	{

// 		dev_data = devm_kzalloc(dev,sizeof(*dev_data), GFP_KERNEL);
// 		if(!dev_data){
// 			dev_err(dev,"Cannot allocate memory\n");
// 			return -ENOMEM;
// 		}

// 		// if(of_property_read_string(child,"label",&name) )
// 		// {
// 		// 	dev_warn(dev,"Missing label information\n");
// 		// 	snprintf(dev_data->label,sizeof(dev_data->label),"unkngpio%d",i);
// 		// }else{
// 		// 	strcpy(dev_data->label,name);
// 		// 	dev_info(dev,"GPIO label = %s\n",dev_data->label);
			
// 		// }

// 		/* Try to get GPIO from child node; fall back to of_get_named_gpio if
// 		 * the devm_gpiod_* helper isn't available on this kernel.
// 		 */
// #ifdef HAVE_DEVM_GPIOD_GET_FROM_CHILD
// 		dev_data->desc = devm_gpiod_get_from_child(dev, "bone", child,
// 							GPIOD_ASIS, dev_data->label);
// 		if (IS_ERR(dev_data->desc)) {
// 			ret = PTR_ERR(dev_data->desc);
// 			if (ret == -ENOENT)
// 				dev_err(dev, "No GPIO has been assigned to the requested function and/or index\n");
// 			return ret;
// 		}
// #else
// 		{
// 			int gpio_num = of_get_named_gpio(child, "gpios", 0);
// 			if (gpio_num < 0) {
// 				dev_err(dev, "No GPIO assigned in DT or invalid GPIO (%d)\n", gpio_num);
// 				return gpio_num;
// 			}
// 			dev_data->desc = gpio_to_desc(gpio_num);
// 			if (!dev_data->desc) {
// 				dev_err(dev, "Failed to get gpio descriptor from gpio number %d\n", gpio_num);
// 				return -EINVAL;
// 			}
// 		}
// #endif

// 		int value = gpiod_get_value(dev_data->desc);
// 		if (value < 0) {
// 			dev_err(dev, "Failed to read GPIO value\n");
// 			return value;
// 		}
// 		pr_info("GPIO value=%d\n", value);

// 		// /* set the gpio direction to output */
// 		// ret = gpiod_direction_output(dev_data->desc,0);	
// 		// if(ret){
// 		// 	dev_err(dev,"gpio direction set failed \n");
// 		// 	return ret;
// 		// }

// 		// /*Create devices under /sys/class/bone_gpios */
// 		// gpio_drv_data.device_gpio[i] = device_create_with_groups(gpio_drv_data.class_gpio,dev,0,dev_data,gpio_attr_groups,\
// 		// 						dev_data->label);
// 		// if(IS_ERR(gpio_drv_data.dev[i])){
// 		// 	dev_err(dev,"Error in device_create \n");
// 		// 	return PTR_ERR(gpio_drv_data.dev[i]);
// 		// }
				

// 		i++;

// 	}
	
	pr_info("GPIO Interrupt Platform Device Probed\n");
    return 0;
}

 struct platform_device_id pcdevs_ids[] = 
{
	{.name = "gpio-interrupt",.driver_data = GPIO_INTERRUPT},
	{ } /*Null termination */
};

 struct of_device_id org_pcdev_dt_match[] = 
{
	{.compatible = "dk,gpio-test",.data = (void*)GPIO_INTERRUPT},
	{ } /*Null termination*/
};

struct platform_driver gpio_interrupt_platform_driver = 
{
	.probe = gpio_interrupt_platform_driver_probe,
	.remove = gpio_interrupt_platform_driver_remove,
	.id_table = pcdevs_ids,
	.driver = {
		.name = "gpio-interrupt",
		.of_match_table = of_match_ptr(org_pcdev_dt_match)
	}
};

loff_t gpio_lseek(struct file *filp, loff_t offset, int whence);
ssize_t gpio_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t gpio_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
int gpio_open(struct inode *inode, struct file *filp);
int gpio_release(struct inode *inode, struct file *flip);

loff_t gpio_lseek(struct file *filp, loff_t offset, int whence)
{
	pr_info("lseek called\n");
	return 0;

}

ssize_t gpio_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("read called\n");
	return 0;
}

ssize_t gpio_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("write called\n");
	return 0;
}

int gpio_open(struct inode *inode, struct file *filp)
{
	pr_info("open was successful\n");

	return 0;
}

int gpio_release(struct inode *inode, struct file *flip)
{
	pr_info("release was successful\n");

	return 0;
}


/* file operations of the driver */
struct file_operations gpio_fops=
{
	.open = gpio_open,
	.release = gpio_release,
	.read = gpio_read,
	.write = gpio_write,
	.owner = THIS_MODULE
};

static int __init gpio_interrupt_init(void)
{
    int ret;

	/*Dynamically allocate a device number for MAX_DEVICES */
	ret = alloc_chrdev_region(&gpio_drv_data.device_num_base,0,MAX_DEVICES,"gpio interrupt devices");
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		goto out;
	}

	/*2. Initialize the cdev structure with fops*/
	cdev_init(&gpio_drv_data.gpio_cdev,&gpio_fops);

	/* 3. Register a device (cdev structure) with VFS */
	gpio_drv_data.gpio_cdev.owner = THIS_MODULE;
	ret = cdev_add(&gpio_drv_data.gpio_cdev,gpio_drv_data.device_num_base,1);
	if(ret < 0){
		pr_err("Cdev add failed\n");
		goto unreg_chrdev;
	}

	/*4. create device class under /sys/class/ */
	gpio_drv_data.class_gpio = class_create("gpio_interrupt_class");
	if(IS_ERR(gpio_drv_data.class_gpio)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(gpio_drv_data.class_gpio);
		goto cdev_del;
	}

	/*5.  populate the sysfs with device information */
	gpio_drv_data.device_gpio = device_create(gpio_drv_data.class_gpio,NULL,gpio_drv_data.device_num_base,NULL,"gpio_interrupt_device");
	if(IS_ERR(gpio_drv_data.device_gpio)){
		pr_err("Device create failed\n");
		ret = PTR_ERR(gpio_drv_data.device_gpio);
		goto class_del;
	}
	/*3. Register a platform driver */
	platform_driver_register(&gpio_interrupt_platform_driver);

	pr_info("gpio interrupt platform driver loaded\n");
	return 0;
class_del:
	class_destroy(gpio_drv_data.class_gpio);
cdev_del:
	cdev_del(&gpio_drv_data.gpio_cdev);	
unreg_chrdev:
	unregister_chrdev_region(gpio_drv_data.device_num_base,1);
out:
	pr_info("Module insertion failed\n");
	return ret;
}


static void gpio_interrupt_exit(void)
{
   	/*Unregister the platform driver */
	platform_driver_unregister(&gpio_interrupt_platform_driver);


	device_destroy(gpio_drv_data.class_gpio,gpio_drv_data.device_num_base);
	class_destroy(gpio_drv_data.class_gpio);
	cdev_del(&gpio_drv_data->gpio_cdev);
	unregister_chrdev_region(gpio_drv_data.device_num_base,MAX_DEVICES);

	pr_info("gpio interrupt platform driver unloaded\n");
}


module_init(gpio_interrupt_init);
module_exit(gpio_interrupt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Diksha Kumari");
MODULE_DESCRIPTION("GPIO Interrupt Example");
MODULE_VERSION("1.0");



