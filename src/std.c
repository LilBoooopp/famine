#define _GNU_SOURCE

#include <dirent.h>
#include <syscall.h>
#include <stddef.h>
#include "../include/m_std.h"

// #define NOSTD 1
#if NOSTD
#include "../include/m_syscall.h"

bool sys_ret_is_err(size_t v) {
	return (-4095UL <= v && v <= -1UL);
}

uint16_t sys_ret_errno(size_t v) {
	if (sys_ret_is_err(v)) {
		return -v;
	}
	return 0;
}

unsigned int my_errno = 0;

static size_t sys_ret_set_errno(size_t ret) {
	if (sys_ret_is_err(ret)) { \
		errno = sys_ret_errno(ret);
		return -1;
	}
	return ret;
}

#define syscall_errno(...) sys_ret_set_errno(syscall(__VA_ARGS__))

int sys_open(const char *filename, int flags, mode_t mode) {
	return syscall_errno(SYS_open, (size_t)filename, flags, mode);
}

int sys_fstatat(int dfd, const char *restrict filename, struct stat *restrict statbuf, int flag) {
	return syscall_errno(SYS_newfstatat, dfd, (size_t)filename, (size_t)statbuf, flag);
}

int sys_fstat(int fd, struct stat *restrict statbuf) {
	return sys_fstatat(fd, "", statbuf, AT_EMPTY_PATH);
}

int sys_lstat(const char *restrict path, struct stat *restrict buf) {
	return sys_fstatat(AT_FDCWD, path, buf, AT_SYMLINK_NOFOLLOW);
}

int sys_close(int fd) {
	return syscall_errno(SYS_close, fd);
}

void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
	// FIXME: Use mmap2 if available
	return (void*)syscall_errno(SYS_mmap, (size_t)addr, length, prot, flags, fd, offset);
}

int sys_munmap(void *addr, size_t length) {
	return syscall_errno(SYS_munmap, (size_t)addr, length);
}

ssize_t sys_write(unsigned int fd, const char *buf, size_t count) {
	return syscall_errno(SYS_write, fd, (size_t)buf, count);
}

void sys_exit(int error_code) {
	syscall(SYS_exit, error_code);
	__builtin_unreachable();
}

ssize_t sys_getdents64(unsigned int fd, struct linux_dirent64 *dirents, unsigned int count) {
	return syscall_errno(SYS_getdents64, fd, (size_t)dirents, count);
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
#endif

struct my_DIR *my_opendir(struct my_DIR *dir, const char *name) {
	int fd = open(name, O_RDONLY|O_DIRECTORY|O_CLOEXEC, 0);
	if (fd < 0)
		return NULL;
	dir->fd = fd;
	dir->buf_pos = 0;
	dir->buf_end = 0;
	return dir;
}

struct linux_dirent64 *my_readdir64(struct my_DIR *dir) {
	struct linux_dirent64 *de;

	if (dir->buf_pos >= dir->buf_end) {
		size_t len = getdents64(dir->fd, (void*)dir->buf, sizeof(dir->buf));
		if (len <= 0) {
			return NULL;
		}
		dir->buf_end = len;
		dir->buf_pos = 0;
	}
	de = (void *)(dir->buf + dir->buf_pos);
	dir->buf_pos += de->d_reclen;
	return de;
}

void my_closedir(struct my_DIR *dir) {
	close(dir->fd);
}
