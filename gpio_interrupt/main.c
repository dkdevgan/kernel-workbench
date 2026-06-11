// #include <linux/module.h>
// #include<linux/fs.h>
// #include<linux/cdev.h>
// #include<linux/device.h>
// #include<linux/kdev_t.h>
// #include<linux/uaccess.h>
// #include <linux/platform_device.h>
// // Device Tree support
// #include <linux/of.h>
// #include <linux/of_device.h>
// #include<linux/slab.h>
// #include<linux/mod_devicetable.h>


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



#define MAX_DEVICES 10



/*Driver private data structure */
struct pcdrv_private_data
{
	int total_devices;
	dev_t device_num_base;
	struct class *class_gpio;
	struct device *device_gpio;
};

enum pcdev_names
{
	GPIO_INTERRUPT
};

struct gpiodev_private_data
{
	char label[20];
	struct gpio_desc *desc;
};

static irqreturn_t gpio_test_irq_handler(
        int irq,
        void *dev_id)
{
    pr_info("Button pressed\n");

    return IRQ_HANDLED;
}

/*Driver's private data */
struct pcdrv_private_data gpio_drv_data;
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

ssize_t value_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{

	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	int ret;
	long value;

	if (!dev_data) {
		dev_err(dev, "No driver data available in value_store\n");
		return -ENODEV;
	}

	if (!dev_data->desc) {
		dev_err(dev, "No gpio descriptor in value_store\n");
		return -ENODEV;
	}

	ret = kstrtol(buf, 0, &value);
	if (ret)
		return ret;

	gpiod_set_value(dev_data->desc, value);

	return count;
}


static DEVICE_ATTR_RW(value);
struct attribute *pcd_attrs[] = 
{
	&dev_attr_value.attr,
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
	pr_info("GPIO Interrupt Platform Device Removed\n");
}



int gpio_interrupt_platform_driver_probe(struct platform_device *pdev)
{
    pr_info("GPIO Interrupt Platform Device Detected\n");

	struct device *dev = &pdev->dev;

	int i = 0;

	int ret;

	const char *name;


	/*parent device node */
	struct device_node *parent = pdev->dev.of_node;
	struct device_node *child = NULL;

	struct gpiodev_private_data *dev_data;

	dev_data = devm_kzalloc(dev,sizeof(*dev_data), GFP_KERNEL);
	if(!dev_data){
		dev_err(dev,"Cannot allocate memory\n");
		return -ENOMEM;
	}

	

	dev_data->desc = devm_gpiod_get(&pdev->dev,
                        "button",
                        GPIOD_IN);
	
	platform_set_drvdata(pdev, dev_data);

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

	int irq = gpiod_to_irq(dev_data->desc);
	if(irq < 0){
		dev_err(dev,"Failed to get IRQ number for the GPIO\n");
		return irq;
	}
	pr_info("IRQ=%d\n", irq);

	ret = request_irq(
        irq,
        gpio_test_irq_handler,
        IRQF_TRIGGER_FALLING,
        "gpio_test",
        dev_data);
	
	if (ret) {
		dev_err(dev, "Failed to request IRQ: %d\n", ret);
		return ret;
	}

	pr_info("Requested IRQ successfully\n");

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


static int __init gpio_interrupt_init(void)
{
    int ret;

	/*Dynamically allocate a device number for MAX_DEVICES */
	ret = alloc_chrdev_region(&gpio_drv_data.device_num_base,0,MAX_DEVICES,"gpio interrupt devices");
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		return ret;
	}

	/*Create device class under /sys/class */
	gpio_drv_data.class_gpio = class_create("gpio_interrupt_class");
	if(IS_ERR(gpio_drv_data.class_gpio)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(gpio_drv_data.class_gpio);
		unregister_chrdev_region(gpio_drv_data.device_num_base,MAX_DEVICES);
		return ret;
	}

	/*Register a platform driver */
	platform_driver_register(&gpio_interrupt_platform_driver);
	
	pr_info("gpio interrupt platform driver loaded\n");

    return 0;
}


static void gpio_interrupt_exit(void)
{
    	/*Unregister the platform driver */
	platform_driver_unregister(&gpio_interrupt_platform_driver);

	/*Class destroy */
	class_destroy(gpio_drv_data.class_gpio);

	/*Unregister device numbers for MAX_DEVICES */
	unregister_chrdev_region(gpio_drv_data.device_num_base,MAX_DEVICES);
	
	pr_info("gpio interrupt platform driver unloaded\n");
}


module_init(gpio_interrupt_init);
module_exit(gpio_interrupt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Diksha Kumari");
MODULE_DESCRIPTION("GPIO Interrupt Example");
MODULE_VERSION("1.0");



