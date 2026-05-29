#ifndef UFDT_OVERLAY_H
#define UFDT_OVERLAY_H

#include <libfdt.h>

/* Given a buffer in RAM containing the contents of a .dtb file,
 * it initializes an FDT in-place and returns a pointer to the
 * given buffer, or NULL in case of error.
 * In case of error, it may printf() diagnostic messages.
 */
struct fdt_header *ufdt_install_blob(void *blob, size_t blob_size);

/* Given a main_fdt_header buffer and an overlay_fdtp buffer containing the
 * contents of a .dtbo file, it creates a new FDT containing the applied
 * overlay_fdtp in a dto_malloc'd buffer and returns it, or NULL in case of
 * error.
 * It is allowed to modify the buffers (both main_fdt_header and overlay_fdtp
 * buffer) passed in.
 * It does not dto_free main_fdt_header and overlay_fdtp buffer passed in.
 */
struct fdt_header *ufdt_apply_overlay(struct fdt_header *main_fdt_header,
                                      size_t main_fdt_size,
                                      void *overlay_fdtp,
                                      size_t overlay_size);

struct fdt_header *ufdt_apply_multi_overlay(struct fdt_header *main_fdt_header,
                                    size_t main_fdt_size,
                                    struct fdt_entry_node *overlay_dt_list);
#endif /* UFDT_OVERLAY_H */
