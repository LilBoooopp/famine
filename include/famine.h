#ifndef FAMINE_H
# define FAMINE_H

# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif

# include <elf.h>
# include <fcntl.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <unistd.h>


# if DEBUG
#  include <stdio.h>
#  define LOG(...) fprintf(stderr, __VA_ARGS__)
# else
#  define LOG(...) ((void)0)
# endif

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

# define PAGE_SIZE	0x1000
# define STUB_VADDR	0xc000000

/* sentinels patched inside the stub at injection time */
# define PLACE_ENTRY	0xAAAAAAAAAAAAAAAAULL
# define PLACE_PHDR		0xFEEDFACEFEEDFACEULL

extern unsigned char	stub_bin[];
extern unsigned int		stub_bin_len;

typedef struct s_famine
{
	void	*out; // writable output image
	size_t	out_size; // total bytes in out
	size_t	stub_offset; // page-aligned file offset of a stub
	Elf64_Ehdr *ehdr; // -> out
	Elf64_Phdr *phdr; // -> prograsm header table in out
	Elf64_Phdr *note; //PT_NOTE segment to repurpose
	uint64_t	phdr_vaddr;
}	t_famine;

// elf.c
int	is_valid_elf(const Elf64_Ehdr *elf);
int	find_segments(t_famine *f);

void	inject_stub(t_famine *f);

int	infect_file(const char *in_path, const char *out_path);

#endif
