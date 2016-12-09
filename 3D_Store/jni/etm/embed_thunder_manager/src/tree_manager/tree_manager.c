//#include "tree_manager/tree_interface.h"
#include "tree_manager/tree_manager.h"
#include "tree_manager/tree_store.h"
#include "tree_manager/tree_impl.h"

#include "utility/utility.h"

#include "em_common/em_mempool.h"
#include "em_common/em_time.h"
#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_TREE_MANAGER

#include "em_common/em_logger.h"

/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
static TREE_MANAGER g_tree_mgr;
static SLAB *gp_tree_node_slab = NULL;		//TREE_NODE
static SLAB *gp_node_name_slab = NULL;		
static SLAB *gp_tree_slab = NULL;		//TREE

//static BOOL g_have_changed_node=TRUE;


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/




/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 trm_init_slabs(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "trm_init_slabs" );

	sd_assert(gp_tree_slab==NULL);
	if(gp_tree_slab==NULL)
	{
		ret_val = em_mpool_create_slab(sizeof(TREE), MIN_TREE_MEMORY, 0, &gp_tree_slab);
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}

	sd_assert(gp_tree_node_slab==NULL);
	if(gp_tree_node_slab==NULL)
	{
		ret_val = em_mpool_create_slab(sizeof(TREE_NODE), MIN_TREE_NODE_MEMORY, 0, &gp_tree_node_slab);
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}

	sd_assert(gp_node_name_slab==NULL);
	if(gp_node_name_slab==NULL)
	{
		ret_val = em_mpool_create_slab(MAX_FILE_NAME_BUFFER_LEN, MIN_NODE_NAME_MEMORY, 0, &gp_node_name_slab);
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}

	
	return SUCCESS;
	
ErrorHanle:
	if(gp_node_name_slab!=NULL)
	{
		em_mpool_destory_slab(gp_node_name_slab);
		gp_node_name_slab=NULL;
	}
	
	if(gp_tree_node_slab!=NULL)
	{
		em_mpool_destory_slab(gp_tree_node_slab);
		gp_tree_node_slab=NULL;
	}
	
	if(gp_tree_slab!=NULL)
	{
		em_mpool_destory_slab(gp_tree_slab);
		gp_tree_slab=NULL;
	}
	
	return ret_val;
}
_int32 trm_uninit_slabs(void)
{
	EM_LOG_DEBUG( "trm_uninit_slabs" );

	if(gp_node_name_slab!=NULL)
	{
		em_mpool_destory_slab(gp_node_name_slab);
		gp_node_name_slab=NULL;
	}
	
	if(gp_tree_node_slab!=NULL)
	{
		em_mpool_destory_slab(gp_tree_node_slab);
		gp_tree_node_slab=NULL;
	}
	
	if(gp_tree_slab!=NULL)
	{
		em_mpool_destory_slab(gp_tree_slab);
		gp_tree_slab=NULL;
	}
	
	return SUCCESS;
}

_int32 trm_init(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "trm_init" );

	em_memset(&g_tree_mgr,0, sizeof(TREE_MANAGER));

	em_map_init(&g_tree_mgr._all_trees, trm_id_comp);

	return ret_val;
}

_int32 trm_uninit(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "trm_uninit" );

	trm_close_all_trees();

	sd_assert(em_map_size(&g_tree_mgr._all_trees)==0);

	return ret_val;
	
//ErrorHanle:

}

_int32 trm_id_comp(void *E1, void *E2)
{
	_u32 id1 = (_u32)E1,id2 = (_u32)E2;
	_int32 ret_val = 0;
	
	if((id1>MAX_NODE_ID)&&(id2>MAX_NODE_ID))
	{
		id1-=MAX_NODE_ID;
		id2-=MAX_NODE_ID;
		ret_val =  ((_int32)id1) -((_int32)id2);
	}
	else
	if(id1>MAX_NODE_ID)
	{
		ret_val =  1;
	}
	else
	if(id2>MAX_NODE_ID)
	{
		ret_val =  -1;
	}
	else
	{
		ret_val =  ((_int32)id1) -((_int32)id2);
	}

	return ret_val;
//	return ((_int32)E1) -((_int32)E2);
}
_int32 trm_hash_comp(void *E1, void *E2)
{
	_u64 hash1 = 0,hash2 = 0;
	
	hash1 = *((_u64*)E1);
	hash2 = *((_u64*)E2);
	
	if(hash1>hash2) return 1;
	else if(hash1<hash2) return -1;
	else return 0;

/*
	_u64 hash1 = 0,hash2 = 0; 
	_int32 hash_value1 = 0,hash_value2 = 0; 
	hash1 = *((_u64*)E1);
	hash2 = *((_u64*)E2);
	
	hash_value1 = (_int32)(hash1&0x00000000FFFFFFFF);
	hash_value2 = (_int32)(hash2&0x00000000FFFFFFFF);

	// len 
	if(hash_value1!=hash_value2)
	{
		return hash_value1-hash_value2;
	}
	else
	{
		hash_value1 = (_int32)(hash1>>32);
		hash_value2 = (_int32)(hash2>>32);
		return hash_value1-hash_value2;
	}
*/
}


_int32 trm_close_all_trees(void)
{
	return trm_clear_tree_map();
}
_int32 trm_add_root_to_file(TREE *p_tree)
{
	_int32 ret_val = SUCCESS;
	if(p_tree->_root._offset==0)
	{
		ret_val = trm_add_node_to_file(p_tree,&p_tree->_root);
		CHECK_VALUE(ret_val);
	}
	return SUCCESS;
}

_int32 trm_build_tree(TREE *p_tree)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	TREE_NODE *p_node = NULL,*p_node_tmp = NULL;

      EM_LOG_DEBUG("trm_build_tree:%u",em_map_size(&p_tree->_all_nodes)); 
	  
      cur_node = EM_MAP_BEGIN(p_tree->_all_nodes);
      while(cur_node != EM_MAP_END(p_tree->_all_nodes))
      {
             p_node = (TREE_NODE *)EM_MAP_VALUE(cur_node);

		if(!p_node->_is_root)
		{
			sd_assert((_u32)(p_node->_parent)!=TRM_INVALID_NODE_ID);
			p_node_tmp = trm_get_node_from_map(p_tree,(_u32)(p_node->_parent));
			//sd_assert(p_node_tmp!=NULL);
			if(p_node_tmp==NULL) 
			{
				ret_val = INVALID_PARENT_ID;
	      			EM_LOG_ERROR("trm_build_tree:ERROR:INVALID_PARENT_ID:%u,%u",p_node->_node_id,(_u32)(p_node->_parent)); 
				CHECK_VALUE(ret_val);
			}
			
			p_node->_parent = p_node_tmp;
		}
		
		if(p_node->_pre_node!=NULL)
		{
			p_node_tmp = trm_get_node_from_map(p_tree,(_u32)(p_node->_pre_node));
			//sd_assert(p_node_tmp!=NULL);
			if(p_node_tmp==NULL) 
			{
				ret_val = INVALID_NODE_ID;
	      			EM_LOG_ERROR("trm_build_tree:ERROR:INVALID_PRENODE_ID:%u,%u",p_node->_node_id,(_u32)(p_node->_pre_node)); 
				CHECK_VALUE(ret_val);
			}
			
			p_node->_pre_node = p_node_tmp;
		}
		
		if(p_node->_nxt_node!=NULL)
		{
			p_node_tmp = trm_get_node_from_map(p_tree,(_u32)(p_node->_nxt_node));
			//sd_assert(p_node_tmp!=NULL);
			if(p_node_tmp==NULL) 
			{
	      			EM_LOG_ERROR("trm_build_tree:ERROR:INVALID_NXTNODE_ID:%u,%u",p_node->_node_id,(_u32)(p_node->_nxt_node)); 
				ret_val = INVALID_NODE_ID;
				CHECK_VALUE(ret_val);
			}
			
			p_node->_nxt_node = p_node_tmp;
		}
		
		if(p_node->_first_child!=NULL)
		{
			p_node_tmp = trm_get_node_from_map(p_tree,(_u32)(p_node->_first_child));
			//sd_assert(p_node_tmp!=NULL);
			if(p_node_tmp==NULL) 
			{
	      			EM_LOG_ERROR("trm_build_tree:ERROR:INVALID_1ST_CHILD_ID:%u,%u",p_node->_node_id,(_u32)(p_node->_first_child)); 
				ret_val = INVALID_NODE_ID;
				CHECK_VALUE(ret_val);
			}
			
			p_node->_first_child = p_node_tmp;
		}
		
		if(p_node->_last_child!=NULL)
		{
			p_node_tmp = trm_get_node_from_map(p_tree,(_u32)(p_node->_last_child));
			//sd_assert(p_node_tmp!=NULL);
			if(p_node_tmp==NULL) 
			{
	      			EM_LOG_ERROR("trm_build_tree:ERROR:INVALID_LAST_CHILD_ID:%u,%u",p_node->_node_id,(_u32)(p_node->_last_child)); 
				ret_val = INVALID_NODE_ID;
				CHECK_VALUE(ret_val);
			}
			
			p_node->_last_child = p_node_tmp;
		}
		
	      cur_node = EM_MAP_NEXT(p_tree->_all_nodes, cur_node);
      }

	return SUCCESS;

}

