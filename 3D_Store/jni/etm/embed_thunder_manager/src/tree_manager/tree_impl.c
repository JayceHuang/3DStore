
#include "tree_manager/tree_impl.h"
#include "tree_manager/tree_store.h"
#include "tree_manager/tree_manager.h"
#include "scheduler/scheduler.h"

#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_TREE_MANAGER

#include "em_common/em_logger.h"

_int32 trm_clear_up_tree_file(TREE *p_tree)
{
	_int32 ret_val = SUCCESS;
	
	if(!trm_is_tree_file_need_clear_up(p_tree)) return SUCCESS;

	EM_LOG_DEBUG( "trm_clear_up_tree_file:%s" ,p_tree->_tree_file);

	trm_close_tree_impl(p_tree);

	/* 整理文件 */
	ret_val = trm_clear_file_impl((void*)p_tree);
	sd_assert(ret_val == SUCCESS);
	trm_add_node_to_map(p_tree,&p_tree->_root);

	/* 重新载入所有节点 */
	ret_val = trm_load_tree_impl(p_tree );
	CHECK_VALUE(ret_val);

	ret_val = trm_add_root_to_file(p_tree);
	CHECK_VALUE(ret_val);
	
	EM_LOG_DEBUG( "trm_clear_up_tree_file SUCCESS:%s" ,p_tree->_tree_file);
	return SUCCESS;
}
_int32 trm_clear_up_tree_file_and_close(TREE *p_tree)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG( "trm_clear_up_tree_file_and_close:%s" ,p_tree->_tree_file);

	trm_close_tree_impl(p_tree);

	/* 整理文件 */
	if(p_tree->_need_clear&&!p_tree->_file_locked)
	{
		ret_val = trm_clear_file_impl((void*)p_tree);
		sd_assert(ret_val == SUCCESS);
		EM_LOG_DEBUG( "trm_clear_up_tree_file_and_close ret_val:%d" ,ret_val);
	}
	
	trm_remove_tree_from_map(p_tree);
	trm_tree_free(p_tree);
	EM_LOG_DEBUG( "trm_clear_up_tree_file_and_close SUCCESS:%s" ,p_tree->_tree_file);
	return SUCCESS;
}

void trm_tree_scheduler_impl(TREE *p_tree)
{
//	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "trm_tree_scheduler_impl:%u", p_tree->_root._node_id);

	/* save tasks if changed */
	trm_save_nodes(p_tree );
	
	/* close the task_store file if long time no action */
	trm_close_file(p_tree,FALSE);

	trm_clear_up_tree_file(p_tree);
	
	return;
	
}

_int32 trm_load_tree_impl( TREE *p_tree )
{
	return trm_load_nodes_from_file(p_tree);
}

_int32 trm_correct_tree_path( const char * file_path,char  corrected_path[MAX_FULL_PATH_BUFFER_LEN])
{
	//_int32 ret_val = SUCCESS;
	char *p_pos = NULL,*p_pos2 = NULL;
	_int32 path_len = 0;
	
	p_pos = em_strchr((char *)file_path,'/',0);
	p_pos2 = em_strchr((char *)file_path,'\\',0);
	if((p_pos!=NULL)||(p_pos2!=NULL))
	{
		/* full path */
		em_strncpy(corrected_path,file_path,MAX_FULL_PATH_BUFFER_LEN);
	}
	else
	{
		p_pos = em_get_system_path();
		path_len = em_strlen(p_pos);
		if((p_pos==NULL)||(path_len==0))
		{
			return INVALID_FILE_PATH;
		}

		em_strncpy(corrected_path,p_pos,MAX_FULL_PATH_BUFFER_LEN);
		if((corrected_path[path_len-1]!='/')&&(corrected_path[path_len-1]!='\\'))
		{
			em_strcat(corrected_path,"/",1);
		}
		em_strcat(corrected_path,file_path,em_strlen(file_path));
	}
	return SUCCESS;
}

