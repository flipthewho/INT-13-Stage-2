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
##  tests
basics tests
```shell
toor@uwuntu:~/INT-13-Stage-2/tool$ ./tool -r
Secret: 
toor@uwuntu:~/INT-13-Stage-2/tool$ ./tool -w
Enter the secret: 123
toor@uwuntu:~/INT-13-Stage-2/tool$ ./tool -r
Secret: 123
```
all is ok
lets make code unsafe and made this:
```C
ssize_t proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
    char *kbuf;
    kbuf = kmalloc(count, GFP_KERNEL);
    // basic logic
    kbuf[count + 10] = '\0';
    kfree(kbuf);
    return count;
}
```
so, here we escaping from array
lets test it out... and we did it!
```shell
[  434.304835] /proc/secret created
[ 4461.821504] /proc/secret removed
[ 6490.271968] /proc/secret created
[ 6521.174838] ==================================================================
[ 6521.175059] BUG: KASAN: slab-out-of-bounds in proc_write+0xb3/0xd0 [Ksecret]
[ 6521.175230] Write of size 1 at addr ffff888121d9f46d by task tool/2264

[ 6521.175506] CPU: 9 PID: 2264 Comm: tool Tainted: G           OE      6.9.7kasan-disgrace #2
[ 6521.175664] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS 1.15.0-1 04/01/2014
[ 6521.175816] Call Trace:
[ 6521.175944]  <TASK>
[ 6521.176066]  dump_stack_lvl+0x76/0xa0
[ 6521.176245]  print_report+0xd1/0x670
[ 6521.176397]  ? __pfx__raw_spin_lock_irqsave+0x10/0x10
[ 6521.176544]  ? kasan_complete_mode_report_info+0x26/0x200
[ 6521.176684]  kasan_report+0xd6/0x120
[ 6521.176810]  ? proc_write+0xb3/0xd0 [Ksecret]
[ 6521.176948]  ? proc_write+0xb3/0xd0 [Ksecret]
[ 6521.177084]  __asan_report_store1_noabort+0x17/0x30
[ 6521.177219]  proc_write+0xb3/0xd0 [Ksecret]
[ 6521.177367]  proc_reg_write+0x1d5/0x2a0
[ 6521.177519]  ? fpregs_restore_userregs+0xe8/0x210
[ 6521.181441]  vfs_write+0x234/0xfb0
[ 6521.182346]  ? syscall_exit_to_user_mode+0x87/0x260
[ 6521.183265]  ? __pfx_vfs_write+0x10/0x10
[ 6521.184194]  ? __kasan_check_read+0x11/0x20
[ 6521.185112]  ? __fget_light+0x5b/0x480
[ 6521.186017]  ? do_syscall_64+0x8b/0x180
[ 6521.186910]  ksys_write+0x11e/0x250
[ 6521.187801]  ? __pfx_ksys_write+0x10/0x10
[ 6521.188683]  ? __kasan_check_write+0x14/0x30
[ 6521.189565]  ? __count_memcg_events+0x264/0x380
[ 6521.190450]  __x64_sys_write+0x72/0xc0
[ 6521.191317]  x64_sys_call+0x7e/0x25c0
[ 6521.192182]  do_syscall_64+0x7e/0x180
[ 6521.193025]  ? __kasan_check_read+0x11/0x20
[ 6521.193858]  ? fpregs_assert_state_consistent+0x21/0xb0
[ 6521.194692]  ? irqentry_exit_to_user_mode+0x7c/0x260
[ 6521.195530]  ? irqentry_exit+0x43/0x50
[ 6521.196368]  ? exc_page_fault+0x7b/0x110
[ 6521.197206]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[ 6521.198063] RIP: 0033:0x7a0a28d1c574
[ 6521.198903] Code: c7 00 16 00 00 00 b8 ff ff ff ff c3 66 2e 0f 1f 84 00 00 00 00 00 f3 0f 1e fa 80 3d d5 ea 0e 00 00 74 13 b8 01 00 00 00 0f 05 <48> 3d 00 f0 ff ff 77 54 c3 0f 1f 00 55 48 89 e5 48 83 ec 20 48 89
[ 6521.200604] RSP: 002b:00007ffc6594d618 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[ 6521.201355] RAX: ffffffffffffffda RBX: 00007ffc6594db98 RCX: 00007a0a28d1c574
[ 6521.202096] RDX: 0000000000000003 RSI: 00007ffc6594d630 RDI: 0000000000000003
[ 6521.202838] RBP: 00007ffc6594da40 R08: 000063bf6bba76b4 R09: 0000000000000410
[ 6521.203591] R10: 0000000000000000 R11: 0000000000000202 R12: 0000000000000002
[ 6521.204356] R13: 0000000000000000 R14: 000063bf662dad58 R15: 00007a0a28e8f000
[ 6521.205109]  </TASK>

[ 6521.206527] Allocated by task 2264:
[ 6521.207225]  kasan_save_stack+0x39/0x70
[ 6521.207234]  kasan_save_track+0x14/0x40
[ 6521.207237]  kasan_save_alloc_info+0x37/0x60
[ 6521.207240]  __kasan_kmalloc+0xc3/0xd0
[ 6521.207243]  __kmalloc+0x21f/0x530
[ 6521.207250]  proc_write+0x23/0xd0 [Ksecret]
[ 6521.207258]  proc_reg_write+0x1d5/0x2a0
[ 6521.207262]  vfs_write+0x234/0xfb0
[ 6521.207265]  ksys_write+0x11e/0x250
[ 6521.207268]  __x64_sys_write+0x72/0xc0
[ 6521.207270]  x64_sys_call+0x7e/0x25c0
[ 6521.207274]  do_syscall_64+0x7e/0x180
[ 6521.207278]  entry_SYSCALL_64_after_hwframe+0x76/0x7e

[ 6521.207974] The buggy address belongs to the object at ffff888121d9f460
                which belongs to the cache kmalloc-8 of size 8
[ 6521.209341] The buggy address is located 10 bytes to the right of
                allocated 3-byte region [ffff888121d9f460, ffff888121d9f463)

[ 6521.211398] The buggy address belongs to the physical page:
[ 6521.212111] page: refcount:1 mapcount:0 mapping:0000000000000000 index:0xffff888121d9f060 pfn:0x121d9f
[ 6521.212119] flags: 0x17ffffc0000800(slab|node=0|zone=2|lastcpupid=0x1fffff)
[ 6521.212124] page_type: 0xffffffff()
[ 6521.212141] raw: 0017ffffc0000800 ffff888100042280 dead000000000122 0000000000000000
[ 6521.212144] raw: ffff888121d9f060 000000008080007e 00000001ffffffff 0000000000000000
[ 6521.212146] page dumped because: kasan: bad access detected

[ 6521.212836] Memory state around the buggy address:
[ 6521.213535]  ffff888121d9f300: fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc
[ 6521.214250]  ffff888121d9f380: fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc
[ 6521.214973] >ffff888121d9f400: fc fc fc fc fc fc fc fc fc fc fc fc 03 fc fc fc
[ 6521.215684]                                                           ^
[ 6521.216443]  ffff888121d9f480: fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc
[ 6521.217190]  ffff888121d9f500: fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc
[ 6521.217910] ==================================================================
[ 6521.218690] Disabling lock debugging due to kernel taint
toor@uwuntu:~$ 

```
so, i wrote `123` again. and KASAN says: `[ 6521.209341] The buggy address is located 10 bytes to the right of allocated 3-byte region [ffff888121d9f460, ffff888121d9f463)`
our three bytesis `123` prompt and `kbuf[count + 10]` is 10-bytes-shift to the right