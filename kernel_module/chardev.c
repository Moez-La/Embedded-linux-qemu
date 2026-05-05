#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "mon_device"
#define BUFFER_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Moez Chagraoui");
MODULE_DESCRIPTION("Character device driver with mutex");

static int major;
static char buffer[BUFFER_SIZE];
static int buffer_len = 0;
static DEFINE_MUTEX(dev_mutex);

static int dev_open(struct inode *inode, struct file *file)
{
    pr_info("%s: device opened\n", DEVICE_NAME);
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    pr_info("%s: device closed\n", DEVICE_NAME);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *user_buf,
                        size_t count, loff_t *offset)
{
    int bytes_read;

    mutex_lock(&dev_mutex);

    if (*offset >= buffer_len) {
        mutex_unlock(&dev_mutex);
        return 0;
    }

    bytes_read = min((size_t)(buffer_len - *offset), count);

    if (copy_to_user(user_buf, buffer + *offset, bytes_read)) {
        mutex_unlock(&dev_mutex);
        return -EFAULT;
    }

    *offset += bytes_read;
    pr_info("%s: %d bytes read\n", DEVICE_NAME, bytes_read);

    mutex_unlock(&dev_mutex);
    return bytes_read;
}

static ssize_t dev_write(struct file *file, const char __user *user_buf,
                         size_t count, loff_t *offset)
{
    mutex_lock(&dev_mutex);

    if (count > BUFFER_SIZE) {
        mutex_unlock(&dev_mutex);
        return -EINVAL;
    }

    if (copy_from_user(buffer, user_buf, count)) {
        mutex_unlock(&dev_mutex);
        return -EFAULT;
    }

    buffer_len = count;
    pr_info("%s: %zu bytes written\n", DEVICE_NAME, count);

    mutex_unlock(&dev_mutex);
    return count;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .release = dev_release,
    .read    = dev_read,
    .write   = dev_write,
};

static int __init chardev_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_err("%s: failed to register device\n", DEVICE_NAME);
        return major;
    }
    pr_info("%s: registered with major number %d\n", DEVICE_NAME, major);
    pr_info("Run: mknod /dev/%s c %d 0\n", DEVICE_NAME, major);
    return 0;
}

static void __exit chardev_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("%s: unregistered\n", DEVICE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);