_int32 trm_open_tree_impl( const char * file_path,_int32 flag ,_u32 * p_tree_id)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree = NULL,*p_tree_tmp = NULL;

	ret_val = trm_tree_malloc(&p_tree);
	CHECK_VALUE(ret_val);

	p_tree->_flag = flag;

	ret_val = trm_correct_tree_path( file_path,p_tree->_tree_file);
	if(ret_val!=SUCCESS)
	{
		trm_tree_free(p_tree);
		CHECK_VALUE(ret_val);
	}

	if(trm_is_tree_opened(p_tree,&p_tree_tmp)==TRUE)
	{
		trm_tree_free(p_tree);
		#if 0
		/* whether if create tree while it not exist. */
		#define ETM_TF_CREATE		(0x1)
		/* read and write (default) */
		#define ETM_TF_RDWR		(0x0)
		/* read only. */
		#define ETM_TF_RDONLY		(0x2)
		/* write only. */
		#define ETM_TF_WRONLY		(0x4)
		#define ETM_TF_MASK       (0xFF)
		if(p_tree_tmp->_flag!=0x2)
		{
			/* 同一时间只能有一个人有写权限 */
			sd_assert(0x2 == flag);
		}
		#endif
		p_tree_tmp->_open_count++;
		*p_tree_id = p_tree_tmp->_root._node_id;
		return SUCCESS;
	}

	trm_tree_init(p_tree);
		
	ret_val = trm_create_tree_file(p_tree);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	ret_val = trm_get_total_node_num(p_tree,&p_tree->_total_node_num);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	if(p_tree->_total_node_num==0)
	{
		p_tree->_total_node_num++;
		trm_save_total_nodes_num(p_tree);
	}
	
	p_tree->_root._is_root = TRUE;
	p_tree->_root._full_info= TRUE;
	p_tree->_root._tree= (void*)p_tree;
	p_tree->_root._node_id = trm_create_tree_id();

	ret_val = trm_add_node_to_map(p_tree,&p_tree->_root);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	ret_val = trm_load_tree_impl(p_tree );
	if(ret_val!=SUCCESS) goto ErrorHanle;

	ret_val = trm_add_root_to_file(p_tree);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	ret_val = trm_add_tree_to_map(p_tree);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	*p_tree_id = p_tree->_root._node_id;
	return SUCCESS;
	
ErrorHanle:
	
	trm_close_tree_completely(p_tree);

	return ret_val;
}
_int32 trm_close_tree_impl( TREE *p_tree)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG( "trm_close_tree_impl" );

	//trm_save_total_nodes_num(p_tree);
	
	/* save tasks if changed */
	trm_save_nodes(p_tree);
	
	/* close the task_store file if long time no action */
	trm_close_file(p_tree,TRUE);
	//es_close_file(TRUE);

	//trm_clear_data_hash_map(p_tree);
	//trm_clear_name_hash_map(p_tree);
	trm_clear_node_map(p_tree);
	

	return ret_val;
}
_int32 trm_close_tree_completely(TREE *p_tree)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "trm_close_tree_completely" );

	trm_close_tree_impl(p_tree);
	trm_remove_tree_from_map(p_tree);
	trm_tree_free(p_tree);
	return ret_val;
}

