#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define PROC_DIR "ksecret_dir"
#define PROC_FILE "ksecret_file"

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;

static char *secret_data;
static size_t data_size;

static ssize_t proc_read(struct file *file, char *buf, size_t count, loff_t *pos) {
    
    ssize_t retval = 0;

    if (*pos >= data_size) {
        return 0;
    }

    if (*pos + count > data_size) {
        count = data_size - *pos;
    }

    if (copy_to_user(buf, secret_data + *pos, count)) {
        retval = -EFAULT;
    } else {
        *pos += count;
        retval = count;
    }

    return retval;
}

static ssize_t proc_write(struct file *file, const char *buf, size_t count, loff_t *pos) {
    
    ssize_t retval = 0;
    char *new_data;

    new_data = kmalloc(count, GFP_KERNEL);
    if (!new_data) {
        return -ENOMEM;
    }

    if (copy_from_user(new_data, buf, count)) {
        kfree(new_data);
        return -EFAULT;
    }

    kfree(secret_data);
    secret_data = new_data;
    data_size = count;
    retval = count;

    return retval;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
}

static int __init ksecret_init(void) {
    
    proc_dir = proc_mkdir(PROC_DIR, NULL);
    if (!proc_dir) {
        return -ENOMEM;
    }

    proc_file = proc_create(PROC_FILE, 0666, proc_dir, &proc_file_ops);
    if (!proc_file) {
        proc_remove(proc_dir);
        return -ENOMEM;
    }

    printk(KERN_INFO "ksecret module initialized\n");
    return 0;
}

static void __exit ksecret_exit(void) {
    proc_remove(proc_file);
    proc_remove(proc_dir);
    kfree(secret_data);
    printk(KERN_INFO "ksecret module exited.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("disgrace");
MODULE_DESCRIPTION("driver to keep secrets via procfs in kspace");

module_init(ksecret_init);
module_exit(ksecret_exit);