_int32 trm_load_nodes_from_file(TREE *p_tree)
{
	_int32 ret_val = SUCCESS,node_count=0,read_node_ret = 0;
	TREE_NODE *p_node=NULL,*p_node_tmp=NULL;
	_u32 root_id = p_tree->_root._node_id;
READ_AGAIN:
	read_node_ret = 0;
	node_count=0;
      EM_LOG_DEBUG("trm_load_nodes_from_file"); 
	/* read out all the nodes from the file one by one ,DO NOT STOP! */
	for(p_node_tmp=trm_get_node_from_file(p_tree,&read_node_ret);p_node_tmp!=NULL;p_node_tmp=trm_get_node_from_file(p_tree,&read_node_ret))
	{
      		EM_LOG_DEBUG("trm_load_nodes_from_file,get node_id=%u",p_node_tmp->_node_id); 
			
		if(read_node_ret!=SUCCESS) 
		{
			/* 读数据文件出错,必须用备份文件恢复 */
			if(p_node!=NULL)
			{
				trm_node_uninit(p_node);
				trm_node_free(p_node);
			}
			goto ErrorHanle;
		}
		
		if(p_node_tmp->_is_root)
		{
			em_memset(&p_tree->_root, 0, sizeof(TREE_NODE));
			em_memcpy(&p_tree->_root, p_node_tmp, sizeof(TREE_NODE));
			p_tree->_root._node_id = root_id;
			node_count++;
			continue;
		}
		
		if(p_node==NULL)
		{
			ret_val = trm_node_malloc(&p_node);
			if(ret_val!=SUCCESS) goto ErrorHanle;
		}
		em_memset(p_node, 0, sizeof(TREE_NODE));
		em_memcpy(p_node, p_node_tmp, sizeof(TREE_NODE));
		
		/* add to all_nodes_map */
		ret_val = trm_add_node_to_map(p_tree,p_node);
		if(ret_val!=SUCCESS) 
		{
	      		EM_LOG_ERROR("trm_load_nodes_from_file:ERROR:ret_val=%d!",ret_val); 
			trm_node_uninit(p_node);
			continue;
		}
		
		p_node=NULL;
		node_count++;
	}

	if(p_node!=NULL)
	{
		trm_node_uninit(p_node);
		trm_node_free(p_node);
	}
	
	if(read_node_ret!=SUCCESS) 
	{
		/* 读数据文件出错,必须用备份文件恢复 */
		goto ErrorHanle;
	}
	
	ret_val = trm_build_tree(p_tree);
	if(ret_val!=SUCCESS) goto ErrorHanle;

      EM_LOG_DEBUG("trm_load_nodes_from_file:%u,end!",node_count); 
	return SUCCESS;
	
ErrorHanle:
	//trm_clear_data_hash_map(p_tree);
	//trm_clear_name_hash_map(p_tree);
	trm_clear_node_map(p_tree);
	if(trm_recover_file(p_tree))
	{
		ret_val = trm_add_node_to_map(p_tree,&p_tree->_root);
		if(ret_val==SUCCESS) 
			goto READ_AGAIN;
	}
	else
	{
	      	EM_LOG_ERROR("trm_load_nodes_from_file:ERROR:trm_recover_file!"); 
		
		ret_val = trm_create_tree_file(p_tree);
		CHECK_VALUE(ret_val);
		ret_val = trm_get_total_node_num(p_tree,&p_tree->_total_node_num);
		CHECK_VALUE(ret_val);
		
		if(p_tree->_total_node_num==0)
		{
			p_tree->_total_node_num++;
			trm_save_total_nodes_num(p_tree);
		}
		
		p_tree->_root._children_num = 0;
		p_tree->_root._first_child= NULL;
		p_tree->_root._last_child= NULL;
		p_tree->_root._offset= 0;
		p_tree->_root._is_root= TRUE;
		p_tree->_root._full_info= TRUE;

		ret_val = trm_add_node_to_map(p_tree,&p_tree->_root);
		CHECK_VALUE(ret_val);
		
	}
	return ret_val;
}

_int32 trm_save_nodes(TREE *p_tree)
{
 	 _int32 ret_val = SUCCESS;
     MAP_ITERATOR  cur_node = NULL; 
	 TREE_NODE * p_node = NULL;

      EM_LOG_DEBUG("trm_save_nodes:%s",p_tree->_tree_file); 

	  if(p_tree->_have_changed_node==FALSE)
	  	return SUCCESS;
	  
      cur_node = EM_MAP_BEGIN(p_tree->_all_nodes);
      while(cur_node != EM_MAP_END(p_tree->_all_nodes))
      {
             p_node = (TREE_NODE *)EM_MAP_VALUE(cur_node);
	      if(p_node->_change_flag!=0)
	      	{	
	      		trm_save_node_to_file(p_tree,p_node);
	      	}
	      cur_node = EM_MAP_NEXT(p_tree->_all_nodes, cur_node);
      }
	  p_tree->_have_changed_node=FALSE;
	  
	  ret_val = trm_check_node_file(p_tree,em_map_size(&p_tree->_all_nodes));
	  if(ret_val!=SUCCESS)	
	  {
	  #if 1
	  	/* 文件已被破坏,不能在运行过程中修复,因为trm_recover_file后树节点的偏移地址有可能与内存中的不一致,程序必须重启! */
	  	em_set_critical_error(WRITE_TREE_FILE_ERR);
		CHECK_VALUE(WRITE_TREE_FILE_ERR);
	  #else
	  	if(FALSE==trm_recover_file(p_tree))
	  	{
	      		EM_LOG_ERROR("trm_save_nodes:ERROR:trm_recover_file FAILED!:%s",p_tree->_tree_file); 
			CHECK_VALUE(INVALID_NODE_ID);
	  	}
	  #endif
	  }
	  
	trm_backup_file(p_tree);
	  
	return SUCCESS;
}


_int32 trm_set_node_change(TREE *p_tree,TREE_NODE * p_node,_u32 change_flag)
{
      EM_LOG_DEBUG("trm_set_node_change:node_id=%u,%u",p_node->_node_id,change_flag); 
	p_node->_change_flag|=change_flag;
	
	if(!p_tree->_have_changed_node)
		p_tree->_have_changed_node=TRUE;
	
	return SUCCESS;
	
}

_int32 trm_save_total_nodes_num(TREE *p_tree)
{
      EM_LOG_DEBUG("trm_save_total_nodes_num :%u",p_tree->_total_node_num); 
	return trm_save_total_node_num(p_tree,p_tree->_total_node_num);
}




////////////////////////////////////////////////////////////////////


_u32 trm_create_node_id(TREE * p_tree)
{
      EM_LOG_DEBUG("trm_create_node_id"); 

	  if(p_tree->_total_node_num == 0) p_tree->_total_node_num = 1;

	p_tree->_total_node_num++;  // node_id 从2开始

	if(p_tree->_total_node_num==MAX_NODE_ID)
	{
		sd_assert(FALSE);
		p_tree->_total_node_num = 2;  // node_id 从2开始
	}
	
	trm_save_total_node_num(p_tree,p_tree->_total_node_num);
	return p_tree->_total_node_num;
}

_int32 trm_decrease_node_id(TREE * p_tree)
{
      EM_LOG_DEBUG("trm_decrease_node_id"); 

	  sd_assert(p_tree->_total_node_num!=1);

	p_tree->_total_node_num--;

	trm_save_total_node_num(p_tree,p_tree->_total_node_num);
	
	return SUCCESS;
}