_int32 trm_close_tree_by_id( _u32 tree_id)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree = NULL;
	EM_LOG_DEBUG( "trm_close_tree_by_id" );

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		return INVALID_TREE_ID;
	}
	
	if(p_tree->_open_count>0)
	{
		/* 多人同时打开该树 */
		p_tree->_open_count--;
		return SUCCESS;
	}
	
	ret_val = trm_clear_up_tree_file_and_close(p_tree);
	return ret_val;
}
_int32 trm_destroy_tree_impl(const char * file_path)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree = NULL,*p_tree_tmp = NULL;
	EM_LOG_DEBUG( "trm_destroy_tree_impl:%s" ,file_path);

	ret_val = trm_tree_malloc(&p_tree);
	CHECK_VALUE(ret_val);

	p_tree->_flag = O_FS_CREATE|O_FS_RDWR;//ETM_TF_CREATE|ETM_TF_RDWR;

	ret_val = trm_correct_tree_path( file_path,p_tree->_tree_file);
	if(ret_val!=SUCCESS)
	{
		trm_tree_free(p_tree);
		CHECK_VALUE(ret_val);
	}

	if(trm_is_tree_opened(p_tree,&p_tree_tmp)==TRUE)
	{
		/* 不能发生多人打开一棵树，却有人想要销毁该树的情况! */
		sd_assert(p_tree_tmp->_open_count==0);
		trm_close_tree_completely(p_tree_tmp); 
	}
	
	ret_val = em_delete_file(p_tree->_tree_file);

	/* 把备份文件一并删除 */
	sd_strcat(p_tree->_tree_file, ".bak", 4);
	em_delete_file(p_tree->_tree_file);
	
	trm_tree_free(p_tree);
	
	return ret_val;
}
BOOL trm_tree_exist_impl(const char * file_path)
{
	_int32 ret_val = SUCCESS;
	char  tree_file[MAX_FULL_PATH_BUFFER_LEN];				//存储所有树节点信息的文件全名:tree_store.dat
	EM_LOG_DEBUG( "trm_tree_exist_impl:%s" ,file_path);

	ret_val = trm_correct_tree_path( file_path,tree_file);
	if(ret_val!=SUCCESS)
	{
		return FALSE;
	}

	return em_file_exist(tree_file);
}

_int32 trm_create_node_impl(_u32 tree_id,_u32 parent_id,const char * name, _u32 name_len,void * data, _u32 data_len,_u32 * p_node_id)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL,*p_parent_node=NULL;

	EM_LOG_DEBUG( "trm_create_node_impl:tree_id=%u,parent_id=%u" ,tree_id,parent_id);

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_parent_node = trm_get_node_from_map(p_tree,parent_id);
	if(p_parent_node==NULL) 
	{
		CHECK_VALUE(INVALID_PARENT_ID);
	}
	
	ret_val = trm_node_malloc(&p_node);
	CHECK_VALUE(ret_val);

	ret_val = trm_node_init(p_tree,p_node,name,name_len,data, data_len);
	if(ret_val!=SUCCESS) 
	{
		goto ErrorHanle_init;
	}

	ret_val = trm_node_add_child(p_tree,p_parent_node,p_node);
	if(ret_val!=SUCCESS) 
	{
		goto ErrorHanle_child;
	}

	ret_val = trm_add_node_to_map(p_tree,p_node);
	if(ret_val!=SUCCESS) 
	{
		goto ErrorHanle_map;
	}
	
	//ret_val = trm_add_data_hash_to_map(p_tree,p_node);
	//if(ret_val!=SUCCESS) 
	//{
	//	goto ErrorHanle_hash_map;
	//}
	
	ret_val = trm_add_node_to_file(p_tree,p_node);
	if(ret_val!=SUCCESS) 
	{
		goto ErrorHanle_file;
	}
	
	*p_node_id =p_node->_node_id;
	
	//trm_tree_scheduler_impl(p_tree);
	
	return SUCCESS;
		
ErrorHanle_file:
//	trm_remove_data_hash_from_map(p_tree,p_node);
//ErrorHanle_hash_map:
	trm_remove_node_from_map(p_tree,p_node);
ErrorHanle_map:
	trm_node_remove_child(p_tree,p_parent_node,p_node);
ErrorHanle_child:
	trm_decrease_node_id(p_tree);
	trm_node_uninit(p_node);
ErrorHanle_init:
	trm_node_free(p_node);
	EM_LOG_ERROR( "trm_create_node_impl Error=%d!:tree_id=%u,parent_id=%u" ,ret_val,tree_id,parent_id);
	return ret_val;
}
_int32 trm_delete_all_children(_u32 tree_id,_u32 node_id)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}
	
	if(p_node->_children_num!=0)
	{
		ret_val = trm_node_remove_all_child(p_tree,p_node);
		CHECK_VALUE(ret_val);
	}
	
	trm_tree_scheduler_impl(p_tree);
	
	return SUCCESS;
}

