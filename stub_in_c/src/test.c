#include <elf.h>
#define _GNU_SOURCE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "m_syscall.h"

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

size_t write_hex(int fd, size_t n) {
	char hex_n[18];
	size_t digits = 0;
	hex_n[sizeof(hex_n) - 1] = '0';
	while (n > 0) {
		hex_n[sizeof(hex_n) - 1 - digits] = (n % 10) + '0';
		digits += 1;
		n /= 10;
	}
	if (digits == 0) digits = 1;
	hex_n[sizeof(hex_n) - 1 - digits] = 'x';
	digits += 1;
	hex_n[sizeof(hex_n) - 1 - digits] = '0';
	digits += 1;
	return sys_write(fd, &hex_n[sizeof(hex_n)-digits], digits);
}

size_t putstr(int fd, char const *str) {
	return sys_write(fd, str, my_strlen(str));
}

int my_main(size_t argc, char **argv, char **envp, size_t *auxv) {
	putstr(STDOUT_FILENO, "argv[0]: ");
	putstr(STDOUT_FILENO, argv[0]);
	putstr(STDOUT_FILENO, "\n");
	putstr(STDOUT_FILENO, "envp[0]: ");
	putstr(STDOUT_FILENO, envp[0]);
	putstr(STDOUT_FILENO, "\n");
	putstr(STDOUT_FILENO, "auxv[0]: ");
	write_hex(STDOUT_FILENO, auxv[0]);
	putstr(STDOUT_FILENO, ": ");
	write_hex(STDOUT_FILENO, auxv[1]);
	putstr(STDOUT_FILENO, "\n");
	size_t load_base = 0;
	for (size_t i = 0; auxv[i] != AT_NULL; i += 2) {
		if (auxv[i] == AT_PHDR) {
			load_base = auxv[i + 1];
			break;
		}
	}
	putstr(STDOUT_FILENO, "load_base: ");
	write_hex(STDOUT_FILENO, load_base);
	putstr(STDOUT_FILENO, "\n");
	return 0;
}

__attribute__((used))
size_t my_start(size_t *rsp) {
	size_t argc = rsp[0];
	char **argv = (char**)&rsp[1];
	char **envp = &argv[argc + 1];
	size_t envp_len = 0;
	while (envp[envp_len] != NULL) {
		envp_len++;
	}
	size_t *auxv = (size_t*)&envp[envp_len + 1];

	my_main(argc, argv, envp, auxv);
	// sys_exit(my_main(argc, argv, envp, auxv));
	// __builtin_unreachable();

	size_t load_base = 0;
	for (size_t i = 0; auxv[i] != AT_NULL; i += 2) {
		if (auxv[i] == AT_PHDR) {
			load_base = auxv[i + 1];
			break;
		}
	}
	sys_exit(0);
	return load_base;
}

__attribute__((section(".signature")))
__attribute__((used))
char const signature[] = "Famine version 1.0 (c)oded by cbopp-mvan-wij";

__attribute__((section("._start")))
__attribute__((naked))
void _start() {
	// callee-saved RBX, RSP, RBP, and R12–R15
	asm volatile(
		"movq %%rdx, %%rbx\n" // save rtld_fini
		"leaq (%%rsp), %%rdi\n" // pass address of rsp to my_start
		"call my_start\n"
		"movq %%rax, %%rbp\n" // rbp = load_base
		"movq %%rbx, %%rdx\n" // restore rtld_fini
		"hlt\n"
		:
	);
}
