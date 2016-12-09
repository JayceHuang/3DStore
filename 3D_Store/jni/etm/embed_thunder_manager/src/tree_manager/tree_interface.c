
#include "tree_manager/tree_interface.h"
#include "tree_manager/tree_store.h"
#include "tree_manager/tree_manager.h"
#include "tree_manager/tree_impl.h"

#include "em_asyn_frame/em_msg.h"

#include "em_common/em_time.h"
#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_TREE_MANAGER

#include "em_common/em_logger.h"

_int32 init_tree_manager_module(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "init_tree_manager_module" );

	ret_val = trm_init_slabs();
	CHECK_VALUE(ret_val);
	
	ret_val = trm_init();
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	return SUCCESS;
	
ErrorHanle:
	trm_uninit_slabs();
	CHECK_VALUE(ret_val);
	return ret_val;
}
_int32 uninit_tree_manager_module(void)
{
	EM_LOG_DEBUG( "uninit_tree_manager_module" );

	trm_uninit();
	
	trm_uninit_slabs();
	
	return SUCCESS;
}

void trm_scheduler(void)
{
	EM_LOG_DEBUG( "trm_scheduler" );

	trm_tree_scheduler();
	
	return;
	
}

_int32 trm_clear(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "trm_clear" );

	ret_val = trm_close_all_trees();

	return ret_val;
	
}

_int32 trm_load_default_tree( void )
{
	return SUCCESS; //trm_load_nodes_from_file();
}

