/*-
 * Copyright (c) 2015 Oleksandr Tymoshenko <gonzo@FreeBSD.org>
 * All rights reserved.
 *
 * This software was developed by Semihalf under sponsorship from
 * the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "ufdt_overlay.h"

#include "libufdt.h"


/*
 * The original version of fdt_overlay.c is slow in searching for particular
 * nodes and adding subnodes/properties due to the operations on flattened
 * device tree (FDT).
 *
 * Here we introduce `libufdt` which builds a real tree structure (named
 * ufdt -- unflattned device tree) from FDT. In the real tree, we can perform
 * certain operations (e.g., merge 2 subtrees, search for a node by path) in
 * almost optimal time complexity with acceptable additional memory usage.
 *
 * This file is the improved version of fdt_overlay.c by using the real tree
 * structure defined in libufdt.
 *
 * How the device tree overlay works and some
 * special terms (e.g., fixups, local fixups, fragment, etc)
 * are described in the document
 * external/dtc/Documentation/dt-object-internal.txt.
 */

/* BEGIN of operations about phandles in ufdt. */

/*
 * Increases u32 value at pos by offset.
 */
static void fdt_increase_u32(void *pos, uint32_t offset) {
  uint32_t val;

  dto_memcpy(&val, pos, sizeof(val));
  val = cpu_to_fdt32(fdt32_to_cpu(val) + offset);
  dto_memcpy(pos, &val, sizeof(val));
}

/*
 * Gets the max phandle of a given ufdt.
 */
static uint32_t ufdt_get_max_phandle(struct ufdt *tree) {
  struct static_phandle_table sorted_table = tree->phandle_table;
  if (sorted_table.len > 0)
    return sorted_table.data[sorted_table.len - 1].phandle;
  else
    return 0;
}

/*
 * Tries to increase the phandle value of a node
 * if the phandle exists.
 */
static void ufdt_node_try_increase_phandle(struct ufdt_node *node,
                                           uint32_t offset) {
  int len = 0;
  char *prop_data = ufdt_node_get_fdt_prop_data_by_name(node, "phandle", &len);
  if (prop_data != NULL && len == sizeof(fdt32_t)) {
    fdt_increase_u32(prop_data, offset);
  }
  prop_data = ufdt_node_get_fdt_prop_data_by_name(node, "linux,phandle", &len);
  if (prop_data != NULL && len == sizeof(fdt32_t)) {
    fdt_increase_u32(prop_data, offset);
  }
}

/*
 * Increases all phandles by offset in a ufdt
 * in O(n) time.
 */
static void ufdt_try_increase_phandle(struct ufdt *tree, uint32_t offset) {
  struct static_phandle_table sorted_table = tree->phandle_table;
  int i;

  for (i = 0; i < sorted_table.len; i++) {
    struct ufdt_node *target_node = sorted_table.data[i].node;

    ufdt_node_try_increase_phandle(target_node, offset);
  }
}

/* END of operations about phandles in ufdt. */

/*
 * In the overlay_tree, there are some references (phandle)
 * pointing to somewhere in the main_tree.
 * Fix-up operations is to resolve the right address
 * in the overlay_tree.
 */

/* BEGIN of doing fixup in the overlay ufdt. */

/*
 * Returns exact memory location specified by fixup in format
 * /path/to/node:property:offset.
 * A property might contain multiple values and the offset is used to locate a
 * reference inside the property.
 * e.g.,
 * "property"=<1, 2, &ref, 4>, we can use /path/to/node:property:8 to get ref,
 * where 8 is sizeof(uint32) + sizeof(unit32).
 */
