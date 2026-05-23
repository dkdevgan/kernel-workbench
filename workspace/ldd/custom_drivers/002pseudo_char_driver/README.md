# Kernel Workbench – Pseudo Character Driver

A Linux pseudo character driver implementing basic file operations such as open, read, write, release, and seek. The driver allocates a device dynamically, registers with the kernel VFS layer, and exposes a device node for user-space interaction.

This project demonstrates the flow involved in creating a basic Linux character driver and interfacing with it from user space.

## Features

- Dynamic character device number allocation
- Character device registration using `cdev`
- Read and write support
- File position handling using `llseek`
- User-space data transfer via `copy_to_user()` and `copy_from_user()`
- Device class and node creation
- Internal kernel memory buffer

## Project Structure

```text
.
├── main.c        # Character driver implementation
├── Makefile      # Kernel module build rules
└── README.md
```

## Driver Flow

Initialization sequence:

1. Allocate device number
2. Initialize `cdev`
3. Register device with VFS
4. Create device class
5. Create device node

Cleanup sequence:

1. Destroy device
2. Destroy class
3. Remove `cdev`
4. Release device number

## Build

Compile:

```bash
make
```

Generated artifacts:

- `main.ko` → loadable kernel module
- `main.o`, `.mod`, `.cmd` → build artifacts

## Load Driver

Insert module:

```bash
sudo insmod main.ko
```

Verify:

```bash
dmesg | tail
```

Check device creation:

```bash
ls /dev/PCD
```

View allocated device numbers:

```bash
cat /proc/devices
```

## Device Interaction

Write data:

```bash
echo "hello kernel" > /dev/PCD
```

Read data:

```bash
cat /dev/PCD
```

Or test manually:

```bash
echo "sample data" | sudo tee /dev/PCD
sudo cat /dev/PCD
```

## Check Kernel Logs

```bash
dmesg | tail -30
```

Logs show:

- bytes requested
- file offset
- read/write activity
- updated positions

## Remove Driver

Unload:

```bash
sudo rmmod main
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

This project provides a foundation for exploring Linux character drivers, device registration flow, file operations, and interactions between kernel space and user space.
