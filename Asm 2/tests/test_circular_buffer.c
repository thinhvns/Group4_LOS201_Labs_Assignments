/*
 * File: test_circular_buffer.c
 * Purpose: Unit tests for the thread-safe circular buffer.
 * Author: Tran Nguyen Hai Dang - SE202009
 */

#include "../app/circular_buffer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_CAPACITY CIRCULAR_BUFFER_CAPACITY

static int check(int condition, const char *description)
{
    if (!condition) {
        fprintf(stderr, "[FAIL] %s\n", description);
        return -1;
    }

    printf("[PASS] %s\n", description);
    return 0;
}

int main(void)
{
    circular_buffer_t buffer;
    sensor_sample_t input;
    sensor_sample_t output;
    int failed = 0;
    int ret;

    printf("=== Circular Buffer Unit Tests ===\n");

    /* Test 1: Initialize buffer. */
    ret = circular_buffer_init(&buffer);
    if (check(ret == 0, "Initialize circular buffer") != 0) {
        return EXIT_FAILURE;
    }

    /* Test 2: Push and pop one sample. */
    input.timestamp_ms = 1000;
    input.temperature_tenths = 324;
    input.humidity_tenths = 738;

    ret = circular_buffer_push(&buffer, &input);
    failed |= check(ret == 0, "Push one sample") != 0;

    ret = circular_buffer_pop(&buffer, &output);
    failed |= check(ret == 0, "Pop one sample") != 0;

    if (ret == 0) {
        failed |= check(
            output.timestamp_ms == 1000 &&
            output.temperature_tenths == 324 &&
            output.humidity_tenths == 738,
            "Preserve sample data"
        ) != 0;
    }

    /* Test 3: Verify FIFO ordering. */
    for (int i = 0; i < 3; i++) {
        input.timestamp_ms = (uint64_t)(2000 + i);
        input.temperature_tenths = 300 + i;
        input.humidity_tenths = 600 + i;

        ret = circular_buffer_push(&buffer, &input);
        if (ret != 0) {
            failed |= check(0, "Push FIFO test sample") != 0;
            break;
        }
    }

    for (int i = 0; i < 3; i++) {
        ret = circular_buffer_pop(&buffer, &output);

        if (ret != 0) {
            failed |= check(0, "Pop FIFO test sample") != 0;
            break;
        }

        char description[80];

        (void)snprintf(
            description,
            sizeof(description),
            "FIFO order: sample %d",
            i + 1
        );

        failed |= check(
            output.timestamp_ms == (uint64_t)(2000 + i) &&
            output.temperature_tenths == 300 + i &&
            output.humidity_tenths == 600 + i,
            description
        ) != 0;
    }

    /*
     * Test 4: Fill the buffer to capacity, then drain it.
     * This tests index wraparound and buffer capacity.
     */
        /*
     * Test 4: Verify buffer capacity and non-blocking push.
     */
    for (int i = 0; i < TEST_CAPACITY; i++) {
        input.timestamp_ms = (uint64_t)i;
        input.temperature_tenths = 250;
        input.humidity_tenths = 600;

        ret = circular_buffer_push(&buffer, &input);

        if (ret != 0) {
            failed |= check(0, "Fill buffer to capacity") != 0;
            break;
        }
    }

    failed |= check(
        ret == 0,
        "Fill buffer to capacity"
    ) != 0;

    /*
     * A full buffer must reject the new sample immediately.
     */
    input.timestamp_ms = 999999;
    input.temperature_tenths = 400;
    input.humidity_tenths = 800;

    ret = circular_buffer_try_push(&buffer, &input);

    failed |= check(
        ret == EAGAIN,
        "Non-blocking push returns EAGAIN when buffer is full"
    ) != 0;

    /*
     * Drain the buffer and verify FIFO order.
     */
    for (int i = 0; i < TEST_CAPACITY; i++) {
        ret = circular_buffer_pop(&buffer, &output);

        if (ret != 0 || output.timestamp_ms != (uint64_t)i) {
            failed |= check(0, "Drain buffer in FIFO order") != 0;
            break;
        }
    }

    if (ret == 0) {
        failed |= check(
            output.timestamp_ms == (uint64_t)(TEST_CAPACITY - 1),
            "Drain buffer in FIFO order"
        ) != 0;
    }

    /*
     * Verify that try_push also rejects a closed buffer.
     */
    ret = circular_buffer_close(&buffer);
    failed |= check(ret == 0, "Close circular buffer") != 0;

    ret = circular_buffer_try_push(&buffer, &input);

    failed |= check(
        ret == ECANCELED,
        "Non-blocking push rejects a closed buffer"
    ) != 0;

        /* Test 5: A closed and empty buffer rejects further operations. */

    ret = circular_buffer_push(&buffer, &input);
    failed |= check(
        ret == ECANCELED,
        "Reject push after close"
    ) != 0;

    ret = circular_buffer_pop(&buffer, &output);
    failed |= check(
        ret == ECANCELED,
        "Return ECANCELED when closed buffer is empty"
    ) != 0;

    /* Test 6: Destroy synchronization objects. */
    ret = circular_buffer_destroy(&buffer);
    failed |= check(ret == 0, "Destroy circular buffer") != 0;

    if (failed) {
        printf("\n=== TESTS FAILED ===\n");
        return EXIT_FAILURE;
    }

    printf("\n=== ALL CIRCULAR BUFFER TESTS PASSED ===\n");
    return EXIT_SUCCESS;
}