static void *ufdt_get_fixup_location(struct ufdt *tree, const char *fixup) {
  char *path, *prop_ptr, *offset_ptr, *end_ptr;
  int prop_offset, prop_len;
  const char *prop_data;

  /*
   * TODO(akaineko): Keep track of substring lengths so we don't have to
   * dto_malloc a copy and split it up.
   */
  path = dto_strdup(fixup);
  prop_ptr = dto_strchr(path, ':');
  if (prop_ptr == NULL) {
    dto_error("Missing property part in '%s'\n", path);
    goto fail;
  }

  *prop_ptr = '\0';
  prop_ptr++;

  offset_ptr = dto_strchr(prop_ptr, ':');
  if (offset_ptr == NULL) {
    dto_error("Missing offset part in '%s'\n", path);
    goto fail;
  }

  *offset_ptr = '\0';
  offset_ptr++;

  prop_offset = dto_strtoul(offset_ptr, &end_ptr, 10 /* base */);
  if (*end_ptr != '\0') {
    dto_error("'%s' is not valid number\n", offset_ptr);
    goto fail;
  }

  struct ufdt_node *target_node;
  target_node = ufdt_get_node_by_path(tree, path);
  if (target_node == NULL) {
    dto_error("Path '%s' not found\n", path);
    goto fail;
  }

  prop_data =
      ufdt_node_get_fdt_prop_data_by_name(target_node, prop_ptr, &prop_len);
  if (prop_data == NULL) {
    dto_error("Property '%s' not found in  '%s' node\n", prop_ptr, path);
    goto fail;
  }
  /*
   * Note that prop_offset is the offset inside the property data.
   */
  if (prop_len < prop_offset + (int)sizeof(uint32_t)) {
    dto_error("%s: property length is too small for fixup\n", path);
    goto fail;
  }

  dto_free(path);
  return (char *)prop_data + prop_offset;

fail:
  dto_free(path);
  return NULL;
}

/*
 * Process one entry in __fixups__ { } node.
 * @fixups is property value, array of NUL-terminated strings
 *   with fixup locations.
 * @fixups_len length of the fixups array in bytes.
 * @phandle is value for these locations.
 */
static int ufdt_do_one_fixup(struct ufdt *tree, const char *fixups,
                             int fixups_len, int phandle) {
  void *fixup_pos;
  uint32_t val;

  val = cpu_to_fdt32(phandle);

  while (fixups_len > 0) {
    fixup_pos = ufdt_get_fixup_location(tree, fixups);
    if (fixup_pos != NULL) {
      dto_memcpy(fixup_pos, &val, sizeof(val));
    } else {
      return -1;
    }

    fixups_len -= dto_strlen(fixups) + 1;
    fixups += dto_strlen(fixups) + 1;
  }

  return 0;
}

/*
 * Handle __fixups__ node in overlay tree.
 */

static int ufdt_overlay_do_fixups(struct ufdt *main_tree,
                                  struct ufdt *overlay_tree) {
  int len = 0;
  struct ufdt_node *main_symbols_node, *overlay_fixups_node;

  main_symbols_node = ufdt_get_node_by_path(main_tree, "/__symbols__");
  overlay_fixups_node = ufdt_get_node_by_path(overlay_tree, "/__fixups__");

  if (!main_symbols_node) {
    dto_error("Bad main_symbols in ufdt_overlay_do_fixups\n");
    return -1;
  }

  if (!overlay_fixups_node) {
    dto_error("Bad overlay_fixups in ufdt_overlay_do_fixups\n");
    return -1;
  }

  struct ufdt_node **it;
  for_each_prop(it, overlay_fixups_node) {
    /*
     * A property in __fixups__ looks like:
     * symbol_name =
     * "/path/to/node:prop:offset0\x00/path/to/node:prop:offset1..."
     * So we firstly find the node "symbol_name" and obtain its phandle in
     * __symbols__ of the main_tree.
     */

    struct ufdt_node *fixups = *it;
    char *symbol_path = ufdt_node_get_fdt_prop_data_by_name(
        main_symbols_node, name_of(fixups), &len);

    if (!symbol_path) {
      dto_error("Couldn't find '%s' symbol in main dtb\n", name_of(fixups));
      return -1;
    }

    struct ufdt_node *symbol_node;
    symbol_node = ufdt_get_node_by_path(main_tree, symbol_path);

    if (!symbol_node) {
      dto_error("Couldn't find '%s' path in main dtb\n", symbol_path);
      return -1;
    }

    uint32_t phandle = ufdt_node_get_phandle(symbol_node);

    const char *fixups_paths = ufdt_node_get_fdt_prop_data(fixups, &len);

    if (ufdt_do_one_fixup(overlay_tree, fixups_paths, len, phandle) < 0) {
      dto_error("Failed one fixup in ufdt_do_one_fixup\n");
      return -1;
    }
  }

  return 0;
}

