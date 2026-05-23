# Kernel Workbench – Platform Device and Platform Driver Framework

A Linux platform driver implementation demonstrating communication between platform devices and a platform driver using platform data. The project registers multiple pseudo devices, dynamically allocates device-specific resources, and exposes character device nodes through the Linux device model.

This project demonstrates the Linux platform bus model and the flow used by drivers to bind with hardware-specific information.

## Features

- Platform device registration
- Platform driver registration and matching
- Device-driver binding using platform bus
- Platform data support
- Dynamic memory allocation using device-managed APIs
- Character device integration
- Dynamic major/minor allocation
- Device node creation through sysfs
- Automatic resource cleanup using `devm_*`

## Project Structure

```text
.
├── pcd_device_setup.c
├── pcd_platform_driver.c
├── platform.h
├── Makefile
└── README.md
```

## Components

### Device Setup Module

Responsible for:

- Defining platform data
- Registering platform devices
- Attaching device-specific configuration

Registered devices:

```text
pseudo-char-device.0
pseudo-char-device.1
```

Platform information:

| Device | Size | Permission | Serial Number |
|----------|------|-------------|----------------|
| Device 0 | 512 bytes | Read/Write | PCDDEVABC1111 |
| Device 1 | 1024 bytes | Read/Write | PCDDEVXYZ2222 |

### Platform Driver

Responsibilities:

- Detect matching devices
- Receive platform data
- Allocate private device structures
- Allocate buffers
- Register character devices
- Create device nodes

Device nodes:

```text
/dev/PCD-0
/dev/PCD-1
```

## Driver Flow

Initialization:

1. Allocate character device numbers
2. Create device class
3. Register platform driver
4. Platform bus invokes probe()
5. Read platform data
6. Allocate resources
7. Initialize cdev
8. Create device node

Cleanup:

1. Remove device nodes
2. Delete cdev
3. Unregister platform driver
4. Destroy class
5. Release device numbers

## Memory Management

This project uses device-managed memory APIs:

```c
devm_kzalloc()
devm_kfree()
```

Benefits:

- Simplifies cleanup paths
- Automatic resource release
- Reduces memory leak risk

## Build

Compile:

```bash
make
```

Build generates:

```text
pcd_device_setup.ko
pcd_platform_driver.ko
```

## Load Modules

Load platform driver:

```bash
sudo insmod pcd_platform_driver.ko
```

Load platform device setup:

```bash
sudo insmod pcd_device_setup.ko
```

Check logs:

```bash
dmesg | tail -50
```

Expected probe sequence:

```text
A device is detected
Device probe is successful
```

Verify device nodes:

```bash
ls /dev/PCD-*
```

## Remove Modules

Unload setup module:

```bash
sudo rmmod pcd_device_setup
```

Unload driver:

```bash
sudo rmmod pcd_platform_driver
```

## Clean Build Artifacts

```bash
make clean
```

## Notes

This project demonstrates a common Linux driver architecture pattern where hardware-specific information is provided through platform devices and consumed by a generic platform driver through probe/remove callbacks.

The design also introduces device-managed memory allocation and scalable per-device resource handling.
