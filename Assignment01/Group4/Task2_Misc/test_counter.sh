#!/bin/sh

# Assignment 01 - Task 2
# Counter Device Driver Test
# Student: Nguyen Xuan Phu - SE203361
# Target: ARMv7 Linux 5.15

MODULE="/lib/modules/5.15.0/counter_driver.ko"

fail()
{
    echo "[FAIL] $1"
    exit 1
}

check_read()
{
    expected="$1"
    actual=$(cat /dev/counter) || fail "Cannot read counter"

    echo "Expected: $expected | Actual: $actual"

    if [ "$actual" != "$expected" ]; then
        fail "Counter read mismatch"
    fi

    echo "[PASS] Read counter"
}

echo "======================================"
echo "ASSIGNMENT 01 - TASK 2 TEST"
echo "COUNTER DEVICE DRIVER"
echo "======================================"

echo ""
echo "===== TEST 1: LOAD MODULE ====="

insmod "$MODULE" || fail "Cannot load module"

lsmod | grep counter_driver || fail "Module not loaded"

echo "[PASS] Module loaded"


echo ""
echo "===== TEST 2: CREATE DEVICE ====="

minor=$(awk '$2=="counter" {print $1}' /proc/misc)

[ -n "$minor" ] || fail "Counter not registered"

if [ ! -e /dev/counter ]; then
    mknod /dev/counter c 10 "$minor" ||
        fail "Cannot create device"
fi

ls -l /dev/counter

echo "[PASS] Device created"


echo ""
echo "===== TEST 3: INITIAL PROCFS ====="

cat /proc/counter_info ||
    fail "Cannot read procfs"


echo ""
echo "===== TEST 4: AUTO INCREMENT ====="

check_read 0
check_read 1
check_read 2


echo ""
echo "===== TEST 5: SET COUNTER = 100 ====="

echo 100 > /dev/counter ||
    fail "Cannot set counter"

echo "--- Procfs after write ---"
cat /proc/counter_info

check_read 100


echo ""
echo "===== TEST 6: RESET COUNTER ====="

echo 0 > /dev/counter ||
    fail "Cannot reset counter"

echo "--- Procfs after reset ---"
cat /proc/counter_info

check_read 0


echo ""
echo "===== TEST 7: INVALID INPUT ====="

if (echo -5 > /dev/counter) 2>/dev/null; then
    fail "Negative value accepted"
fi

echo "[PASS] Negative value rejected"

if (echo abc > /dev/counter) 2>/dev/null; then
    fail "Invalid string accepted"
fi

echo "[PASS] Invalid string rejected"


echo ""
echo "===== TEST 8: FINAL STATISTICS ====="

cat /proc/counter_info ||
    fail "Cannot read final statistics"

grep -q "Current counter: 1" /proc/counter_info ||
    fail "Incorrect final counter"

grep -q "Total reads: 5" /proc/counter_info ||
    fail "Incorrect read count"

grep -q "Total writes: 2" /proc/counter_info ||
    fail "Incorrect write count"

echo "[PASS] Final statistics"


echo ""
echo "===== TEST 9: UNLOAD MODULE ====="

rmmod counter_driver ||
    fail "Cannot unload module"

if [ -e /proc/counter_info ]; then
    fail "Procfs entry still exists"
fi

rm -f /dev/counter

echo "[PASS] Module unloaded"


echo ""
echo "===== KERNEL LOG ====="

dmesg | grep counter_driver | tail -15


echo ""
echo "======================================"
echo "ALL COUNTER DRIVER TESTS PASSED"
echo "======================================"