TREE_NODE * trm_get_node_from_map(TREE * p_tree,_u32 node_id)
{
	_int32 ret_val = SUCCESS;
	TREE_NODE * p_node=NULL;

      EM_LOG_DEBUG("trm_get_node_from_map:node_id=%u",node_id); 
	  
	  sd_assert(node_id!=TRM_INVALID_NODE_ID);
	  
	  if(node_id>MAX_NODE_ID)
	  {
	  	return &(p_tree->_root);
	  }
	  
	ret_val=em_map_find_node(&p_tree->_all_nodes, (void *)node_id, (void **)&p_node);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_node!=NULL);
	return p_node;
}
_int32 trm_add_node_to_map(TREE * p_tree,TREE_NODE * p_node)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("trm_add_node_to_map:_node_id=%u",p_node->_node_id);
	  
	  sd_assert(p_node->_node_id!=TRM_INVALID_NODE_ID);
	  //sd_assert(p_node->_node_id!=TRM_ROOT_NODE_ID);
	  
	info_map_node._key =(void*) p_node->_node_id;
	info_map_node._value = (void*)p_node;
	ret_val = em_map_insert_node( &p_tree->_all_nodes, &info_map_node );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

_int32 trm_remove_node_from_map(TREE * p_tree,TREE_NODE * p_node)
{
      EM_LOG_DEBUG("trm_remove_node_from_map:_node_id=%u",p_node->_node_id); 
	  sd_assert(p_node->_node_id!=TRM_INVALID_NODE_ID);
	  sd_assert(p_node->_node_id!=TRM_ROOT_NODE_ID);
	  
	return em_map_erase_node(&p_tree->_all_nodes, (void*)p_node->_node_id);
}
_int32 trm_clear_node_map(TREE * p_tree)
{
      MAP_ITERATOR  cur_node = NULL; 
	TREE_NODE * p_node=NULL;

      EM_LOG_DEBUG("trm_clear_node_map:%u",em_map_size(&p_tree->_all_nodes)); 
	  
      cur_node = EM_MAP_BEGIN(p_tree->_all_nodes);
      while(cur_node != EM_MAP_END(p_tree->_all_nodes))
      {
             p_node = (TREE_NODE *)EM_MAP_VALUE(cur_node);
		if(p_node->_is_root!=TRUE)
		{
	      		trm_node_uninit(p_node);
	      		trm_node_free(p_node);
		}
		em_map_erase_iterator(&p_tree->_all_nodes, cur_node);
	      cur_node = EM_MAP_BEGIN(p_tree->_all_nodes);
      }
	return SUCCESS;
}

BOOL trm_is_node_exist(TREE * p_tree,_u32 node_id)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = NULL;

      EM_LOG_DEBUG("trm_is_node_exist"); 
	ret_val = em_map_find_iterator( &p_tree->_all_nodes, (void *)node_id, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == EM_MAP_END( p_tree->_all_nodes ) )
	{
		return FALSE;
	}

	return TRUE;
}


_u32 trm_create_tree_id(void)
{
      EM_LOG_DEBUG("trm_create_tree_id"); 

	g_tree_mgr._total_tree_num++;

	if(g_tree_mgr._total_tree_num==MAX_TREE_ID)
	{
		sd_assert(FALSE);
		g_tree_mgr._total_tree_num = 1;
	}
	
	return g_tree_mgr._total_tree_num+MAX_NODE_ID;
}

_int32 trm_decrease_tree_id(void)
{
      EM_LOG_DEBUG("trm_decrease_tree_id"); 

	sd_assert(g_tree_mgr._total_tree_num!=0);
	g_tree_mgr._total_tree_num--;

	return SUCCESS;
}

TREE * trm_get_tree_from_map(_u32 tree_id)
{
	_int32 ret_val = SUCCESS;
	TREE * p_tree=NULL;

      EM_LOG_DEBUG("trm_get_tree_from_map:tree_id=%u",tree_id); 
	  
	ret_val=em_map_find_node(&g_tree_mgr._all_trees, (void *)tree_id, (void **)&p_tree);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_node!=NULL);
	return p_tree;
}
_int32 trm_add_tree_to_map(TREE * p_tree)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("trm_add_tree_to_map:tree_id=%u,%s",p_tree->_root._node_id,p_tree->_tree_file);
	  
	info_map_node._key =(void*) p_tree->_root._node_id;
	info_map_node._value = (void*)p_tree;
	ret_val = em_map_insert_node( &g_tree_mgr._all_trees, &info_map_node );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

_int32 trm_remove_tree_from_map(TREE * p_tree)
{
      EM_LOG_DEBUG("trm_remove_tree_from_map:tree_id=%u,%s",p_tree->_root._node_id,p_tree->_tree_file);
	  
	return em_map_erase_node(&g_tree_mgr._all_trees, (void*)p_tree->_root._node_id);
}
_int32 trm_clear_tree_map(void)
{
      MAP_ITERATOR  cur_node = NULL; 
	TREE * p_tree=NULL;

      EM_LOG_DEBUG("trm_clear_tree_map:%u",em_map_size(&g_tree_mgr._all_trees)); 
	  
      cur_node = EM_MAP_BEGIN(g_tree_mgr._all_trees);
      while(cur_node != EM_MAP_END(g_tree_mgr._all_trees))
      {
             p_tree = (TREE *)EM_MAP_VALUE(cur_node);
      		trm_close_tree_impl(p_tree);
		em_map_erase_iterator(&g_tree_mgr._all_trees, cur_node);
		trm_tree_free(p_tree);
	      cur_node = EM_MAP_BEGIN(g_tree_mgr._all_trees);
      }
	return SUCCESS;
}

BOOL trm_is_tree_opened(TREE * p_tree,TREE ** pp_tree_old)
{
      MAP_ITERATOR  cur_node = NULL; 
	TREE * p_tree_tmp=NULL;

      EM_LOG_DEBUG("trm_is_tree_opened"); 
	  
      cur_node = EM_MAP_BEGIN(g_tree_mgr._all_trees);
      while(cur_node != EM_MAP_END(g_tree_mgr._all_trees))
      {
             p_tree_tmp = (TREE *)EM_MAP_VALUE(cur_node);
		if(em_stricmp(p_tree->_tree_file,p_tree_tmp->_tree_file)==0)
		{
	      		*pp_tree_old = p_tree_tmp;
			return TRUE;
		}
	      cur_node = EM_MAP_NEXT(g_tree_mgr._all_trees,cur_node);
      }
	return FALSE;
}
_int32 trm_tree_scheduler(void)
{
      MAP_ITERATOR  cur_node = NULL; 
	TREE * p_tree=NULL;

      EM_LOG_DEBUG("trm_tree_scheduler"); 
	  
      cur_node = EM_MAP_BEGIN(g_tree_mgr._all_trees);
      while(cur_node != EM_MAP_END(g_tree_mgr._all_trees))
      {
             p_tree = (TREE *)EM_MAP_VALUE(cur_node);
		trm_tree_scheduler_impl(p_tree);
	      cur_node = EM_MAP_NEXT(g_tree_mgr._all_trees,cur_node);
      }
	return SUCCESS;
}

_u64 trm_generate_data_hash(void* data,_u32 data_len)
{
	return sd_generate_hash_from_size_crc_hashvalue((const _u8*)data,data_len);
}
#if 0
TREE_NODE * trm_get_first_node_from_data_hash_map(TREE * p_tree,TREE_NODE * p_parent,_u64 hash)
{
	TREE_NODE * p_node_tmp = NULL;
	LIST_ITERATOR cur_iter = NULL;
	MAP_ITERATOR result_iterator;
	LIST * p_list = trm_get_node_list_from_data_hash_map(p_tree, hash,&result_iterator);

	if(p_list!=NULL)
	{
		cur_iter = LIST_BEGIN(*p_list);
		while(cur_iter != LIST_END(*p_list))
		{
			p_node_tmp = (TREE_NODE*)LIST_VALUE(cur_iter);
			if(p_node_tmp->_parent == p_parent)
			{
				return p_node_tmp;
			}
			cur_iter = LIST_NEXT(cur_iter);
		}
	}
	return p_node_tmp;
}
LIST * trm_get_node_list_from_data_hash_map(TREE * p_tree,_u64 hash,MAP_ITERATOR * p_result_iterator)
{
	_int32 ret_val = SUCCESS;
	LIST * p_list=NULL;
	MAP_ITERATOR result_iterator;

       EM_LOG_DEBUG("trm_get_node_list_from_data_hash_map");
	   *p_result_iterator  = NULL;
	  if(hash==0)
	  {
	  	return NULL;
	  }
	  
	//ret_val=em_map_find_node(&p_tree->_all_data_hash_lists, (void *)hash, (void **)&p_list);
	ret_val=em_map_find_iterator(&p_tree->_all_data_hash_lists, (void *)hash,&result_iterator);
	sd_assert(ret_val==SUCCESS);
	if(result_iterator!=NULL)
	{
		p_list = (LIST*)MAP_VALUE(result_iterator);
		*p_result_iterator = result_iterator;
	}
	return p_list;
}
_int32 trm_add_data_hash_to_map(TREE * p_tree,TREE_NODE * p_node)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
	MAP_ITERATOR result_iterator;
	LIST * p_list = NULL;
       EM_LOG_DEBUG("trm_add_data_hash_to_map:_node_id=%u",p_node->_node_id);
	  
	sd_assert(p_node->_node_id!=TRM_INVALID_NODE_ID);
	if(p_node->_data_hash == 0) return INVALID_DATA_HASH;

	p_list = trm_get_node_list_from_data_hash_map(p_tree,p_node->_data_hash,&result_iterator);
	if(p_list == NULL)
	{
		ret_val = em_malloc(sizeof(LIST),(void**)&p_list);
		CHECK_VALUE(ret_val);
		em_list_init(p_list);
		ret_val = em_list_push(p_list, (void *)p_node);
		CHECK_VALUE(ret_val);
		info_map_node._key =(void*) p_node->_data_hash;
		info_map_node._value = (void*)p_list;
		ret_val = em_map_insert_node( &p_tree->_all_data_hash_lists, &info_map_node );
		CHECK_VALUE(ret_val);
	}
	else
	{
		ret_val = em_list_push(p_list, (void *)p_node);
		CHECK_VALUE(ret_val);
	}
	return SUCCESS;
}

