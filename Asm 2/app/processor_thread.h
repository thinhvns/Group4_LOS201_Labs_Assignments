/*
 * File: processor_thread.h
 * Purpose: Sensor data processing thread interface.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#ifndef PROCESSOR_THREAD_H
#define PROCESSOR_THREAD_H

#include <mqueue.h>
#include <stdatomic.h>

#include "circular_buffer.h"
#include "sms_ipc.h"

/*
 * Arguments passed to the Processor Thread.
 *
 * Both message queue descriptors must be opened with O_NONBLOCK
 * to prevent the processing thread from blocking on a full queue.
 */
typedef struct {
    circular_buffer_t *input_buffer;
    mqd_t alarm_queue;
    mqd_t log_queue;
    atomic_bool *stop_requested;
    int32_t temperature_threshold_tenths;
    int32_t humidity_threshold_tenths;
} processor_thread_args_t;

/*
 * Consume raw sensor samples, calculate rolling averages,
 * and publish processed samples and alarms.
 */
void *processor_thread(void *arg);

#endif /* PROCESSOR_THREAD_H */
