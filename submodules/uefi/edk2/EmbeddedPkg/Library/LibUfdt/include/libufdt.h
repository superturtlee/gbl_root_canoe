
#ifndef LIBUFDT_H
#define LIBUFDT_H

#include "libufdt_sysdeps.h"
#include "ufdt_types.h"
#include <libfdt_env.h>

#define false 0
#define true 1

/*
 * BEGIN of ufdt_node_dict methods
 * Since in the current implementation, it's actually a hash table.
 * So most of operation's time complexity are O(1) with high probability
 * (w.h.p.).
 */

/*
 * Allocates some new spaces and creates a new ufdt_node_dict.
 *
 * @return: a pointer to the newly created ufdt_node_dict or
 *          NULL if dto_malloc failed
 */
struct ufdt_node_dict ufdt_node_dict_construct();

/*
 * Frees all space dto_malloced, not including ufdt_nodes in the table.
 */
void ufdt_node_dict_destruct(struct ufdt_node_dict *dict);

/*
 * Adds a ufdt_node (as pointer) to the ufdt_node_dict.
 * @return: 0 if success
 *          < 0 otherwise
 *
 * @Time: O(length of node->name) w.h.p.
 */
int ufdt_node_dict_add(struct ufdt_node_dict *dict, struct ufdt_node *node);

/*
 * Returns the pointer to the entry in ufdt_node_dict with node->name =
 * name[0..len-1], for direct modification of the entry.
 * e.g., Delete an entry from the ufdt_node_dict.
 *
 * @return: a pointer to the entry in ufdt_node_dict or
 *          NULL if no such entry.
 *
 * @Time: O(len = |name|) w.h.p.
 */
struct ufdt_node **ufdt_node_dict_find_len(struct ufdt_node_dict *dict,
                                           const char *name, int len);
struct ufdt_node **ufdt_node_dict_find(struct ufdt_node_dict *dict,
                                       const char *name);

/*
 * Returns the pointer to the ufdt_node with node->name =
 * name[0..len-1], for direct modification of the node.
 *
 * @return: a pointer to the node or
 *          NULL if no such node in ufdt_node_dict with same name.
 *
 * @Time: O(len = |name|) w.h.p.
 */
struct ufdt_node *ufdt_node_dict_find_node_len(struct ufdt_node_dict *dict,
                                               const char *name, int len);
struct ufdt_node *ufdt_node_dict_find_node(struct ufdt_node_dict *dict,
                                           const char *name);

/*
 * Prints the each (index, node->name) pair in the dict to stdout in the
 * following format:
 * ```
 * [idx0] [name0]
 * [idx1] [name1]
 * ...
 * ```
 */
void ufdt_node_dict_print(struct ufdt_node_dict *dict);

/*
 * END of ufdt_node_dict methods
 */

/*
 * BEGIN of ufdt_node methods
 */

/*
 * Allocates spaces for new ufdt_node who represents a fdt node at fdt_tag_ptr.
 * In order to get name pointer, it's neccassary to give the pointer to the
 * entire fdt it belongs to.
 *
 *
 * @return: a pointer to the newly created ufdt_node or
 *          NULL if dto_malloc failed
 */
struct ufdt_node *ufdt_node_construct(void *fdtp, fdt32_t *fdt_tag_ptr);

/*
 * Frees all nodes in the subtree rooted at *node.
 * Also dto_frees those ufdt_node_dicts in each node.
 */
void ufdt_node_destruct(struct ufdt_node *node);

/*
 * Adds the child as a subnode of the parent.
 * It's been done by add entries in parent->prop_list or node_list depending on
 * the tag type of child.
 *
 * @return: 0 if success
 *          < 0 otherwise
 *
 * @Time: O(1) w.h.p.
 */
int ufdt_node_add_child(struct ufdt_node *parent, struct ufdt_node *child);

/* BEGIN of FDT_PROP related functions .*/

/*
 * Gets pointer to FDT_PROP subnode of node with name equals to name[0..len-1]
 *
 * @return: a pointer to the subnode or
 *          NULL if no such subnode.
 *
 * @Time: O(len = length of name) w.h.p.
 */
struct ufdt_node *ufdt_node_get_property_by_name_len(
    const struct ufdt_node *node, const char *name, int len);
struct ufdt_node *ufdt_node_get_property_by_name(const struct ufdt_node *node,
                                                 const char *name);

/*
 * Gets the pointer to the FDT_PROP node's data in the corresponding fdt.
 * Also writes the length of such data to *out_len if out_len is not NULL.
 *
 * @return: a pointer to some data located in fdt or
 *          NULL if *node is not a FDT_PROP
 */
char *ufdt_node_get_fdt_prop_data(const struct ufdt_node *node, int *out_len);

