
#include "libufdt.h"
#include "ufdt_util.h"

/*
 * BEGIN of Hash table internal implementations.
 */

#define DICT_LIMIT_NUM 2
#define DICT_LIMIT_DEN 3

static bool _ufdt_node_dict_is_too_full(struct ufdt_node_dict *dict) {
  /*
   * We say a dict is too full if it's DICT_LIMIT_NUM / DICT_LIMIT_DEN full.
   */
  if (dict->num_used * DICT_LIMIT_DEN > dict->mem_size * DICT_LIMIT_NUM)
    return true;
  return false;
}

/*
 * If collision happened, use linear probing to find idx in the hash table.
 */
static int _ufdt_node_dict_find_index_by_name_len(struct ufdt_node **hash_table,
                                                  int size, const char *name,
                                                  int len) {
  if (!name || !size) return -1;
  /*
   * All size should be 2^k for some k
   */
  int hsh = get_hash_len(name, len);
  int idx = hsh & (size - 1);
  for (int cnt = 0; cnt < size; ++cnt) {
    if (hash_table[idx] == NULL) return idx;
    if (node_name_eq(hash_table[idx], name, len) == 1) return idx;
    ++idx;
    idx &= (size - 1);
  }
  return -1;
}

static int _ufdt_node_dict_find_index_by_name(struct ufdt_node **hash_table,
                                              int size, const char *name) {
  return _ufdt_node_dict_find_index_by_name_len(hash_table, size, name,
                                                dto_strlen(name));
}

static int _ufdt_node_dict_find_index_in_ht(struct ufdt_node **hash_table,
                                            int size, struct ufdt_node *x) {
  if (x == NULL) return -1;
  return _ufdt_node_dict_find_index_by_name(hash_table, size, name_of(x));
}

/*
 * END of Hash table internal implementations.
 */

/*
 * ufdt_node_dict methods.
 */

struct ufdt_node_dict ufdt_node_dict_construct() {
  struct ufdt_node_dict res;
  res.mem_size = DTNL_INIT_SZ;
  res.num_used = 0;
  res.nodes = dto_malloc(DTNL_INIT_SZ * sizeof(struct ufdt_node *));
  if (res.nodes == NULL) {
    res.mem_size = 0;
    return res;
  }
  dto_memset(res.nodes, 0, DTNL_INIT_SZ * sizeof(struct ufdt_node *));
  return res;
}

void ufdt_node_dict_destruct(struct ufdt_node_dict *dict) {
  if (dict == NULL) return;
  dto_free(dict->nodes);
  dict->mem_size = dict->num_used = 0;
}

static int ufdt_node_dict_resize(struct ufdt_node_dict *dict) {
  if (dict == NULL) return -1;

  int new_size = dict->mem_size << 1;

  struct ufdt_node **new_nodes =
      dto_malloc(new_size * sizeof(struct ufdt_node *));

  dto_memset(new_nodes, 0, new_size * sizeof(struct ufdt_node *));

  for (int i = 0; i < dict->mem_size; i++) {
    struct ufdt_node *node = dict->nodes[i];
    if (node == NULL) continue;
    int idx = _ufdt_node_dict_find_index_in_ht(new_nodes, new_size, node);
    if (idx < 0) {
      dto_error(
          "failed to find new index in ufdt_node_dict resize for entry :%s:\n",
          name_of(node));
      dto_free(new_nodes);
      return -1;
    }
    new_nodes[idx] = node;
  }

  dto_free(dict->nodes);

  dict->mem_size = new_size;
  dict->nodes = new_nodes;
  return 0;
}

int ufdt_node_dict_add(struct ufdt_node_dict *dict, struct ufdt_node *node) {
  if (node == NULL) return -1;
  if (dict == NULL) return -1;

  int idx = _ufdt_node_dict_find_index_in_ht(dict->nodes, dict->mem_size, node);
  if (idx < 0) {
    dto_error("failed to find new index in ufdt_node_dict add for entry :%s:\n",
              name_of(node));
    return -1;
  }

  if (dict->nodes[idx] == NULL) ++dict->num_used;
  dict->nodes[idx] = node;

  /*
   * When the hash table is too full, double the size and rehashing.
   */
  int err = 0;
  if (_ufdt_node_dict_is_too_full(dict)) {
    err = ufdt_node_dict_resize(dict);
  }

  return err;
}

/*
 * BEGIN of ufdt_dict searching related methods.
 */

/*
 * return a pointer to the hash table entry
 */
struct ufdt_node **ufdt_node_dict_find_len(struct ufdt_node_dict *dict,
                                           const char *name, int len) {
  if (dict == NULL) return NULL;
  int idx = _ufdt_node_dict_find_index_by_name_len(dict->nodes, dict->mem_size,
                                                   name, len);
  if (idx < 0) return NULL;
  if (dict->nodes[idx] == NULL) return NULL;
  return dict->nodes + idx;
}

struct ufdt_node **ufdt_node_dict_find(struct ufdt_node_dict *dict,
                                       const char *name) {
  return ufdt_node_dict_find_len(dict, name, dto_strlen(name));
}

struct ufdt_node *ufdt_node_dict_find_node_len(struct ufdt_node_dict *dict,
                                               const char *name, int len) {
  struct ufdt_node **res = ufdt_node_dict_find_len(dict, name, len);
  if (res == NULL) return NULL;
  return *res;
}

struct ufdt_node *ufdt_node_dict_find_node(struct ufdt_node_dict *dict,
                                           const char *name) {
  return ufdt_node_dict_find_node_len(dict, name, dto_strlen(name));
}

/*
 * END of ufdt_dict searching related methods.
 */

void ufdt_node_dict_print(struct ufdt_node_dict *dict) {
  struct ufdt_node **it;
  for_each(it, dict) dto_print("%ld -> %s\n", it - dict->nodes, name_of(*it));
}
