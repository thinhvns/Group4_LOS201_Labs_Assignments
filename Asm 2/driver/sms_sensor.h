/*
 * File: sms_sensor.h
 * Purpose: Shared interface between SMS sensor kernel driver
 *          and userspace application.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define SMS_DEVICE_NAME "/dev/sms_sensor"
#define SMS_PROC_NAME   "sms_stats"

#define SMS_DEFAULT_INTERVAL_MS 100U

#define SMS_IOCTL_MAGIC 'S'

/*
 * Set minimum interval between two successful sensor reads.
 * Argument: pointer to a 32-bit unsigned integer (milliseconds).
 */
#define SMS_SET_INTERVAL \
    _IOW(SMS_IOCTL_MAGIC, 1, __u32)

#endif /* SMS_SENSOR_H */
