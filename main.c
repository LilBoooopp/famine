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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include "stub.h"
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Elf64_Ehdr (file header)
 *	e_ident[16]	magic + class(32/64) + endianness + OS ABI
 * 	e_type		ET_EXEC (non-PIE) / ET_DYN (PIE or shared object)
 * 	e_machine	target ISA (EM_X86_64)
 * 	e_entry		virtual address of entry point (_start)
 * 	e_phoff		file offset of the program header table
 * 	e_phentsize	size of one program header entry
 * 	e_phnum		number of program header entries
 *
 * Elf64_Phdr (program header /segment)
 *	p_type	PT_LOAD, PT, NOT, PT_PHDR, ...
 *	p_flags	PF_R | PF_W | PF_X
 *	p_offset	file offset of the segment's data
 *	p_vaddr		virtual address the segment is loaded at
 *	p_filesz	size of the segment in the file
 *	p_memsz		size of the segment in memory (>= p_filesz)
 *	p_align		alighment: p_vaddr must be congruent to p_offset mod p_align
 */

/**
 * @brief validates the ELF magic bytes at the start of file (0x7f)
 */
int check_mem(Elf64_Ehdr *elf)
{
	if (memcmp(elf->e_ident, ELFMAG, SELFMAG) != 0)
		return (printf("Error: Not an ELF file\n"), 1);
	return (0);
}

/**
 * @brief Check for 64-bit
 * e_ident[EI_CLASS] is ELFCLASS32 (1) or ELFCLASS64 (2).
 */
int check_64bit(Elf64_Ehdr *elf) {
	if (elf->e_ident[EI_CLASS] != ELFCLASS64)
		return (printf("Error: Not a 64-bit ELF\n"), 1);
	return (0);
}

/**
 * @brief validates architecture.
 * EM_X86_64 = 62
 */
int check_8664(Elf64_Ehdr *elf) {
	if (elf->e_machine != EM_X86_64)
		return (printf("File architecture not supported. x86_64 only\n"), 1);
	return (0);
}

/**
 * @brief Checks that file is executable or shared library.
 */
int check_exec(Elf64_Ehdr *elf) {
	if (elf->e_type != ET_EXEC && elf->e_type != ET_DYN)
		return (printf("Error: Nt an executable\n"), 1);
	return (0);
}

int check_elf(Elf64_Ehdr *elf) {
	int ret = 0;
	ret += check_mem(elf);
	ret += check_64bit(elf);
	ret += check_8664(elf);
	ret += check_exec(elf);
	return (ret);
}

/*
* Turn the PT_NOTE segment into a PT_LOAD segment that maps our stub,
* and redirect the entry point to it. The stub runs first, then jumps
* back to the original entry point so the host binary behaves normally.
*/
void inject_stub(void *woody, Elf64_Phdr *note_segment, Elf64_Ehdr *ehdr, size_t stub_offset, uint64_t phdr_link_vaddr)
{
  	uint64_t placeholder;
  	uint64_t real_value;
  	void *patch_site;

  	memcpy(woody + stub_offset, stub_bin, stub_bin_len);

  	placeholder = 0xAAAAAAAAAAAAAAAA;
  	real_value = ehdr->e_entry;
  	patch_site = memmem(woody + stub_offset, stub_bin_len, &placeholder, 8);
  	if (patch_site)
  		memcpy(patch_site, &real_value, 8);

  	placeholder = 0xFEEDFACEFEEDFACE;
  	real_value = phdr_link_vaddr;
  	patch_site = memmem(woody + stub_offset, stub_bin_len, &placeholder, 8);
  	if (patch_site)
  		memcpy(patch_site, &real_value, 8);

	note_segment->p_type = PT_LOAD;
	note_segment->p_flags = PF_R | PF_X;
	note_segment->p_offset = stub_offset;
	note_segment->p_vaddr = 0xc000000;
	note_segment->p_paddr = 0xc000000;
	note_segment->p_filesz = stub_bin_len;
	note_segment->p_memsz = stub_bin_len;
	note_segment->p_align = 0x1000;

	ehdr->e_entry = 0xc000000;
}

int main(int argc, char **argv) {
	if (argc != 2)
  		return (1);

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		return (printf("Error: could not open file\n"), 1);

	struct stat st;
	if (fstat(fd, &st) < 0)
		return (printf("Error: fstat failed\n"), close(fd), 1);
	if (st.st_size < (off_t)sizeof(Elf64_Ehdr))
		return (printf("Error: file too small\n"), close(fd), 1);

	void *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED)
		return (printf("Error: mmap failed\n"), close(fd), 1);

	if (check_elf((Elf64_Ehdr *) map)) {
		munmap(map, st.st_size);
		close(fd);
		return (1);
	}

	size_t page_size = 0x1000;
	size_t stub_offset = (st.st_size + page_size - 1) & ~(page_size - 1);
	size_t output_size = stub_offset + stub_bin_len;

	void *woody = malloc(output_size);
	if (!woody)
		return (munmap(map, st.st_size), close(fd), 1);
	memset(woody, 0, output_size);
	memcpy(woody, map, st.st_size);
	munmap(map, st.st_size);
	close(fd);

	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)woody;
	Elf64_Phdr *phdr = (Elf64_Phdr *)(woody + ehdr->e_phoff);

	// finding .note and PT_PHDR (for load_base computation in stub)
	Elf64_Phdr *note_segment = NULL;
	uint64_t phdr_link_vaddr = ehdr->e_phoff;  // fallback: works for PIE
	for (int i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type == PT_NOTE && !note_segment)
			note_segment = &phdr[i];
		if (phdr[i].p_type == PT_PHDR)
			phdr_link_vaddr = phdr[i].p_vaddr;
	}
	if (!note_segment)
		return (printf("Error: PT_NOTE segment not found\n"), free(woody), 1);
	
	inject_stub(woody, note_segment, ehdr, stub_offset, phdr_link_vaddr);
	
	// write to disk
	int fd_output = open("woody", O_WRONLY | O_CREAT | O_TRUNC, 0755);
	if (fd_output < 0)
		return (printf("Error: Woody open() failed"), free(woody), 1);
	if (write(fd_output, woody, output_size) < 0)
		return (printf("Error: write() failed"), free(woody), close(fd_output), 1);
	close(fd_output);
	
	free(woody);
	return (0);
}
