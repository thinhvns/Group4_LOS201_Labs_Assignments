#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "../driver/sms_sensor.h"

#define TEST_DEVICE SMS_DEVICE_NAME

static int test_interval(int fd, uint32_t interval_ms)
{
    if (ioctl(fd, SMS_SET_INTERVAL, &interval_ms) == 0) {
        printf("[PASS] Set interval to %u ms\n", interval_ms);
        return 0;
    }

    if (errno == EINVAL) {
        printf("[PASS] Invalid interval %u ms rejected (EINVAL)\n",
               interval_ms);
        return 0;
    }

    fprintf(stderr,
            "[FAIL] Interval %u ms: %s\n",
            interval_ms,
            strerror(errno));

    return -1;
}

int main(void)
{
    int fd;
    int failures = 0;

    printf("=== SMS Sensor ioctl Test ===\n");

    fd = open(TEST_DEVICE, O_RDONLY);

    if (fd < 0) {
        perror("[FAIL] open " TEST_DEVICE);
        return EXIT_FAILURE;
    }

    /* Valid interval: 200 ms */
    if (test_interval(fd, 200) < 0)
        failures++;

    /* Invalid interval: zero */
    if (test_interval(fd, 0) < 0)
        failures++;

    /* Invalid interval: above the supported maximum */
    if (test_interval(fd, 60001) < 0)
        failures++;

    /* Restore the default interval */
    if (test_interval(fd, SMS_DEFAULT_INTERVAL_MS) < 0)
        failures++;

    close(fd);

    if (failures == 0) {
        printf("=== ALL IOCTL TESTS PASSED ===\n");
        return EXIT_SUCCESS;
    }

    printf("=== %d IOCTL TEST(S) FAILED ===\n", failures);
    return EXIT_FAILURE;
}
