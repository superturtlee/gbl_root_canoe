#include "libufdt.h"

#include "fdt_internal.h"
#include "ufdt_util.h"


struct ufdt *ufdt_construct(void *fdtp) {
  struct ufdt *res_ufdt = dto_malloc(sizeof(struct ufdt));
  res_ufdt->fdtp = fdtp;
  res_ufdt->root = NULL;

  return res_ufdt;
}

void ufdt_destruct(struct ufdt *tree) {
  ufdt_node_destruct(tree->root);
  dto_free(tree->phandle_table.data);
}

static struct ufdt_node *ufdt_new_node(void *fdtp, int node_offset) {
  if (fdtp == NULL) {
    dto_error("Failed to get new_node because tree is NULL\n");
    return NULL;
  }

  fdt32_t *fdt_tag_ptr =
      (fdt32_t *)fdt_offset_ptr(fdtp, node_offset, sizeof(fdt32_t));
  struct ufdt_node *res = ufdt_node_construct(fdtp, fdt_tag_ptr);
  return res;
}

static struct ufdt_node *fdt_to_ufdt_tree(void *fdtp, int cur_fdt_tag_offset,
                                          int *next_fdt_tag_offset,
                                          int cur_tag) {
  if (fdtp == NULL) {
    return NULL;
  }
  uint32_t tag;
  struct ufdt_node *res, *child_node;

  res = NULL;
  child_node = NULL;
  tag = cur_tag;

  switch (tag) {
    case FDT_END_NODE:
    case FDT_NOP:
    case FDT_END:
      break;

    case FDT_PROP:
      res = ufdt_new_node(fdtp, cur_fdt_tag_offset);
      break;

    case FDT_BEGIN_NODE:
      res = ufdt_new_node(fdtp, cur_fdt_tag_offset);

      do {
        cur_fdt_tag_offset = *next_fdt_tag_offset;
        tag = fdt_next_tag(fdtp, cur_fdt_tag_offset, next_fdt_tag_offset);
        child_node = fdt_to_ufdt_tree(fdtp, cur_fdt_tag_offset,
                                      next_fdt_tag_offset, tag);
        ufdt_node_add_child(res, child_node);
      } while (tag != FDT_END_NODE);
      break;

    default:
      break;
  }

  return res;
}

void ufdt_print(struct ufdt *tree) { ufdt_node_print(tree->root, 0); }

struct ufdt_node *ufdt_get_node_by_path_len(struct ufdt *tree, const char *path,
                                            int len) {
  /*
   * RARE: aliases
   * In device tree, we can assign some alias to specific nodes by defining
   * these relation in "/aliases" node.
   * The node has the form:
   * {
   *   a = "/a_for_apple";
   *   b = "/b_for_banana";
   * };
   * So the path "a/subnode_1" should be expanded to "/a_for_apple/subnode_1".
   */
  if (*path != '/') {
    const char *end = path + len;

    const char *next_slash;
    next_slash = dto_memchr(path, '/', end - path);
    if (!next_slash) next_slash = end;

    struct ufdt_node *aliases_node =
        ufdt_node_get_node_by_path(tree->root, "/aliases");
    aliases_node = ufdt_node_get_property_by_name_len(aliases_node, path,
                                                      next_slash - path);

    int path_len = 0;
    const char *alias_path =
        ufdt_node_get_fdt_prop_data(aliases_node, &path_len);

    if (alias_path == NULL) {
      dto_error("Failed to find alias %s\n", path);
      return NULL;
    }

    struct ufdt_node *target_node =
        ufdt_node_get_node_by_path_len(tree->root, alias_path, path_len);

    return ufdt_node_get_node_by_path_len(target_node, next_slash,
                                          end - next_slash);
  }
  return ufdt_node_get_node_by_path_len(tree->root, path, len);
}

struct ufdt_node *ufdt_get_node_by_path(struct ufdt *tree, const char *path) {
  return ufdt_get_node_by_path_len(tree, path, dto_strlen(path));
}

struct ufdt_node *ufdt_get_node_by_phandle(struct ufdt *tree,
                                           uint32_t phandle) {
  struct ufdt_node *res = NULL;
  /*
   * Do binary search in phandle_table.data.
   * [s, e) means the possible range which contains target node.
   */
  int s = 0, e = tree->phandle_table.len;
  while (e - s > 1) {
    int mid = s + ((e - s) >> 1);
    uint32_t mid_phandle = tree->phandle_table.data[mid].phandle;
    if (phandle < mid_phandle)
      e = mid;
    else
      s = mid;
  }
  if (e - s > 0 && tree->phandle_table.data[s].phandle == phandle) {
    res = tree->phandle_table.data[s].node;
  }
  return res;
}

