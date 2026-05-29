
#ifndef UFDT_UTIL_H
#define UFDT_UTIL_H

#include "fdt_internal.h"
#include "ufdt_types.h"

static const char *tag_name(uint32_t tag) {
  switch (tag) {
    case FDT_BEGIN_NODE:
      return "FDT_BEGIN_NODE";
    case FDT_END_NODE:
      return "FDT_END_NODE";
    case FDT_PROP:
      return "FDT_PROP";
    case FDT_END:
      return "FDT_END";
  }
  return "";
}

static const char *get_name(void *fdtp, struct ufdt_node *node) {
  if (!fdtp || !node) return NULL;

  const struct fdt_node_header *nh;
  const struct fdt_property *prop;

  uint32_t tag = tag_of(node);

  switch (tag) {
    case FDT_BEGIN_NODE:
      nh = (const struct fdt_node_header *)node->fdt_tag_ptr;
      return nh->name;
    case FDT_PROP:
      prop = (const struct fdt_property *)node->fdt_tag_ptr;
      return fdt_string(fdtp, fdt32_to_cpu(prop->nameoff));
    default:
      return "";
  }
}

static const void *value_of(const struct ufdt_node *node) {
  if (!node) {
    dto_error( "Failed to get value since tree or node is NULL\n");
    return NULL;
  }
  return node->fdt_tag_ptr;
}

static int get_hash_len(const char *str, int len) {
  int res = 0;
  for (int i = 0; i < len; i++) {
    res = res * HASH_BASE;
    res = res + str[i];
  }
  return res;
}
static int get_hash(const char *str) { return get_hash_len(str, dto_strlen(str)); }

#endif /* UFDT_UTIL_H */
