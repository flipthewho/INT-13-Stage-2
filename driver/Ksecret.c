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
    // pass, read func later
}

static ssize_t proc_write(struct file *file, const char *buf, size_t count, loff_t *pos) {
    // pass, write func later
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

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
    printk(KERN_INFO "ksecret module exited.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("disgrace");
MODULE_DESCRIPTION("driver to keep secrets via procfs in kspace");

module_init(ksecret_init);
module_exit(ksecret_exit);