_int32 trm_delete_node_impl(_u32 tree_id,_u32 node_id)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}
	
	if(p_node->_children_num!=0)
	{
		ret_val = trm_node_remove_all_child(p_tree,p_node);
		CHECK_VALUE(ret_val);
	}
	
	trm_disable_node_in_file(p_tree,p_node);
	
	//trm_remove_data_hash_from_map(p_tree,p_node);
	
	//trm_remove_name_hash_from_map(p_tree,p_node);
	
	trm_remove_node_from_map(p_tree,p_node);

	trm_node_remove_child(p_tree,trm_node_get_parent(p_node),p_node);
	
	trm_node_uninit(p_node);

	trm_node_free(p_node);
	
	trm_tree_scheduler_impl(p_tree);
	
	return SUCCESS;
}
_int32 trm_set_name_impl(_u32 tree_id,_u32 node_id,const char * name, _u32 name_len)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}
	

	if((name==NULL) ||(name_len==0))
	{
		if(p_node->_name_len != 0)
		{
			trm_node_name_free(p_node->_name);
			p_node->_name = NULL;
			p_node->_name_len = 0;
			//trm_remove_name_hash_from_map(p_tree,p_node);
			trm_set_node_change(p_tree,p_node,TRN_CHANGE_NAME);
			trm_set_node_change(p_tree,p_node,TRN_CHANGE_TOTAL_LEN);
			trm_tree_scheduler_impl(p_tree);
		}
		return SUCCESS;
	}
	
	//if(p_node->_name_len!=0)
	//{
	//	trm_remove_name_hash_from_map(p_tree,p_node);
	//}
	if(p_node->_name==NULL)
	{
		ret_val= trm_node_name_malloc(&p_node->_name);
		CHECK_VALUE(ret_val);
	}
	else
	{
		em_memset(p_node->_name,0,MAX_FILE_NAME_BUFFER_LEN);
	}
	
	if(name_len != p_node->_name_len)
	{
		p_node->_name_len = name_len;
		trm_set_node_change(p_tree,p_node,TRN_CHANGE_TOTAL_LEN);
	}
	
	em_memcpy(p_node->_name,name,name_len);
	p_node->_name_hash = trm_generate_name_hash(name,name_len);
	//trm_add_name_hash_to_map(p_tree,p_node);
	trm_set_node_change(p_tree,p_node,TRN_CHANGE_NAME);

	trm_tree_scheduler_impl(p_tree);
	
	return SUCCESS;
}

_int32 trm_get_name_impl(_u32 tree_id,_u32 node_id,char * name_buffer)
{
	//_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}

	 return trm_get_node_name(p_tree,p_node,name_buffer);
}