_int32 trm_open_tree( void * p_param )
{
	//_int32 ret_val = SUCCESS;
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	const char * file_path =(const char *) _p_param->_para1;
	_int32 flag =(_int32) _p_param->_para2;
	_u32* p_tree_id =(_u32*) _p_param->_para3;

	_p_param->_result = trm_open_tree_impl( file_path,flag ,p_tree_id);
	
	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_close_tree( void * p_param )
{
	//_int32 ret_val = SUCCESS;
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 tree_id =(_u32) _p_param->_para1;

	_p_param->_result = trm_close_tree_by_id( tree_id);
	
	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 trm_destroy_tree( void * p_param )
{
	//_int32 ret_val = SUCCESS;
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	const char * file_path =(const char * ) _p_param->_para1;

	_p_param->_result = trm_destroy_tree_impl( file_path);
	
	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 trm_tree_exist( void * p_param )
{
	//_int32 ret_val = SUCCESS;
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	const char * file_path =(const char * ) _p_param->_para1;
	BOOL * ret_b = (BOOL * ) _p_param->_para2;
	
	*ret_b = trm_tree_exist_impl( file_path);
	
	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 trm_create_node( void * p_param )
{
	POST_PARA_7* _p_param = (POST_PARA_7*)p_param;
	_u32 parent_id =(_u32) _p_param->_para1;
	void* data =(void*) _p_param->_para2;
	_u32 data_len =(_u32) _p_param->_para3;
	_u32* p_node_id =(_u32*) _p_param->_para4;
	_u32 tree_id =(_u32) _p_param->_para5;
	const char* name =(void*) _p_param->_para6;
	_u32 name_len =(_u32) _p_param->_para7;

	_p_param->_result = trm_create_node_impl(tree_id,parent_id,name,name_len,data,data_len,p_node_id);
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_delete_node( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	_u32 tree_id =(_u32) _p_param->_para2;

		_p_param->_result = trm_delete_node_impl(tree_id,node_id);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_set_parent( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	_u32 parent_id =(_u32) _p_param->_para2;
	_u32 tree_id =(_u32) _p_param->_para3;
		_p_param->_result = trm_set_parent_impl(tree_id,node_id,parent_id);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_move_up( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	_u32 steps =(_u32) _p_param->_para2;
	_u32 tree_id =(_u32) _p_param->_para3;
	
		_p_param->_result = trm_move_up_impl(tree_id,node_id,steps);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_move_down( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	_u32 steps =(_u32) _p_param->_para2;
	_u32 tree_id =(_u32) _p_param->_para3;
		_p_param->_result = trm_move_down_impl(tree_id,node_id,steps);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_get_parent( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	_u32 * p_parent_id =(_u32*) _p_param->_para2;
	_u32 tree_id =(_u32) _p_param->_para3;
		_p_param->_result = trm_get_parent_impl(tree_id,node_id,p_parent_id);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_get_children( void * p_param )
{
	//_int32 ret_val = SUCCESS;
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	_u32 * id_buffer =(_u32*) _p_param->_para2;
	_u32 * buffer_len =(_u32*) _p_param->_para3;
	_u32 tree_id =(_u32) _p_param->_para4;
		_p_param->_result = trm_get_children_impl(tree_id,node_id,id_buffer,buffer_len);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 trm_get_data( void * p_param )
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	void * data_buffer =(void *) _p_param->_para2;
	_u32 * buffer_len =(_u32*) _p_param->_para3;
	_u32 tree_id =(_u32) _p_param->_para4;
		_p_param->_result = trm_get_data_impl(tree_id,node_id,data_buffer,buffer_len);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_set_data( void * p_param )
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	void * new_data =(void *) _p_param->_para2;
	_u32  data_len =(_u32) _p_param->_para3;
	_u32 tree_id =(_u32) _p_param->_para4;
		_p_param->_result = trm_set_data_impl(tree_id,node_id,new_data,data_len);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_get_name( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	char * name_buffer =(void *) _p_param->_para2;
	_u32 tree_id =(_u32) _p_param->_para3;
		_p_param->_result = trm_get_name_impl(tree_id,node_id,name_buffer);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_set_name( void * p_param )
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u32 node_id =(_u32) _p_param->_para1;
	const char * name =(void *) _p_param->_para2;
	_u32  name_len =(_u32) _p_param->_para3;
	_u32 tree_id =(_u32) _p_param->_para4;

		_p_param->_result = trm_set_name_impl(tree_id,node_id,name,name_len);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_get_first_child( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 tree_id =(_u32) _p_param->_para1;
	_u32 parent_id =(_u32) _p_param->_para2;
	_u32 * p_node_id =(_u32*) _p_param->_para3;
		_p_param->_result = trm_get_first_child_impl(tree_id,parent_id,p_node_id);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_get_next_brother( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 tree_id =(_u32) _p_param->_para1;
	_u32 node_id =(_u32) _p_param->_para2;
	_u32 * p_bro_id =(_u32*) _p_param->_para3;
		_p_param->_result = trm_get_next_brother_impl(tree_id,node_id,p_bro_id);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_get_prev_brother( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 tree_id =(_u32) _p_param->_para1;
	_u32 node_id =(_u32) _p_param->_para2;
	_u32 * p_bro_id =(_u32*) _p_param->_para3;
		_p_param->_result = trm_get_prev_brother_impl( tree_id,  node_id, p_bro_id);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_find_first_node( void * p_param )
{
	POST_PARA_7* _p_param = (POST_PARA_7*)p_param;
	_u32 tree_id =(_u32) _p_param->_para1;
	_u32 parent_id =(_u32) _p_param->_para2;
	const char * name =(void *) _p_param->_para3;
	_u32  name_len =(_u32) _p_param->_para4;
	void * data =(void *) _p_param->_para5;
	_u32  data_len =(_u32) _p_param->_para6;
	_u32 * p_node_id =(_u32*) _p_param->_para7;
		_p_param->_result = trm_find_first_node_impl(tree_id,parent_id,name,name_len,data,data_len,p_node_id);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_find_next_node( void * p_param )
{
	POST_PARA_8* _p_param = (POST_PARA_8*)p_param;
	_u32 tree_id =(_u32) _p_param->_para1;
	_u32 parent_id =(_u32) _p_param->_para2;
	const char * name =(void *) _p_param->_para3;
	_u32  name_len =(_u32) _p_param->_para4;
	void * data =(void *) _p_param->_para5;
	_u32  data_len =(_u32) _p_param->_para6;
	_u32 node_id =(_u32) _p_param->_para7;
	_u32 * p_nxt_id =(_u32*) _p_param->_para8;
		_p_param->_result = trm_find_next_node_impl(tree_id,parent_id,node_id,name,name_len,data,data_len,p_nxt_id);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 trm_find_prev_node( void * p_param )
{
	POST_PARA_8* _p_param = (POST_PARA_8*)p_param;
	_u32 tree_id =(_u32) _p_param->_para1;
	_u32 parent_id =(_u32) _p_param->_para2;
	const char * name =(void *) _p_param->_para3;
	_u32  name_len =(_u32) _p_param->_para4;
	void * data =(void *) _p_param->_para5;
	_u32  data_len =(_u32) _p_param->_para6;
	_u32 node_id =(_u32) _p_param->_para7;
	_u32 * p_prev_id =(_u32*) _p_param->_para8;
		_p_param->_result = trm_find_prev_node_impl(tree_id,parent_id,node_id,name,name_len,data,data_len,p_prev_id);
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}



