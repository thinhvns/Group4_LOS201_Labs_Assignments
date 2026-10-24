/*
 * Assignment 01 - Task 1
 * IOCTL Test Application
 *
 * Student: Nguyen Xuan Phu
 * Student ID: SE203361
 *
 * Target: ARMv7 Linux 5.15
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "asgn1_ioctl.h"

#define DEVICE "/dev/asgn1"

/*
 * Read the device and verify its contents.
 * Returns 0 on success, -1 on failure.
 */
static int check_read(const char *expected)
{
    int fd;
    ssize_t n;
    char buffer[256] = {0};

    fd = open(DEVICE, O_RDONLY);

    if (fd < 0) {
        perror("open for read");
        return -1;
    }

    n = read(fd, buffer, sizeof(buffer) - 1);

    close(fd);

    if (n < 0) {
        perror("read");
        return -1;
    }

    buffer[n] = '\0';

    printf("Expected: [%s]\n", expected);
    printf("Actual  : [%s]\n", buffer);

    if (strcmp(buffer, expected) != 0) {
        printf("[FAIL] Read mismatch\n");
        return -1;
    }

    printf("[PASS] Read result\n");

    return 0;
}

/*
 * Print driver statistics using GET_STATS.
 * Returns 0 on success, -1 on failure.
 */
static int show_stats(int fd)
{
    struct asgn1_stats stats;

    memset(&stats, 0, sizeof(stats));

    if (ioctl(fd, ASGN1_GET_STATS, &stats) < 0) {
        perror("GET_STATS");
        return -1;
    }

    printf("Open count      : %d\n", stats.open_count);
    printf("Write count     : %d\n", stats.write_count);
    printf("Read count      : %d\n", stats.read_count);
    printf("Buffer length   : %d\n", stats.buffer_len);
    printf("Last write size : %d\n", stats.last_write_size);

    return 0;
}

int main(void)
{
    int fd;
    int mode;
    char version[64] = {0};
    struct asgn1_stats stats;

    printf("====================================\n");
    printf("ASSIGNMENT 01 - TASK 1 IOCTL TEST\n");
    printf("Student: Nguyen Xuan Phu\n");
    printf("Student ID: SE203361\n");
    printf("====================================\n");

    fd = open(DEVICE, O_RDWR);

    if (fd < 0) {
        perror("open /dev/asgn1");
        return 1;
    }

    /* TEST 1: GET_VERSION */
    printf("\n===== TEST 1: GET_VERSION =====\n");

    if (ioctl(fd, ASGN1_GET_VERSION, version) < 0) {
        perror("GET_VERSION");
        goto fail;
    }

    printf("Version: %s\n", version);
    printf("[PASS] GET_VERSION\n");

    /* TEST 2: RESET_BUFFER */
    printf("\n===== TEST 2: INITIAL RESET =====\n");

    if (ioctl(fd, ASGN1_RESET_BUFFER) < 0) {
        perror("RESET_BUFFER");
        goto fail;
    }

    printf("[PASS] RESET_BUFFER\n");

    /* TEST 3: KERNEL MODE */
    printf("\n===== TEST 3: KERNEL MODE =====\n");

    mode = 0;

    if (ioctl(fd, ASGN1_SET_MODE, &mode) < 0) {
        perror("SET_MODE 0");
        goto fail;
    }

    printf("[PASS] SET_MODE = 0\n");

    if (write(fd, "Hello", 5) != 5) {
        perror("Kernel mode write");
        goto fail;
    }

    if (check_read("[KERNEL]Hello") < 0)
        goto fail;

    printf("[PASS] Kernel mode\n");

    /* TEST 4: ECHO MODE */
    printf("\n===== TEST 4: ECHO MODE =====\n");

    mode = 1;

    if (ioctl(fd, ASGN1_SET_MODE, &mode) < 0) {
        perror("SET_MODE 1");
        goto fail;
    }

    printf("[PASS] SET_MODE = 1\n");

    if (write(fd, "World", 5) != 5) {
        perror("Echo mode write");
        goto fail;
    }

    if (check_read("World") < 0)
        goto fail;

    printf("[PASS] Echo mode\n");

    /* TEST 5: GET_STATS */
    printf("\n===== TEST 5: GET_STATS =====\n");

    if (show_stats(fd) < 0)
        goto fail;

    memset(&stats, 0, sizeof(stats));

    if (ioctl(fd, ASGN1_GET_STATS, &stats) < 0)
        goto fail;

    if (stats.write_count != 2 ||
        stats.read_count != 2 ||
        stats.buffer_len != 5 ||
        stats.last_write_size != 5) {

        printf("[FAIL] Incorrect statistics\n");
        goto fail;
    }

    printf("[PASS] GET_STATS\n");

    /* TEST 6: RESET AND VERIFY */
    printf("\n===== TEST 6: RESET BUFFER =====\n");

    if (ioctl(fd, ASGN1_RESET_BUFFER) < 0) {
        perror("RESET_BUFFER");
        goto fail;
    }

    if (show_stats(fd) < 0)
        goto fail;

    memset(&stats, 0, sizeof(stats));

    if (ioctl(fd, ASGN1_GET_STATS, &stats) < 0)
        goto fail;

    if (stats.buffer_len != 0 ||
        stats.read_count != 0 ||
        stats.write_count != 0) {

        printf("[FAIL] Reset statistics\n");
        goto fail;
    }

    if (check_read("") < 0)
        goto fail;

    printf("[PASS] RESET_BUFFER\n");

    /* TEST 7: INVALID MODE */
    printf("\n===== TEST 7: INVALID MODE =====\n");

    mode = 2;
    errno = 0;

    if (ioctl(fd, ASGN1_SET_MODE, &mode) == -1 &&
        errno == EINVAL) {

        printf("[PASS] Invalid mode rejected\n");

    } else {

        printf("[FAIL] Invalid mode accepted\n");
        goto fail;
    }

    /* TEST 8: INVALID IOCTL */
    printf("\n===== TEST 8: INVALID IOCTL =====\n");

    errno = 0;

    if (ioctl(fd, _IO('Z', 99)) == -1 &&
        errno == ENOTTY) {

        printf("[PASS] Invalid ioctl rejected\n");

    } else {

        printf("[FAIL] Invalid ioctl handling\n");
        goto fail;
    }

    close(fd);

    printf("\n====================================\n");
    printf("ALL IOCTL TESTS PASSED\n");
    printf("====================================\n");

    return 0;

fail:
    printf("\n[FAIL] IOCTL test failed\n");

    close(fd);

    return 1;
}
