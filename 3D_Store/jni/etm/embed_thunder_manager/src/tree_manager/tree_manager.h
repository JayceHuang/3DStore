#if !defined(__TREE_MANAGER_H_20090915)
#define __TREE_MANAGER_H_20090915

#include "tree_manager/tree_interface.h"
#include "em_common/em_list.h"
#include "em_common/em_map.h"

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 trm_init_slabs(void);
_int32 trm_uninit_slabs(void);
_int32 trm_init(void);
_int32 trm_uninit(void);
_int32 trm_id_comp(void *E1, void *E2);
_int32 trm_hash_comp(void *E1, void *E2);
_int32 trm_close_all_trees(void);
_int32 trm_add_root_to_file(TREE *p_tree);
_int32 trm_build_tree(TREE *p_tree);
_int32 trm_load_nodes_from_file(TREE *p_tree);
_int32 trm_save_nodes(TREE *p_tree);
_int32 trm_set_node_change(TREE *p_tree,TREE_NODE * p_node,_u32 change_flag);
_int32 trm_save_total_nodes_num(TREE *p_tree);
_u32 trm_create_node_id(TREE * p_tree);
_int32 trm_decrease_node_id(TREE * p_tree);
TREE_NODE * trm_get_node_from_map(TREE * p_tree,_u32 node_id);
_int32 trm_add_node_to_map(TREE * p_tree,TREE_NODE * p_node);
_int32 trm_remove_node_from_map(TREE * p_tree,TREE_NODE * p_node);
_int32 trm_clear_node_map(TREE * p_tree);
BOOL trm_is_node_exist(TREE * p_tree,_u32 node_id);
_u32 trm_create_tree_id(void);
_int32 trm_decrease_tree_id(void);
TREE * trm_get_tree_from_map(_u32 tree_id);
_int32 trm_add_tree_to_map(TREE * p_tree);
_int32 trm_remove_tree_from_map(TREE * p_tree);
_int32 trm_clear_tree_map(void);
BOOL trm_is_tree_opened(TREE * p_tree,TREE ** pp_tree_old);
_int32 trm_tree_scheduler(void);
_u64 trm_generate_data_hash(void* data,_u32 data_len);
#if 0
TREE_NODE * trm_get_first_node_from_data_hash_map(TREE * p_tree,TREE_NODE * p_parent,_u64 hash);
LIST * trm_get_node_list_from_data_hash_map(TREE * p_tree,_u64 hash,MAP_ITERATOR * p_result_iterator);
_int32 trm_add_data_hash_to_map(TREE * p_tree,TREE_NODE * p_node);
_int32 trm_remove_data_hash_from_map(TREE * p_tree,TREE_NODE * p_node);
_int32 trm_clear_data_hash_map(TREE * p_tree);
BOOL trm_is_data_hash_exist(TREE * p_tree,_u64 hash);
#endif
_u64 trm_generate_name_hash(const char* name,_u32 name_len);
#if 0
TREE_NODE * trm_get_node_from_name_hash_map(TREE * p_tree,_u64 hash);
LIST * trm_get_node_list_from_name_hash_map(TREE * p_tree,_u64 hash,MAP_ITERATOR * p_result_iterator);
_int32 trm_add_name_hash_to_map(TREE * p_tree,TREE_NODE * p_node);
_int32 trm_remove_name_hash_from_map(TREE * p_tree,TREE_NODE * p_node);
_int32 trm_clear_name_hash_map(TREE * p_tree);
BOOL trm_is_name_hash_exist(TREE * p_tree,_u64 hash);
#endif
_int32 trm_node_malloc(TREE_NODE **pp_slip);
_int32 trm_node_free(TREE_NODE * p_slip);
_int32 trm_node_init(TREE * p_tree,TREE_NODE *p_node,const char * name, _u32 name_len,void* data,_u32 data_len);
_int32 trm_node_uninit(TREE_NODE *p_node);
_int32 trm_tree_malloc(TREE **pp_slip);
_int32 trm_tree_free(TREE * p_slip);
_int32 trm_tree_init(TREE *p_tree);
_int32 trm_tree_uninit(TREE *p_tree);
_int32 trm_node_set_parent(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_parent);
TREE_NODE * trm_node_get_parent(TREE_NODE *p_node);
_int32 trm_node_set_pre(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_pre);
TREE_NODE * trm_node_get_pre(TREE_NODE *p_node);
_int32 trm_node_set_nxt(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_nxt);
TREE_NODE * trm_node_get_nxt(TREE_NODE *p_node);
_int32 trm_node_set_first_child(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_child);
TREE_NODE * trm_node_get_first_child(TREE_NODE *p_node);
_int32 trm_node_set_last_child(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_child);
TREE_NODE * trm_node_get_last_child(TREE_NODE *p_node);
BOOL trm_node_check_tree(TREE_NODE *p_node,_u32 tree_id);
BOOL trm_node_check_child(TREE_NODE *p_node,_u32 child_id);
_int32 trm_node_add_child(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_child);
_u32 trm_node_find_by_data(TREE_NODE *p_node,void * data, _u32 data_len);
_int32 trm_node_remove_all_child(TREE *p_tree,TREE_NODE *p_node);
_int32 trm_node_remove_child(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_child);

TREE_NODE * trm_find_first_child_by_name_hash(TREE *p_tree,TREE_NODE *p_parent,_u64 hash);
TREE_NODE * trm_find_last_child_by_name_hash(TREE *p_tree,TREE_NODE *p_parent,_u64 hash);
TREE_NODE * trm_find_next_node_by_name_hash(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,_u64 hash);
TREE_NODE * trm_find_prev_node_by_name_hash(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,_u64 hash);

TREE_NODE * trm_find_first_child_by_data_hash(TREE *p_tree,TREE_NODE *p_parent,_u64 hash);
TREE_NODE * trm_find_last_child_by_data_hash(TREE *p_tree,TREE_NODE *p_parent,_u64 hash);
TREE_NODE * trm_find_next_node_by_data_hash(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,_u64 hash);
TREE_NODE * trm_find_prev_node_by_data_hash(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,_u64 hash);

char * trm_correct_input_name_string(const char * name,_u32 name_len);
TREE_NODE * trm_find_first_child_by_name_impl(TREE *p_tree,TREE_NODE *p_parent,const char * name,char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN]);
TREE_NODE * trm_find_last_child_by_name_impl(TREE *p_tree,TREE_NODE *p_parent,const char * name,char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN]);
TREE_NODE * trm_find_next_node_by_name_impl(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,const char * name,char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN]);
TREE_NODE * trm_find_prev_node_by_name_impl(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,const char * name,char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN]);

TREE_NODE * trm_find_first_child_by_name(TREE *p_tree,TREE_NODE *p_parent,const char * name,_u32 name_len);
TREE_NODE * trm_find_next_node_by_name(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,const char * name,_u32 name_len);
TREE_NODE * trm_find_prev_node_by_name(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,const char * name,_u32 name_len);

_int32 trm_node_name_malloc(char **pp_slip);
_int32 trm_node_name_free(char * p_slip);
_int32 trm_get_node_name(TREE *p_tree,TREE_NODE *p_node,char * name_buffer);


#endif /* __TREE_MANAGER_H_20090915 */
