/*
 * File: logger_thread.c
 * Purpose: Receive log messages and write them to a rotating log file.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#define _POSIX_C_SOURCE 200809L

#include "logger_thread.h"
#include "sms_ipc.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LOGGER_RECEIVE_TIMEOUT_SEC 1

/*
 * Return a readable label for each log message type.
 */
static const char *log_type_name(sms_log_type_t type)
{
    switch (type) {
    case SMS_LOG_SAMPLE:
        return "SAMPLE";
    case SMS_LOG_WARNING:
        return "WARNING";
    case SMS_LOG_ERROR:
        return "ERROR";
    case SMS_LOG_SHUTDOWN:
        return "SHUTDOWN";
    default:
        return "UNKNOWN";
    }
}

/*
 * Rotate the current log file to <path>.1.
 * The previous backup is replaced.
 */
static int rotate_log_file(FILE **log_file, const char *path)
{
    char backup_path[512];
    struct stat file_stat;

    if (log_file == NULL || *log_file == NULL || path == NULL) {
        return EINVAL;
    }

    if (snprintf(
            backup_path,
            sizeof(backup_path),
            "%s.1",
            path) >= (int)sizeof(backup_path)) {
        return ENAMETOOLONG;
    }

    if (fflush(*log_file) != 0) {
        return errno != 0 ? errno : EIO;
    }

    if (fclose(*log_file) != 0) {
        *log_file = NULL;
        return errno != 0 ? errno : EIO;
    }
    *log_file = NULL;

    if (rename(path, backup_path) != 0 && errno != ENOENT) {
        return errno;
    }

    *log_file = fopen(path, "a");
    if (*log_file == NULL) {
        return errno != 0 ? errno : EIO;
    }

    if (stat(path, &file_stat) == 0) {
        fprintf(
            stderr,
            "[Logger] Log rotated: %s -> %s\n",
            path,
            backup_path);
    }

    return 0;
}

/*
 * Write one message to the log file.
 */
static int write_log_message(
    FILE **log_file,
    const char *path,
    const sms_log_message_t *message)
{
    struct stat file_stat;
    time_t now;
    struct tm local_time;
    char time_text[32];

    if (log_file == NULL || *log_file == NULL ||
        path == NULL || message == NULL) {
        return EINVAL;
    }

    now = time(NULL);

    if (localtime_r(&now, &local_time) == NULL ||
        strftime(
            time_text,
            sizeof(time_text),
            "%Y-%m-%d %H:%M:%S",
            &local_time) == 0) {
        (void)snprintf(time_text, sizeof(time_text), "unknown-time");
    }

    if (message->type == SMS_LOG_SAMPLE) {
        const sms_processed_sample_t *sample = &message->sample;

        if (fprintf(
                *log_file,
                "[%s] SAMPLE timestamp_ms=%" PRIu64
                " temperature=%d.%dC humidity=%d.%d%%"
                " avg_temperature=%d.%dC avg_humidity=%d.%d%%"
                " temperature_delta=%d.%dC\n",
                time_text,
                sample->timestamp_ms,
                sample->temperature_tenths / 10,
                sample->temperature_tenths % 10 < 0
                    ? -(sample->temperature_tenths % 10)
                    : sample->temperature_tenths % 10,
                sample->humidity_tenths / 10,
                sample->humidity_tenths % 10 < 0
                    ? -(sample->humidity_tenths % 10)
                    : sample->humidity_tenths % 10,
                sample->average_temperature_tenths / 10,
                sample->average_temperature_tenths % 10 < 0
                    ? -(sample->average_temperature_tenths % 10)
                    : sample->average_temperature_tenths % 10,
                sample->average_humidity_tenths / 10,
                sample->average_humidity_tenths % 10 < 0
                    ? -(sample->average_humidity_tenths % 10)
                    : sample->average_humidity_tenths % 10,
                sample->temperature_delta_tenths / 10,
                sample->temperature_delta_tenths % 10 < 0
                    ? -(sample->temperature_delta_tenths % 10)
                    : sample->temperature_delta_tenths % 10) < 0) {
            return errno != 0 ? errno : EIO;
        }
    } else {
        if (fprintf(
                *log_file,
                "[%s] %s: %s\n",
                time_text,
                log_type_name(message->type),
                message->text) < 0) {
            return errno != 0 ? errno : EIO;
        }
    }

    if (fstat(fileno(*log_file), &file_stat) != 0) {
        return errno != 0 ? errno : EIO;
    }

    /*
     * Include buffered data when checking the rotation threshold.
     */
    if (fflush(*log_file) != 0) {
        return errno != 0 ? errno : EIO;
    }

    if ((unsigned long long)file_stat.st_size >= SMS_LOG_MAX_SIZE) {
        return rotate_log_file(log_file, path);
    }

    return 0;
}