_int32 trm_remove_data_hash_from_map(TREE * p_tree,TREE_NODE * p_node)
{
	_int32 ret_val = SUCCESS;
	//PAIR info_map_node;
	MAP_ITERATOR result_iterator;
	LIST * p_list = NULL;
	LIST_ITERATOR cur_iter = NULL;
	TREE_NODE * p_node_tmp = NULL;
       EM_LOG_DEBUG("trm_remove_data_hash_from_map:_node_id=%u",p_node->_node_id);
	  
	sd_assert(p_node->_node_id!=TRM_INVALID_NODE_ID);
	if(p_node->_data_hash == 0) return INVALID_DATA_HASH;

	p_list = trm_get_node_list_from_data_hash_map(p_tree,p_node->_data_hash,&result_iterator);
	if(p_list == NULL)
	{
		return INVALID_DATA_HASH;
	}
	else
	{
		cur_iter = LIST_BEGIN(*p_list);
		while(cur_iter != LIST_END(*p_list))
		{
			p_node_tmp = (TREE_NODE*)LIST_VALUE(cur_iter);
			if(p_node_tmp == p_node)
			{
				em_list_erase(p_list, cur_iter);
				if(em_list_size(p_list)==0)
				{
					EM_SAFE_DELETE(p_list);
					em_map_erase_iterator(&p_tree->_all_data_hash_lists, result_iterator);
				}
				return SUCCESS;
			}
			cur_iter = LIST_NEXT(cur_iter);
		}
	}
	/* Error:Not found!*/
	CHECK_VALUE(INVALID_DATA_HASH);
	return ret_val;
}
_int32 trm_clear_data_hash_map(TREE * p_tree)
{
      MAP_ITERATOR  cur_node = NULL; 
	TREE_NODE * p_node=NULL;
	LIST * p_list = NULL;

      EM_LOG_DEBUG("trm_clear_data_hash_map:%u",em_map_size(&p_tree->_all_data_hash_lists)); 
	  
      cur_node = EM_MAP_BEGIN(p_tree->_all_data_hash_lists);
      while(cur_node != EM_MAP_END(p_tree->_all_data_hash_lists))
      {
             p_list = (LIST *)EM_MAP_VALUE(cur_node);
		sd_assert(em_list_size(p_list)!=0);
		em_list_clear(p_list);
		EM_SAFE_DELETE(p_list);
		em_map_erase_iterator(&p_tree->_all_data_hash_lists, cur_node);
	      cur_node = EM_MAP_BEGIN(p_tree->_all_data_hash_lists);
      }
	return SUCCESS;
}

BOOL trm_is_data_hash_exist(TREE * p_tree,_u64 hash)
{
/*
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = NULL;

      EM_LOG_DEBUG("trm_is_node_exist"); 
	ret_val = em_map_find_iterator( &p_tree->_all_nodes, (void *)node_id, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == EM_MAP_END( p_tree->_all_nodes ) )
	{
		return FALSE;
	}

	return TRUE;
*/
	return FALSE;
}
#endif /* DATA HASH */
_u64 trm_generate_name_hash(const char* name,_u32 name_len)
{
	return sd_generate_hash_from_size_crc_hashvalue((const _u8*)name,name_len);
}
#if 0
TREE_NODE * trm_get_node_from_name_hash_map(TREE * p_tree,_u64 hash)
{
	/*
	_int32 ret_val = SUCCESS;
	TREE_NODE * p_node=NULL;

	  if(hash==0)
	  {
	  	return NULL;
	  }
	  
	ret_val=em_map_find_node(&p_tree->_all_data_hash_lists, (void *)hash, (void **)&p_node);
	sd_assert(ret_val==SUCCESS);
	return p_node;
	*/
	return NULL;
}
LIST * trm_get_node_list_from_name_hash_map(TREE * p_tree,_u64 hash,MAP_ITERATOR * p_result_iterator)
{
	_int32 ret_val = SUCCESS;
	LIST * p_list=NULL;
	MAP_ITERATOR result_iterator;

       EM_LOG_DEBUG("trm_get_node_list_from_name_hash_map");
	   *p_result_iterator  = NULL;
	  if(hash==0)
	  {
	  	return NULL;
	  }
	  
	//ret_val=em_map_find_node(&p_tree->_all_data_hash_lists, (void *)hash, (void **)&p_list);
	ret_val=em_map_find_iterator(&p_tree->_all_name_hash_lists, (void *)hash,&result_iterator);
	sd_assert(ret_val==SUCCESS);
	if(result_iterator!=NULL)
	{
		p_list = (LIST*)MAP_VALUE(result_iterator);
		*p_result_iterator = result_iterator;
	}
	return p_list;
}
_int32 trm_add_name_hash_to_map(TREE * p_tree,TREE_NODE * p_node)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
	MAP_ITERATOR result_iterator;
	LIST * p_list = NULL;
       EM_LOG_DEBUG("trm_add_name_hash_to_map:_node_id=%u",p_node->_node_id);
	  
	sd_assert(p_node->_node_id!=TRM_INVALID_NODE_ID);
	if(p_node->_name_hash == 0) return INVALID_NAME_HASH;

	p_list = trm_get_node_list_from_name_hash_map(p_tree,p_node->_name_hash,&result_iterator);
	if(p_list == NULL)
	{
		ret_val = em_malloc(sizeof(LIST),(void**)&p_list);
		CHECK_VALUE(ret_val);
		em_list_init(p_list);
		ret_val = em_list_push(p_list, (void *)p_node);
		CHECK_VALUE(ret_val);
		info_map_node._key =(void*) p_node->_name_hash;
		info_map_node._value = (void*)p_list;
		ret_val = em_map_insert_node( &p_tree->_all_name_hash_lists, &info_map_node );
		CHECK_VALUE(ret_val);
	}
	else
	{
		ret_val = em_list_push(p_list, (void *)p_node);
		CHECK_VALUE(ret_val);
	}
	return SUCCESS;
}

_int32 trm_remove_name_hash_from_map(TREE * p_tree,TREE_NODE * p_node)
{
	_int32 ret_val = SUCCESS;
	//PAIR info_map_node;
	MAP_ITERATOR result_iterator;
	LIST * p_list = NULL;
	LIST_ITERATOR cur_iter = NULL;
	TREE_NODE * p_node_tmp = NULL;
       EM_LOG_DEBUG("trm_remove_name_hash_from_map:_node_id=%u",p_node->_node_id);
	  
	sd_assert(p_node->_node_id!=TRM_INVALID_NODE_ID);
	if(p_node->_name_hash == 0) return INVALID_NAME_HASH;

	p_list = trm_get_node_list_from_name_hash_map(p_tree,p_node->_name_hash,&result_iterator);
	if(p_list == NULL)
	{
		return INVALID_NAME_HASH;
	}
	else
	{
		cur_iter = LIST_BEGIN(*p_list);
		while(cur_iter != LIST_END(*p_list))
		{
			p_node_tmp = (TREE_NODE*)LIST_VALUE(cur_iter);
			if(p_node_tmp == p_node)
			{
				em_list_erase(p_list, cur_iter);
				if(em_list_size(p_list)==0)
				{
					EM_SAFE_DELETE(p_list);
					em_map_erase_iterator(&p_tree->_all_name_hash_lists, result_iterator);
				}
				return SUCCESS;
			}
			cur_iter = LIST_NEXT(cur_iter);
		}
	}
	/* Error:Not found!*/
	CHECK_VALUE(INVALID_NAME_HASH);
	return ret_val;
}
_int32 trm_clear_name_hash_map(TREE * p_tree)
{
      MAP_ITERATOR  cur_node = NULL; 
	TREE_NODE * p_node=NULL;
	LIST * p_list = NULL;

      EM_LOG_DEBUG("trm_clear_name_hash_map:%u",em_map_size(&p_tree->_all_name_hash_lists)); 
	  
      cur_node = EM_MAP_BEGIN(p_tree->_all_name_hash_lists);
      while(cur_node != EM_MAP_END(p_tree->_all_name_hash_lists))
      {
             p_list = (LIST *)EM_MAP_VALUE(cur_node);
		sd_assert(em_list_size(p_list)!=0);
		em_list_clear(p_list);
		EM_SAFE_DELETE(p_list);
		em_map_erase_iterator(&p_tree->_all_name_hash_lists, cur_node);
	      cur_node = EM_MAP_BEGIN(p_tree->_all_name_hash_lists);
      }
	return SUCCESS;
}