/*
 * Gets pointer to FDT_PROP node's data in fdt with name equals to
 * name[0..len-1], which is a subnode of *node.
 * It's actually a composition of ufdt_node_get_property_by_name and
 * ufdt_node_get_fdt_prop_data
 *
 * @return: a pointer to some data located in fdt or
 *          NULL if no such subnode.
 *
 * @Time: O(len = length of name) w.h.p.
 */
char *ufdt_node_get_fdt_prop_data_by_name_len(const struct ufdt_node *node,
                                              const char *name, int len,
                                              int *out_len);
char *ufdt_node_get_fdt_prop_data_by_name(const struct ufdt_node *node,
                                          const char *name, int *out_len);

/* END of FDT_PROP related functions .*/

/*
 * Gets pointer to FDT_BEGIN_NODE subnode of node with name equals to
 * name[0..len-1].
 *
 * @return: a pointer to the subnode or
 *          NULL if no such subnode.
 *
 * @Time: O(len = length of name) w.h.p.
 */

struct ufdt_node *ufdt_node_get_subnode_by_name_len(const struct ufdt_node *node,
                                                    const char *name, int len);
struct ufdt_node *ufdt_node_get_subnode_by_name(const struct ufdt_node *node,
                                                const char *name);

/*
 * Gets the pointer to FDT_NODE node whose relative path to *node is
 * path[0..len-1].
 * Note that the relative path doesn't support parent node like:
 * "../path/to/node".
 *
 * @return: a pointer to the node or
 *          NULL if no such node.
 *
 * @Time: O(len = length of path) w.h.p.
 */
struct ufdt_node *ufdt_node_get_node_by_path_len(const struct ufdt_node *node,
                                                 const char *path, int len);
struct ufdt_node *ufdt_node_get_node_by_path(const struct ufdt_node *node,
                                             const char *path);

/*
 * Gets the phandle value of the node if it has.
 *
 * @return: phandle value of that node or
 *          0 if *node is not FDT_NODE or there's no "phandle"/"linux,phandle"
 *          property.
 *
 * @Time: O(1) w.h.p.
 */
uint32_t ufdt_node_get_phandle(const struct ufdt_node *node);

/*
 * END of ufdt_node methods
 */

/*
 * BEGIN of ufdt methods.
 */

/*
 * Constructs a ufdt whose base fdt is fdtp.
 * Note that this function doesn't construct the entire tree.
 * To get the whole tree please call `fdt_to_ufdt(fdtp, fdt_size)`
 *
 * @return: an empty ufdt with base fdtp = fdtp
 */
struct ufdt *ufdt_construct(void *fdtp);

/*
 * Frees the space occupied by the ufdt, including all ufdt_nodes and
 * ufdt_node_dicts along
 * with static_phandle_table.
 */
void ufdt_destruct(struct ufdt *tree);

/*
 * Gets the pointer to the ufdt_node in tree with phandle = phandle.
 * The function do a binary search in tree->phandle_table.
 *
 * @return: a pointer to the target ufdt_node
 *          NULL if no ufdt_node has phandle = phandle
 *
 * @Time: O(log(# of nodes in tree)) = O(log(size of underlying fdt))
 */
struct ufdt_node *ufdt_get_node_by_phandle(struct ufdt *tree, uint32_t phandle);

/*
 * Gets the pointer to the ufdt_node in tree with absoulte path =
 * path[0..len-1].
 * Absolute path has form "/path/to/node" or "some_alias/to/node".
 * In later example, some_alias is a property in "/aliases" with data is a path
 * to some node X. Then the funcion will return node with relative
 * path = "to/node" w.r.t. X.
 *
 * @return: a pointer to the target ufdt_node or
 *          NULL if such dnt doesn't exist.
 *
 * @Time: O(len = length of path) w.h.p.
 */
struct ufdt_node *ufdt_get_node_by_path_len(struct ufdt *tree, const char *path,
                                            int len);
struct ufdt_node *ufdt_get_node_by_path(struct ufdt *tree, const char *path);

/*
 * END of ufdt methods.
 */

/*
 * Compares function between 2 nodes, compare by name of each node.
 *
 * @return: x < 0  if a's name is lexicographically smaller
 *          x == 0 if a, b has same name
 *          x > 0  if a's name is lexicographically bigger
 */
int node_cmp(const void *a, const void *b);

/*
 * Determines whether node->name equals to name[0..len-1]
 *
 * @return: true if they're equal.
 *          false otherwise
 */
bool node_name_eq(const struct ufdt_node *node, const char *name, int len);