/*
 * Logger Thread entry point.
 */
void *logger_thread(void *arg)
{
    logger_thread_args_t *args = arg;
    const char *path;
    FILE *log_file;
    struct timespec next_flush;
    int running = 1;

    if (args == NULL || args->stop_requested == NULL ||
        args->log_queue == (mqd_t)-1) {
        fprintf(stderr, "[Logger] Invalid thread arguments\n");
        return NULL;
    }

    path = args->log_path != NULL
        ? args->log_path
        : SMS_LOG_DEFAULT_PATH;

    log_file = fopen(path, "a");
    if (log_file == NULL) {
        fprintf(
            stderr,
            "[Logger] Cannot open %s: %s\n",
            path,
            strerror(errno));
        return NULL;
    }

    if (clock_gettime(CLOCK_REALTIME, &next_flush) != 0) {
        fprintf(
            stderr,
            "[Logger] clock_gettime failed: %s\n",
            strerror(errno));
        fclose(log_file);
        return NULL;
    }

    next_flush.tv_sec += SMS_LOG_FLUSH_INTERVAL_SEC;

    fprintf(stderr, "[Logger] Writing logs to %s\n", path);

    while (running) {
        struct timespec deadline;
        sms_log_message_t message;
        ssize_t bytes_received;
        int write_result;

        if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
            fprintf(stderr, "[Logger] clock_gettime failed\n");
            break;
        }

        deadline.tv_sec += LOGGER_RECEIVE_TIMEOUT_SEC;

        bytes_received = mq_timedreceive(
            args->log_queue,
            (char *)&message,
            sizeof(message),
            NULL,
            &deadline);

        if (bytes_received >= 0) {
            if ((size_t)bytes_received != sizeof(message)) {
                fprintf(
                    stderr,
                    "[Logger] Invalid message size: %zd\n",
                    bytes_received);
            } else {
                message.text[sizeof(message.text) - 1] = '\0';

                write_result = write_log_message(
                    &log_file,
                    path,
                    &message);

                if (write_result != 0) {
                    fprintf(
                        stderr,
                        "[Logger] Failed to write message: %s\n",
                        strerror(write_result));
                }

                if (message.type == SMS_LOG_SHUTDOWN) {
                    running = 0;
                }
            }
        } else if (errno == ETIMEDOUT) {
            /*
             * During shutdown, continue draining the log queue.
             * Exit after the queue is empty and receive times out.
             */
            if (atomic_load(args->stop_requested)) {
                running = 0;
            }
        } else if (errno != EINTR) {
            fprintf(
                stderr,
                "[Logger] mq_timedreceive failed: %s\\n",
                strerror(errno));
            break;
        }

        if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
            break;
        }

        if (deadline.tv_sec >= next_flush.tv_sec) {
            if (fflush(log_file) != 0) {
                fprintf(
                    stderr,
                    "[Logger] fflush failed: %s\n",
                    strerror(errno));
            }

            next_flush = deadline;
            next_flush.tv_sec += SMS_LOG_FLUSH_INTERVAL_SEC;
        }

    }

    if (fflush(log_file) != 0) {
        fprintf(stderr, "[Logger] Final fflush failed: %s\n", strerror(errno));
    }

    if (fclose(log_file) != 0) {
        fprintf(stderr, "[Logger] fclose failed: %s\n", strerror(errno));
    }

    fprintf(stderr, "[Logger] Thread stopped\n");

    return NULL;
}
