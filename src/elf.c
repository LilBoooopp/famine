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
