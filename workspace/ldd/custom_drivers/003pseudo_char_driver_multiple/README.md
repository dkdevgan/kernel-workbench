# Kernel Workbench – Multi-Device Pseudo Character Driver

A Linux character driver implementing multiple pseudo devices with independent buffers, permissions, and private device data. Each device is dynamically registered with the kernel and exposed through separate device nodes.

This project demonstrates scalable character driver design using per-device and driver-private data structures.

## Features

- Multiple character devices under one driver
- Independent memory buffers per device
- Device-specific private data
- Driver-private data structure
- Dynamic major/minor number allocation
- Read / write / seek support
- Per-device access permissions
- Device node creation through kernel device model
- VFS integration using `cdev`

## Devices

The driver creates four devices:

| Device | Buffer Size | Permission | Serial |
|----------|-------------|-------------|---------|
| PCD-1 | 1024 bytes | Read only | PCDDEV1XYZ123 |
| PCD-2 | 512 bytes | Write only | PCDDEV2XYZ123 |
| PCD-3 | 1024 bytes | Read/Write | PCDDEV3XYZ123 |
| PCD-4 | 512 bytes | Read/Write | PCDDEV4XYZ123 |

## Project Structure

```text
.
├── pcd_n.c        # Multi-device driver implementation
├── Makefile
└── README.md
```

## Architecture

Driver private data:

- Total devices
- Base device number
- Device class
- Device metadata

Per-device private data:

- Buffer
- Buffer size
- Serial number
- Permissions
- cdev structure

The open call associates device private data using:

```c
container_of()
```

allowing each device instance to maintain its own state.

## Initialization Flow

1. Allocate device numbers
2. Create device class
3. Initialize each device
4. Register cdev with VFS
5. Create device node

Device nodes created:

```text
/dev/PCD-1
/dev/PCD-2
/dev/PCD-3
/dev/PCD-4
```

## Build

Compile:

```bash
make
```

## Load Driver

Insert module:

```bash
sudo insmod pcd_n.ko
```

Check kernel logs:

```bash
dmesg | tail -30
```

Verify devices:

```bash
ls /dev/PCD-*
```

View allocated devices:

```bash
cat /proc/devices
```

## Test Devices

Write:

```bash
echo "hello" | sudo tee /dev/PCD-3
```

Read:

```bash
sudo cat /dev/PCD-3
```

Permission behavior examples:

```bash
sudo cat /dev/PCD-1
```

Should succeed.

```bash
echo test | sudo tee /dev/PCD-1
```

Should fail due to read-only permissions.

```bash
sudo cat /dev/PCD-2
```

Should fail due to write-only permissions.

## Remove Driver

Unload:

```bash
sudo rmmod pcd_n
```

Verify:

```bash
dmesg | tail
```

## Clean Build Artifacts

```bash
make clean
```

## Notes

This project demonstrates a scalable pattern used in Linux driver development where a single driver manages multiple device instances using private data structures and VFS integration.
