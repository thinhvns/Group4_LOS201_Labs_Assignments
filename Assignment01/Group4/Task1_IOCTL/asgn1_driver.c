/*
 * ASSIGNMENT 01 - TASK 1
 * Character Device Driver with IOCTL Interface
 *
 * Student: Nguyen Xuan Phu
 * Student ID: SE203361
 * University: FPT University
 *
 * Target: Linux Kernel 5.15 - ARMv7
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/string.h>

#include "asgn1_ioctl.h"

/* =====================================================
 * MODULE INFORMATION
 * ===================================================== */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Xuan Phu - SE203361");
MODULE_DESCRIPTION("Assignment 01 - Character Device with IOCTL");
MODULE_VERSION("1.0");

/* =====================================================
 * DEFINITIONS
 * ===================================================== */

#define DEVICE_NAME "asgn1"
#define CLASS_NAME "asgn1_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 241

#define MODE_KERNEL 0
#define MODE_ECHO   1

/* =====================================================
 * GLOBAL VARIABLES
 * ===================================================== */

static int major_number = DRIVER_MAJOR;

static char device_buffer[BUFFER_SIZE];

static int buffer_len = 0;

static int open_count = 0;
static int write_count = 0;
static int read_count = 0;
static int last_write_size = 0;

/* Default mode: Echo */
static int driver_mode = MODE_ECHO;

static struct class *asgn1_class;
static struct device *asgn1_device;

static struct proc_dir_entry *asgn1_proc_entry;

static DEFINE_MUTEX(asgn1_mutex);

/* =====================================================
 * OPEN FUNCTION
 * ===================================================== */

/*
 * Open character device.
 *
 * Returns:
 * 0 on success.
 */
static int asgn1_open(struct inode *inode,
                      struct file *file)
{
    int count;

    mutex_lock(&asgn1_mutex);

    open_count++;
    count = open_count;

    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1: device opened, count=%d\n", count);

    return 0;
}

/* =====================================================
 * RELEASE FUNCTION
 * ===================================================== */

/*
 * Close character device.
 *
 * Returns:
 * 0 on success.
 */
static int asgn1_release(struct inode *inode,
                         struct file *file)
{
    pr_info("asgn1: device closed\n");

    return 0;
}

/* =====================================================
 * READ FUNCTION
 * ===================================================== */

/*
 * Read data from driver buffer.
 *
 * Parameters:
 * file   - opened file
 * buf    - userspace destination
 * len    - requested bytes
 * offset - current read position
 *
 * Returns:
 * Number of bytes read, EOF (0), or negative errno.
 */
static ssize_t asgn1_read(struct file *file,
                          char __user *buf,
                          size_t len,
                          loff_t *offset)
{
    ssize_t bytes_read;

    mutex_lock(&asgn1_mutex);

    /* Validate file offset */
    if (*offset < 0) {
        mutex_unlock(&asgn1_mutex);
        return -EINVAL;
    }

    /* Return EOF if no data remains */
    if (*offset >= buffer_len) {
        mutex_unlock(&asgn1_mutex);
        return 0;
    }

    /* Limit read size to available data */
    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    /* Copy data from kernel to userspace */
    if (copy_to_user(buf,
                     device_buffer + *offset,
                     len)) {

        mutex_unlock(&asgn1_mutex);

        pr_err("asgn1: copy_to_user failed\n");

        return -EFAULT;
    }

    *offset += len;

    bytes_read = len;

    if (bytes_read > 0)
        read_count++;

    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1: read %zd bytes\n", bytes_read);

    return bytes_read;
}

/* =====================================================
 * WRITE FUNCTION
 * ===================================================== */

/*
 * Write data into driver buffer.
 *
 * MODE_KERNEL (0):
 * Automatically adds "[KERNEL]" prefix.
 *
 * MODE_ECHO (1):
 * Stores original data without modification.
 *
 * Returns:
 * Number of input bytes written or negative errno.
 */