int merge_children(struct ufdt_node *node_a, struct ufdt_node *node_b) {
  int err = 0;
  struct ufdt_node *it;
  for (it = ((struct fdt_node_ufdt_node *)node_b)->child; it;) {
    struct ufdt_node *cur_node = it;
    it = it->sibling;
    cur_node->sibling = NULL;
    struct ufdt_node *target_node = NULL;
    if (tag_of(cur_node) == FDT_BEGIN_NODE) {
      target_node = ufdt_node_get_subnode_by_name(node_a, name_of(cur_node));
    } else {
      target_node = ufdt_node_get_property_by_name(node_a, name_of(cur_node));
    }
    if (target_node == NULL) {
      err = ufdt_node_add_child(node_a, cur_node);
    } else {
      err = merge_ufdt_into(target_node, cur_node);
    }
    if (err < 0) return -1;
  }
  /*
   * The ufdt_node* in node_b will be copied to node_a.
   * To prevent the ufdt_node from being freed twice
   * (main_tree and overlay_tree) at the end of function
   * ufdt_apply_overlay(), set this node in node_b
   * (overlay_tree) to NULL.
   */
  ((struct fdt_node_ufdt_node *)node_b)->child = NULL;

  return 0;
}

int merge_ufdt_into(struct ufdt_node *node_a, struct ufdt_node *node_b) {
  if (tag_of(node_a) == FDT_PROP) {
    node_a->fdt_tag_ptr = node_b->fdt_tag_ptr;
    return 0;
  }

  int err = 0;
  err = merge_children(node_a, node_b);
  if (err < 0) return -1;

  return 0;
}

void ufdt_map(struct ufdt *tree, struct ufdt_node_closure closure) {
  ufdt_node_map(tree->root, closure);
}

static int count_phandle_node(struct ufdt_node *node) {
  if (node == NULL) return 0;
  if (tag_of(node) != FDT_BEGIN_NODE) return 0;
  int res = 0;
  if (ufdt_node_get_phandle(node) > 0) res++;
  struct ufdt_node **it;
  for_each_child(it, node) { res += count_phandle_node(*it); }
  return res;
}

static void set_phandle_table_entry(struct ufdt_node *node,
                                    struct phandle_table_entry *data,
                                    int *cur) {
  if (node == NULL || tag_of(node) != FDT_BEGIN_NODE) return;
  int ph = ufdt_node_get_phandle(node);
  if (ph > 0) {
    data[*cur].phandle = ph;
    data[*cur].node = node;
    (*cur)++;
  }
  struct ufdt_node **it;
  for_each_node(it, node) set_phandle_table_entry(*it, data, cur);
  return;
}

int phandle_table_entry_cmp(const void *pa, const void *pb) {
  uint32_t ph_a = ((const struct phandle_table_entry *)pa)->phandle;
  uint32_t ph_b = ((const struct phandle_table_entry *)pb)->phandle;
  if (ph_a < ph_b)
    return -1;
  else if (ph_a == ph_b)
    return 0;
  else
    return 1;
}

struct static_phandle_table build_phandle_table(struct ufdt *tree) {
  struct static_phandle_table res;
  res.len = count_phandle_node(tree->root);
  res.data = dto_malloc(sizeof(struct phandle_table_entry) * res.len);
  int cur = 0;
  set_phandle_table_entry(tree->root, res.data, &cur);
  dto_qsort(res.data, res.len, sizeof(struct phandle_table_entry),
            phandle_table_entry_cmp);
  return res;
}

struct ufdt *fdt_to_ufdt(void *fdtp, size_t fdt_size) {
  (void)(fdt_size); // unused parameter

  struct ufdt *res_tree = ufdt_construct(fdtp);

  int start_offset = fdt_path_offset(fdtp, "/");
  if (start_offset < 0) {
    res_tree->fdtp = NULL;
    return res_tree;
  }

  int end_offset;
  int start_tag = fdt_next_tag(fdtp, start_offset, &end_offset);
  res_tree->root = fdt_to_ufdt_tree(fdtp, start_offset, &end_offset, start_tag);

  res_tree->phandle_table = build_phandle_table(res_tree);

  return res_tree;
}

int ufdt_to_fdt(struct ufdt *tree, void *buf, int buf_size) {
  int err;
  err = fdt_create(buf, buf_size);
  if (err < 0) return -1;

  int n_mem_rsv = fdt_num_mem_rsv(tree->fdtp);
  for (int i = 0; i < n_mem_rsv; i++) {
    uint64_t addr, size;
    fdt_get_mem_rsv(tree->fdtp, i, &addr, &size);
    fdt_add_reservemap_entry(buf, addr, size);
  }

  err = fdt_finish_reservemap(buf);
  if (err < 0) return -1;

  /*
   * Obtains all props for later use because getting them from
   * FDT requires complicated manipulation.
  */
  struct ufdt_node_dict all_props = ufdt_node_dict_construct();
  err = output_ufdt_node_to_fdt(tree->root, buf, &all_props);
  if (err < 0) return -1;

  ufdt_node_dict_destruct(&all_props);

  err = fdt_finish(buf);
  if (err < 0) return -1;

  /*
   * IMPORTANT: fdt_totalsize(buf) might be less than buf_size
   * so this is needed to make use of remain spaces.
   */
  return fdt_open_into(buf, buf, buf_size);
}
