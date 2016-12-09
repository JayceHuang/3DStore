#if !defined(__TREE_INTERFACE_H_20090915)
#define __TREE_INTERFACE_H_20090915
#include "em_common/em_define.h"
#include "em_common/em_errcode.h"

#include "em_common/em_list.h"
#include "em_common/em_map.h"
#include "settings/em_settings.h"
#include "em_common/em_mempool.h"

#define TRM_INVALID_NODE_ID	0
#define TRM_ROOT_NODE_ID	1

#define TRN_CHANGE_PARENT 					(0x00000001)
#define TRN_CHANGE_PRE_NODE 				(0x00000002)
#define TRN_CHANGE_NXT_NODE 				(0x00000004)
#define TRN_CHANGE_FIRST_CHILD 			(0x00000008)
#define TRN_CHANGE_LAST_CHILD 			(0x00000010)
#define TRN_CHANGE_CHILD_NUM 				(0x00000020)
#define TRN_CHANGE_DATA					(0x00000040)
#define TRN_CHANGE_NAME 					(0x00000080)
#define TRN_CHANGE_ITEMS 					(0x00000100)
#define TRN_CHANGE_TOTAL_LEN 				(0x00001000)

typedef enum t_trm_sort_mode
{ 
	TSM_NONE = 0, 				/*  无排序*/
	TSM_NAME = 1,				/*  子节点用名字 排序 */
	TSM_SIZE = 2,				/*  子节点用数据长度 排序 */
	TSM_CREATE = 3,				/*  子节点用创建时间 排序 */
	TSM_MODIFY = 4,	 		/*  子节点用修改时间 排序 */
	TSM_OTHER = 5	 			/*  子节点用其他方式 排序【以后扩展排序方法用】 */
}TRM_SORT_MODE ;

/* Structures for tree */
typedef struct t_tree_node_old{
	_u32 _node_id;		//树内唯一标识，不同于task_id
	_u32 _change_flag;
	struct t_tree_node *_parent;
	struct t_tree_node *_pre_node;
	struct t_tree_node *_nxt_node;
	struct t_tree_node *_first_child;
	struct t_tree_node *_last_child;
	_u32 _children_num;
	_u32 _data_len;
	void *_data;  
	_u32 _name_len;
	char * _name;
	_u32 _offset;		 //该节点存储在tree_store.dat中的偏移,用于读取和改写该节点信息(sd_setfilepos)
}TREE_NODE_OLD;
/* Structures for tree */
typedef struct t_tree_node{
	_u32 _node_id;		//树内唯一标识，不同于task_id
	_u32 _change_flag;
	struct t_tree_node *_parent;
	struct t_tree_node *_pre_node;
	struct t_tree_node *_nxt_node;
	struct t_tree_node *_first_child;
	struct t_tree_node *_last_child;
	_u32 _children_num;
	_u32 _data_len;
	void *_data;  
	_u32 _name_len;
	char * _name;
	_u32 _offset;		 //该节点存储在tree_store.dat中的偏移,用于读取和改写该节点信息(sd_setfilepos)

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
	_u8 *_items;  	/* new */  /* 后续额外字段，为以后扩展用 */

	BOOL _full_info;
	void *_tree; /* new */  
}TREE_NODE;

typedef struct t_tree{
	TREE_NODE _root; 
	_int32 _open_count;		/* 树被同时打开的次数,不包括第一次打开 */
	/////////////////
	_int32 _flag;
	MAP _all_nodes;				//所有节点与节点id的对应关系,用于快速通过node_id找到对应的TREE_NODE  (key=_node_id -> data=ETM_TREE_NODE)
	_u32 _file_id;
	_u32 _total_node_num;		//不同于_tree_node_count，这个主要用来累加生成新的node_id
	//MAP _all_name_hash_lists;				//所有节点与节点id的对应关系,用于快速通过node_id找到对应的TREE_NODE  (key=_node_id -> data=ETM_TREE_NODE)
	//MAP _all_data_hash_lists;				//所有节点与节点id的对应关系,用于快速通过node_id找到对应的TREE_NODE  (key=_node_id -> data=ETM_TREE_NODE)
	char  _tree_file[MAX_FULL_PATH_BUFFER_LEN];				//存储所有树节点信息的文件全名:tree_store.dat
	_u32 _time_stamp ;
	_u32 _pos_file_end /*=TRM_POS_NODE_START*/;
	BOOL _need_clear;
	BOOL _file_locked;
	_int32 _file_thread_id ;
	BOOL _have_changed_node;
	_u32 _check_count; 
	_u32 _backup_count; 
	_u32 _invalid_node_count;
} TREE;

typedef struct t_tree_manager{
	//TREE _tree;				//用于记录界面程序数据的树(ETM_TREE_NODE)
	//MAP _all_nodes;				//所有节点与节点id的对应关系,用于快速通过node_id找到对应的TREE_NODE  (key=_node_id -> data=ETM_TREE_NODE)
	//_u32 _total_node_num;		//不同于_tree_node_count，这个主要用来累加生成新的node_id
	//////////////////
	MAP _all_trees;				//所有TREE与TREE id的对应关系,用于快速通过tree_id找到对应的TREE  (key=tree_id -> data=TREE)
	_u32 _total_tree_num;		//不同于_tree_node_count，这个主要用来累加生成新的tree_id
} TREE_MANAGER;

/*--------------------------------------------------------------------------*/
/*                Interfaces provided by tree_manager			        */
/*--------------------------------------------------------------------------*/
_int32 init_tree_manager_module(void);
_int32 uninit_tree_manager_module(void);
void trm_scheduler(void);
_int32 trm_clear(void);
_int32 trm_load_default_tree( void );
_int32 trm_open_tree( void * p_param );
_int32 trm_close_tree( void * p_param );
_int32 trm_destroy_tree( void * p_param );
_int32 trm_tree_exist( void * p_param );
_int32 trm_save_tree( void * p_param );
_int32 trm_create_node( void * p_param );
_int32 trm_delete_node( void * p_param );
_int32 trm_set_parent( void * p_param );
_int32 trm_move_up( void * p_param );
_int32 trm_move_down( void * p_param );
_int32 trm_get_parent( void * p_param );
_int32 trm_get_children( void * p_param );
_int32 trm_get_data( void * p_param );
_int32 trm_set_data( void * p_param );
_int32 trm_get_name( void * p_param );
_int32 trm_set_name( void * p_param );
_int32 trm_find_node_id_by_data( void * p_param );
_int32 trm_get_first_child( void * p_param );
_int32 trm_get_next_brother( void * p_param );
_int32 trm_get_prev_brother( void * p_param );

_int32 trm_find_first_node( void * p_param );
_int32 trm_find_next_node( void * p_param );
_int32 trm_find_prev_node( void * p_param );

#endif /* __TREE_INTERFACE_H_20090915 */