/*
 * Merges tree_b into tree_a with tree_b has all nodes except root disappeared.
 * Overwrite property in tree_a if there's one with same name in tree_b.
 * Otherwise add the property to tree_a.
 * For subnodes with the same name, recursively run this function.
 *
 * Ex:
 * tree_a : ta {
 *  b = "b";
 *  c = "c";
 *  d {
 *    e = "g";
 *  };
 * };
 *
 * tree_b : tb {
 *  c = "C";
 *  g = "G";
 *  d {
 *    da = "dad";
 *  };
 *  h {
 *    hh = "HH";
 *  };
 * };
 *
 * The resulting trees will be:
 *
 * tree_a : ta {
 *  b = "b";
 *  c = "C";
 *  g = "G";
 *  d {
 *    da = "dad";
 *    e = "g";
 *  };
 *  h {
 *    hh = "HH";
 *  };
 * };
 *
 * tree_b : tb {
 * };
 *
 *
 * @return: 0 if merge success
 *          < 0 otherwise
 *
 * @Time: O(# of nodes in tree_b + total length of all names in tree_b) w.h.p.
 */
int merge_ufdt_into(struct ufdt_node *tree_a, struct ufdt_node *tree_b);

/*
 * BEGIN of ufdt output functions
 */

/*
 * Builds the ufdt for FDT pointed by fdtp.
 * This including build all ufdt_nodes and ufdt_node_dicts, and builds the
 * phandle table as
 * well.
 *
 * @return: the ufdt T representing fdtp or
 *          T with T.fdtp == NULL if fdtp is unvalid.
 *
 * @Time: O(fdt_size + nlogn) where n = # of nodes in fdt.
 */
struct ufdt *fdt_to_ufdt(void *fdtp, size_t fdt_size);

/*
 * Sequentially dumps the tree rooted at *node to FDT buffer fdtp.
 * Basically it calls functions provided by libfdt/fdt_sw.c.
 *
 * All those functions works fast.
 * But when it comes to dump property node to fdt, the function they
 * provide(fdt_property()) is really slow. Since it runs through all strings
 * stored in fdt to find the right `nameoff` for the property node.
 *
 * So we implement our own fdt_property() called `output_property_to_fdt()`, the
 * basic
 * idea is that we keep a hash table that we can search for the nameoff of the
 * string in constant time instead of O(total length of strings) search.
 *
 * @return: 0 if successfully dump or
 *          < 0 otherwise
 *
 * @Time: O(total length of all names + # of nodes in subtree rooted at *root)
 */
int output_ufdt_node_to_fdt(struct ufdt_node *node, void *fdtp,
                            struct ufdt_node_dict *all_props);

/*
 * Sequentially dumps the whole ufdt to FDT buffer fdtp with buffer size
 * buf_size.
 * Basically it calls functions provided by libfdt/fdt_sw.c.
 * The main overhead here is to dump the tree to fdtp by calling
 * output_ufdt_node_to_fdt().
 *
 *
 * @return: 0 if successfully dump or
 *          < 0 otherwise
 *
 * @Time: O(total length of all names + # of nodes in tree)
 */
int ufdt_to_fdt(struct ufdt *tree, void *buf, int buf_size);

/*
 * prints the entire subtree rooted at *node in form:
 * NODE :[node name]:
 *   PROP :[prop name]:
 *   ...
 *   NODE :[subnode1 name]:
 *     ...
 *   NODE :[subnode1 name]:
 *     ...
 *   ...
 * There're (depth * TAB_SIZE) spaces in front of each line.
 */
void ufdt_node_print(const struct ufdt_node *node, int depth);

/*
 * It's just ufdt_node_print(tree->root, 0).
 */
void ufdt_print(struct ufdt *tree);

/*
 * END of ufdt output functions
 */

/*
 * Runs closure.func(node, closure.env) for all nodes in subtree rooted at
 * *node.
 * The order of each node being applied by the function is depth first.
 * Basically it's the same order as the order printed in ufdt_node_print().
 *
 * Example:
 *
 * void print_name(struct ufdt_node *node, void *env) {
 *   printf("%s\n", node->name);
 * }
 *
 * struct ufdt_node_closure clos;
 * clos.func = print_name;
 * clos.env = NULL;
 * ufdt_map(tree, clos);
 *
 * Then you can print all names of nodes in tree.
 *
 * @Time: O((# of nodes in subtree rooted at *node) * avg. running time of the
 * function closure.func)
 */
void ufdt_node_map(struct ufdt_node *node, struct ufdt_node_closure closure);

/*
 * It's just ufdt_node_map(tree->root, closure);
 */
void ufdt_map(struct ufdt *tree, struct ufdt_node_closure closure);

struct static_phandle_table build_phandle_table(struct ufdt *tree);
#endif /* LIBUFDT_H */
