# Embedded Linux Lab 01 - ARMv7

## Student
## Student
- DuongDucThinh - MSSV: SE203340
- TranNguyenHaiDang - MSSV: SE202009
- NguyenXuanPhu - MSSV: SE203361
- NguyenNgoAnhVu - MSSV: SE203026

## Lab
Embedded Linux - Kernel Configuration & Boot System

## Platform
- Architecture: ARMv7
- CPU: Cortex-A9
- Machine: QEMU vexpress-a9
- Linux Kernel: 5.15.0
- U-Boot: 2022.04
- BusyBox: 1.35.0

## Output
- `output/zImage` - Linux kernel
- `output/vexpress-v2p-ca9.dtb` - Device Tree Blob
- `output/u-boot` - U-Boot
- `output/initramfs.cpio.gz` - Initramfs
- `configs/kernel.config` - Kernel configuration
- `configs/busybox.config` - BusyBox configuration

## Boot Command

```bash
qemu-system-arm \
-M vexpress-a9 \
-cpu cortex-a9 \
-m 512M \
-smp 2 \
-nographic \
-kernel output/zImage \
-dtb output/vexpress-v2p-ca9.dtb \
-initrd output/initramfs.cpio.gz \
-append 'console=ttyAMA0,115200 rdinit=/sbin/init mem=512M'
