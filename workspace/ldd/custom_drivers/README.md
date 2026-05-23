# Kernel Workbench – Hello World Kernel Module

A simple Linux kernel module demonstrating module initialization and cleanup flow. The module prints a message when inserted into the kernel and another when removed.

This project is a minimal starting point for exploring Linux kernel module development and build flow.

## Features

- Basic loadable kernel module
- Uses `module_init()` and `module_exit()`
- Kernel logging using `pr_info()`
- Makefile-based kernel module build
- Raspberry Pi 5 board metadata included

## Project Structure

```text
.
├── main.c        # Kernel module source
├── Makefile      # Kernel module build rules
└── README.md
```

## Build

Compile the module:

```bash
make
```

Generated files:

- `main.ko` → loadable kernel module
- `main.o`, `.mod`, `.cmd` → build artifacts

## Load Module

Insert into kernel:

```bash
sudo insmod main.ko
```

Check kernel logs:

```bash
dmesg | tail
```

Expected output:

```text
Hello World
```

## Remove Module

Unload:

```bash
sudo rmmod main
```

Check logs again:

```bash
dmesg | tail
```

Expected output:

```text
GoodBye
```

## Clean Build Artifacts

```bash
make clean
```

## Notes

This project serves as a minimal foundation for experimenting with Linux kernel development and extending toward driver development and kernel internals exploration.
