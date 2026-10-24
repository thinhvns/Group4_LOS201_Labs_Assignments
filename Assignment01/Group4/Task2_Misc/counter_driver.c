/*
 * Assignment 01 - Task 2
 * Misc Device Driver - Counter Device
 *
 * Student: Nguyen Xuan Phu
 * Student ID: SE203361
 *
 * Target: Linux Kernel 5.15 ARMv7
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>

/* =====================================================
 * GLOBAL VARIABLES
 * ===================================================== */

static atomic_t counter = ATOMIC_INIT(0);
static atomic_t read_count = ATOMIC_INIT(0);
static atomic_t write_count = ATOMIC_INIT(0);

static DEFINE_MUTEX(counter_mutex);

static struct proc_dir_entry *counter_proc_entry;

/*
 * Per-open state.
 *
 * Keeps a stable string during partial reads.
 */
struct counter_file_state {
    char text[32];
    size_t length;
    bool initialized;
    bool incremented;
};

/* =====================================================
 * OPEN FUNCTION
 * ===================================================== */

/*
 * Allocate private state for each opened file.
 *
 * Return:
 *   0 on success
 *   -ENOMEM on allocation failure
 */
static int counter_open(struct inode *inode,
                        struct file *file)
{
    struct counter_file_state *state;

    state = kzalloc(sizeof(*state), GFP_KERNEL);

    if (!state)
        return -ENOMEM;

    file->private_data = state;

    return 0;
}

/* =====================================================
 * RELEASE FUNCTION
 * ===================================================== */

/*
 * Free the private state allocated in counter_open().
 *
 * Return:
 *   0 on success
 */
static int counter_release(struct inode *inode,
                           struct file *file)
{
    kfree(file->private_data);

    file->private_data = NULL;

    return 0;
}

/* =====================================================
 * READ FUNCTION
 * ===================================================== */

/*
 * Read the current counter value.
 *
 * The first successful read:
 *   - Returns the counter as a decimal string
 *   - Increments the counter
 *   - Increments the read statistics
 *
 * Subsequent reads of the same opened file return
 * the remaining bytes, then EOF.
 *
 * Return:
 *   Number of bytes read
 *   0 on EOF
 *   Negative errno on failure
 */
static ssize_t counter_read(struct file *file,
                            char __user *user_buf,
                            size_t count,
                            loff_t *ppos)
{
    struct counter_file_state *state;
    ssize_t ret;
    int value;

    state = file->private_data;

    if (!state)
        return -EINVAL;

    if (*ppos < 0)
        return -EINVAL;

    if (count == 0)
        return 0;

    mutex_lock(&counter_mutex);

    /*
     * Prepare the output only once per opened file.
     */
    if (!state->initialized) {

        if (*ppos != 0) {
            mutex_unlock(&counter_mutex);
            return 0;
        }

        value = atomic_read(&counter);

        state->length = scnprintf(
            state->text,
            sizeof(state->text),
            "%d\n",
            value
        );

        state->initialized = true;
    }

    /*
     * simple_read_from_buffer() handles *ppos
     * and returns EOF when the data is exhausted.
     */
    ret = simple_read_from_buffer(
        user_buf,
        count,
        ppos,
        state->text,
        state->length
    );

    /*
     * Increment the counter only once,
     * after data was successfully transferred.
     */
    if (ret > 0 && !state->incremented) {

        atomic_inc(&counter);

        atomic_inc(&read_count);

        state->incremented = true;
    }

    mutex_unlock(&counter_mutex);

    return ret;
}

/* =====================================================
 * WRITE FUNCTION
 * ===================================================== */

/*
 * Set the counter from an ASCII integer.
 *
 * Examples:
 *   echo 100 > /dev/counter
 *   echo 0 > /dev/counter
 *
 * Negative or invalid values return -EINVAL.
 *
 * Return:
 *   Number of bytes written
 *   Negative errno on failure
 */
