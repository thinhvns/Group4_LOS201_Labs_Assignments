/*
 * File: circular_buffer.c
 * Purpose: Thread-safe FIFO circular buffer for sensor samples.
 * Author: Tran Nguyen Hai Dang - SE202009
 */

#include "circular_buffer.h"

#include <errno.h>
#include <string.h>

/*
 * Initialize the circular buffer and its synchronization objects.
 *
 * Return:
 *   0         - success
 *   EINVAL    - invalid argument
 *   Error code from pthread functions otherwise
 */
int circular_buffer_init(circular_buffer_t *buffer)
{
    int ret;

    if (buffer == NULL) {
        return EINVAL;
    }

    memset(buffer, 0, sizeof(*buffer));

    ret = pthread_mutex_init(&buffer->mutex, NULL);
    if (ret != 0) {
        return ret;
    }

    ret = pthread_cond_init(&buffer->not_empty, NULL);
    if (ret != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        return ret;
    }

    ret = pthread_cond_init(&buffer->not_full, NULL);
    if (ret != 0) {
        pthread_cond_destroy(&buffer->not_empty);
        pthread_mutex_destroy(&buffer->mutex);
        return ret;
    }

    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    buffer->closed = 0;
    buffer->initialized = 1;

    return 0;
}

/*
 * Insert one sample into the circular buffer.
 *
 * If the buffer is full, the producer waits until space becomes
 * available or the buffer is closed.
 */
int circular_buffer_push(
    circular_buffer_t *buffer,
    const sensor_sample_t *sample)
{
    int ret;

    if (buffer == NULL || sample == NULL || !buffer->initialized) {
        return EINVAL;
    }

    ret = pthread_mutex_lock(&buffer->mutex);
    if (ret != 0) {
        return ret;
    }

    while (buffer->count == CIRCULAR_BUFFER_CAPACITY &&
           !buffer->closed) {
        ret = pthread_cond_wait(&buffer->not_full, &buffer->mutex);

        if (ret != 0) {
            pthread_mutex_unlock(&buffer->mutex);
            return ret;
        }
    }

    if (buffer->closed) {
        pthread_mutex_unlock(&buffer->mutex);
        return ECANCELED;
    }

    buffer->data[buffer->tail] = *sample;

    buffer->tail =
        (buffer->tail + 1) % CIRCULAR_BUFFER_CAPACITY;

    buffer->count++;

    ret = pthread_cond_signal(&buffer->not_empty);
    if (ret != 0) {
        pthread_mutex_unlock(&buffer->mutex);
        return ret;
    }

    ret = pthread_mutex_unlock(&buffer->mutex);

    return ret;
}

int circular_buffer_try_push(
    circular_buffer_t *buffer,
    const sensor_sample_t *sample)
{
    int ret;

    if (buffer == NULL || sample == NULL || !buffer->initialized) {
        return EINVAL;
    }

    ret = pthread_mutex_lock(&buffer->mutex);
    if (ret != 0) {
        return ret;
    }

    if (buffer->closed) {
        pthread_mutex_unlock(&buffer->mutex);
        return ECANCELED;
    }

    if (buffer->count == CIRCULAR_BUFFER_CAPACITY) {
        pthread_mutex_unlock(&buffer->mutex);
        return EAGAIN;
    }

    buffer->data[buffer->tail] = *sample;
    buffer->tail =
        (buffer->tail + 1) % CIRCULAR_BUFFER_CAPACITY;
    buffer->count++;

    ret = pthread_cond_signal(&buffer->not_empty);
    if (ret != 0) {
        pthread_mutex_unlock(&buffer->mutex);
        return ret;
    }

    return pthread_mutex_unlock(&buffer->mutex);
}
/*
 * Remove one sample from the circular buffer.
 *
 * If the buffer is empty, the consumer waits until a sample becomes
 * available or the buffer is closed.
 *
 * After closure, queued samples can still be consumed.
 */
int circular_buffer_pop(
    circular_buffer_t *buffer,
    sensor_sample_t *sample)
{
    int ret;

    if (buffer == NULL || sample == NULL || !buffer->initialized) {
        return EINVAL;
    }

    ret = pthread_mutex_lock(&buffer->mutex);
    if (ret != 0) {
        return ret;
    }

    while (buffer->count == 0 && !buffer->closed) {
        ret = pthread_cond_wait(&buffer->not_empty, &buffer->mutex);

        if (ret != 0) {
            pthread_mutex_unlock(&buffer->mutex);
            return ret;
        }
    }

    /*
     * The buffer is closed and no samples remain.
     */
    if (buffer->count == 0 && buffer->closed) {
        pthread_mutex_unlock(&buffer->mutex);
        return ECANCELED;
    }

    *sample = buffer->data[buffer->head];

    buffer->head =
        (buffer->head + 1) % CIRCULAR_BUFFER_CAPACITY;

    buffer->count--;

    ret = pthread_cond_signal(&buffer->not_full);
    if (ret != 0) {
        pthread_mutex_unlock(&buffer->mutex);
        return ret;
    }

    ret = pthread_mutex_unlock(&buffer->mutex);

    return ret;
}

/*
 * Close the buffer and wake all waiting producers and consumers.
 *
 * Existing samples remain available for consumers.
 * New push operations are rejected.
 */
int circular_buffer_close(circular_buffer_t *buffer)
{
    int ret;
    int unlock_ret;

    if (buffer == NULL || !buffer->initialized) {
        return EINVAL;
    }

    ret = pthread_mutex_lock(&buffer->mutex);
    if (ret != 0) {
        return ret;
    }

    buffer->closed = 1;

    ret = pthread_cond_broadcast(&buffer->not_empty);
    if (ret != 0) {
        pthread_mutex_unlock(&buffer->mutex);
        return ret;
    }

    ret = pthread_cond_broadcast(&buffer->not_full);
    if (ret != 0) {
        pthread_mutex_unlock(&buffer->mutex);
        return ret;
    }

    unlock_ret = pthread_mutex_unlock(&buffer->mutex);

    return unlock_ret;
}

/*
 * Destroy synchronization objects.
 *
 * All threads using the buffer must have terminated before this
 * function is called.
 */
int circular_buffer_destroy(circular_buffer_t *buffer)
{
    int ret;

    if (buffer == NULL || !buffer->initialized) {
        return EINVAL;
    }

    /*
     * Mark the buffer closed before destroying its synchronization
     * objects. The caller must already have stopped all users.
     */
    ret = circular_buffer_close(buffer);
    if (ret != 0) {
        return ret;
    }

    ret = pthread_cond_destroy(&buffer->not_full);
    if (ret != 0) {
        return ret;
    }

    ret = pthread_cond_destroy(&buffer->not_empty);
    if (ret != 0) {
        return ret;
    }

    ret = pthread_mutex_destroy(&buffer->mutex);
    if (ret != 0) {
        return ret;
    }

    buffer->initialized = 0;

    return 0;
}
