#if !defined(__TREE_IMPL_H_20101105)
#define __TREE_IMPL_H_20101105

#include "tree_manager/tree_interface.h"
#include "em_common/em_list.h"
#include "em_common/em_map.h"

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/

_int32 trm_clear_up_tree_file(TREE *p_tree);
_int32 trm_clear_up_tree_file_and_close(TREE *p_tree);
void trm_tree_scheduler_impl(TREE *p_tree);
_int32 trm_load_tree_impl( TREE *p_tree );
_int32 trm_correct_tree_path( const char * file_path,char  corrected_path[MAX_FULL_PATH_BUFFER_LEN]);
_int32 trm_open_tree_impl( const char * file_path,_int32 flag ,_u32 * p_tree_id);
_int32 trm_close_tree_impl( TREE *p_tree);
_int32 trm_close_tree_completely(TREE *p_tree);
_int32 trm_close_tree_by_id( _u32 tree_id);
_int32 trm_destroy_tree_impl(const char * file_path);
BOOL trm_tree_exist_impl(const char * file_path);
_int32 trm_create_node_impl(_u32 tree_id,_u32 parent_id,const char * name, _u32 name_len,void * data, _u32 data_len,_u32 * p_node_id);
_int32 trm_delete_all_children(_u32 tree_id,_u32 node_id);
_int32 trm_delete_node_impl(_u32 tree_id,_u32 node_id);
_int32 trm_set_name_impl(_u32 tree_id,_u32 node_id,const char * name, _u32 name_len);
_int32 trm_get_name_impl(_u32 tree_id,_u32 node_id,char * name_buffer);
_int32 trm_set_data_impl(_u32 tree_id,_u32 node_id,void * new_data, _u32 new_data_len);
_int32 trm_get_data_impl(_u32 tree_id,_u32 node_id,void * data_buffer,_u32 * buffer_len);
_int32 trm_set_parent_impl(_u32 tree_id,_u32 node_id,_u32 parent_id);
_int32 trm_move_up_impl(_u32 tree_id,_u32 node_id,_u32 steps);
_int32 trm_move_down_impl(_u32 tree_id,_u32 node_id,_u32 steps);
_int32 trm_get_parent_impl(_u32 tree_id,_u32 node_id,_u32 * p_parent_id);
_int32 trm_get_children_impl(_u32 tree_id,_u32 node_id,_u32 * id_buffer,_u32 * buffer_len);
_int32 trm_get_first_child_impl(_u32 tree_id,_u32 parent_id,_u32 * p_node_id);
_int32 trm_get_next_brother_impl(_u32 tree_id,_u32 node_id,_u32 * p_bro_id);
_int32 trm_get_prev_brother_impl(_u32 tree_id,_u32 node_id,_u32 * p_bro_id);
_int32 trm_get_last_child_impl(_u32 tree_id,_u32 parent_id,_u32 * p_node_id);
_int32 trm_find_first_node_impl(_u32 tree_id,_u32 parent_id,const char * name, _u32 name_len,void * data, _u32 data_len,_u32 * p_node_id);
_int32 trm_find_next_node_impl(_u32 tree_id,_u32 parent_id,_u32 node_id,const char * name, _u32 name_len,void * data, _u32 data_len,_u32 * p_nxt_id);
_int32 trm_find_prev_node_impl(_u32 tree_id,_u32 parent_id,_u32 node_id,const char * name, _u32 name_len,void * data, _u32 data_len,_u32 * p_prev_id);

#endif /* __TREE_IMPL_H_20101105 */
