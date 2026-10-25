/*
 * File: sensor_thread.c
 * Purpose: Read sensor samples from the SMS kernel driver.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#define _POSIX_C_SOURCE 200809L

#include "sensor_thread.h"
#include "../driver/sms_sensor.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>


#define SENSOR_LINE_SIZE 128
#define NS_PER_SECOND 1000000000LL
#define NS_PER_MICROSECOND 1000.0L

struct jitter_stats {
    uint64_t count;
    int64_t min_ns;
    int64_t max_ns;
    long double mean_ns;
    long double m2_ns;
};

static void add_milliseconds(
    struct timespec *value,
    uint32_t milliseconds)
{
    value->tv_sec += milliseconds / 1000U;
    value->tv_nsec +=
        (long)(milliseconds % 1000U) * 1000000L;

    if (value->tv_nsec >= NS_PER_SECOND) {
        value->tv_sec++;
        value->tv_nsec -= NS_PER_SECOND;
    }
}

static int64_t difference_nanoseconds(
    const struct timespec *actual,
    const struct timespec *expected)
{
    int64_t seconds =
        (int64_t)actual->tv_sec - (int64_t)expected->tv_sec;
    int64_t nanoseconds =
        (int64_t)actual->tv_nsec - (int64_t)expected->tv_nsec;

    return seconds * NS_PER_SECOND + nanoseconds;
}

static void update_jitter_stats(
    struct jitter_stats *stats,
    int64_t jitter_ns)
{
    long double delta;
    long double delta_after_update;

    if (stats->count == 0) {
        stats->min_ns = jitter_ns;
        stats->max_ns = jitter_ns;
    } else {
        if (jitter_ns < stats->min_ns)
            stats->min_ns = jitter_ns;

        if (jitter_ns > stats->max_ns)
            stats->max_ns = jitter_ns;
    }

    stats->count++;
    delta = (long double)jitter_ns - stats->mean_ns;
    stats->mean_ns += delta / (long double)stats->count;
    delta_after_update =
        (long double)jitter_ns - stats->mean_ns;
    stats->m2_ns += delta * delta_after_update;
}

static void print_jitter_stats(
    const char *label,
    const struct jitter_stats *stats)
{
    long double stddev_ns = 0.0L;

    if (stats->count == 0) {
        printf("[Timing] %s: no samples collected\n", label);
        return;
    }

    if (stats->count > 1) {
        stddev_ns = sqrtl(
            stats->m2_ns / (long double)(stats->count - 1));
    }

    printf(
        "[Timing] %s: samples=%" PRIu64
        ", jitter min=%.3Lf us"
        ", max=%.3Lf us"
        ", mean=%.3Lf us"
        ", stddev=%.3Lf us\n",
        label,
        stats->count,
        (long double)stats->min_ns / NS_PER_MICROSECOND,
        (long double)stats->max_ns / NS_PER_MICROSECOND,
        stats->mean_ns / NS_PER_MICROSECOND,
        stddev_ns / NS_PER_MICROSECOND
    );
}
/*
 * Send a warning message to the Logger Thread.
 */
static void sensor_log_warning(
    mqd_t log_queue,
    const char *message)
{
    sms_log_message_t log_message;

    if (log_queue == (mqd_t)-1 || message == NULL) {
        return;
    }

    memset(&log_message, 0, sizeof(log_message));

    log_message.type = SMS_LOG_WARNING;

    (void)snprintf(
        log_message.text,
        sizeof(log_message.text),
        "%s",
        message
    );

    if (mq_send(
            log_queue,
            (const char *)&log_message,
            sizeof(log_message),
            0) == -1) {
        fprintf(
            stderr,
            "[Sensor] Cannot enqueue warning: %s\n",
            strerror(errno)
        );
    }
}
/*
 * Parse one CSV record:
 *
 * timestamp_ms,temperature,humidity
 *
 * Example:
 * 123456789,25.3,67.8
 *
 * Temperature and humidity are converted to integer tenths.
 */
static int parse_sensor_sample(
    const char *line,
    sensor_sample_t *sample)
{
    unsigned long long timestamp;
    double temperature;
    double humidity;
    char extra;
    int fields;

    if (line == NULL || sample == NULL) {
        return EINVAL;
    }

    fields = sscanf(
        line,
        "%llu,%lf,%lf %c",
        &timestamp,
        &temperature,
        &humidity,
        &extra
    );

    if (fields != 3) {
        return EINVAL;
    }

    if (temperature < 0.0 || temperature > 100.0 ||
        humidity < 0.0 || humidity > 100.0) {
        return ERANGE;
    }

    sample->timestamp_ms = (uint64_t)timestamp;
    sample->temperature_tenths =
        (int32_t)(temperature * 10.0 + 0.5);
    sample->humidity_tenths =
        (int32_t)(humidity * 10.0 + 0.5);

    return 0;
}

/*
 * Sensor acquisition thread.
 *
 * Opens the device, configures the sampling interval, reads CSV
 * samples, converts them to sensor_sample_t, and pushes them into
 * the circular buffer.
 */
