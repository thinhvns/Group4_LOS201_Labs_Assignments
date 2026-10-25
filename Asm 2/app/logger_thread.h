/*
 * File: logger_thread.h
 * Purpose: Logger Thread interface.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H

#include <mqueue.h>
#include <stdatomic.h>

#define SMS_LOG_DEFAULT_PATH "/tmp/sms.log"
#define SMS_LOG_MAX_SIZE (1024U * 1024U)
#define SMS_LOG_FLUSH_INTERVAL_SEC 5

typedef struct {
    mqd_t log_queue;
    atomic_bool *stop_requested;
    const char *log_path;
} logger_thread_args_t;

void *logger_thread(void *arg);

#endif /* LOGGER_THREAD_H */