/* END of doing fixup in the overlay ufdt. */

/*
 * Here is to overlay all fragments in the overlay_tree to the main_tree.
 * What is "overlay fragment"? The main purpose is to add some subtrees to the
 * main_tree in order to complete the entire device tree.
 *
 * A frgament consists of two parts: 1. the subtree to be added 2. where it
 * should be added.
 *
 * Overlaying a fragment requires: 1. find the node in the main_tree 2. merge
 * the subtree into that node in the main_tree.
 */

/* BEGIN of applying fragments. */

/*
 * Overlay the overlay_node over target_node.
 */
static int ufdt_overlay_node(struct ufdt_node *target_node,
                             struct ufdt_node *overlay_node) {
  return merge_ufdt_into(target_node, overlay_node);
}

/*
 * Return value of ufdt_apply_fragment().
 */

enum overlay_result {
  OVERLAY_RESULT_OK,
  OVERLAY_RESULT_MISSING_TARGET,
  OVERLAY_RESULT_MISSING_OVERLAY,
  OVERLAY_RESULT_TARGET_PATH_INVALID,
  OVERLAY_RESULT_TARGET_INVALID,
  OVERLAY_RESULT_MERGE_FAIL,
};

/*
 * Apply one overlay fragment (subtree).
 */
static enum overlay_result ufdt_apply_fragment(struct ufdt *tree,
                                               struct ufdt_node *frag_node) {
  uint32_t target;
  const char *target_path;
  const void *val;
  struct ufdt_node *target_node = NULL;
  struct ufdt_node *overlay_node = NULL;

  val = ufdt_node_get_fdt_prop_data_by_name(frag_node, "target", NULL);
  if (val) {
    dto_memcpy(&target, val, sizeof(target));
    target = fdt32_to_cpu(target);
    target_node = ufdt_get_node_by_phandle(tree, target);
    if (target_node == NULL) {
      dto_error("failed to find target %04x\n", target);
      return OVERLAY_RESULT_TARGET_INVALID;
    }
  }

  if (target_node == NULL) {
    target_path =
        ufdt_node_get_fdt_prop_data_by_name(frag_node, "target-path", NULL);
    if (target_path == NULL) {
      return OVERLAY_RESULT_MISSING_TARGET;
    }

    target_node = ufdt_get_node_by_path(tree, target_path);
    if (target_node == NULL) {
      dto_error("failed to find target-path %s\n", target_path);
      return OVERLAY_RESULT_TARGET_PATH_INVALID;
    }
  }

  overlay_node = ufdt_node_get_node_by_path(frag_node, "__overlay__");
  if (overlay_node == NULL) {
    dto_error("missing __overlay__ sub-node\n");
    return OVERLAY_RESULT_MISSING_OVERLAY;
  }

  int err = ufdt_overlay_node(target_node, overlay_node);

  if (err < 0) {
    dto_error("failed to overlay node %s to target %s\n", name_of(overlay_node),
              name_of(target_node));
    return OVERLAY_RESULT_MERGE_FAIL;
  }

  return OVERLAY_RESULT_OK;
}

/*
 * Applies all fragments to the main_tree.
 */
static int ufdt_overlay_apply_fragments(struct ufdt *main_tree,
                                        struct ufdt *overlay_tree) {
  enum overlay_result err;
  struct ufdt_node **it;
  /*
   * This loop may iterate to subnodes that's not a fragment node.
   * In such case, ufdt_apply_fragment would fail with return value = -1.
   */
  for_each_node(it, overlay_tree->root) {
    err = ufdt_apply_fragment(main_tree, *it);
    if (err == OVERLAY_RESULT_MERGE_FAIL) {
      return -1;
    }
  }
  return 0;
}

/* END of applying fragments. */

