#ifndef UFDT_TYPES_H
#define UFDT_TYPES_H

#include <libfdt.h>

#define ASCII_PRINT_S (32)
#define ASCII_PRINT_E (128)
#define ASCII_PRINT_SZ (ASCII_PRINT_E - ASCII_PRINT_S)

#define FDT_PROP_DELI ':'
#define FDT_NODE_DELI '/'

#define DTNL_INIT_SZ 4

/* Empirical values for hash functions. */
#define HASH_BASE 13131

/* it has type : struct ufdt_node** */
#define for_each(it, node_dict)                                  \
  if ((node_dict) != NULL)                                       \
    for (it = (node_dict)->nodes;                                \
         it != (node_dict)->nodes + (node_dict)->mem_size; ++it) \
      if (*it)

#define for_each_child(it, node)                                    \
  if (tag_of(node) == FDT_BEGIN_NODE)                               \
    for ((it) = &(((struct fdt_node_ufdt_node *)node)->child); *it; \
         it = &((*it)->sibling))

#define for_each_prop(it, node) \
  for_each_child(it, node) if (tag_of(*it) == FDT_PROP)

#define for_each_node(it, node) \
  for_each_child(it, node) if (tag_of(*it) == FDT_BEGIN_NODE)

/*
 * Gets prop name from FDT requires complicated manipulation.
 * To avoid the manipulation, the *name pointer in fdt_prop_ufdt_node
 * is pointed to the final destination of the prop name in FDT.
 * For the FDT_BEGIN_NODE name, it can be obtained from FDT directly.
 */
#define name_of(node)                                                    \
  ((tag_of(node) == FDT_BEGIN_NODE)                                      \
       ? (((const struct fdt_node_header *)((node)->fdt_tag_ptr))->name) \
       : (((const struct fdt_prop_ufdt_node *)node)->name))

struct ufdt_node {
  fdt32_t *fdt_tag_ptr;
  struct ufdt_node *sibling;
};

struct ufdt_node_dict {
  int mem_size;
  int num_used;
  struct ufdt_node **nodes;
};

struct fdt_prop_ufdt_node {
  struct ufdt_node parent;
  const char *name;
};

struct fdt_node_ufdt_node {
  struct ufdt_node parent;
  struct ufdt_node *child;
  struct ufdt_node **last_child_p;
};

struct phandle_table_entry {
  uint32_t phandle;
  struct ufdt_node *node;
};

struct static_phandle_table {
  int len;
  struct phandle_table_entry *data;
};

struct ufdt {
  void *fdtp;
  struct ufdt_node *root;
  struct static_phandle_table phandle_table;
};

typedef void func_on_ufdt_node(struct ufdt_node *, void *);

struct ufdt_node_closure {
  func_on_ufdt_node *func;
  void *env;
};

static uint32_t tag_of(const struct ufdt_node *node) {
  if (!node) return FDT_END;
  return fdt32_to_cpu(*node->fdt_tag_ptr);
}

#endif /* UFDT_TYPES_H */