void *sensor_thread(void *arg)
{
    sensor_thread_args_t *args = arg;
    const char *device_path;
    uint32_t interval_ms;
    int fd = -1;
    int ret;
    int sleep_ret;
    int64_t jitter_ns;
    char line[SENSOR_LINE_SIZE];
    struct timespec next_deadline;
    struct timespec actual_time;
    struct jitter_stats timing = {0};

    if (args == NULL ||
        args->buffer == NULL ||
        args->stop_requested == NULL) {
        fprintf(stderr, "[Sensor] Invalid thread arguments\n");
        return NULL;
    }

    device_path = args->device_path != NULL
        ? args->device_path
        : SMS_DEVICE_NAME;

    interval_ms = args->interval_ms != 0
        ? args->interval_ms
        : SMS_DEFAULT_INTERVAL_MS;

    fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        fprintf(
            stderr,
            "[Sensor] Cannot open %s: %s\n",
            device_path,
            strerror(errno)
        );
        return NULL;
    }

    /*
     * Configure the driver sampling interval.
     */
    if (ioctl(fd, SMS_SET_INTERVAL, &interval_ms) < 0) {
        fprintf(
            stderr,
            "[Sensor] SMS_SET_INTERVAL failed: %s\n",
            strerror(errno)
        );
        close(fd);
        return NULL;
    }

    printf(
        "[Sensor] Opened %s; interval=%" PRIu32 " ms\n",
        device_path,
        interval_ms
    );

    /*
     * Schedule samples against absolute CLOCK_MONOTONIC deadlines.
     * The first sample is scheduled immediately.
     */
    if (clock_gettime(CLOCK_MONOTONIC, &next_deadline) == -1) {
        fprintf(
            stderr,
            "[Sensor] clock_gettime failed: %s\n",
            strerror(errno)
        );
        close(fd);
        return NULL;
    }

    while (!atomic_load(args->stop_requested)) {
        ssize_t bytes_read;
        sensor_sample_t sample;

        /*
         * TIMER_ABSTIME avoids cumulative drift from repeated
         * relative sleeps. clock_nanosleep returns an error number.
         */
        do {
            sleep_ret = clock_nanosleep(
                CLOCK_MONOTONIC,
                TIMER_ABSTIME,
                &next_deadline,
                NULL
            );
        } while (sleep_ret == EINTR &&
                 !atomic_load(args->stop_requested));

        if (sleep_ret != 0) {
            if (sleep_ret == EINTR &&
                atomic_load(args->stop_requested)) {
                break;
            }

            fprintf(
                stderr,
                "[Sensor] clock_nanosleep failed: %s\n",
                strerror(sleep_ret)
            );
            break;
        }

        if (atomic_load(args->stop_requested))
            break;

        bytes_read = read(fd, line, sizeof(line) - 1);

        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }

            fprintf(
                stderr,
                "[Sensor] read() failed: %s\n",
                strerror(errno)
            );
            break;
        }

        if (bytes_read == 0) {
            /*
             * No record was returned. Avoid a busy loop.
             */
            break;
        }

        /*
         * Jitter is the difference between the time read() returns
         * and the scheduled absolute deadline. Positive values mean
         * the sample arrived late.
         */
        if (clock_gettime(CLOCK_MONOTONIC, &actual_time) == -1) {
            fprintf(
                stderr,
                "[Sensor] clock_gettime failed: %s\n",
                strerror(errno)
            );
            break;
        }

        jitter_ns = difference_nanoseconds(
            &actual_time,
            &next_deadline
        );
        update_jitter_stats(&timing, jitter_ns);

        if (timing.count % 1000U == 0U)
            print_jitter_stats("1000-sample report", &timing);

        /*
         * Rebase the next absolute deadline on the actual sample
         * completion time. The driver also enforces the minimum
         * interval, so this avoids stacking two full-period waits.
         */
        next_deadline = actual_time;
        add_milliseconds(&next_deadline, interval_ms);

        line[bytes_read] = '\0';

        ret = parse_sensor_sample(line, &sample);
        if (ret != 0) {
            fprintf(
                stderr,
                "[Sensor] Invalid CSV record: %s\n",
                line
            );
            continue;
        }

        ret = circular_buffer_try_push(args->buffer, &sample);

if (ret == EAGAIN) {
    fprintf(
        stderr,
        "[Sensor] WARNING: Circular buffer full; "
        "dropping sample at timestamp=%" PRIu64 " ms\n",
        sample.timestamp_ms
    );

    sensor_log_warning(
        args->log_queue,
        "Circular buffer full; sensor sample dropped"
    );

    continue;
}

if (ret != 0) {
    if (ret != ECANCELED) {
        fprintf(
            stderr,
            "[Sensor] Buffer push failed: %s\n",
            strerror(ret)
        );
    }

    break;
}

        printf(
            "[Sensor] t=%" PRIu64
            " ms, temperature=%d.%d C, humidity=%d.%d%%\n",
            sample.timestamp_ms,
            sample.temperature_tenths / 10,
            abs(sample.temperature_tenths % 10),
            sample.humidity_tenths / 10,
            abs(sample.humidity_tenths % 10)
        );
    }

    print_jitter_stats("final report", &timing);

    close(fd);

    printf("[Sensor] Thread stopped\n");

    return NULL;
}