BOOL trm_is_name_hash_exist(TREE * p_tree,_u64 hash)
{
/*
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = NULL;

      EM_LOG_DEBUG("trm_is_node_exist"); 
	ret_val = em_map_find_iterator( &p_tree->_all_nodes, (void *)node_id, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == EM_MAP_END( p_tree->_all_nodes ) )
	{
		return FALSE;
	}

	return TRUE;
*/
	return FALSE;
}

#endif /* NAME HASH */
////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

_int32 trm_node_malloc(TREE_NODE **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_tree_node_slab, (void**)pp_slip );
	
      EM_LOG_DEBUG("trm_node_malloc:%d,0x%X",ret_val,*pp_slip); 
	return ret_val;
}

_int32 trm_node_free(TREE_NODE * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
      EM_LOG_DEBUG("trm_node_free,0x%X",p_slip); 
	return em_mpool_free_slip( gp_tree_node_slab, (void*)p_slip );
}

_int32 trm_node_init(TREE * p_tree,TREE_NODE *p_node,const char * name, _u32 name_len,void* data,_u32 data_len)
{
      _int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("trm_node_init"); 
	em_memset(p_node,0,sizeof(TREE_NODE));
	p_node->_node_id = trm_create_node_id(p_tree);
	if( p_node->_node_id == TRM_INVALID_NODE_ID ) 
	{
		return INVALID_NODE_ID;
	}
	//p_node->_parent = p_parent;
	//em_list_init(p_node->_child);
	if((data!=NULL)&&(data_len!=0))
	{
		ret_val = em_malloc(data_len, (void**)&p_node->_data);
		CHECK_VALUE(ret_val);
		em_memcpy(p_node->_data, data, data_len);
		p_node->_data_len = data_len;
		p_node->_data_hash = trm_generate_data_hash(data, data_len);
	}
	
	if((name!=NULL)&&(name_len!=0))
	{
		ret_val= trm_node_name_malloc(&p_node->_name);
		CHECK_VALUE(ret_val);
		em_memset(p_node->_name,0,MAX_FILE_NAME_BUFFER_LEN);
		em_memcpy(p_node->_name, name, name_len);
		p_node->_name_len = name_len;
		p_node->_name_hash = trm_generate_name_hash(name, name_len);
	}
	p_node->_is_root = FALSE;
	p_node->_full_info = TRUE;
	p_node->_tree = (void*)p_tree;
	em_time_ms(&p_node->_create_time);
	return SUCCESS;
}
_int32 trm_node_uninit(TREE_NODE *p_node)
{
      EM_LOG_DEBUG("trm_node_uninit"); 

	EM_SAFE_DELETE(p_node->_data);
	trm_node_name_free(p_node->_name);
	EM_SAFE_DELETE(p_node->_items);
	
	return SUCCESS;
}
_int32 trm_tree_malloc(TREE **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_tree_slab, (void**)pp_slip );
	
	em_memset(*pp_slip,0x00,sizeof(TREE));
      EM_LOG_DEBUG("trm_tree_malloc:%d,0x%X",ret_val,*pp_slip); 
	return ret_val;
}

_int32 trm_tree_free(TREE * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
      EM_LOG_DEBUG("trm_tree_free,0x%X",p_slip); 
	return em_mpool_free_slip( gp_tree_slab, (void*)p_slip );
}

_int32 trm_tree_init(TREE *p_tree)
{
	em_map_init(&p_tree->_all_nodes, trm_id_comp);
	//em_map_init(&p_tree->_all_name_hash_lists, trm_hash_comp);
	//em_map_init(&p_tree->_all_data_hash_lists, trm_hash_comp);
	
	p_tree->_have_changed_node=TRUE;

	return SUCCESS;
}
_int32 trm_tree_uninit(TREE *p_tree)
{
      EM_LOG_DEBUG("trm_tree_uninit"); 

	//trm_close_all_trees();

	sd_assert(em_map_size(&p_tree->_all_nodes)==0);
	//sd_assert(em_map_size(&p_tree->_all_name_hash_lists)==0);
	//sd_assert(em_map_size(&p_tree->_all_data_hash_lists)==0);
	
	return SUCCESS;
}

_int32 trm_node_set_parent(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_parent)
{
	p_node->_parent= p_parent;
	trm_set_node_change(p_tree,p_node,TRN_CHANGE_PARENT);

	return SUCCESS;
}
TREE_NODE * trm_node_get_parent(TREE_NODE *p_node)
{
      return p_node->_parent;
}
_int32 trm_node_set_pre(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_pre)
{
	p_node->_pre_node= p_pre;
	trm_set_node_change(p_tree,p_node,TRN_CHANGE_PRE_NODE);
	
	return SUCCESS;
}
TREE_NODE * trm_node_get_pre(TREE_NODE *p_node)
{
      return p_node->_pre_node;
}
_int32 trm_node_set_nxt(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_nxt)
{
	p_node->_nxt_node= p_nxt;
	trm_set_node_change(p_tree,p_node,TRN_CHANGE_NXT_NODE);
	
	return SUCCESS;
}
TREE_NODE * trm_node_get_nxt(TREE_NODE *p_node)
{
      return p_node->_nxt_node;
}

_int32 trm_node_set_first_child(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_child)
{
	p_node->_first_child = p_child;
	trm_set_node_change(p_tree,p_node,TRN_CHANGE_FIRST_CHILD);
	
	return SUCCESS;
}
TREE_NODE * trm_node_get_first_child(TREE_NODE *p_node)
{
      return p_node->_first_child;
}

_int32 trm_node_set_last_child(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_child)
{
	p_node->_last_child= p_child;
	trm_set_node_change(p_tree,p_node,TRN_CHANGE_LAST_CHILD);
	
	return SUCCESS;
}
TREE_NODE * trm_node_get_last_child(TREE_NODE *p_node)
{
      return p_node->_last_child;
}
BOOL trm_node_check_tree(TREE_NODE *p_node,_u32 tree_id)
{
	TREE *p_tree = NULL;
	
	sd_assert(p_node!=NULL);
	if(!p_node) return FALSE;

	p_tree = (TREE *)p_node->_tree;
	sd_assert(p_tree!=NULL);

	if(p_tree->_root._node_id == tree_id)
		return TRUE;

	return FALSE;
}


BOOL trm_node_check_child(TREE_NODE *p_node,_u32 child_id)
{
	TREE_NODE *p_node_tmp = NULL;
	p_node_tmp = trm_node_get_first_child(p_node);//p_node->_first_child;
	while(p_node_tmp!=NULL)
	{
		if(p_node_tmp->_node_id == child_id)
		{
			return TRUE;
		}
		else
		if(trm_node_check_child(p_node_tmp, child_id)==TRUE)
		{
			return TRUE;
		}
		
		p_node_tmp=trm_node_get_nxt(p_node_tmp);//p_node_tmp->_nxt_node;
	}
	
	return FALSE;
}

