# ASSIGNMENT 01 - EMBEDDED LINUX SYSTEMS

## Group 4 - FPT University

### Group Members

| Full Name | Student ID |
|-----------|------------|
| Duong Duc Thinh | SE203340 |
| Tran Nguyen Hai Dang | SE202009 |
| Nguyen Xuan Phu | SE203361 |
| Nguyen Ngo Anh Vu | SE203026 |

## Assignment Topic

How to Write a Device Driver

## Task 1: IOCTL Character Device Driver

- Implement four IOCTL commands.
- Support Kernel and Echo modes.
- Read/write operations.
- Driver statistics and buffer reset.

## Task 2: Misc Counter Device Driver

- Register miscellaneous device.
- Implement automatic counter increment.
- Support counter reset and update.
- Provide procfs statistics.

## Task 3: Technical Report

- System architecture.
- Driver comparison.
- Driver design analysis.
- Testing and debugging results.

## Task 4: Theory Questions

- Kernel module reference counting.
- Userspace and kernelspace memory access.
- MTD subsystem.
- File offset handling.

## Testing Environment

- Linux Kernel: 5.15.0
- Architecture: ARMv7
- Emulator: QEMU vexpress-a9

## Verification

The included source implementation and QEMU test evidence
were provided by Nguyen Xuan Phu (SE203361).

Both Task 1 and Task 2 passed their functional test suites.