/*
 * Since the overlay_tree will be "merged" into the main_tree, some
 * references (e.g., phandle values that acts as an unique ID) need to be
 * updated so it won't lead to collision that different nodes have the same
 * phandle value.
 *
 * Two things need to be done:
 *
 * 1. ufdt_try_increase_phandle()
 * Update phandle (an unique integer ID of a node in the device tree) of each
 * node in the overlay_tree. To achieve this, we simply increase each phandle
 * values in the overlay_tree by the max phandle value of the main_tree.
 *
 * 2. ufdt_overlay_do_local_fixups()
 * If there are some reference in the overlay_tree that references nodes
 * inside the overlay_tree, we have to modify the reference value (address of
 * the referenced node: phandle) so that it corresponds to the right node inside
 * the overlay_tree. Where the reference exists is kept in __local_fixups__ node
 * in the overlay_tree.
 */

/* BEGIN of updating local references (phandle values) in the overlay ufdt. */

/*
 * local fixups
 */
static int ufdt_local_fixup_prop(struct ufdt_node *target_prop_node,
                                 struct ufdt_node *local_fixup_prop_node,
                                 uint32_t phandle_offset) {
  /*
   * prop_offsets_ptr should be a list of fdt32_t.
   * <offset0 offset1 offset2 ...>
   */
  char *prop_offsets_ptr;
  int len = 0;
  prop_offsets_ptr = ufdt_node_get_fdt_prop_data(local_fixup_prop_node, &len);

  char *prop_data;
  int target_length = 0;

  prop_data = ufdt_node_get_fdt_prop_data(target_prop_node, &target_length);

  if (prop_offsets_ptr == NULL || prop_data == NULL) return -1;

  int i;
  for (i = 0; i < len; i += sizeof(fdt32_t)) {
    int offset = fdt32_to_cpu(*(fdt32_t *)(prop_offsets_ptr + i));
    if (offset + sizeof(fdt32_t) > (size_t)target_length) return -1;
    fdt_increase_u32((prop_data + offset), phandle_offset);
  }
  return 0;
}

static int ufdt_local_fixup_node(struct ufdt_node *target_node,
                                 struct ufdt_node *local_fixups_node,
                                 uint32_t phandle_offset) {
  if (local_fixups_node == NULL) return 0;

  struct ufdt_node **it_local_fixups;
  struct ufdt_node *sub_target_node;

  for_each_prop(it_local_fixups, local_fixups_node) {
    sub_target_node =
        ufdt_node_get_property_by_name(target_node, name_of(*it_local_fixups));

    if (sub_target_node != NULL) {
      int err = ufdt_local_fixup_prop(sub_target_node, *it_local_fixups,
                                      phandle_offset);
      if (err < 0) return -1;
    } else {
      return -1;
    }
  }

  for_each_node(it_local_fixups, local_fixups_node) {
    sub_target_node =
        ufdt_node_get_node_by_path(target_node, name_of(*it_local_fixups));
    if (sub_target_node != NULL) {
      int err = ufdt_local_fixup_node(sub_target_node, *it_local_fixups,
                                      phandle_offset);
      if (err < 0) return -1;
    } else {
      return -1;
    }
  }

  return 0;
}

static int ufdt_overlay_root_node(struct ufdt *tree,
                                     struct ufdt *overlay_tree)
{
     struct ufdt_node *target_node = ufdt_get_node_by_path(tree, "/");
     struct ufdt_node *overlay_node = ufdt_get_node_by_path(overlay_tree, "/");
     struct ufdt_node **it_prop;
     struct ufdt_node *target_prop;

     if(!target_node)
          return 0;

     for_each_prop(it_prop, overlay_node) {
          target_prop =
               ufdt_node_get_property_by_name(target_node, name_of(*it_prop));

          if (target_prop) {
               if(merge_ufdt_into(target_prop, *it_prop))
                    return -1;
          }
     }

     return 0;
}

/*
 * Handle __local_fixups__ node in overlay DTB
 * The __local_fixups__ format we expect is
 * __local_fixups__ {
 *   path {
 *    to {
 *      local_ref1 = <offset>;
 *    };
 *   };
 *   path2 {
 *    to2 {
 *      local_ref2 = <offset1 offset2 ...>;
 *    };
 *   };
 * };
 *
 * which follows the dtc patch from:
 * https://marc.info/?l=devicetree&m=144061468601974&w=4
 */
