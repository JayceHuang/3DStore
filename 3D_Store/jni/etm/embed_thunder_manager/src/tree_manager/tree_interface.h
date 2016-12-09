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
	TSM_NONE = 0, 				/*  ������*/
	TSM_NAME = 1,				/*  �ӽڵ������� ���� */
	TSM_SIZE = 2,				/*  �ӽڵ������ݳ��� ���� */
	TSM_CREATE = 3,				/*  �ӽڵ��ô���ʱ�� ���� */
	TSM_MODIFY = 4,	 		/*  �ӽڵ����޸�ʱ�� ���� */
	TSM_OTHER = 5	 			/*  �ӽڵ���������ʽ �����Ժ���չ���򷽷��á� */
}TRM_SORT_MODE ;

/* Structures for tree */
typedef struct t_tree_node_old{
	_u32 _node_id;		//����Ψһ��ʶ����ͬ��task_id
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
	_u32 _offset;		 //�ýڵ�洢��tree_store.dat�е�ƫ��,���ڶ�ȡ�͸�д�ýڵ���Ϣ(sd_setfilepos)
}TREE_NODE_OLD;
/* Structures for tree */
typedef struct t_tree_node{
	_u32 _node_id;		//����Ψһ��ʶ����ͬ��task_id
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
	_u32 _offset;		 //�ýڵ�洢��tree_store.dat�е�ƫ��,���ڶ�ȡ�͸�д�ýڵ���Ϣ(sd_setfilepos)

///////////////////////////
	BOOL _is_root;
	_u64 _data_hash;  /* new */     /* �ڵ����ݵ�hashֵ�����ڲ���*/
	_u64 _name_hash;  /* new */     /* �ڵ����Ƶ�hashֵ�����ڲ���*/
	_u32 _create_time; /* new */ /* �ڵ㱻����ʱ��ʱ��,1970.1.1��������� */
	_u32 _modify_time; /* new */   /* �ڵ㱻���һ���޸�ʱ��ʱ��,1970.1.1��������� */
	TRM_SORT_MODE _children_sort_mode; /* new */  /* �ýڵ�������ӽڵ������ʽ*/
	_u64 _reserved;  /* new */  /* �����ֶΣ�Ϊ�Ժ���չ�� */
	_u32 _items_num; /* new */  /* ���������ֶεĸ�����Ϊ�Ժ���չ�� */		//_name�����ŵ�item����
	_u32 _items_len;	/* new */  /* ���������ֶε��ܳ��ȣ�Ϊ�Ժ���չ�� */	//_name�����ŵ�item�ĳ���
	_u8 *_items;  	/* new */  /* ���������ֶΣ�Ϊ�Ժ���չ�� */

	BOOL _full_info;
	void *_tree; /* new */  
}TREE_NODE;

typedef struct t_tree{
	TREE_NODE _root; 
	_int32 _open_count;		/* ����ͬʱ�򿪵Ĵ���,��������һ�δ� */
	/////////////////
	_int32 _flag;
	MAP _all_nodes;				//���нڵ���ڵ�id�Ķ�Ӧ��ϵ,���ڿ���ͨ��node_id�ҵ���Ӧ��TREE_NODE  (key=_node_id -> data=ETM_TREE_NODE)
	_u32 _file_id;
	_u32 _total_node_num;		//��ͬ��_tree_node_count�������Ҫ�����ۼ������µ�node_id
	//MAP _all_name_hash_lists;				//���нڵ���ڵ�id�Ķ�Ӧ��ϵ,���ڿ���ͨ��node_id�ҵ���Ӧ��TREE_NODE  (key=_node_id -> data=ETM_TREE_NODE)
	//MAP _all_data_hash_lists;				//���нڵ���ڵ�id�Ķ�Ӧ��ϵ,���ڿ���ͨ��node_id�ҵ���Ӧ��TREE_NODE  (key=_node_id -> data=ETM_TREE_NODE)
	char  _tree_file[MAX_FULL_PATH_BUFFER_LEN];				//�洢�������ڵ���Ϣ���ļ�ȫ��:tree_store.dat
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
	//TREE _tree;				//���ڼ�¼����������ݵ���(ETM_TREE_NODE)
	//MAP _all_nodes;				//���нڵ���ڵ�id�Ķ�Ӧ��ϵ,���ڿ���ͨ��node_id�ҵ���Ӧ��TREE_NODE  (key=_node_id -> data=ETM_TREE_NODE)
	//_u32 _total_node_num;		//��ͬ��_tree_node_count�������Ҫ�����ۼ������µ�node_id
	//////////////////
	MAP _all_trees;				//����TREE��TREE id�Ķ�Ӧ��ϵ,���ڿ���ͨ��tree_id�ҵ���Ӧ��TREE  (key=tree_id -> data=TREE)
	_u32 _total_tree_num;		//��ͬ��_tree_node_count�������Ҫ�����ۼ������µ�tree_id
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
