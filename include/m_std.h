#pragma once

#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool sys_ret_is_err(size_t v);
uint16_t sys_ret_errno(size_t v);

struct linux_dirent64 {
    unsigned long  d_ino;
    unsigned long  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

size_t sys_open(const char *filename, int flags, mode_t mode);
size_t sys_fstatat(int dfd, const char *filename, struct stat *statbuf, int flag);
size_t sys_fstat(int fd, struct stat *statbuf);
size_t sys_lstat(const char *path, struct stat *buf);
size_t sys_close(unsigned int fd);
size_t sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
size_t sys_munmap(void *addr, size_t length);
size_t sys_write(unsigned int fd, const char *buf, size_t count);
size_t sys_exit(int error_code);
size_t sys_getdents64(unsigned int fd, struct linux_dirent64 *dirents, unsigned int count);


int my_memcmp(const void *s1, const void *s2, size_t n);
void *my_memset(void *s, int c, size_t n);
void *my_memcpy(void *dest, const void *src, size_t n);
void *my_memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);
size_t my_strlen(char const *s);

struct my_DIR
{
	int fd;
	int buf_pos;
	int buf_end;
	volatile int lock[1];
	/* NOTE: Any changes to this struct must preserve the property:
	 * offsetof(struct __dirent, buf) % sizeof(off_t) == 0 */
	char buf[2048]; // an array of dirent's
};
typedef char my_DIR_buf_alignment_check[1-2*(int)(offsetof(struct my_DIR, buf) % sizeof(off_t))];

struct my_DIR *my_opendir(struct my_DIR *dir, const char *name);
struct linux_dirent64 *my_readdir64(struct my_DIR *dir);
void my_closedir(struct my_DIR *dir);
