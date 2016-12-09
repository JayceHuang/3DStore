//
//////////////////////////////////////////////////////////////////////

#if !defined(__RANGE_INTERFACE_20080617)
#define __RANGE_INTERFACE_20080617

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

/*
     RANGE store the order range struct list, and merge the closed range
*/


typedef struct  _tag_range
{
     _u32  _index;
     _u32   _num;	  
} RANGE;

typedef struct tagEXACT_RANGE
{
	_u64 _pos;
	_u64 _length;
} EXACT_RANGE;

typedef EXACT_RANGE BT_RANGE;
typedef struct _exact_range_list_node
{
	EXACT_RANGE _exact_range;
	struct _exact_range_list_node *_next_node;
	struct _exact_range_list_node *_prev_node;
} EXACT_RANGE_LIST_NODE;

typedef struct _exact_range_list
{
	_u32 _node_size;
	EXACT_RANGE_LIST_NODE *_head_node;
	EXACT_RANGE_LIST_NODE *_tail_node;
} EXACT_RANGE_LIST;

typedef struct _range_list_node
{       
	RANGE   _range;
	struct _range_list_node* _next_node;
	struct _range_list_node* _prev_node;
} RANGE_LIST_NODE ,* PRANGE_LIST_NODE;

typedef struct _tag_range_list
{       
	_u32 _node_size;
	RANGE_LIST_NODE* _head_node;
	RANGE_LIST_NODE* _tail_node;
}RANGE_LIST;

typedef PRANGE_LIST_NODE RANGE_LIST_ITEROATOR;

/* creat range list node slab, must  eb invoke in the initial state of the download program*/
_u32 init_range_module(void);
_u32 uninit_range_module(void);

/* the platform of the range list alloc*/
_int32 get_range_cfg_parameter(void);
_int32 init_range_slabs(void);
_int32 destroy_range_slabs(void);
_int32 range_list_alloc_node(RANGE_LIST_NODE** pp_node);
_int32 range_list_free_node(RANGE_LIST_NODE* p_node);
_int32 alloc_range_list(RANGE_LIST** pp_node);
_int32 free_range_list(RANGE_LIST* p_node);

/* the platform of the range list*/
_int32 range_list_init(RANGE_LIST* range_list);

_int32 exact_range_list_init(EXACT_RANGE_LIST* range_list);

_int32 range_list_clear(RANGE_LIST* range_list);

_int32 range_list_size(RANGE_LIST* range_list);

RANGE range_list_get_first_overlap_range(RANGE_LIST* range_list, const RANGE* r);

BOOL range_list_is_relevant(RANGE_LIST* range_list, const RANGE* r);

/*
     0  表示r 与range_list 中一项在头部重叠
     1  表示r 与range_list 中一项在尾部重叠
     2  表示r 与range_list 无重叠
*/
_u32 range_list_is_head_relevant(RANGE_LIST* range_list, const RANGE* r);

_int32 range_list_add_range(RANGE_LIST* range_list, const RANGE*  new_range, RANGE_LIST_NODE* begin_node,RANGE_LIST_NODE** _p_ret_node);

_int32 range_list_add_range_list(RANGE_LIST* range_list, RANGE_LIST* new_range_list);

_int32 range_list_delete_range(RANGE_LIST* range_list, const RANGE*  del_range, RANGE_LIST_NODE* begin_node,RANGE_LIST_NODE** _p_ret_node);

_int32 range_list_delete_range_list(RANGE_LIST* range_list, RANGE_LIST* del_range_list);

_int32 exact_range_list_erase (EXACT_RANGE_LIST * range_list, EXACT_RANGE_LIST_NODE * erase_node);

_int32 add_exact_range_to_list (EXACT_RANGE_LIST * range_list, const EXACT_RANGE * new_range,
		   EXACT_RANGE_LIST_NODE * insert_node);

_int32 exact_range_list_delete_range (EXACT_RANGE_LIST * range_list,
			 const EXACT_RANGE * del_range,
			 EXACT_RANGE_LIST_NODE * begin_node,
			 EXACT_RANGE_LIST_NODE ** _p_ret_node);

_int32 out_put_range_list(RANGE_LIST* range_list);

_int32 force_out_put_range_list(RANGE_LIST* range_list);


_int32 range_list_get_total_num(RANGE_LIST* range_list);

BOOL range_list_is_include(const RANGE_LIST* range_list, const RANGE* r);

BOOL range_is_overlap(RANGE* l, RANGE* r);

_int32 range_list_get_head_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR *head_node);
_int32 range_list_get_next_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR cur_node, RANGE_LIST_ITEROATOR *next_node);
_int32 range_list_get_tail_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR *tail_node);

/*调整 range_list 使得都在adjust_range_list 范围内部*/
_int32 range_list_adjust(RANGE_LIST* range_list, RANGE_LIST* adjust_range_list);

BOOL range_list_is_contained(const RANGE_LIST* range_list1, RANGE_LIST* range_list2);


void range_list_cp_range_list(RANGE_LIST* l_range_list, RANGE_LIST* r_range_list);

/*typedef struct range_list{
	LIST _range_list;
	_u32 _event_notice_handle;
}RANGE_LIST;

_int32 range_list_init(RANGE_LIST *list);

_int32 range_list_unint(RANGE_LIST *list);*/


_u32 get_data_unit_size(void);
_u32 compute_unit_num(_u64 block_size);
_u64 get_data_pos_from_index(_u32 index);

_u64  range_to_length(RANGE* r,  _u64 file_size);
/*返回包含pos-length的 最小range，注意返回的range的边界不一定=pos ，可能比pos小*/
RANGE  pos_length_to_range(_u64 pos, _u64 length,  _u64 file_size);

/*返回包含pos-length的 最小完整range，返回的range 一定是完整的包括最后一块 !*/
RANGE  pos_length_to_range2(_u64 pos, _u64 length,  _u64 file_size);

/*return the max range node of the range list*/
_int32 range_list_get_max_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR *ret_node);

_int32 range_list_get_rand_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR *ret_node);

_int32 range_list_get_lasted_node(RANGE_LIST *range_list, _u32 _index, RANGE_LIST_ITEROATOR *ret_node);

RANGE relevant_range(RANGE* l, RANGE *r);


/*返回包含pos-length 中包含的完整block对应的range 范围，不足一个block的返回range 中_num=0*/
RANGE  pos_length_to_valid_range(_u64 pos, _u64 length,  _u64 file_size, _u32 block_size);

BOOL range_list_is_contained2(RANGE_LIST* range_list1, RANGE_LIST* range_list2);
_int32 get_range_list_overlap_list(RANGE_LIST* range_list1, RANGE_LIST* range_list2, RANGE_LIST* ret_list);

_int32 range_list_pop_first_range(RANGE_LIST* range_list);

_int32  range_list_erase(RANGE_LIST* range_list, RANGE_LIST_NODE*  erase_node);

_int32 range_list_is_head_relevant2(RANGE_LIST* range_list, const RANGE* r);
#ifdef __cplusplus
}
#endif

#endif /* !defined(__RANGE_INTERFACE_20080617)*/