_int32 trm_set_data_impl(_u32 tree_id,_u32 node_id,void * new_data, _u32 new_data_len)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}
	

	if((new_data==NULL) ||(new_data_len==0))
	{
		if(p_node->_data_len != 0)
		{
			EM_SAFE_DELETE(p_node->_data);
			p_node->_data_len = 0;
			//trm_remove_data_hash_from_map(p_tree,p_node);
			trm_set_node_change(p_tree,p_node,TRN_CHANGE_DATA);
			trm_set_node_change(p_tree,p_node,TRN_CHANGE_TOTAL_LEN);
			
			trm_tree_scheduler_impl(p_tree);
		}
		return SUCCESS;
	}
	
	//if(p_node->_data_len!=0)
	//{
	//	trm_remove_data_hash_from_map(p_tree,p_node);
	//}
	
	if((new_data_len != p_node->_data_len)||(p_node->_data==NULL))
	{
		EM_SAFE_DELETE(p_node->_data);
		p_node->_data_len = 0;
		ret_val = em_malloc(new_data_len, (void**)&p_node->_data);
		CHECK_VALUE(ret_val);
		p_node->_data_len = new_data_len;
		trm_set_node_change(p_tree,p_node,TRN_CHANGE_TOTAL_LEN);
	}

	em_memset(p_node->_data,0,p_node->_data_len);
	em_memcpy(p_node->_data,new_data,p_node->_data_len);
	p_node->_data_hash = trm_generate_data_hash(p_node->_data,p_node->_data_len); 
	//trm_add_data_hash_to_map(p_tree,p_node);
	trm_set_node_change(p_tree,p_node,TRN_CHANGE_DATA);
	
	trm_tree_scheduler_impl(p_tree);
	return SUCCESS;
}
_int32 trm_get_data_impl(_u32 tree_id,_u32 node_id,void * data_buffer,_u32 * buffer_len)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}

	if(p_node->_data_len == 0 )
	{
		*buffer_len = 0;
		return ret_val;
	}

	if((*buffer_len < p_node->_data_len)||(data_buffer==NULL))
	{
		ret_val = NOT_ENOUGH_BUFFER;
		*buffer_len = p_node->_data_len;
		return ret_val;
	}

	if(p_node->_data_len!=0)
	{
		if(p_node->_data==NULL)
		{
			ret_val = trm_get_data_from_file(p_tree,p_node,data_buffer,buffer_len);
		}
		else
		{
			em_memcpy(data_buffer,p_node->_data,p_node->_data_len);
		}
	}
	else
	{
		ret_val = INVALID_NODE_DATA;
	}
	*buffer_len = p_node->_data_len;

	return ret_val;
}


_int32 trm_set_parent_impl(_u32 tree_id,_u32 node_id,_u32 parent_id)
{
	//_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL,*p_parent_node=NULL;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}
	
	if(p_node->_parent->_node_id == parent_id)
	{
		return SUCCESS;
	}

	if(trm_node_check_child(p_node,parent_id)==TRUE)
	{
		CHECK_VALUE(INVALID_PARENT_ID);
	}
	
	p_parent_node = trm_get_node_from_map(p_tree,parent_id);
	if(p_parent_node==NULL) 
	{
		CHECK_VALUE(INVALID_PARENT_ID);
	}
	
	trm_node_remove_child(p_tree,p_node->_parent,p_node);
	
	trm_node_add_child(p_tree,p_parent_node,p_node);

	trm_tree_scheduler_impl(p_tree);
	
	return SUCCESS;
}

_int32 trm_move_up_impl(_u32 tree_id,_u32 node_id,_u32 steps)
{
	//_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL,*p_node_tmp=NULL;
	_u32 i;
	
	if(steps==0) return SUCCESS;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}
	
	if(p_node==trm_node_get_first_child(trm_node_get_parent(p_node)))
	{
		return SUCCESS;
	}

	p_node_tmp = p_node;
	for(i=0;(i<steps)&&(p_node_tmp!=NULL);i++)
	{
		p_node_tmp = trm_node_get_pre(p_node_tmp);// p_node_tmp->_pre_node;
	}

	/* pick off  */
	sd_assert(trm_node_get_pre(p_node)!=NULL);
	trm_node_set_nxt(p_tree,trm_node_get_pre(p_node),trm_node_get_nxt(p_node));
	
	if(trm_node_get_nxt(p_node)!=NULL)
	{
		trm_node_set_pre(p_tree,trm_node_get_nxt(p_node),trm_node_get_pre(p_node));
	}
	
	if(p_node==trm_node_get_last_child(trm_node_get_parent(p_node)))
	{
		trm_node_set_last_child(p_tree,trm_node_get_parent(p_node),trm_node_get_pre(p_node));
	}

	/* steps too large,put to top */
	if(p_node_tmp==NULL)
	{
		/* first node */
		p_node_tmp = trm_node_get_first_child(trm_node_get_parent(p_node));
	}

	/* insert */
	trm_node_set_pre(p_tree,p_node,trm_node_get_pre(p_node_tmp));
	if(trm_node_get_pre(p_node_tmp)!=NULL)
	{
		trm_node_set_nxt(p_tree,trm_node_get_pre(p_node_tmp),p_node);
	}

	trm_node_set_nxt(p_tree,p_node,p_node_tmp);
	trm_node_set_pre(p_tree,p_node_tmp,p_node);
	
	if(p_node_tmp==trm_node_get_first_child(trm_node_get_parent(p_node)))
	{
		trm_node_set_first_child(p_tree,trm_node_get_parent(p_node),p_node);
	}
	
	return SUCCESS;	
}

