/*
 * File: alert_thread.c
 * Purpose: Receive and report sensor alarms.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#define _POSIX_C_SOURCE 200809L

#include "alert_thread.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define ALERT_RECEIVE_TIMEOUT_SEC 1

static const char *alarm_type_name(sms_alarm_type_t type)
{
    switch (type) {
    case SMS_ALARM_HIGH_TEMPERATURE:
        return "HIGH TEMPERATURE";
    case SMS_ALARM_HIGH_HUMIDITY:
        return "HIGH HUMIDITY";
    case SMS_ALARM_TEMP_CHANGE:
        return "RAPID TEMPERATURE CHANGE";
    default:
        return "UNKNOWN ALARM";
    }
}

static void report_alarm(
    const sms_alarm_message_t *alarm,
    mqd_t log_queue)
{
    const sms_processed_sample_t *sample = &alarm->sample;
    sms_log_message_t log_message;

    fprintf(
        stderr,
        "[Alert] %s | timestamp=%" PRIu64
        " ms | temperature=%d.%d C"
        " | humidity=%d.%d%%"
        " | delta_T=%d.%d C\n",
        alarm_type_name(alarm->type),
        sample->timestamp_ms,
        sample->temperature_tenths / 10,
        sample->temperature_tenths % 10 < 0
            ? -(sample->temperature_tenths % 10)
            : sample->temperature_tenths % 10,
        sample->humidity_tenths / 10,
        sample->humidity_tenths % 10 < 0
            ? -(sample->humidity_tenths % 10)
            : sample->humidity_tenths % 10,
        sample->temperature_delta_tenths / 10,
        sample->temperature_delta_tenths % 10 < 0
            ? -(sample->temperature_delta_tenths % 10)
            : sample->temperature_delta_tenths % 10);

    if (log_queue == (mqd_t)-1) {
        return;
    }

    memset(&log_message, 0, sizeof(log_message));

    log_message.type = SMS_LOG_WARNING;
    log_message.sample = *sample;

    (void)snprintf(
        log_message.text,
        sizeof(log_message.text),
        "Alarm: %s",
        alarm_type_name(alarm->type));

    if (mq_send(
            log_queue,
            (const char *)&log_message,
            sizeof(log_message),
            0) == -1) {
        if (errno == EAGAIN) {
            fprintf(
                stderr,
                "[Alert] Log queue full; alarm log dropped\n");
        } else {
            fprintf(
                stderr,
                "[Alert] Cannot enqueue alarm log: %s\n",
                strerror(errno));
        }
    }
}

void *alert_thread(void *arg)
{
    alert_thread_args_t *args = arg;

    if (args == NULL ||
        args->stop_requested == NULL ||
        args->alarm_queue == (mqd_t)-1) {
        fprintf(stderr, "[Alert] Invalid thread arguments\n");
        return NULL;
    }

    fprintf(stderr, "[Alert] Thread started\n");

    for (;;) {
        struct timespec deadline;
        sms_alarm_message_t alarm;
        ssize_t bytes_received;

        if (clock_gettime(CLOCK_REALTIME, &deadline) == -1) {
            fprintf(
                stderr,
                "[Alert] clock_gettime failed: %s\n",
                strerror(errno));
            break;
        }

        deadline.tv_sec += ALERT_RECEIVE_TIMEOUT_SEC;

        bytes_received = mq_timedreceive(
            args->alarm_queue,
            (char *)&alarm,
            sizeof(alarm),
            NULL,
            &deadline);

        if (bytes_received >= 0) {
            if ((size_t)bytes_received != sizeof(alarm)) {
                fprintf(
                    stderr,
                    "[Alert] Invalid message size: %zd\n",
                    bytes_received);
            } else {
                report_alarm(&alarm, args->log_queue);
            }

            continue;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == ETIMEDOUT) {
            if (atomic_load(args->stop_requested)) {
                break;
            }

            continue;
        }

        fprintf(
            stderr,
            "[Alert] mq_timedreceive failed: %s\n",
            strerror(errno));
        break;
    }

    fprintf(stderr, "[Alert] Thread stopped\n");

    return NULL;
}
