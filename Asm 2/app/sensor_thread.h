/*
 * File: sensor_thread.h
 * Purpose: Sensor acquisition thread interface.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

#include <mqueue.h>
#include <stdatomic.h>
#include <stdint.h>

#include "circular_buffer.h"
#include "sms_ipc.h"

/*
 * Arguments passed to the Sensor Thread.
 */
typedef struct {
    circular_buffer_t *buffer;
    const char *device_path;
    uint32_t interval_ms;
    atomic_bool *stop_requested;

    /* Non-blocking POSIX message queue descriptor for logging. */
    mqd_t log_queue;
} sensor_thread_args_t;

/*
 * Read sensor samples from the kernel driver and push them
 * into the circular buffer.
 */
void *sensor_thread(void *arg);

#endif /* SENSOR_THREAD_H */
