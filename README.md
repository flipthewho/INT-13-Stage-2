# Ksecret driver + userspace tool | Documentation

__Done by Yashnikov Valeriy at INT-13, Stage-2__
Credentials: ho1lodno@yandex.ru
## Motivation
The driver was developed as part of my mission at INT-13, PT-START 2024. Personal motivation is understanding Linux kernel modules and C basics of development. 
## Installation
You can install the package:
```shell
git clone https://github.com/flipthewho/INT-13-Stage-2.git
cd INT-13-Stage-2/
```
Installing driver:
```shell
cd driver/
make
sudo insmod Ksecret.ko
```
Building tool:
```shell
cd tool/
gcc KStool.c -o KStool
```
## Usage
```
usage: ./KStool -d | -r | -w
  -d   delete the secret from memory
  -r   read the secret from memory
  -w   write the secret to memory from stdin
```

## Example
```shell
$ ./KStool 
usage: ./KStool -d | -r | -w
  -d   delete the secret from memory
  -r   read the secret from memory
  -w   write the secret to memory from stdin
$ ./KStool -r
secret: 
$ ./KStool -w
enter the secret: 
123ioctl
$ ./KStool -r
secret: 123ioctl
$ ./KStool -d
$ ./KStool -r
secret: 
```

## Possible QA
__Q:__ How can i remove temp files like ` Ksecret.mod Ksecret.mod.c Ksecret.mod.o  Ksecret.o modules.order Module.symvers` ?
__A:__ `$ make clean` in current driver directory

__Q:__  How can i remove module?
__A:__ `$ sudo rmmod Ksecret` 