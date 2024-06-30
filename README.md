# INT-13, Stage-2 | Kernel module
#### done by Yashnikov Valeriy at ho1lodno@yandex.ru

# driver developing
## basics
first of all, we should can read and write in kernel space.
```C
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    // ...
}

static ssize_t proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
    // ...
}
```
## detailing RW
### read
added __proc_read__ function reads data from kernel space memory and copies it to user space using the copy_to_user function.
```c
ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
if (*pos > 0 || count < secret_size)
	return 0;

if (copy_to_user(buf, secret_data, secret_size))
	return -EFAULT;

*pos = secret_size;
return secret_size;
}
```
### write
__proc_write__ function takes data from user space and copies it into kernel space memory using the _copy_from_user_ function
```c
ssize_t proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {

if (count > MAX_SECRET_SIZE)
	return -EINVAL;

if (copy_from_user(secret_data, buf, count))
	return -EFAULT;

secret_size = count;
return count;
}
```
if (count > MAX_SECRET_SIZE) can help to reduce UB errors
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
__deleting__ realized with empty str, bit about __writing__ i thought about an hour. so, i decided to use stdin. i think this is the best way just because we can hide ANYTHING just with pipeline 
any file, any string, for exmple:
`echo "hello kernel!" | ./KStool -w`

# building and installing
building kernel module
```shell
disgrace@home:~/INT-13-Stage-2/driver$ make
make -C /lib/modules/6.10.0-rc5-disgrace/build M=/home/disgrace/INT-13-Stage-2/driver modules
make[1]: Entering directory '/usr/src/linux-headers-6.10.0-rc5-disgrace'
  CC [M]  /home/disgrace/INT-13-Stage-2/driver/Ksecret.o
  MODPOST /home/disgrace/INT-13-Stage-2/driver/Module.symvers
  CC [M]  /home/disgrace/INT-13-Stage-2/driver/Ksecret.mod.o
  LD [M]  /home/disgrace/INT-13-Stage-2/driver/Ksecret.ko
make[1]: Leaving directory '/usr/src/linux-headers-6.10.0-rc5-disgrace'
disgrace@home:~/INT-13-Stage-2/driver$ ls
Ksecret.c  Ksecret.ko  Ksecret.mod  Ksecret.mod.c  Ksecret.mod.o  Ksecret.o  Makefile  modules.order  Module.symvers

disgrace@home:~/INT-13-Stage-2/driver$ sudo insmod Ksecret.ko 
[sudo] password for disgrace: 

disgrace@home:~/INT-13-Stage-2/driver$ lsmod | grep Ksecret
Ksecret                12288  0
```
i installed Ksecret.ko with __insmod__
building tool and checks 
```shell
disgrace@home:~/INT-13-Stage-2/tool$ ls
KStool.c
disgrace@home:~/INT-13-Stage-2/tool$ gcc KStool.c -o tool
disgrace@home:~/INT-13-Stage-2/tool$ ./tool 
usage: ./tool -d | -r | -w
  -d   delete the secret from memory
  -r   read the secret from memory
  -w   write the secret to memory from stdin
disgrace@home:~/INT-13-Stage-2/tool$ ./tool -d
disgrace@home:~/INT-13-Stage-2/tool$ ./tool -r
Secret: 
disgrace@home:~/INT-13-Stage-2/tool$ ./tool -w
Enter the secret: 1321oops
disgrace@home:~/INT-13-Stage-2/tool$ ./tool -r
Secret: 1321oops
disgrace@home:~/INT-13-Stage-2/tool$ ./tool -d
disgrace@home:~/INT-13-Stage-2/tool$ ./tool -r
Secret: 
disgrace@home:~/INT-13-Stage-2/tool$ 
```
so, as we can see, all works
now we can try to build kernel with KASAN and test our modules

