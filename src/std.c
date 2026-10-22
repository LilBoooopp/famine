#define _GNU_SOURCE

#include "../include/m_syscall.h"
#include "../include/m_std.h"
#include <syscall.h>
#include <stddef.h>

bool sys_ret_is_err(size_t v) {
	return (-4095UL <= v && v <= -1UL);
}

uint16_t sys_ret_errno(size_t v) {
	if (sys_ret_is_err(v)) {
		return -v;
	}
	return 0;
}

size_t sys_open(const char *filename, int flags, mode_t mode) {
	return syscall(SYS_open, (size_t)filename, flags, mode);
}

size_t sys_fstatat(int dfd, const char *restrict filename, struct stat *restrict statbuf, int flag) {
	return syscall(SYS_newfstatat, dfd, (size_t)filename, (size_t)statbuf, flag);
}

size_t sys_fstat(int fd, struct stat *restrict statbuf) {
	return sys_fstatat(fd, "", statbuf, AT_EMPTY_PATH);
}

size_t sys_lstat(const char *restrict path, struct stat *restrict buf) {
	return sys_fstatat(AT_FDCWD, path, buf, AT_SYMLINK_NOFOLLOW);
}

size_t sys_close(unsigned int fd) {
	return syscall(SYS_close, fd);
}

size_t sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
	// FIXME: Use mmap2 if available
	return syscall(SYS_mmap, (size_t)addr, length, prot, flags, fd, offset);
}

size_t sys_munmap(void *addr, size_t length) {
	return syscall(SYS_munmap, (size_t)addr, length);
}

size_t sys_write(unsigned int fd, const char *buf, size_t count) {
	return syscall(SYS_write, fd, (size_t)buf, count);
}

size_t sys_exit(int error_code) {
	return syscall(SYS_exit, error_code);
}




int my_memcmp(const void *s1, const void *s2, size_t n) {
	for (size_t i = 0; i < n; i++) {
		int diff = ((unsigned char const *)s1)[i] - ((unsigned char const *)s2)[i];
		if (diff != 0) return diff;
	}
	return 0;
}

void *my_memset(void *s, int c, size_t n) {
	for (size_t i = 0; i < n; i++) {
		((unsigned char*)s)[n] = c;
	}
	return s;
}

void *my_memcpy(void *restrict dest, const void *restrict src, size_t n) {
	for (size_t i = 0; i < n; i++) {
		((unsigned char*)dest)[i] = ((unsigned char const *)src)[i];
	}
	return dest;
}

void *my_memmem(const void *restrict haystack, size_t haystacklen, const void *restrict needle, size_t needlelen) {
	if (haystacklen < needlelen) return NULL;
	if (needlelen == 0) return (void*)haystack;
	for (size_t i = 0; i < haystacklen - needlelen; i++) {
		size_t j;
		for (j = 0; j < needlelen; j++) {
			if (((unsigned char const *)haystack)[i + j] != ((unsigned char *)needle)[j]) {
				break;
			}
		}
		if (j == needlelen) {
			return &((unsigned char *)haystack)[i];
		}
	}
	return NULL;
}

size_t my_strlen(char const *s) {
	size_t len = 0;
	while (s[len] != '\0') {
		len++;
	}
	return len;
}
