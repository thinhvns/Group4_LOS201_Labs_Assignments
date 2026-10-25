/*
 * File: alert_thread.h
 * Purpose: Alert Thread interface.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#ifndef ALERT_THREAD_H
#define ALERT_THREAD_H

#include <mqueue.h>
#include <stdatomic.h>

#include "sms_ipc.h"

typedef struct {
    mqd_t alarm_queue;
    mqd_t log_queue;
    atomic_bool *stop_requested;
} alert_thread_args_t;

void *alert_thread(void *arg);

#endif /* ALERT_THREAD_H */