_int32 trm_move_down_impl(_u32 tree_id,_u32 node_id,_u32 steps)
{
	//_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL,*p_node_tmp=NULL;
	_u32 i;

	if(steps==0) return SUCCESS;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}
	
	if(p_node==trm_node_get_last_child(trm_node_get_parent(p_node)))
	{
		return SUCCESS;
	}

	p_node_tmp = p_node;
	for(i=0;(i<steps)&&(p_node_tmp!=NULL);i++)
	{
		p_node_tmp = trm_node_get_nxt(p_node_tmp);//p_node_tmp->_nxt_node;
	}

	/* pick off  */
	if(trm_node_get_pre(p_node)!=NULL)
	{
		trm_node_set_nxt(p_tree,trm_node_get_pre(p_node),trm_node_get_nxt(p_node));
	}
	
	sd_assert(trm_node_get_nxt(p_node)!=NULL);
	trm_node_set_pre(p_tree,trm_node_get_nxt(p_node),trm_node_get_pre(p_node));
	
	if(p_node==trm_node_get_first_child(trm_node_get_parent(p_node)))
	{
		trm_node_set_first_child(p_tree,trm_node_get_parent(p_node),trm_node_get_nxt(p_node));
	}

	/* steps too large,put to bottom */
	if(p_node_tmp==NULL)
	{
		/* last node */
		p_node_tmp = trm_node_get_last_child(trm_node_get_parent(p_node));
	}

	/* insert */
	if(trm_node_get_nxt(p_node_tmp)!=NULL)
	{
		trm_node_set_pre(p_tree,trm_node_get_nxt(p_node_tmp),p_node);
	}
	trm_node_set_nxt(p_tree,p_node,trm_node_get_nxt(p_node_tmp));
	trm_node_set_nxt(p_tree,p_node_tmp,p_node);
	trm_node_set_pre(p_tree,p_node,p_node_tmp);

	if(p_node_tmp==trm_node_get_last_child(trm_node_get_parent(p_node)))
	{
		trm_node_set_last_child(p_tree,trm_node_get_parent(p_node),p_node);
	}

	return SUCCESS;	
}

_int32 trm_get_parent_impl(_u32 tree_id,_u32 node_id,_u32 * p_parent_id)
{
	//_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}

	*p_parent_id = p_node->_parent->_node_id;
	
	return SUCCESS;
}

_int32 trm_get_children_impl(_u32 tree_id,_u32 node_id,_u32 * id_buffer,_u32 * buffer_len)
{
	_int32 ret_val = SUCCESS;
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL,*p_child=NULL;
	_u32 i=0;
	
	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}

	if(p_node->_children_num == 0 )
	{
		*buffer_len = 0;
		return ret_val;
	}

	if((*buffer_len < p_node->_children_num)||(id_buffer==NULL))
	{
		ret_val = NOT_ENOUGH_BUFFER;
		*buffer_len = p_node->_children_num;
		return ret_val;
	}

	for(p_child=p_node->_first_child;p_child!=NULL;p_child=p_child->_nxt_node)
	{
		id_buffer[i++] = p_child->_node_id;
	}
	sd_assert(i==p_node->_children_num);
	*buffer_len = p_node->_children_num;
	
	return SUCCESS;
}



