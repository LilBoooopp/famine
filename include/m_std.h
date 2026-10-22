#pragma once

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

bool sys_ret_is_err(size_t v);
uint16_t sys_ret_errno(size_t v);

size_t sys_open(const char *filename, int flags, mode_t mode);
size_t sys_fstatat(int dfd, const char *filename, struct stat *statbuf, int flag);
size_t sys_fstat(int fd, struct stat *statbuf);
size_t sys_lstat(const char *path, struct stat *buf);
size_t sys_close(unsigned int fd);
size_t sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
size_t sys_munmap(void *addr, size_t length);
size_t sys_write(unsigned int fd, const char *buf, size_t count);
size_t sys_exit(int error_code);


int my_memcmp(const void *s1, const void *s2, size_t n);
void *my_memset(void *s, int c, size_t n);
void *my_memcpy(void *dest, const void *src, size_t n);
void *my_memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);
size_t my_strlen(char const *s);
