#!/bin/bash

set -e

PROJECT_DIR="$HOME/asm2_work/task1_work"
KERNEL_DIR="$HOME/embedded_lab1/kernel/linux-5.15"

KERNEL="$KERNEL_DIR/arch/arm/boot/zImage"
DTB="$KERNEL_DIR/arch/arm/boot/dts/vexpress-v2p-ca9.dtb"
INITRAMFS="$PROJECT_DIR/output/initramfs_asgn2_app.cpio.gz"

echo "=============================================="
echo " Assignment 02 - Embedded Linux"
echo " Student: Tran Nguyen Hai Dang"
echo " Student ID: SE202009"
echo "=============================================="

# Check required files
for file in "$KERNEL" "$DTB" "$INITRAMFS"; do
    if [ ! -f "$file" ]; then
        echo "[ERROR] File not found: $file"
        exit 1
    fi
done

# Check QEMU installation
if ! command -v qemu-system-arm >/dev/null 2>&1; then
    echo "[ERROR] qemu-system-arm is not installed."
    exit 1
fi

echo "[INFO] Kernel:    $KERNEL"
echo "[INFO] DTB:       $DTB"
echo "[INFO] Initramfs: $INITRAMFS"
echo "[INFO] Starting QEMU..."
echo "[INFO] Exit QEMU with Ctrl+A, then X."
echo "=============================================="

exec qemu-system-arm \
    -M vexpress-a9 \
    -m 512M \
    -smp 2 \
    -kernel "$KERNEL" \
    -dtb "$DTB" \
    -initrd "$INITRAMFS" \
    -append "console=ttyAMA0,115200 rdinit=/init mem=512M" \
    -nographic
