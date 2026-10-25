/*
 * File: sms_ipc.h
 * Purpose: Shared IPC message definitions for the SMS application.
 * Author: Tran Nguyen Hai Dang
 * Student ID: SE202009
 */

#ifndef SMS_IPC_H
#define SMS_IPC_H

#include <stdint.h>

/* POSIX message queue names. */
#define SMS_ALARM_QUEUE_NAME "/alarm_queue"
#define SMS_LOG_QUEUE_NAME   "/log_queue"

/* Rolling-average window size. */
#define SMS_ROLLING_WINDOW 10

/* Alert thresholds, represented in tenths. */
#define SMS_TEMP_THRESHOLD_TENTHS     400
#define SMS_HUMIDITY_THRESHOLD_TENTHS 800
#define SMS_TEMP_DELTA_THRESHOLD_TENTHS 50

/* Message types sent to the Logger Thread. */
typedef enum {
    SMS_LOG_SAMPLE = 1,
    SMS_LOG_WARNING,
    SMS_LOG_ERROR,
    SMS_LOG_SHUTDOWN
} sms_log_type_t;

/* Alarm categories sent to the Alert Thread. */
typedef enum {
    SMS_ALARM_HIGH_TEMPERATURE = 1,
    SMS_ALARM_HIGH_HUMIDITY,
    SMS_ALARM_TEMP_CHANGE
} sms_alarm_type_t;

/*
 * Processed sensor information.
 *
 * All temperature and humidity values use tenths:
 * 324 means 32.4 degrees Celsius.
 * 738 means 73.8 percent humidity.
 */
typedef struct {
    uint64_t timestamp_ms;

    int32_t temperature_tenths;
    int32_t humidity_tenths;

    int32_t average_temperature_tenths;
    int32_t average_humidity_tenths;

    int32_t temperature_delta_tenths;
} sms_processed_sample_t;

/* Message sent to the Alert Thread. */
typedef struct {
    sms_alarm_type_t type;
    sms_processed_sample_t sample;
} sms_alarm_message_t;

/* Message sent to the Logger Thread. */
typedef struct {
    sms_log_type_t type;
    sms_processed_sample_t sample;
    char text[128];
} sms_log_message_t;

#endif /* SMS_IPC_H */
