#if !defined(__TREE_STORE_H_20090915)
#define __TREE_STORE_H_20090915


#include "tree_manager/tree_interface.h"
#include "em_common/fm_common.h"

/* 以下为虚拟结构体，只在存储到文件中才体现 ,因此切不可定义宏STRUCT_FOR_STORING */
#ifdef STRUCT_FOR_STORING

/* Structures of tree for  storing */
typedef struct t_tree_store_file{
	EM_STORE_HEAD _head;
	_u32 _total_node_num;		//这个主要用来累加生成新的node_id
	_u8 _reserved[DEFAULT_FILE_RESERVED_SIZE];  /*(64)bytes保留字段为以后扩展用 */
	TREE_NODE_STORE _nodes;  //...
}TREE_STORE_FILE;

/// file format:crc ver len count FILE_RESERVED ETM_TREE_NODE_STORE ETM_TREE_NODE_STORE ETM_TREE_NODE_STORE....

#endif /* 0 */

/* Structures of tree node for  storing */
typedef struct t_tree_node_store_old{
	_u16 _valid; 	//该节点信息是否有效，如果该节点被删除，或数据长度被修改(修改的执行是把旧的信息置为_valid=0，然后在文件末尾重写该节点更新后的信息)
	_u16 _crc;		//从u32 _node_id开始到void *_data最后一byte的检验值
	_u32 _len;	//从u32 _node_id开始到void *_data最后一byte的长度
	_u32 _node_id;		//树内唯一标识，不同于task_id
	_u32 _parent_id;
	_u32 _pre_node_id;
	_u32 _nxt_node_id;
	_u32 _first_child_id;
	_u32 _last_child_id;
	_u32 _children_num;
	_u32 _data_len;
	_u32 _name_len;
	//void *_data;  
	//char *_name;  
}TREE_NODE_STORE_OLD;
/* Structures of tree node head for  storing */
typedef struct t_tree_node_store_head{
	_u16 _valid; 
	_u16 _crc;		
	_u32 _len;	//从TREE_NODE_INFO_STORE u32 _node_id开始到items最后一byte的长度
}TREE_NODE_STORE_HEAD;
/* Structures of tree node info for  storing */
typedef struct t_tree_node_info_store{
	_u32 _node_id;		//树内唯一标识，不同于task_id
	_u32 _parent_id;
	_u32 _pre_node_id;
	_u32 _nxt_node_id;
	_u32 _first_child_id;
	_u32 _last_child_id;
	_u32 _children_num;
	_u32 _data_len;
	_u32 _name_len;
///////////////////////////
	BOOL _is_root;
	_u64 _data_hash;  /* new */     /* 节点数据的hash值，用于查找*/
	_u64 _name_hash;  /* new */     /* 节点名称的hash值，用于查找*/
	_u32 _create_time; /* new */ /* 节点被创建时的时间,1970.1.1至今的秒数 */
	_u32 _modify_time; /* new */   /* 节点被最近一次修改时的时间,1970.1.1至今的秒数 */
	TRM_SORT_MODE _children_sort_mode; /* new */  /* 该节点下面的子节点的排序方式*/
	_u64 _reserved;  /* new */  /* 保留字段，为以后扩展用 */
	_u32 _items_num; /* new */  /* 后续额外字段的个数，为以后扩展用 */		//_name后面存放的item个数
	_u32 _items_len;	/* new */  /* 后续额外字段的总长度，为以后扩展用 */	//_name后面存放的item的长度
///////////////////////////
	//void *_data;  
	//char *_name;  
	// items .....
}TREE_NODE_INFO_STORE;

/* Structures of tree node for  storing */
typedef struct t_tree_node_store{
	TREE_NODE_STORE_HEAD _head; 
	TREE_NODE_INFO_STORE _info;
	//void *_data;  
	//char *_name;  
	// items .....
}TREE_NODE_STORE;

#define TREE_STORE_FILE_VERSION 			(2)
#define MAX_INVALID_NODE_NUM 			(50)

#define TREE_STORE_FILE 			"etm_tree_store.dat"

#define TRM_POS_TOTAL_NODE_NUM 		(sizeof(EM_STORE_HEAD))
#define TRM_POS_NODE_START_OLD 			(TRM_POS_TOTAL_NODE_NUM+sizeof(_u32))
#define TRM_POS_FILE_RESERVED 			(TRM_POS_TOTAL_NODE_NUM+sizeof(_u32))  /* DEFAULT_FILE_RESERVED_SIZE (64)bytes保留字段为以后扩展用 */
#define TRM_POS_NODE_START 			(TRM_POS_FILE_RESERVED+DEFAULT_FILE_RESERVED_SIZE)

#define TRM_POS_VALID 					(p_node->_offset)
//////////////
#define TRM_POS_DATA_OLD 					(p_node->_offset+sizeof(TREE_NODE_STORE_OLD))
#define TRM_POS_NAME_OLD 					(TRM_POS_DATA_OLD+p_node->_data_len)
//////////////
#define TRM_POS_DATA 					(p_node->_offset+sizeof(TREE_NODE_STORE))
#define TRM_POS_NAME 					(TRM_POS_DATA+p_node->_data_len)
#define TRM_POS_ITEM 					(TRM_POS_NAME+p_node->_name_len)

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/
char * trm_get_tree_file_path(TREE * p_tree);
_int32 trm_close_file(TREE * p_tree,BOOL force_close);
_int32 trm_create_tree_file(TREE * p_tree);
BOOL trm_is_tree_file_need_clear_up(TREE * p_tree);
_int32 trm_clear_file_impl(void* param);
_int32 trm_backup_file(TREE * p_tree);
BOOL trm_recover_file(TREE * p_tree);
_int32 trm_stop_clear_file(TREE * p_tree);
_int32 trm_get_total_node_num(TREE * p_tree,_u32 * total_node_num);
_int32 trm_save_total_node_num(TREE * p_tree,_u32  total_node_num);
_int32 trm_get_node_crc_value(TREE_NODE* p_node,_u16 * p_crc_value,_u32 * p_data_len);
_int32 trm_add_node_to_file(TREE * p_tree,TREE_NODE* p_node);
_int32 trm_disable_node_in_file(TREE * p_tree,TREE_NODE* p_node);
TREE_NODE *  trm_get_node_from_file(TREE * p_tree,_int32 * err_code);
_int32 trm_copy_node_to_info_store(TREE_NODE * p_node,TREE_NODE_INFO_STORE* p_tree_node_info_store);
_int32 trm_copy_info_store_to_node(TREE_NODE_INFO_STORE* p_tree_node_info_store,TREE_NODE * p_node);
_int32 trm_save_len_changed_node_to_file(TREE * p_tree,TREE_NODE * p_node);
_int32 trm_save_node_to_file(TREE * p_tree,TREE_NODE * p_node);
char * trm_get_name_from_file(TREE * p_tree,TREE_NODE * p_node);
_int32  trm_get_data_from_file(TREE * p_tree,TREE_NODE * p_node,void* data_buffer,_u32 * buffer_len);
_int32  trm_get_items_from_file(TREE * p_tree,TREE_NODE * p_node,void* item_buffer,_u32 * buffer_len);
_int32  trm_check_node_file(TREE * p_tree,_u32 node_num);
_int32  trm_convert_old_ver_to_current(TREE * p_tree);

#endif /* __TREE_STORE_H_20090915 */