static ssize_t asgn1_write(struct file *file,
                           const char __user *buf,
                           size_t len,
                           loff_t *offset)
{
    const char prefix[] = "[KERNEL]";

    size_t prefix_len;
    int current_mode;
    int current_buffer_len;

    if (len == 0)
        return 0;

    mutex_lock(&asgn1_mutex);

    /* Determine active mode */
    current_mode = driver_mode;

    if (current_mode == MODE_KERNEL)
        prefix_len = sizeof(prefix) - 1;
    else
        prefix_len = 0;

    /* Check buffer capacity */
    if (len > BUFFER_SIZE - 1 - prefix_len) {

        mutex_unlock(&asgn1_mutex);

        pr_err("asgn1: write exceeds buffer size\n");

        return -EMSGSIZE;
    }

    /* Add prefix in Kernel mode */
    if (current_mode == MODE_KERNEL) {

        memcpy(device_buffer,
               prefix,
               prefix_len);
    }

    /* Copy original data from userspace */
    if (copy_from_user(device_buffer + prefix_len,
                       buf,
                       len)) {

        mutex_unlock(&asgn1_mutex);

        pr_err("asgn1: copy_from_user failed\n");

        return -EFAULT;
    }

    /* Update buffer */
    buffer_len = prefix_len + len;

    device_buffer[buffer_len] = '\0';

    /* Update statistics */
    write_count++;

    last_write_size = len;

    current_buffer_len = buffer_len;

    mutex_unlock(&asgn1_mutex);

    pr_info(
        "asgn1: write success, mode=%d, length=%d\n",
        current_mode,
        current_buffer_len
    );

    return len;
}

/* =====================================================
 * IOCTL FUNCTION
 * ===================================================== */

/*
 * Handle device-specific IOCTL commands.
 *
 * Supported commands:
 *
 * ASGN1_RESET_BUFFER
 * ASGN1_GET_STATS
 * ASGN1_SET_MODE
 * ASGN1_GET_VERSION
 *
 * Returns:
 * 0 on success or negative errno.
 */
static long asgn1_ioctl(struct file *file,
                        unsigned int cmd,
                        unsigned long arg)
{
    struct asgn1_stats stats;

    int mode;

    char version[64] =
        "asgn1_driver v1.0 - SE203361";

    switch (cmd) {

    /* ---------------------------------------------
     * IOCTL 1: RESET BUFFER
     * --------------------------------------------- */

    case ASGN1_RESET_BUFFER:

        mutex_lock(&asgn1_mutex);

        memset(device_buffer, 0, BUFFER_SIZE);

        buffer_len = 0;

        /* Reset read/write counters */
        write_count = 0;
        read_count = 0;
        last_write_size = 0;

        mutex_unlock(&asgn1_mutex);

        pr_info("asgn1: RESET_BUFFER called\n");

        return 0;

    /* ---------------------------------------------
     * IOCTL 2: GET STATISTICS
     * --------------------------------------------- */

    case ASGN1_GET_STATS:

        mutex_lock(&asgn1_mutex);

        stats.open_count = open_count;
        stats.write_count = write_count;
        stats.read_count = read_count;
        stats.buffer_len = buffer_len;
        stats.last_write_size = last_write_size;

        mutex_unlock(&asgn1_mutex);

        if (copy_to_user((void __user *)arg,
                         &stats,
                         sizeof(stats))) {

            return -EFAULT;
        }

        pr_info("asgn1: GET_STATS called\n");

        return 0;

    /* ---------------------------------------------
     * IOCTL 3: SET MODE
     * --------------------------------------------- */

    case ASGN1_SET_MODE:

        if (copy_from_user(&mode,
                           (void __user *)arg,
                           sizeof(mode))) {

            return -EFAULT;
        }

        /* Only Mode 0 and Mode 1 are supported */
        if (mode != MODE_KERNEL &&
            mode != MODE_ECHO) {

            pr_warn("asgn1: invalid mode=%d\n", mode);

            return -EINVAL;
        }

        mutex_lock(&asgn1_mutex);

        driver_mode = mode;

        mutex_unlock(&asgn1_mutex);

        if (mode == MODE_KERNEL) {

            pr_info(
                "asgn1: SET_MODE = 0 (Kernel mode)\n"
            );

        } else {

            pr_info(
                "asgn1: SET_MODE = 1 (Echo mode)\n"
            );
        }

        return 0;

    /* ---------------------------------------------
     * IOCTL 4: GET VERSION
     * --------------------------------------------- */

    case ASGN1_GET_VERSION:

        if (copy_to_user((void __user *)arg,
                         version,
                         sizeof(version))) {

            return -EFAULT;
        }

        pr_info("asgn1: GET_VERSION called\n");

        return 0;

    /* ---------------------------------------------
     * INVALID IOCTL
     * --------------------------------------------- */

    default:

        pr_warn(
            "asgn1: unsupported ioctl command\n"
        );

        return -ENOTTY;
    }
}

