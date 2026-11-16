/* C library calls gnuplot uses that libnix lacks, given under our own
 * names (so a later libnix that has them doesn't clash). Taken into every
 * file of the Amiga build with -include. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GP_COMPAT_H
#define GP_COMPAT_H
#include <stddef.h>
char *gp_compat_strndup(const char *s, size_t n);
#define strndup gp_compat_strndup
#endif
