# LAB-03: Kernel Debugging and Performance Analysis

## Student Information
- Name: Nguyen Xuan Phu
- Student ID: SE203361
- University: FPT University
- Course: LOS201 - Embedded Linux Systems

## Environment
- Host OS: Ubuntu Linux
- Target Kernel: Linux 5.15.0
- Target Architecture: ARMv7
- Emulator: QEMU vexpress-a9

## Part A - GDB Remote Debugging
Identified and fixed three bugs:
1. Array Out-of-Bounds
2. Use-After-Free
3. Integer Overflow

## Part B - Memory Analysis
Tools: Valgrind and /proc/self/status

Results:
- Before fixing: 16,900 bytes definitely lost
- After fixing: 0 bytes lost
- Analyzed VmRSS on ARM/QEMU

## Part C - Performance Profiling
Tools: strace and perf

- Analyzed system calls
- Identified CPU hotspots
- Compared Ubuntu Host and ARM/QEMU execution

## Part D - Real-Time Latency Analysis
Tools: cyclictest, stress-ng, gnuplot

Results:
- Normal Max Latency: 80 us
- Highest observed Max Latency under stress: 764 us
- Generated latency histograms
- Analyzed PREEMPT_RT theoretically

## Directory Structure
- apps/: Source code and compiled applications
- logs/: Debugging logs and performance results
- output/: QEMU initramfs
- bao_cao/: Laboratory report

## Status
LAB-03 practical tasks completed.
