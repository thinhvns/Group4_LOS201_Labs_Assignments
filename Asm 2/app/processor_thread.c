/*
 * File: processor_thread.c
 * Purpose: Process sensor samples and detect alarm conditions.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#define _POSIX_C_SOURCE 200809L

#include "processor_thread.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*
 * Send a processed sample to the Logger Thread.
 * The queue descriptor must be non-blocking.
 */
static void send_log_message(
    mqd_t queue,
    const sms_processed_sample_t *sample)
{
    sms_log_message_t message;

    if (queue == (mqd_t)-1 || sample == NULL) {
        return;
    }

    memset(&message, 0, sizeof(message));

    message.type = SMS_LOG_SAMPLE;
    message.sample = *sample;

    if (mq_send(
            queue,
            (const char *)&message,
            sizeof(message),
            0) == -1) {
        if (errno == EAGAIN) {
            fprintf(
                stderr,
                "[Processor] Log queue full; dropping log message\n");
        } else {
            fprintf(
                stderr,
                "[Processor] Cannot send log message: errno=%d\n",
                errno);
        }
    }
}

/*
 * Send one alarm to the Alert Thread.
 * The queue descriptor must be non-blocking.
 */
static void send_alarm_message(
    mqd_t queue,
    sms_alarm_type_t type,
    const sms_processed_sample_t *sample)
{
    sms_alarm_message_t message;

    if (queue == (mqd_t)-1 || sample == NULL) {
        return;
    }

    memset(&message, 0, sizeof(message));

    message.type = type;
    message.sample = *sample;

    if (mq_send(
            queue,
            (const char *)&message,
            sizeof(message),
            0) == -1) {
        if (errno == EAGAIN) {
            fprintf(
                stderr,
                "[Processor] Alarm queue full; alarm dropped "
                "(type=%d)\n",
                (int)type);
        } else {
            fprintf(
                stderr,
                "[Processor] Cannot send alarm: errno=%d\n",
                errno);
        }
    }
}

/*
 * Calculate an integer rolling average.
 */
static int32_t calculate_average(
    const int32_t *values,
    size_t count)
{
    int64_t sum = 0;
    size_t i;

    for (i = 0; i < count; ++i) {
        sum += values[i];
    }

    return (int32_t)(sum / (int64_t)count);
}

/*
 * Processor Thread entry point.
 *
 * The thread drains the circular buffer until it is closed and
 * all remaining samples have been consumed.
 */
void *processor_thread(void *arg)
{
    processor_thread_args_t *args = arg;

    int32_t temperatures[SMS_ROLLING_WINDOW] = {0};
    int32_t humidities[SMS_ROLLING_WINDOW] = {0};

    size_t window_count = 0;
    size_t window_index = 0;

    int have_previous_temperature = 0;
    int32_t previous_temperature = 0;

    if (args == NULL || args->input_buffer == NULL) {
        fprintf(stderr, "[Processor] Invalid thread arguments\n");
        return NULL;
    }

    fprintf(
        stderr,
        "[Processor] Started; rolling window=%d samples\n",
        SMS_ROLLING_WINDOW);

    for (;;) {
        sensor_sample_t raw;
        sms_processed_sample_t processed;
        int ret;
        int64_t delta = 0;

        /*
         * pop() waits while the buffer is empty.
         * The application must close the buffer during shutdown
         * so this thread can finish after draining pending samples.
         */
        ret = circular_buffer_pop(args->input_buffer, &raw);

        if (ret == ECANCELED) {
            break;
        }

        if (ret != 0) {
            fprintf(
                stderr,
                "[Processor] Buffer pop failed: %s\n",
                strerror(ret));
            break;
        }

        temperatures[window_index] = raw.temperature_tenths;
        humidities[window_index] = raw.humidity_tenths;

        window_index = (window_index + 1U) % SMS_ROLLING_WINDOW;

        if (window_count < SMS_ROLLING_WINDOW) {
            ++window_count;
        }

        memset(&processed, 0, sizeof(processed));

        processed.timestamp_ms = raw.timestamp_ms;
        processed.temperature_tenths = raw.temperature_tenths;
        processed.humidity_tenths = raw.humidity_tenths;

        processed.average_temperature_tenths =
            calculate_average(temperatures, window_count);

        processed.average_humidity_tenths =
            calculate_average(humidities, window_count);

        /*
         * Detect temperature change relative to the immediately
         * preceding sample. The first sample has no delta alarm.
         */
        if (have_previous_temperature) {
            delta = (int64_t)raw.temperature_tenths
                    - previous_temperature;

            if (delta > INT32_MAX) {
                delta = INT32_MAX;
            } else if (delta < INT32_MIN) {
                delta = INT32_MIN;
            }
        }

        processed.temperature_delta_tenths = (int32_t)delta;

        /*
         * Publish every processed sample to the logger.
         */
        send_log_message(args->log_queue, &processed);

        /*
         * Alarm thresholds use strict greater-than comparisons.
         */
        if (raw.temperature_tenths > args->temperature_threshold_tenths) {            send_alarm_message(
                args->alarm_queue,
                SMS_ALARM_HIGH_TEMPERATURE,
                &processed);
        }

        if (raw.humidity_tenths > args->humidity_threshold_tenths) {
            send_alarm_message(
                args->alarm_queue,
                SMS_ALARM_HIGH_HUMIDITY,
                &processed);
        }

        if (have_previous_temperature &&
            (delta > SMS_TEMP_DELTA_THRESHOLD_TENTHS ||
             delta < -SMS_TEMP_DELTA_THRESHOLD_TENTHS)) {
            send_alarm_message(
                args->alarm_queue,
                SMS_ALARM_TEMP_CHANGE,
                &processed);
        }

        printf(
            "[Processor] timestamp=%" PRIu64
            " ms, T=%d.%d C, H=%d.%d%%, "
            "avg_T=%d.%d C, avg_H=%d.%d%%, delta_T=%d.%d C\n",
            processed.timestamp_ms,
            processed.temperature_tenths / 10,
            processed.temperature_tenths % 10 < 0
                ? -(processed.temperature_tenths % 10)
                : processed.temperature_tenths % 10,
            processed.humidity_tenths / 10,
            processed.humidity_tenths % 10 < 0
                ? -(processed.humidity_tenths % 10)
                : processed.humidity_tenths % 10,
            processed.average_temperature_tenths / 10,
            processed.average_temperature_tenths % 10 < 0
                ? -(processed.average_temperature_tenths % 10)
                : processed.average_temperature_tenths % 10,
            processed.average_humidity_tenths / 10,
            processed.average_humidity_tenths % 10 < 0
                ? -(processed.average_humidity_tenths % 10)
                : processed.average_humidity_tenths % 10,
            processed.temperature_delta_tenths / 10,
            processed.temperature_delta_tenths % 10 < 0
                ? -(processed.temperature_delta_tenths % 10)
                : processed.temperature_delta_tenths % 10);

        previous_temperature = raw.temperature_tenths;
        have_previous_temperature = 1;
    }

    fprintf(stderr, "[Processor] Thread stopped\n");

    return NULL;
}
