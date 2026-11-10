#pragma once

#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

int close(int __fd); // comes from unistd, but causes issues with the syscalls

bool sys_ret_is_err(size_t v);
uint16_t sys_ret_errno(size_t v);

struct linux_dirent64 {
    uint64_t  d_ino;
    uint64_t  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

// #define NOSTD 1
#if NOSTD

int sys_open(const char *filename, int flags, mode_t mode);
int sys_fstatat(int dfd, const char *filename, struct stat *statbuf, int flag);
int sys_fstat(int fd, struct stat *statbuf);
int sys_lstat(const char *path, struct stat *buf);
int sys_close(int fd);
void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int sys_munmap(void *addr, size_t length);
ssize_t sys_write(unsigned int fd, const char *buf, size_t count);
void sys_exit(int error_code);
ssize_t sys_getdents64(unsigned int fd, struct linux_dirent64 *dirents, unsigned int count);

#define open       sys_open
#define fstatat    sys_fstatat
#define fstat      sys_fstat
#define lstat      sys_lstat
#define close      sys_close
#define mmap       sys_mmap
#define munmap     sys_munmap
#define write      sys_write
#define exit       sys_exit
#define getdents64 sys_getdents64

int my_memcmp(const void *s1, const void *s2, size_t n);
void *my_memset(void *s, int c, size_t n);
void *my_memcpy(void *dest, const void *src, size_t n);
void *my_memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);
size_t my_strlen(char const *s);

#define memcmp my_memcmp
#define memset my_memset
#define memcpy my_memcpy
#define memmem my_memmem
#define strlen my_strlen

#endif // NOSTD

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

#define opendir my_opendir
#define readdir my_readdir64
#define readdir64 my_readdir64
#define closedir my_closedir

extern unsigned int my_errno;

#ifdef errno
#undef errno
#endif
#define errno my_errno
