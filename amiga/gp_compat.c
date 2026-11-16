/* See gp_compat.h. MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdlib.h>
#include <string.h>
#include "gp_compat.h"

char *gp_compat_strndup(const char *s, size_t n)
{
	size_t len = 0;
	char *d;
	while (len < n && s[len]) len++;
	if (!(d = malloc(len + 1))) return NULL;
	memcpy(d, s, len);
	d[len] = '\0';
	return d;
}
