#!/bin/sh

# Assignment 01 - Task 1
# IOCTL Character Device Driver Test
# Nguyen Xuan Phu - SE203361
# Target: Linux 5.15 ARMv7

MODULE="/lib/modules/5.15.0/asgn1_driver.ko"
DEVICE="/dev/asgn1"
TEST_APP="/root/test_ioctl"

fail()
{
    echo "[FAIL] $1"
    exit 1
}

echo "======================================"
echo "ASSIGNMENT 01 - TASK 1 AUTOMATED TEST"
echo "======================================"

echo ""
echo "===== TEST 1: LOAD MODULE ====="

if grep -q '^asgn1_driver ' /proc/modules; then
    fail "Module already loaded. Reboot QEMU first."
fi

insmod "$MODULE" || fail "Cannot load module"

lsmod | grep asgn1_driver || fail "Module not loaded"

echo "[PASS] Module loaded"

echo ""
echo "===== TEST 2: CREATE DEVICE ====="

created_manual=0

if [ ! -c "$DEVICE" ]; then
    mknod "$DEVICE" c 241 0 || fail "Cannot create device"
    created_manual=1
fi

ls -l "$DEVICE" || fail "Device not found"

echo "[PASS] Device available"

echo ""
echo "===== TEST 3: IOCTL FUNCTIONS ====="

[ -x "$TEST_APP" ] || fail "Test application missing"

"$TEST_APP" || fail "IOCTL functional tests failed"

echo "[PASS] IOCTL functional tests"

echo ""
echo "===== TEST 4: PROCFS ====="

cat /proc/asgn1_info || fail "Cannot read procfs"

echo "[PASS] Procfs"

echo ""
echo "===== TEST 5: KERNEL LOG ====="

dmesg | grep 'asgn1:' | tail -40

echo ""
echo "===== TEST 6: UNLOAD MODULE ====="

rmmod asgn1_driver || fail "Cannot unload module"

if [ -e /proc/asgn1_info ]; then
    fail "Procfs entry still exists"
fi

if [ "$created_manual" -eq 1 ]; then
    rm -f "$DEVICE"
fi

echo "[PASS] Module unloaded"

echo ""
echo "===== FINAL KERNEL LOG ====="

dmesg | grep 'asgn1:' | tail -15

echo ""
echo "======================================"
echo "ALL ASGN1 DRIVER TESTS PASSED"
echo "======================================"