_int32 trm_get_first_child_impl(_u32 tree_id,_u32 parent_id,_u32 * p_node_id)
{
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,parent_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}

	if(p_node->_children_num!=0)
	{
		*p_node_id = p_node->_first_child->_node_id;	
	}
	else
	{
		*p_node_id = TRM_INVALID_NODE_ID;
	}

	return SUCCESS;
}

_int32 trm_get_next_brother_impl(_u32 tree_id,_u32 node_id,_u32 * p_bro_id)
{
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}

	if(p_node->_nxt_node!=NULL)
	{
		*p_bro_id = p_node->_nxt_node->_node_id;	
	}
	else
	{
		*p_bro_id = TRM_INVALID_NODE_ID;
	}

	return SUCCESS;
}

_int32 trm_get_prev_brother_impl(_u32 tree_id,_u32 node_id,_u32 * p_bro_id)
{
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}

	if(p_node->_pre_node!=NULL)
	{
		*p_bro_id = p_node->_pre_node->_node_id;	
	}
	else
	{
		*p_bro_id = TRM_INVALID_NODE_ID;
	}

	return SUCCESS;
}
_int32 trm_get_last_child_impl(_u32 tree_id,_u32 parent_id,_u32 * p_node_id)
{
	TREE *p_tree=NULL;
	TREE_NODE *p_node=NULL;

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_node = trm_get_node_from_map(p_tree,parent_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}

	if(p_node->_children_num!=0)
	{
		*p_node_id = p_node->_last_child->_node_id;	
	}
	else
	{
		*p_node_id = TRM_INVALID_NODE_ID;
	}

	return SUCCESS;
}

_int32 trm_find_first_node_impl(_u32 tree_id,_u32 parent_id,const char * name, _u32 name_len,void * data, _u32 data_len,_u32 * p_node_id)
{
	TREE *p_tree=NULL;
	TREE_NODE *p_parent=NULL,*p_node=NULL;
	_u64 name_hash=0,data_hash=0;
	char * p_star = NULL;

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_parent = trm_get_node_from_map(p_tree,parent_id);
	if(p_parent==NULL) 
	{
		CHECK_VALUE(INVALID_PARENT_ID);
	}

	/* 关键字中是否带有*号 */
	if((name!=NULL)&&(name_len!=0))
	{
		p_star = em_strchr((char*)name,'*',0);
		if((p_star!=NULL)&&(p_star-name>name_len))
		{
			p_star = NULL;
		}
	}

	if(p_star==NULL)
	{
		name_hash =trm_generate_name_hash(name,name_len);
	}
	
	data_hash =trm_generate_data_hash(data,data_len);
	if(name_hash!=0)
	{
		/* 先用name hash进行精确查找 */
		p_node = trm_find_first_child_by_name_hash(p_tree,p_parent,name_hash);
	}
	
	if((data_hash!=0)&&(p_node==NULL))
	{
		/* name hash找不到,再用data hash 进行精确查找 */
		p_node = trm_find_first_child_by_data_hash(p_tree,p_parent,data_hash);
	}

	if(p_node!=NULL)
	{
		*p_node_id = p_node->_node_id;
		return SUCCESS;
	}

	/* name hash和data hash 的精确查找都没有结果,再用名字进行模糊查找 */
	p_node = trm_find_first_child_by_name(p_tree,p_parent,name, name_len);
	if(p_node!=NULL)
	{
		*p_node_id = p_node->_node_id;
		return SUCCESS;
	}

	return NODE_NOT_FOUND;
}

