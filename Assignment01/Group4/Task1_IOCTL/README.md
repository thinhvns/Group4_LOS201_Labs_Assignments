# Assignment 01 - Task 1: IOCTL Character Device Driver

## Overview

This project implements a Linux Character Device Driver
with IOCTL support for Embedded Linux (LOS201).

## Environment

- Linux Kernel: 5.15.0
- Architecture: ARMv7
- Emulator: QEMU vexpress-a9
- Cross Compiler: arm-linux-gnueabihf-gcc
- Programming Language: C

## Features

The driver supports four IOCTL commands:

1. ASGN1_RESET_BUFFER - Clear the device buffer.
2. ASGN1_GET_STATS - Retrieve driver statistics.
3. ASGN1_SET_MODE - Set read-only or read/write mode.
4. ASGN1_GET_VERSION - Retrieve driver version.

## Files

- asgn1_driver.c: Linux kernel module source code.
- asgn1_ioctl.h: IOCTL command definitions.
- test_ioctl.c: User-space test program.
- Makefile: Kernel module build configuration.

## Build

Build the kernel module:

```bash
make
```

Build the ARM test application:

```bash
arm-linux-gnueabihf-gcc -static -Wall -Wextra \
-o test_ioctl test_ioctl.c
```

The Makefile uses the local Linux 5.15 kernel source
at ~/embedded_lab1/kernel/linux-5.15.

## Testing

The driver was tested on QEMU ARM Cortex-A9.

Test results:

- GET_VERSION: PASS
- GET_STATS: PASS
- SET_MODE (Read-only): PASS
- Read-only protection: PASS
- SET_MODE (Read/Write): PASS
- Read/Write permission: PASS
- RESET_BUFFER: PASS
- Invalid mode protection: PASS

## Author

Nguyen Xuan Phu

FPT University - LOS201 Embedded Linux