static int ufdt_overlay_do_local_fixups(struct ufdt *tree,
                                        uint32_t phandle_offset) {
  struct ufdt_node *overlay_node = ufdt_get_node_by_path(tree, "/");
  struct ufdt_node *local_fixups_node =
      ufdt_get_node_by_path(tree, "/__local_fixups__");

  int err =
      ufdt_local_fixup_node(overlay_node, local_fixups_node, phandle_offset);

  if (err < 0) return -1;

  return 0;
}

static int ufdt_overlay_local_ref_update(struct ufdt *main_tree,
                                         struct ufdt *overlay_tree) {
  uint32_t phandle_offset = 0;

  phandle_offset = ufdt_get_max_phandle(main_tree);
  if (phandle_offset > 0) {
    ufdt_try_increase_phandle(overlay_tree, phandle_offset);
  }

  int err = ufdt_overlay_do_local_fixups(overlay_tree, phandle_offset);
  if (err < 0) {
    dto_error("failed to perform local fixups in overlay\n");
    return -1;
  }
  return 0;
}

/* END of updating local references (phandle values) in the overlay ufdt. */

static int ufdt_overlay_apply(struct ufdt *main_tree, struct ufdt *overlay_tree,
                              size_t overlay_length) {
  if (overlay_length < sizeof(struct fdt_header)) {
    dto_error("Overlay_length %zu smaller than header size %zu\n",
              overlay_length, sizeof(struct fdt_header));
    return -1;
  }

  if(ufdt_overlay_root_node(main_tree, overlay_tree))
       return -1;

  if (ufdt_overlay_local_ref_update(main_tree, overlay_tree) < 0) {
    dto_error("failed to perform local fixups in overlay\n");
    return -1;
  }

  if (ufdt_overlay_do_fixups(main_tree, overlay_tree) < 0) {
    dto_error("failed to perform fixups in overlay\n");
    return -1;
  }
  if (ufdt_overlay_apply_fragments(main_tree, overlay_tree) < 0) {
    dto_error("failed to apply fragments\n");
    return -1;
  }

  return 0;
}

struct fdt_header *ufdt_install_blob(void *blob, size_t blob_size) {
  struct fdt_header *pHeader;
  int err;

  dto_debug("ufdt_install_blob (0x%08jx)\n", (uintmax_t)blob);

  if (blob_size < sizeof(struct fdt_header)) {
    dto_error("Blob_size %zu smaller than the header size %zu\n", blob_size,
              sizeof(struct fdt_header));
    return NULL;
  }

  pHeader = (struct fdt_header *)blob;
  err = fdt_check_header(pHeader);
  if (err < 0) {
    if (err == -FDT_ERR_BADVERSION) {
      dto_error("incompatible blob version: %d, should be: %d",
                fdt_version(pHeader), FDT_LAST_SUPPORTED_VERSION);

    } else {
      dto_error("error validating blob: %s", fdt_strerror(err));
    }
    return NULL;
  }

  return pHeader;
}

/*
* From Google, based on dt_overlay_apply() logic
* Will dto_malloc a new fdt blob and return it. Will not dto_free parameters.
*/
struct fdt_header *ufdt_apply_overlay(struct fdt_header *main_fdt_header,
                                 size_t main_fdt_size,
                                 void *overlay_fdtp,
                                 size_t overlay_size) {
  size_t out_fdt_size;

  if (main_fdt_header == NULL) {
    return NULL;
  }

  if (overlay_size < 8 || overlay_size != fdt_totalsize(overlay_fdtp)) {
    dto_error("Bad overlay size!\n");
    return NULL;
  }
  if (main_fdt_size < 8 || main_fdt_size != fdt_totalsize(main_fdt_header)) {
    dto_error("Bad fdt size!\n");
    return NULL;
  }

  out_fdt_size = fdt_totalsize(main_fdt_header) + overlay_size;
  /* It's actually more than enough */
  struct fdt_header *out_fdt_header = dto_malloc(out_fdt_size);

  if (out_fdt_header == NULL) {
    dto_error("failed to allocate memory for DTB blob with overlays\n");
    return NULL;
  }

  struct ufdt *main_tree, *overlay_tree;

  main_tree = fdt_to_ufdt(main_fdt_header, main_fdt_size);

  overlay_tree = fdt_to_ufdt(overlay_fdtp, overlay_size);

  int err = ufdt_overlay_apply(main_tree, overlay_tree, overlay_size);
  if (err < 0) {
    goto fail;
  }

  err = ufdt_to_fdt(main_tree, out_fdt_header, out_fdt_size);
  if (err < 0) {
    dto_error("Failed to dump the device tree to out_fdt_header\n");
    goto fail;
  }

  ufdt_destruct(main_tree);
  ufdt_destruct(overlay_tree);
  return out_fdt_header;

fail:
  ufdt_destruct(main_tree);
  ufdt_destruct(overlay_tree);
  dto_free(out_fdt_header);
  return NULL;
}

