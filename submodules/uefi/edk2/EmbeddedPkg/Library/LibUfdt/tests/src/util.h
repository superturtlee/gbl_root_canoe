#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>


char *load_file(const char *filename, size_t *pLen);
int write_fdt_to_file(const char *filename, void *fdt);

#endif