_int32 trm_node_add_child(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_child)
{
      EM_LOG_DEBUG("trm_node_add_child:node_id=%u,child_id=%u",p_node->_node_id,p_child->_node_id); 

	sd_assert(trm_node_get_parent(p_child) == NULL);
	sd_assert(trm_node_get_pre(p_child) == NULL);
	sd_assert(trm_node_get_nxt(p_child) == NULL);
	
	  if(trm_node_get_first_child(p_node)==NULL)
	  {
		trm_node_set_first_child(p_tree,p_node,p_child);
		trm_node_set_last_child(p_tree,p_node,p_child);
	  }
	  else
	  {
		trm_node_set_pre(p_tree,p_child,trm_node_get_last_child(p_node));
		trm_node_set_nxt(p_tree,trm_node_get_last_child(p_node),p_child);
		trm_node_set_last_child(p_tree,p_node,p_child);
	  }
	  
	p_node->_children_num++;
	trm_set_node_change(p_tree,p_node,TRN_CHANGE_CHILD_NUM);
	
	trm_node_set_parent(p_tree,p_child,p_node);
	  
	return SUCCESS;
}
_u32 trm_node_find_by_data(TREE_NODE *p_node,void * data, _u32 data_len)
{
	TREE_NODE *p_node_tmp = NULL;
	_u32 node_id = TRM_INVALID_NODE_ID;
	p_node_tmp = p_node->_first_child;
	while(p_node_tmp!=NULL)
	{
		if((p_node_tmp->_data_len == data_len)&&(em_memcmp(data,p_node_tmp->_data,data_len)==0))
		{
			return p_node_tmp->_node_id;
		}
		else
		{
			node_id = trm_node_find_by_data(p_node_tmp, data,data_len);
			if(node_id!=TRM_INVALID_NODE_ID)
			{
				return node_id;
			}
		}
		p_node_tmp=p_node_tmp->_nxt_node;
	}
	
	return TRM_INVALID_NODE_ID;
}
_int32 trm_node_remove_all_child(TREE *p_tree,TREE_NODE *p_node)
{
	_int32 ret_val = SUCCESS;
	TREE_NODE *p_node_tmp = NULL;

	p_node_tmp = trm_node_get_first_child(p_node); //p_node->_first_child;
	while(p_node_tmp!=NULL)
	{
		ret_val = trm_node_remove_all_child(p_tree,p_node_tmp);
		CHECK_VALUE(ret_val);

		trm_disable_node_in_file(p_tree,p_node_tmp);
		
		//trm_remove_data_hash_from_map(p_tree,p_node_tmp);
		
		//trm_remove_name_hash_from_map(p_tree,p_node_tmp);
		
		trm_remove_node_from_map(p_tree,p_node_tmp);
	
		trm_node_remove_child(p_tree,p_node,p_node_tmp);
		
		trm_node_uninit(p_node_tmp);

		trm_node_free(p_node_tmp);
		
		p_node_tmp = trm_node_get_first_child(p_node);
	}
	
	return SUCCESS;
}


_int32 trm_node_remove_child(TREE *p_tree,TREE_NODE *p_node,TREE_NODE *p_child)
{
	
      EM_LOG_DEBUG("trm_node_delete_child:node_id=%u,child_id=%u",p_node->_node_id,p_child->_node_id); 
	  sd_assert(trm_node_get_parent(p_child) == p_node);
	  
	  if(p_child==trm_node_get_first_child(p_node))
	  {
	  	trm_node_set_first_child(p_tree,p_node,trm_node_get_nxt(p_child));
	  }

	  if(p_child==trm_node_get_last_child(p_node))
	  {
	  	trm_node_set_last_child(p_tree,p_node,trm_node_get_pre(p_child));
	  }

	  if(NULL!=trm_node_get_pre(p_child))
	  {
	  	trm_node_set_nxt(p_tree,trm_node_get_pre(p_child),trm_node_get_nxt(p_child));
	  }

	  if(NULL!=trm_node_get_nxt(p_child))
	  {
	  	trm_node_set_pre(p_tree,trm_node_get_nxt(p_child),trm_node_get_pre(p_child));
	  }

	trm_node_set_parent(p_tree,p_child,NULL);
	trm_node_set_pre(p_tree,p_child,NULL);
	trm_node_set_nxt(p_tree,p_child,NULL);

	  
	  p_node->_children_num--;
	trm_set_node_change(p_tree,p_node,TRN_CHANGE_CHILD_NUM);
	  
	return SUCCESS;
}

TREE_NODE * trm_find_first_child_by_name_hash(TREE *p_tree,TREE_NODE *p_parent,_u64 hash)
{
	TREE_NODE *p_node=NULL;
	#ifdef FIND_NODE_RECURSIVE
	TREE_NODE *p_node_tmp=NULL;
	#endif /* FIND_NODE_RECURSIVE */
	
	p_node = trm_node_get_first_child(p_parent);
	while(p_node!=NULL)
	{
		if(p_node->_name_hash== hash)
		{
			return p_node;
		}
		#ifdef FIND_NODE_RECURSIVE
		else
		{
			/* 进入下一层进行查找 */
			p_node_tmp = trm_find_first_child_by_name_hash(p_tree,p_node, hash);
			if(p_node_tmp!=NULL)
			{
				return p_node_tmp;
			}
		}
		#endif /* FIND_NODE_RECURSIVE */
		p_node=trm_node_get_nxt(p_node);
	}
	
	return p_node;
}
TREE_NODE * trm_find_last_child_by_name_hash(TREE *p_tree,TREE_NODE *p_parent,_u64 hash)
{
	TREE_NODE *p_node=NULL;
	#ifdef FIND_NODE_RECURSIVE
	TREE_NODE *p_node_tmp=NULL;
	#endif /* FIND_NODE_RECURSIVE */
	
	p_node = trm_node_get_last_child(p_parent);
	while(p_node!=NULL)
	{
		if(p_node->_name_hash== hash)
		{
			return p_node;
		}
		#ifdef FIND_NODE_RECURSIVE
		else
		{
			/* 进入下一层进行查找 */
			p_node_tmp = trm_find_last_child_by_name_hash(p_tree,p_node, hash);
			if(p_node_tmp!=NULL)
			{
				return p_node_tmp;
			}
		}
		#endif /* FIND_NODE_RECURSIVE */
		p_node=trm_node_get_pre(p_node);
	}
	
	return p_node;
}

TREE_NODE * trm_find_next_node_by_name_hash(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,_u64 hash)
{
	TREE_NODE *p_node_curent=p_node,*p_node_tmp=NULL,*p_node_des=NULL;

	p_node_tmp = p_node_curent;
	#ifdef FIND_NODE_RECURSIVE 
	/* 遍历所有子节点 */
	p_node_des = trm_find_first_child_by_name_hash(p_tree,p_node_tmp, hash);
	if(p_node_des != NULL) return p_node_des;
	#endif /* FIND_NODE_RECURSIVE */
	/* 同一层下一个 */
	p_node_tmp = trm_node_get_nxt(p_node_tmp);
	
	while(p_node_curent != p_parent)
	{
		while(p_node_tmp)
		{
			if(p_node_tmp->_name_hash== hash)
			{
				return p_node_tmp;
			}
			#ifdef FIND_NODE_RECURSIVE 
			/* 遍历所有子节点 */
			p_node_des = trm_find_first_child_by_name_hash(p_tree,p_node_tmp, hash);
			if(p_node_des != NULL) return p_node_des;
			#endif /* FIND_NODE_RECURSIVE */
			
			/* 同一层下一个 */
			p_node_tmp = trm_node_get_nxt(p_node_tmp);
		}
		
		/* 同一层已经查找完毕,向上一层查找 */
		p_node_curent = trm_node_get_parent(p_node_curent);
		#ifdef FIND_NODE_RECURSIVE 
		/* 父层下一个 */
		p_node_tmp = trm_node_get_nxt(p_node_curent);
		#else
		sd_assert(p_node_curent == p_parent);
		if(p_node_curent != p_parent)
		{
			break;
		}
		#endif /* FIND_NODE_RECURSIVE */
	} 
	
	return p_node_des;
}
TREE_NODE * trm_find_prev_node_by_name_hash(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,_u64 hash)
{
	TREE_NODE *p_node_curent=p_node,*p_node_tmp=NULL,*p_node_des=NULL;

	p_node_tmp = p_node_curent;
	/* 同一层上一个 */
	p_node_tmp = trm_node_get_pre(p_node_tmp);
	
	while(p_node_curent != p_parent)
	{
		while(p_node_tmp)
		{
			if(p_node_tmp->_name_hash== hash)
			{
				return p_node_tmp;
			}
			#ifdef FIND_NODE_RECURSIVE 
			/* 遍历所有子节点 */
			p_node_des = trm_find_last_child_by_name_hash(p_tree,p_node_tmp, hash);
			if(p_node_des != NULL) return p_node_des;
			#endif /* FIND_NODE_RECURSIVE */
			
			/* 同一层上一个 */
			p_node_tmp = trm_node_get_pre(p_node_tmp);
		}
		
		/* 同一层已经查找完毕,向上一层查找 */
		p_node_curent = trm_node_get_parent(p_node_curent);
		#ifdef FIND_NODE_RECURSIVE 
		/* 父层上一个 */
		p_node_tmp = trm_node_get_pre(p_node_curent);
		#else
		sd_assert(p_node_curent == p_parent);
		if(p_node_curent != p_parent)
		{
			break;
		}
		#endif /* FIND_NODE_RECURSIVE */
	} 
	
	return p_node_des;
}

