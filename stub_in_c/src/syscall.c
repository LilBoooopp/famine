#define _GNU_SOURCE

#include "./m_syscall.h"

size_t sys_open(const char *filename, int flags, mode_t mode) {
	return syscall(SYS_open, (size_t)filename, flags, mode);
}

size_t sys_fstatat(int dfd, const char *restrict filename, struct stat *restrict statbuf, int flag) {
	return syscall(SYS_newfstatat, dfd, (size_t)filename, (size_t)statbuf, flag);
}

size_t sys_fstat(int fd, struct stat *restrict statbuf) {
	return sys_fstatat(fd, "", statbuf, AT_EMPTY_PATH);
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