# tests with KASAN
## preparing
create disk
```shel
disgrace@home:~/vms$ qemu-img create -f qcow2 uwuntu.img 50G
Formatting 'uwuntu.img', fmt=qcow2 ...
```
and install VM
```shell
disgrace@home:~/vms$ qemu-system-x86_64 -boot d -enable-kvm -smp 12 -m 16384 
-hda uwuntu.img -cdrom /media/disgrace/Ventoy/iso/ubuntu-22.04.4-desktop-amd64.iso
```
getting sources
```shell
toor@uwuntu-vm:~/kernel$ wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.9.7.tar.xz
--2024-06-30 19:02:37--   ...
```
and turning KASAN on
![alt text](https://github.com/flipthewho/INT-13-Stage-2/blob/main/media/Pasted%20image%2020240630191304.png)
lets check
```shell
...
CONFIG_CC_HAS_WORKING_NOSANITIZE_ADDRESS=y
CONFIG_KASAN=y
CONFIG_KASAN_GENERIC=y
...
```
all is ok. lets build kernel
![alt text](https://github.com/flipthewho/INT-13-Stage-2/blob/main/media/Pasted%20image%2020240630191853.png)

```shell
dpkg-deb (subprocess): compressing tar member: internal zstd write error: 'No space left on device': No space left on device
...
/dev/sda3       50770432 47917792    241236 100% /
```
ahahahha. desktop ubuntu need much more space to compile kernel 
(may be gui -> more drivers), so i switched to ubuntu server asap
building kernel with kasan
```shell
toor@uwuntu:~/kernels$ sudo dpkg -i linux-headers-6.9.7kasan-disgrace_6.9.7-2_amd64.deb linux-image-6.9.7kasan-disgrace_6.9.7-2_amd64.deb linux-libc-dev_6.9.7-2_amd64.deb 
Selecting previously unselected package linux-headers-6.9.7kasan-disgrace.
(Reading database ... 77776 files and directories currently installed.)
Preparing to unpack linux-headers-6.9.7kasan-disgrace_6.9.7-2_amd64.deb ...
Unpacking linux-headers-6.9.7kasan-disgrace (6.9.7-2) ...
...
Warning: os-prober will not be executed to detect other bootable partitions.
Systems on them will not be added to the GRUB boot configuration.
Check GRUB_DISABLE_OS_PROBER documentation entry.
Adding boot menu entry for UEFI Firmware Settings ...
done
Setting up linux-libc-dev:amd64 (6.9.7-2) ...
```
lets check kernel
```shell
toor@uwuntu:~$ uname -a
Linux uwuntu 6.9.7kasan-disgrace #2 SMP PREEMPT_DYNAMIC Sun Jun 30 19:01:58 UTC 2024 x86_64 x86_64 x86_64 GNU/Linux
toor@uwuntu:~$ cat /boot/config-6.9.7kasan-disgrace | grep KASAN
CONFIG_KASAN_SHADOW_OFFSET=0xdffffc0000000000
CONFIG_HAVE_ARCH_KASAN=y
CONFIG_HAVE_ARCH_KASAN_VMALLOC=y
CONFIG_CC_HAS_KASAN_GENERIC=y
CONFIG_KASAN=y
CONFIG_CC_HAS_KASAN_MEMINTRINSIC_PREFIX=y
CONFIG_KASAN_GENERIC=y
# CONFIG_KASAN_OUTLINE is not set
CONFIG_KASAN_INLINE=y
CONFIG_KASAN_STACK=y
# CONFIG_KASAN_VMALLOC is not set
# CONFIG_KASAN_MODULE_TEST is not set
# CONFIG_KASAN_EXTRA_INFO is not set
```


building module an tool 
```shell
toor@uwuntu:~/INT-13-Stage-2/driver$ make
make -C /lib/modules/6.8.0-31-generic/build M=/home/toor/INT-13-Stage-2/driver modules
make[1]: Entering directory '/usr/src/linux-headers-6.8.0-31-generic'
warning: the compiler differs from the one used to build the kernel
  The kernel was built by: x86_64-linux-gnu-gcc-13 (Ubuntu 13.2.0-23ubuntu4) 13.2.0
  You are using:           gcc-13 (Ubuntu 13.2.0-23ubuntu4) 13.2.0
  CC [M]  /home/toor/INT-13-Stage-2/driver/Ksecret.o
/home/toor/INT-13-Stage-2/driver/Ksecret.c:19:9: warning: no previous prototype for ‘proc_read’ [-Wmissing-prototypes]
   19 | ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
      |         ^~~~~~~~~
/home/toor/INT-13-Stage-2/driver/Ksecret.c:30:9: warning: no previous prototype for ‘proc_write’ [-Wmissing-prototypes]
   30 | ssize_t proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
      |         ^~~~~~~~~~
  MODPOST /home/toor/INT-13-Stage-2/driver/Module.symvers
  CC [M]  /home/toor/INT-13-Stage-2/driver/Ksecret.mod.o
  LD [M]  /home/toor/INT-13-Stage-2/driver/Ksecret.ko
  BTF [M] /home/toor/INT-13-Stage-2/driver/Ksecret.ko
Skipping BTF generation for /home/toor/INT-13-Stage-2/driver/Ksecret.ko due to unavailability of vmlinux
make[1]: Leaving directory '/usr/src/linux-headers-6.8.0-31-generic'
toor@uwuntu:~/INT-13-Stage-2/driver$ ls
Ksecret.c  Ksecret.ko  Ksecret.mod  Ksecret.mod.c  Ksecret.mod.o  Ksecret.o  Makefile  Module.symvers  modules.order
toor@uwuntu:~/INT-13-Stage-2/driver$ sudo insmod Ksecret.ko 
...
toor@uwuntu:~/INT-13-Stage-2/tool$ gcc KStool.c -o tool
toor@uwuntu:~/INT-13-Stage-2/tool$ ls
KStool.c  tool
toor@uwuntu:~/INT-13-Stage-2/tool$ 
```
