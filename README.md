# INT-13, Stage-2 | Kernel module
#### done by Yashnikov Valeriy at ho1lodno@yandex.ru

# driver developing
## basics
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
## detailing RW
### read
added __proc_read__ function reads data from kernel space memory and copies it to user space using the copy_to_user function.
```c
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
```
here we checking EOF and index out of bounds, kinda UB errors
### write
we can allocate a new memory area in the kernel to store the data, and frees the old memory area with __proc_write__ function takes data from user space and copies it into kernel space memory using the _copy_from_user_ function
```c
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
```
# userspace tool 
so, we got some options
```c
void usage(const char *prog_name) {
fprintf(stderr, "usage: %s -d | -r | -w\n", prog_name);
fprintf(stderr, " -d delete the secret from memory\n");
fprintf(stderr, " -r read the secret from memory\n");
fprintf(stderr, " -w write the secret to memory from stdin\n");
}
```
deleting realized with empty str, bit about writing i thought about an hour. so, i decided to use stdin. i think this is the best way just because we can hide ANYTHING just with pipeline 
any file, any string, for exmple: `echo "hello kernel!" | ./KStool -w`