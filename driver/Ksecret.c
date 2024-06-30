#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define PROCFS_NAME "secret"
#define MAX_SECRET_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("disgrace");
MODULE_DESCRIPTION("driver to keep secrets via procfs in kspace");

static struct proc_dir_entry *proc_file;
static char *secret_data;
static int secret_size = 0;

ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    if (*pos > 0 || count < secret_size)
        return 0;

    if (copy_to_user(buf, secret_data, secret_size))
        return -EFAULT;

    *pos = secret_size;
    return secret_size;
}

ssize_t proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
    if (count > MAX_SECRET_SIZE)
        return -EINVAL;

    if (copy_from_user(secret_data, buf, count))
        return -EFAULT;

    secret_size = count;
    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init secrets_init(void) {
    proc_file = proc_create(PROCFS_NAME, 0666, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    secret_data = kzalloc(MAX_SECRET_SIZE, GFP_KERNEL);
    if (!secret_data) {
        remove_proc_entry(PROCFS_NAME, NULL);
        return -ENOMEM;
    }

    printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
    return 0;
}

static void __exit secrets_exit(void) {
    remove_proc_entry(PROCFS_NAME, NULL);
    kfree(secret_data);
    printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}

module_init(secrets_init);
module_exit(secrets_exit);