/* =====================================================
 * FILE OPERATIONS
 * ===================================================== */

static const struct file_operations asgn1_fops = {

    .owner = THIS_MODULE,

    .open = asgn1_open,

    .read = asgn1_read,

    .write = asgn1_write,

    .release = asgn1_release,

    .unlocked_ioctl = asgn1_ioctl,

    .llseek = no_llseek,
};

/* =====================================================
 * PROCFS
 * /proc/asgn1_info
 * ===================================================== */

/*
 * Show character driver information.
 *
 * Returns:
 * 0 on success.
 */
static int asgn1_proc_show(struct seq_file *m,
                           void *v)
{
    mutex_lock(&asgn1_mutex);

    seq_printf(m,
               "Assignment 01 Character Device Driver\n");

    seq_printf(m,
               "=====================================\n");

    seq_printf(m,
               "Device name : %s\n",
               DEVICE_NAME);

    seq_printf(m,
               "Major       : %d\n",
               major_number);

    seq_printf(m,
               "Buffer size : %d\n",
               BUFFER_SIZE);

    seq_printf(m,
               "Buffer len  : %d\n",
               buffer_len);

    seq_printf(m,
               "Open count  : %d\n",
               open_count);

    seq_printf(m,
               "Read count  : %d\n",
               read_count);

    seq_printf(m,
               "Write count : %d\n",
               write_count);

    seq_printf(m,
               "Mode        : %d\n",
               driver_mode);

    seq_printf(m,
               "Last data   : %s\n",
               buffer_len > 0 ?
               device_buffer :
               "(empty)");

    mutex_unlock(&asgn1_mutex);

    return 0;
}

/*
 * Open procfs entry.
 */
static int asgn1_proc_open(struct inode *inode,
                           struct file *file)
{
    return single_open(file,
                       asgn1_proc_show,
                       NULL);
}

/* Linux Kernel 5.15 proc_ops */
static const struct proc_ops asgn1_proc_ops = {

    .proc_open = asgn1_proc_open,

    .proc_read = seq_read,

    .proc_lseek = seq_lseek,

    .proc_release = single_release,
};

/* =====================================================
 * SYSFS ATTRIBUTES
 * ===================================================== */

/*
 * Read buffer length through sysfs.
 */
static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    ssize_t ret;

    mutex_lock(&asgn1_mutex);

    ret = scnprintf(buf,
                    PAGE_SIZE,
                    "%d\n",
                    buffer_len);

    mutex_unlock(&asgn1_mutex);

    return ret;
}

/*
 * Read open count through sysfs.
 */
static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    ssize_t ret;

    mutex_lock(&asgn1_mutex);

    ret = scnprintf(buf,
                    PAGE_SIZE,
                    "%d\n",
                    open_count);

    mutex_unlock(&asgn1_mutex);

    return ret;
}

/*
 * Read last stored data through sysfs.
 */
static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
    ssize_t ret;

    mutex_lock(&asgn1_mutex);

    ret = scnprintf(buf,
                    PAGE_SIZE,
                    "%s\n",
                    buffer_len > 0 ?
                    device_buffer :
                    "(empty)");

    mutex_unlock(&asgn1_mutex);

    return ret;
}

/* Define read-only sysfs attributes */
static DEVICE_ATTR_RO(buffer_len);
static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(last_data);