/////////////////////////////////////////////////////////
TREE_NODE * trm_find_first_child_by_data_hash(TREE *p_tree,TREE_NODE *p_parent,_u64 hash)
{
	TREE_NODE *p_node=NULL;
	#ifdef FIND_NODE_RECURSIVE
	TREE_NODE *p_node_tmp=NULL;
	#endif /* FIND_NODE_RECURSIVE */
	
	p_node = trm_node_get_first_child(p_parent);
	while(p_node!=NULL)
	{
		if(p_node->_data_hash== hash)
		{
			return p_node;
		}
		#ifdef FIND_NODE_RECURSIVE 
		else
		{
			p_node_tmp = trm_find_first_child_by_data_hash(p_tree,p_node, hash);
			if(p_node_tmp!=NULL)
			{
				return p_node_tmp;
			}
		}
		#endif /* FIND_NODE_RECURSIVE */
		p_node=trm_node_get_nxt(p_node);
	}
	
	return p_node;
}
TREE_NODE * trm_find_last_child_by_data_hash(TREE *p_tree,TREE_NODE *p_parent,_u64 hash)
{
	TREE_NODE *p_node=NULL;
	#ifdef FIND_NODE_RECURSIVE
	TREE_NODE *p_node_tmp=NULL;
	#endif /* FIND_NODE_RECURSIVE */
	
	p_node = trm_node_get_last_child(p_parent);
	while(p_node!=NULL)
	{
		if(p_node->_data_hash== hash)
		{
			return p_node;
		}
		#ifdef FIND_NODE_RECURSIVE 
		else
		{
			p_node_tmp = trm_find_last_child_by_data_hash(p_tree,p_node, hash);
			if(p_node_tmp!=NULL)
			{
				return p_node_tmp;
			}
		}
		#endif /* FIND_NODE_RECURSIVE */
		p_node=trm_node_get_pre(p_node);
	}
	
	return p_node;
}

TREE_NODE * trm_find_next_node_by_data_hash(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,_u64 hash)
{
	TREE_NODE *p_node_curent=p_node,*p_node_tmp=NULL,*p_node_des=NULL;

	p_node_tmp = p_node_curent;
	#ifdef FIND_NODE_RECURSIVE 
	/* 遍历所有子节点 */
	p_node_des = trm_find_first_child_by_data_hash(p_tree,p_node_tmp, hash);
	if(p_node_des != NULL) return p_node_des;
	#endif /* FIND_NODE_RECURSIVE */
	
	/* 同一层下一个 */
	p_node_tmp = trm_node_get_nxt(p_node_tmp);
	
	while(p_node_curent != p_parent)
	{
		while(p_node_tmp)
		{
			if(p_node_tmp->_data_hash== hash)
			{
				return p_node_tmp;
			}
			#ifdef FIND_NODE_RECURSIVE 
			/* 遍历所有子节点 */
			p_node_des = trm_find_first_child_by_data_hash(p_tree,p_node_tmp, hash);
			if(p_node_des != NULL) return p_node_des;
			#endif /* FIND_NODE_RECURSIVE */
			
			/* 同一层下一个 */
			p_node_tmp = trm_node_get_nxt(p_node_tmp);
		}
		
		/* 同一层已经查找完毕,向上一层查找 */
		p_node_curent = trm_node_get_parent(p_node_curent);
		#ifdef FIND_NODE_RECURSIVE 
		/* 父层下一个 */
		p_node_tmp = trm_node_get_nxt(p_node_curent);
		#else
		sd_assert(p_node_curent == p_parent);
		if(p_node_curent != p_parent)
		{
			break;
		}
		#endif /* FIND_NODE_RECURSIVE */
	} 
	
	return p_node_des;
}
TREE_NODE * trm_find_prev_node_by_data_hash(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,_u64 hash)
{
	TREE_NODE *p_node_curent=p_node,*p_node_tmp=NULL,*p_node_des=NULL;

	p_node_tmp = p_node_curent;
	/* 同一层上一个 */
	p_node_tmp = trm_node_get_pre(p_node_tmp);
	
	while(p_node_curent != p_parent)
	{
		while(p_node_tmp)
		{
			if(p_node_tmp->_data_hash== hash)
			{
				return p_node_tmp;
			}
			#ifdef FIND_NODE_RECURSIVE 
			/* 遍历所有子节点 */
			p_node_des = trm_find_last_child_by_data_hash(p_tree,p_node_tmp, hash);
			if(p_node_des != NULL) return p_node_des;
			#endif /* FIND_NODE_RECURSIVE */
			
			/* 同一层上一个 */
			p_node_tmp = trm_node_get_pre(p_node_tmp);
		}
		
		/* 同一层已经查找完毕,向上一层查找 */
		p_node_curent = trm_node_get_parent(p_node_curent);
		#ifdef FIND_NODE_RECURSIVE 
		/* 父层上一个 */
		p_node_tmp = trm_node_get_pre(p_node_curent);
		#else
		sd_assert(p_node_curent == p_parent);
		if(p_node_curent != p_parent)
		{
			break;
		}
		#endif /* FIND_NODE_RECURSIVE */
	} 
	
	return p_node_des;
}
char * trm_correct_input_name_string(const char * name,_u32 name_len)
{
	//_int32 ret_val = SUCCESS;
	static char in_str[MAX_FILE_NAME_BUFFER_LEN];
	_u32 in_str_len = 0;
	char * p_pos = NULL;
	
	if((name==NULL)||(name_len==0)||(em_strlen(name)==0)) return NULL;

	em_memset(in_str,0x00,MAX_FILE_NAME_BUFFER_LEN);
	em_strncpy(in_str,name,MAX_FILE_NAME_BUFFER_LEN-1);
	if(name_len<MAX_FILE_NAME_BUFFER_LEN)
	{
		in_str[name_len] = '\0';
		in_str_len = em_strlen(in_str);
	}

	p_pos = em_strchr(in_str,'*',0);
	if(p_pos!=NULL)
	{
		if(p_pos == in_str)
		{
			em_memmove(in_str,in_str+1,in_str_len);
		}
		else
		{
			p_pos[0] = '\0';
		}
	}
	in_str_len = em_strlen(in_str);
	if(in_str_len ==0) return NULL;

	return in_str;
}

TREE_NODE * trm_find_first_child_by_name_impl(TREE *p_tree,TREE_NODE *p_parent,const char * name,char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN])
{
	_int32 ret_val = SUCCESS;
	TREE_NODE *p_node=NULL;
	#ifdef FIND_NODE_RECURSIVE
	TREE_NODE *p_node_tmp=NULL;
	#endif /* FIND_NODE_RECURSIVE */
	
	p_node = trm_node_get_first_child(p_parent);
	while(p_node!=NULL)
	{
		ret_val = trm_get_node_name(p_tree,p_node,tmp_buffer);
		#ifndef FIND_NODE_RECURSIVE
		if(ret_val==SUCCESS&&p_node->_name_len==em_strlen(name))
		{
			if(sd_strcmp(tmp_buffer,name)==0)
			{
				return p_node;
			}
		}
		#else
		if(ret_val==SUCCESS&&p_node->_name_len>=em_strlen(name))
		{
			if(sd_stristr(tmp_buffer,name,0)!=NULL)
			{
				return p_node;
			}
			else
			{
				p_node_tmp = trm_find_first_child_by_name_impl(p_tree,p_node, name,tmp_buffer);
				if(p_node_tmp!=NULL)
				{
					return p_node_tmp;
				}
			}
		}
		#endif /* FIND_NODE_RECURSIVE */
		p_node=trm_node_get_nxt(p_node);
	}

	return p_node;
}
TREE_NODE * trm_find_last_child_by_name_impl(TREE *p_tree,TREE_NODE *p_parent,const char * name,char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN])
{
	_int32 ret_val = SUCCESS;
	TREE_NODE *p_node=NULL;
	#ifdef FIND_NODE_RECURSIVE
	TREE_NODE *p_node_tmp=NULL;
	#endif /* FIND_NODE_RECURSIVE */
	
	p_node = trm_node_get_last_child(p_parent);
	while(p_node!=NULL)
	{
		ret_val = trm_get_node_name(p_tree,p_node,tmp_buffer);
		#ifndef FIND_NODE_RECURSIVE
		if(ret_val==SUCCESS&&p_node->_name_len==em_strlen(name))
		{
			if(sd_strcmp(tmp_buffer,name)==0)
			{
				return p_node;
			}
		}
		#else
		if(ret_val==SUCCESS&&p_node->_name_len>=em_strlen(name))
		{
			if(sd_stristr(tmp_buffer,name,0)!=NULL)
			{
				return p_node;
			}
		}
		else
		{
			p_node_tmp = trm_find_last_child_by_name_impl(p_tree,p_node, name,tmp_buffer);
			if(p_node_tmp!=NULL)
			{
				return p_node_tmp;
			}
		}
		#endif /* FIND_NODE_RECURSIVE */
		p_node=trm_node_get_pre(p_node);
	}
	
	return p_node;
}

