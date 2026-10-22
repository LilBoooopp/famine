#pragma once

#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <syscall.h>
#include <sys/stat.h>
#include <sys/types.h>

unsigned long syscall0(long n);
unsigned long syscall1(long n, long a1);
unsigned long syscall2(long n, long a1, long a2);
unsigned long syscall3(long n, long a1, long a2, long a3);
unsigned long syscall4(long n, long a1, long a2, long a3, long a4);
unsigned long syscall5(long n, long a1, long a2, long a3, long a4, long a5);
unsigned long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);

#define __SYSCALL_NARGS_X(a,b,c,d,e,f,g,h,n,...) n
#define __SYSCALL_NARGS(...) __SYSCALL_NARGS_X(__VA_ARGS__,7,6,5,4,3,2,1,0,)
#define __SYSCALL_CONCAT_X(a,b) a##b
#define __SYSCALL_CONCAT(a,b) __SYSCALL_CONCAT_X(a,b)
#define __SYSCALL_DISP(b,...) __SYSCALL_CONCAT(b,__SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define syscall(...) __SYSCALL_DISP(syscall,__VA_ARGS__)

static bool sys_ret_is_err(size_t v) {
	return (-4095UL <= v && v <= -1UL);
}

static uint16_t sys_ret_errno(size_t v) {
	if (sys_ret_is_err(v)) {
		return -v;
	}
	return 0;
}

size_t sys_open(const char *filename, int flags, mode_t mode);
size_t sys_fstatat(int dfd, const char *filename, struct stat *statbuf, int flag);
size_t sys_fstat(int fd, struct stat *statbuf);
size_t sys_close(unsigned int fd);
size_t sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
size_t sys_munmap(void *addr, size_t length);
size_t sys_write(unsigned int fd, const char *buf, size_t count);
size_t sys_exit(int error_code);