static ssize_t counter_write(struct file *file,
                             const char __user *user_buf,
                             size_t count,
                             loff_t *ppos)
{
    char input[32];
    int value;
    int ret;

    if (count == 0)
        return 0;

    if (count >= sizeof(input))
        return -EINVAL;

    if (copy_from_user(input, user_buf, count))
        return -EFAULT;

    input[count] = '\0';

    /*
     * Convert ASCII input to an integer.
     */
    ret = kstrtoint(input, 10, &value);

    if (ret)
        return -EINVAL;

    /*
     * Reject negative counter values.
     */
    if (value < 0)
        return -EINVAL;

    mutex_lock(&counter_mutex);

    /*
     * Set the new counter value.
     */
    atomic_set(&counter, value);

    /*
     * Update successful write statistics.
     */
    atomic_inc(&write_count);

    mutex_unlock(&counter_mutex);

    *ppos += count;

    return count;
}

/* =====================================================
 * FILE OPERATIONS
 * ===================================================== */

static const struct file_operations counter_fops = {
    .owner   = THIS_MODULE,
    .open    = counter_open,
    .read    = counter_read,
    .write   = counter_write,
    .release = counter_release,
    .llseek  = no_llseek,
};

/* =====================================================
 * MISC DEVICE
 * ===================================================== */

static struct miscdevice counter_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "counter",
    .fops  = &counter_fops,
    .mode  = 0666,
};

/* =====================================================
 * PROCFS INFORMATION
 * ===================================================== */

/*
 * Display the current counter and statistics
 * through /proc/counter_info.
 *
 * Return:
 *   0 on success
 */
static int counter_proc_show(struct seq_file *m,
                             void *v)
{
    int value;
    int reads;
    int writes;

    mutex_lock(&counter_mutex);

    value = atomic_read(&counter);
    reads = atomic_read(&read_count);
    writes = atomic_read(&write_count);

    mutex_unlock(&counter_mutex);

    seq_printf(m, "Counter Device Information\n");
    seq_printf(m, "--------------------------\n");

    seq_printf(m, "Current counter: %d\n", value);
    seq_printf(m, "Total reads: %d\n", reads);
    seq_printf(m, "Total writes: %d\n", writes);

    return 0;
}

/*
 * Open /proc/counter_info using seq_file.
 *
 * Return:
 *   0 on success or negative errno
 */
static int counter_proc_open(struct inode *inode,
                             struct file *file)
{
    return single_open(
        file,
        counter_proc_show,
        NULL
    );
}

/*
 * proc_ops is used by Linux Kernel 5.15.
 */
static const struct proc_ops counter_proc_ops = {
    .proc_open    = counter_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* =====================================================
 * MODULE INITIALIZATION
 * ===================================================== */

/*
 * Register the misc device and procfs entry.
 *
 * Return:
 *   0 on success
 *   Negative errno on failure
 */
static int __init counter_init(void)
{
    int ret;

    atomic_set(&counter, 0);
    atomic_set(&read_count, 0);
    atomic_set(&write_count, 0);

    /*
     * Register /dev/counter.
     */
    ret = misc_register(&counter_miscdev);

    if (ret) {
        pr_err(
            "counter_driver: misc_register failed: %d\n",
            ret
        );

        return ret;
    }

    /*
     * Create /proc/counter_info.
     */
    counter_proc_entry = proc_create(
        "counter_info",
        0444,
        NULL,
        &counter_proc_ops
    );

    if (!counter_proc_entry) {

        pr_err(
            "counter_driver: proc_create failed\n"
        );

        misc_deregister(&counter_miscdev);

        return -ENOMEM;
    }

    pr_info(
        "counter_driver: module loaded successfully\n"
    );

    pr_info(
        "counter_driver: /dev/counter registered\n"
    );

    pr_info(
        "counter_driver: /proc/counter_info created\n"
    );

    return 0;
}

/* =====================================================
 * MODULE CLEANUP
 * ===================================================== */

/*
 * Remove procfs entry and unregister misc device.
 */
static void __exit counter_exit(void)
{
    proc_remove(counter_proc_entry);

    misc_deregister(&counter_miscdev);

    pr_info(
        "counter_driver: module unloaded successfully\n"
    );
}

/* =====================================================
 * MODULE INFORMATION
 * ===================================================== */

module_init(counter_init);
module_exit(counter_exit);

MODULE_LICENSE("GPL");

MODULE_AUTHOR(
    "Nguyen Xuan Phu - SE203361"
);

MODULE_DESCRIPTION(
    "Misc Counter Device Driver - Assignment 01"
);

MODULE_VERSION("1.0");
