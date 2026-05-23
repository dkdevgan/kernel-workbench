obj-m := pcd_device_setup.o pcd_platform_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules 
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
help: 
	make -C /lib/modules/6.8.0-87-generic/build/ M=$(PWD) help