/* Attribute list */
static struct attribute *asgn1_attrs[] = {

    &dev_attr_buffer_len.attr,

    &dev_attr_open_count.attr,

    &dev_attr_last_data.attr,

    NULL,
};

static const struct attribute_group asgn1_attr_group = {

    .attrs = asgn1_attrs,
};

/* =====================================================
 * MODULE INITIALIZATION
 * ===================================================== */

/*
 * Initialize character driver.
 *
 * Registers:
 * - Character device
 * - Device class
 * - Device node
 * - Sysfs attributes
 * - Procfs entry
 *
 * Returns:
 * 0 on success or negative errno.
 */
static int __init asgn1_init(void)
{
    int ret;

    pr_info("asgn1: initializing module\n");

    /* Initialize default state */
    memset(device_buffer, 0, BUFFER_SIZE);

    buffer_len = 0;
    open_count = 0;
    write_count = 0;
    read_count = 0;
    last_write_size = 0;

    driver_mode = MODE_ECHO;

    /* Register character device */
    ret = register_chrdev(major_number,
                          DEVICE_NAME,
                          &asgn1_fops);

    if (ret < 0) {

        pr_err(
            "asgn1: register_chrdev failed: %d\n",
            ret
        );

        return ret;
    }

    /* Create device class */
    asgn1_class = class_create(THIS_MODULE,
                               CLASS_NAME);

    if (IS_ERR(asgn1_class)) {

        ret = PTR_ERR(asgn1_class);

        unregister_chrdev(major_number,
                          DEVICE_NAME);

        return ret;
    }

    /* Create device */
    asgn1_device = device_create(
        asgn1_class,
        NULL,
        MKDEV(major_number, 0),
        NULL,
        DEVICE_NAME
    );

    if (IS_ERR(asgn1_device)) {

        ret = PTR_ERR(asgn1_device);

        class_destroy(asgn1_class);

        unregister_chrdev(major_number,
                          DEVICE_NAME);

        return ret;
    }

    /* Create sysfs attributes */
    ret = sysfs_create_group(
        &asgn1_device->kobj,
        &asgn1_attr_group
    );

    if (ret) {

        device_destroy(asgn1_class,
                       MKDEV(major_number, 0));

        class_destroy(asgn1_class);

        unregister_chrdev(major_number,
                          DEVICE_NAME);

        return ret;
    }

    /* Create procfs entry */
    asgn1_proc_entry = proc_create(
        "asgn1_info",
        0444,
        NULL,
        &asgn1_proc_ops
    );

    if (!asgn1_proc_entry) {

        sysfs_remove_group(
            &asgn1_device->kobj,
            &asgn1_attr_group
        );

        device_destroy(asgn1_class,
                       MKDEV(major_number, 0));

        class_destroy(asgn1_class);

        unregister_chrdev(major_number,
                          DEVICE_NAME);

        return -ENOMEM;
    }

    pr_info(
        "asgn1: module loaded successfully\n"
    );

    pr_info(
        "asgn1: /dev/asgn1 registered, major=%d\n",
        major_number
    );

    pr_info(
        "asgn1: /proc/asgn1_info created\n"
    );

    pr_info(
        "asgn1: sysfs attributes created\n"
    );

    return 0;
}

/* =====================================================
 * MODULE CLEANUP
 * ===================================================== */

/*
 * Unregister all driver resources.
 */
static void __exit asgn1_exit(void)
{
    /* Remove procfs */
    proc_remove(asgn1_proc_entry);

    /* Remove sysfs */
    sysfs_remove_group(
        &asgn1_device->kobj,
        &asgn1_attr_group
    );

    /* Remove device */
    device_destroy(
        asgn1_class,
        MKDEV(major_number, 0)
    );

    /* Destroy class */
    class_destroy(asgn1_class);

    /* Unregister character device */
    unregister_chrdev(
        major_number,
        DEVICE_NAME
    );

    pr_info(
        "asgn1: module unloaded successfully\n"
    );
}

/* =====================================================
 * REGISTER MODULE
 * ===================================================== */

module_init(asgn1_init);
module_exit(asgn1_exit);