_int32 trm_find_next_node_impl(_u32 tree_id,_u32 parent_id,_u32 node_id,const char * name, _u32 name_len,void * data, _u32 data_len,_u32 * p_nxt_id)
{
	TREE *p_tree=NULL;
	TREE_NODE *p_parent=NULL,*p_node=NULL,*p_node_tmp=NULL;
	_u64 name_hash=0,data_hash=0;
	char * p_star = NULL;

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_parent = trm_get_node_from_map(p_tree,parent_id);
	if(p_parent==NULL) 
	{
		CHECK_VALUE(INVALID_PARENT_ID);
	}

	if(trm_node_check_child(p_parent,node_id)!=TRUE)
	{
		CHECK_VALUE(INVALID_CHILD);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}


	/* 关键字中是否带有*号 */
	if((name!=NULL)&&(name_len!=0))
	{
		p_star = em_strchr((char*)name,'*',0);
		if((p_star!=NULL)&&(p_star-name>name_len))
		{
			p_star = NULL;
		}
	}

	if(p_star==NULL)
	{
		name_hash =trm_generate_name_hash(name,name_len);
	}
	
	data_hash =trm_generate_data_hash(data,data_len);
	if(name_hash!=0)
	{
		p_node_tmp = trm_find_next_node_by_name_hash(p_tree,p_parent,p_node,name_hash);
	}
	
	if((data_hash!=0)&&(p_node_tmp==NULL))
	{
		p_node_tmp = trm_find_next_node_by_data_hash(p_tree,p_parent,p_node,data_hash);
	}

	if(p_node_tmp!=NULL)
	{
		*p_nxt_id = p_node_tmp->_node_id;
		return SUCCESS;
	}

	/* name hash和data hash 的精确查找都没有结果,再用名字进行模糊查找 */
	p_node = trm_find_next_node_by_name(p_tree,p_parent,p_node,name, name_len);
	if(p_node!=NULL)
	{
		*p_nxt_id = p_node->_node_id;
		return SUCCESS;
	}

	return NODE_NOT_FOUND;
}

_int32 trm_find_prev_node_impl(_u32 tree_id,_u32 parent_id,_u32 node_id,const char * name, _u32 name_len,void * data, _u32 data_len,_u32 * p_prev_id)
{
	TREE *p_tree=NULL;
	TREE_NODE *p_parent=NULL,*p_node=NULL,*p_node_tmp=NULL;
	_u64 name_hash=0,data_hash=0;
	char * p_star = NULL;

	p_tree = trm_get_tree_from_map(tree_id);
	if(p_tree==NULL) 
	{
		CHECK_VALUE(INVALID_TREE_ID);
	}
	
	p_parent = trm_get_node_from_map(p_tree,parent_id);
	if(p_parent==NULL) 
	{
		CHECK_VALUE(INVALID_PARENT_ID);
	}

	if(trm_node_check_child(p_parent,node_id)!=TRUE)
	{
		CHECK_VALUE(INVALID_CHILD);
	}
	
	p_node = trm_get_node_from_map(p_tree,node_id);
	if(p_node==NULL) 
	{
		CHECK_VALUE(INVALID_NODE_ID);
	}


	/* 关键字中是否带有*号 */
	if((name!=NULL)&&(name_len!=0))
	{
		p_star = em_strchr((char*)name,'*',0);
		if((p_star!=NULL)&&(p_star-name>name_len))
		{
			p_star = NULL;
		}
	}

	if(p_star==NULL)
	{
		name_hash =trm_generate_name_hash(name,name_len);
	}
	
	data_hash =trm_generate_data_hash(data,data_len);
	if(name_hash!=0)
	{
		p_node_tmp = trm_find_prev_node_by_name_hash(p_tree,p_parent,p_node,name_hash);
	}
	
	if((data_hash!=0)&&(p_node_tmp==NULL))
	{
		p_node_tmp = trm_find_prev_node_by_data_hash(p_tree,p_parent,p_node,data_hash);
	}

	if(p_node_tmp!=NULL)
	{
		*p_prev_id = p_node_tmp->_node_id;
		return SUCCESS;
	}

	/* name hash和data hash 的精确查找都没有结果,再用名字进行模糊查找 */
	p_node = trm_find_prev_node_by_name(p_tree,p_parent,p_node,name, name_len);
	if(p_node!=NULL)
	{
		*p_prev_id = p_node->_node_id;
		return SUCCESS;
	}

	return NODE_NOT_FOUND;
}



