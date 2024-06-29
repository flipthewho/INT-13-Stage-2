# INT-13, Stage-2 | Kernel module
#### done by Yashnikov Valeriy at ho1lodno@yandex.ru

## starting point 
### driver developing 
first of all, we should can read and write in kernel space.
```C
static ssize_t proc_read(struct file *file, char *buf, size_t count, loff_t *pos) {
    // ...
}

static ssize_t proc_write(struct file *file, const char *buf, size_t count, loff_t *pos) {
    // ...
}
```
and simple inits with basic files/dirs existence checks
```C
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
```