struct fdt_header *ufdt_apply_multi_overlay(struct fdt_header *main_fdt_header,
                                    size_t main_fdt_size,
                                    struct fdt_entry_node *overlay_dt_list) {
  size_t out_fdt_size = 0;
  struct ufdt *main_tree, *overlay_tree;
  struct fdt_header *out_fdt_header;
  struct fdt_entry_node *temp = NULL;
  int err, i=0;

  if (main_fdt_header == NULL ||
      main_fdt_size < 8 ||
      main_fdt_size != fdt_totalsize(main_fdt_header)) {
    dto_error("Bad main fdt!\n");
    return NULL;
  }
  if (overlay_dt_list == NULL) {
    dto_error("Dts list is invalid: NULL!\n");
    return NULL;
  }

  /* Save the DTs List address to reuse later */
  temp = overlay_dt_list;

  out_fdt_size = main_fdt_size;
  while (overlay_dt_list != NULL) {
    if ((void *)overlay_dt_list->address == NULL ||
        overlay_dt_list->size < 8 ||
        overlay_dt_list->size != fdt_totalsize(
                                       (void *)overlay_dt_list->address)) {
      dto_error("Bad overlay fdt! index: %d\n", i);
      return NULL;
    }
    out_fdt_size += overlay_dt_list->size;
    overlay_dt_list = overlay_dt_list->next;
    i++;
  }

  out_fdt_header = dto_malloc(out_fdt_size);
  if (out_fdt_header == NULL) {
    dto_error("failed to allocate memory for DTB blob with overlays\n");
    return NULL;
  }

  main_tree = fdt_to_ufdt((void *)main_fdt_header, main_fdt_size);

  /* Apply overlay with first dts from the list, and iterate the loop from
     index 1 to avoid unnecessary rebuilding of phandle table
   */

  /* Recover list from saved copy */
  overlay_dt_list = temp;
  i=1;
  overlay_tree = fdt_to_ufdt((void *)overlay_dt_list->address,
                             overlay_dt_list->size);
  err = ufdt_overlay_apply(main_tree, overlay_tree, overlay_dt_list->size);
  ufdt_destruct(overlay_tree);
  if (err < 0) {
    dto_error("Failed to apply devie tree\n");
    goto fail;
  }

  overlay_dt_list = overlay_dt_list->next;
  while (overlay_dt_list != NULL) {
    overlay_tree = fdt_to_ufdt((void *)overlay_dt_list->address,
                               overlay_dt_list->size);
    /* Rebuild the phandle_table for the combined tree.*/
    main_tree->phandle_table = build_phandle_table(main_tree);

    err = ufdt_overlay_apply(main_tree, overlay_tree, overlay_dt_list->size);
    ufdt_destruct(overlay_tree);
    if (err < 0) {
      dto_error("Failed to apply devie tree, index: %d\n", i);
      goto fail;
    }
    overlay_dt_list = overlay_dt_list->next;
    i++;
  }

  err = ufdt_to_fdt(main_tree, out_fdt_header, out_fdt_size);
  if (err < 0) {
    dto_error("Failed to dump the device tree to out_fdt_header\n");
    goto fail;
  }

  ufdt_destruct(main_tree);
  return out_fdt_header;

fail:
  ufdt_destruct(main_tree);
  dto_free(out_fdt_header);
  return NULL;
}
