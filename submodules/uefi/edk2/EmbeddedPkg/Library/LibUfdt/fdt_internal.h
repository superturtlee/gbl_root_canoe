#ifndef FDT_INTERNAL_H
#define FDT_INTERNAL_H

/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 *
 * libfdt is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 *
 *  a) This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of the
 *     License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this library; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *     MA 02110-1301 USA
 *
 * Alternatively,
 *
 *  b) Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *     1. Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *     NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *     OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *     EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "libfdt.h"
#include "libufdt_sysdeps.h"


/*
 * Most of the codes below are copied from external/dtc/libfdt/libfdt_internal.h
 * and external/dtc/libfdt/fdt_sw.c .
 */

#define FDT_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define FDT_TAGALIGN(x) (FDT_ALIGN((x), FDT_TAGSIZE))

static inline const void *_fdt_offset_ptr(const void *fdt, int offset) {
  return (const char *)fdt + fdt_off_dt_struct(fdt) + offset;
}

static inline void *_fdt_offset_ptr_w(void *fdt, int offset) {
  return (void *)(uintptr_t)_fdt_offset_ptr(fdt, offset);
}

#define FDT_SW_MAGIC (~FDT_MAGIC)

static int _fdt_sw_check_header(void *fdt) {
  if (fdt_magic(fdt) != FDT_SW_MAGIC) return -FDT_ERR_BADMAGIC;
  /* FIXME: should check more details about the header state */
  return 0;
}

#define FDT_SW_CHECK_HEADER(fdt)                            \
  {                                                         \
    int err;                                                \
    if ((err = _fdt_sw_check_header(fdt)) != 0) return err; \
  }

static void *_fdt_grab_space(void *fdt, size_t len) {
  int offset = fdt_size_dt_struct(fdt);
  int spaceleft;

  spaceleft =
      fdt_totalsize(fdt) - fdt_off_dt_struct(fdt) - fdt_size_dt_strings(fdt);

  if ((offset + (int)len < offset) || (offset + (int)len > spaceleft))
    return NULL;

  fdt_set_size_dt_struct(fdt, offset + len);
  return _fdt_offset_ptr_w(fdt, offset);
}

static int _fdt_add_string(void *fdt, const char *s) {
  char *strtab = (char *)fdt + fdt_totalsize(fdt);
  int strtabsize = fdt_size_dt_strings(fdt);
  int len = dto_strlen(s) + 1;
  int struct_top, offset;

  /* Add it */
  offset = -strtabsize - len;
  struct_top = fdt_off_dt_struct(fdt) + fdt_size_dt_struct(fdt);
  if (fdt_totalsize(fdt) + offset < (size_t)struct_top)
    return 0; /* no more room :( */

  dto_memcpy(strtab + offset, s, len);
  fdt_set_size_dt_strings(fdt, strtabsize + len);
  return offset;
}

/*
 * From Google, speed up fdt_property() by passing nameoff to the function.
 * Adds new string to fdt if *pnameoff is 0.
 * Otherwise, use the nameoff provided.
 */
static int fast_fdt_sw_property(void *fdt, const char *name, const void *val,
                                int len, int *pnameoff,
                                struct fdt_property **prop_ptr) {
  FDT_SW_CHECK_HEADER(fdt);

  if (*pnameoff == 0) {
    *pnameoff = _fdt_add_string(fdt, name);
    if (*pnameoff == 0) return -FDT_ERR_NOSPACE;
  }

  *prop_ptr = _fdt_grab_space(fdt, sizeof(**prop_ptr) + FDT_TAGALIGN(len));
  if (*prop_ptr == NULL) return -FDT_ERR_NOSPACE;

  (*prop_ptr)->tag = cpu_to_fdt32(FDT_PROP);
  (*prop_ptr)->nameoff = cpu_to_fdt32(*pnameoff);
  (*prop_ptr)->len = cpu_to_fdt32(len);
  dto_memcpy((*prop_ptr)->data, val, len);
  return 0;
}

#endif /* FDT_INTERNAL_H */