TREE_NODE * trm_find_next_node_by_name_impl(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,const char * name,char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN])
{
	_int32 ret_val = SUCCESS;
	TREE_NODE *p_node_curent=p_node,*p_node_tmp=NULL,*p_node_des=NULL;

	p_node_tmp = p_node_curent;
	#ifdef FIND_NODE_RECURSIVE
	/* 遍历所有子节点 */
	p_node_des = trm_find_first_child_by_name_impl(p_tree,p_node_tmp, name,tmp_buffer);
	if(p_node_des != NULL) return p_node_des;
	#endif /* FIND_NODE_RECURSIVE */
	
	/* 同一层下一个 */
	p_node_tmp = trm_node_get_nxt(p_node_tmp);
	
	while(p_node_curent != p_parent)
	{
		while(p_node_tmp)
		{
			ret_val = trm_get_node_name(p_tree,p_node_tmp,tmp_buffer);
			#ifdef FIND_NODE_RECURSIVE
			if(ret_val==SUCCESS&&p_node_tmp->_name_len>=em_strlen(name))
			{
				if(sd_stristr(tmp_buffer,name,0)!=NULL)
				{
					return p_node_tmp;
				}
			}

			/* 遍历所有子节点 */
			p_node_des = trm_find_first_child_by_name_impl(p_tree,p_node_tmp,  name,tmp_buffer);
			if(p_node_des != NULL) return p_node_des;
			#else
			if(ret_val==SUCCESS&&p_node_tmp->_name_len==em_strlen(name))
			{
				if(sd_strcmp(tmp_buffer,name)==0)
				{
					return p_node_tmp;
				}
			}
			#endif /* FIND_NODE_RECURSIVE */
			/* 同一层下一个 */
			p_node_tmp = trm_node_get_nxt(p_node_tmp);
		}
		
		/* 同一层已经查找完毕,向上一层查找 */
		p_node_curent = trm_node_get_parent(p_node_curent);
		#ifdef FIND_NODE_RECURSIVE
		/* 父层下一个 */
		p_node_tmp = trm_node_get_nxt(p_node_curent);
		#else
		sd_assert(p_node_curent == p_parent);
		if(p_node_curent != p_parent)
		{
			break;
		}
		#endif /* FIND_NODE_RECURSIVE */
	} 
	
	return p_node_des;
}
TREE_NODE * trm_find_prev_node_by_name_impl(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,const char * name,char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN])
{
	_int32 ret_val = SUCCESS;
	TREE_NODE *p_node_curent=p_node,*p_node_tmp=NULL,*p_node_des=NULL;

	p_node_tmp = p_node_curent;
	/* 同一层上一个 */
	p_node_tmp = trm_node_get_pre(p_node_tmp);
	
	while(p_node_curent != p_parent)
	{
		while(p_node_tmp)
		{
			ret_val = trm_get_node_name(p_tree,p_node_tmp,tmp_buffer);
			#ifdef FIND_NODE_RECURSIVE
			if(ret_val==SUCCESS&&p_node_tmp->_name_len>=em_strlen(name))
			{
				if(sd_stristr(tmp_buffer,name,0)!=NULL)
				{
					return p_node_tmp;
				}
			}

			/* 遍历所有子节点 */
			p_node_des = trm_find_last_child_by_name_impl(p_tree,p_node_tmp,  name,tmp_buffer);
			if(p_node_des != NULL) return p_node_des;
			#else
			if(ret_val==SUCCESS&&p_node_tmp->_name_len==em_strlen(name))
			{
				if(sd_strcmp(tmp_buffer,name)==0)
				{
					return p_node_tmp;
				}
			}
			#endif /* FIND_NODE_RECURSIVE */
			/* 同一层上一个 */
			p_node_tmp = trm_node_get_pre(p_node_tmp);
		}
		
		/* 同一层已经查找完毕,向上一层查找 */
		p_node_curent = trm_node_get_parent(p_node_curent);
		#ifdef FIND_NODE_RECURSIVE
		/* 父层上一个 */
		p_node_tmp = trm_node_get_pre(p_node_curent);
		#else
		sd_assert(p_node_curent == p_parent);
		if(p_node_curent != p_parent)
		{
			break;
		}
		#endif /* FIND_NODE_RECURSIVE */
	} 
	
	return p_node_des;
}

TREE_NODE * trm_find_first_child_by_name(TREE *p_tree,TREE_NODE *p_parent,const char * name,_u32 name_len)
{
	TREE_NODE *p_node=NULL;
	char * in_str = NULL;
	char node_name[MAX_FILE_NAME_BUFFER_LEN];
	
	in_str = trm_correct_input_name_string(name, name_len);

	if(in_str==NULL) return NULL;

	p_node = trm_find_first_child_by_name_impl(p_tree,p_parent, in_str,node_name);
	
	return p_node;
}

TREE_NODE * trm_find_next_node_by_name(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,const char * name,_u32 name_len)
{
	char * in_str = NULL;
	TREE_NODE *p_node_tmp=NULL;
	char node_name[MAX_FILE_NAME_BUFFER_LEN];
	
	in_str = trm_correct_input_name_string(name, name_len);

	if(in_str==NULL) return NULL;

	p_node_tmp = trm_find_next_node_by_name_impl(p_tree,p_parent,p_node, in_str,node_name);
	
	return p_node_tmp;
}
TREE_NODE * trm_find_prev_node_by_name(TREE *p_tree,TREE_NODE *p_parent,TREE_NODE *p_node,const char * name,_u32 name_len)
{
	char * in_str = NULL;
	TREE_NODE *p_node_tmp=NULL;
	char node_name[MAX_FILE_NAME_BUFFER_LEN];
	
	in_str = trm_correct_input_name_string(name, name_len);

	if(in_str==NULL) return NULL;

	p_node_tmp = trm_find_prev_node_by_name_impl(p_tree,p_parent, p_node,in_str,node_name);
	
	return p_node_tmp;
}


/////////////////////////////////////////////////////////
_int32 trm_node_name_malloc(char **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_node_name_slab, (void**)pp_slip );
	
      EM_LOG_DEBUG("trm_node_name_malloc:%d,0x%X",ret_val,*pp_slip); 
	  if(ret_val==SUCCESS)
	  {
	  	em_memset(*pp_slip,0,MAX_FILE_NAME_BUFFER_LEN);
	  }
	return ret_val;
}

_int32 trm_node_name_free(char * p_slip)
{
	//sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
      EM_LOG_DEBUG("trm_node_name_free,0x%X",p_slip); 
	return em_mpool_free_slip( gp_node_name_slab, (void*)p_slip );
}


_int32 trm_get_node_name(TREE *p_tree,TREE_NODE *p_node,char * name_buffer)
{
	_int32 ret_val = SUCCESS;

	if(p_node->_is_root)
	{
		/*  注意:由于char  _tree_file[MAX_FULL_PATH_BUFFER_LEN]的最大长度大于MAX_FILE_NAME_BUFFER_LEN，因此可能会导致数据丢失! */
		em_strncpy(name_buffer, p_tree->_tree_file, MAX_FILE_NAME_BUFFER_LEN);
		return SUCCESS;
	}
	
	
	if(p_node->_name_len!=0)
	{
		if(p_node->_name==NULL)
		{
			char *trm_name = trm_get_name_from_file(p_tree,p_node);
			if (trm_name) 
			{
				em_strncpy(name_buffer, trm_name, MAX_FILE_NAME_BUFFER_LEN);
			}
			else 
			{
				ret_val = INVALID_NODE_NAME;
			}

		}
		else
		{
			em_strncpy(name_buffer, p_node->_name, MAX_FILE_NAME_BUFFER_LEN);
		}
	}
	else
	{
		ret_val = INVALID_NODE_NAME;
	}

	return ret_val;
}
	


