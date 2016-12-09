#if !defined(__DATA_RECEIVE_20080709)
#define __DATA_RECEIVE_20080709

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/range.h"
#include "utility/list.h"
#include "connect_manager/resource.h"


typedef struct _tag_range_data_buffer
{
       RANGE  _range;
	_u32    _data_length;   
	_u32    _buffer_length;
    	char*   _data_ptr;          
}RANGE_DATA_BUFFER;

typedef struct _tag_range_data_buffer_list
{
      LIST   _data_list;  

}RANGE_DATA_BUFFER_LIST;


typedef struct _tag_data_receiver
{
        _u32 _block_size;
        RANGE_LIST _total_receive_range;
	 RANGE_LIST _cur_cache_range;	
	 RANGE_DATA_BUFFER_LIST _buffer_list; 	 
  
}DATA_RECEIVER;


#define GET_TOTAL_RECV_RANGE_LIST(data_rece) (&(data_rece._total_receive_range))

/* creat range data buffer  slab, must  eb invoke in the initial state of the download program*/
_int32 get_data_receiver_cfg_parameter(void);
_int32 init_range_data_buffer_slab(void);
_int32 destroy_range_data_buffer_slab(void);
_int32 alloc_range_data_buffer_node(RANGE_DATA_BUFFER** pp_node);
_int32 free_range_data_buffer_node(RANGE_DATA_BUFFER* p_node);

_int32 get_max_flush_unit_num(void);

/* the platform of the data buffer list*/
void buffer_list_init(RANGE_DATA_BUFFER_LIST *  buffer_list);
_u32 buffer_list_size(RANGE_DATA_BUFFER_LIST *buffer_list);
_int32 buffer_list_add(RANGE_DATA_BUFFER_LIST *buffer_list, RANGE*  range, char *data,  _u32 length, _u32 buffer_len);
_int32 buffer_list_splice(RANGE_DATA_BUFFER_LIST *l_buffer_list, RANGE_DATA_BUFFER_LIST *r_buffer_list);

_int32 buffer_list_delete(RANGE_DATA_BUFFER_LIST *buffer_list, RANGE*  range, RANGE_LIST* ret_range_list);
_int32 drop_buffer_list(RANGE_DATA_BUFFER_LIST *buffer_list);
_int32 drop_buffer_list_without_buffer(RANGE_DATA_BUFFER_LIST *buffer_list);
_int32 drop_buffer_from_range_buffer(RANGE_DATA_BUFFER *p_range_buffer);


void data_receiver_init(DATA_RECEIVER *data_receive);
void data_receiver_unit(DATA_RECEIVER *data_receive);

void data_receiver_set_blocksize(DATA_RECEIVER *data_receive, _u32  block_size);

BOOL data_receiver_is_empty(DATA_RECEIVER *data_receive);
/*
_int32 data_receiver_set_range(DATA_RECEIVER *data_receive, RANGE*  range);
_int32 data_receiver_add_range(DATA_RECEIVER *data_receive, RANGE*  add_range);
_int32 data_receiver_del_range(DATA_RECEIVER *data_receive, RANGE*  del_range);
_int32 data_receiver_add_range_list(DATA_RECEIVER *data_receive, RANGE_LIST* add_range_list);
_int32 data_receiver_del_range_list(DATA_RECEIVER *data_receive, RANGE_LIST* del_range_list);
*/
_int32 data_receiver_add_buffer(DATA_RECEIVER *data_receive, RANGE*  range, char *data,  _u32 length, _u32 buffer_len);
_int32 data_receiver_del_buffer(DATA_RECEIVER *data_receive, RANGE*  range);
void data_receiver_destroy(DATA_RECEIVER *data_receive);

_int32 range_list_erase_buffer_range_list(RANGE_LIST* range_list, RANGE_DATA_BUFFER_LIST* buffer_list);
_int32 range_list_add_buffer_range_list(RANGE_LIST* range_list, RANGE_DATA_BUFFER_LIST* buffer_list);


_int32 data_receiver_erase_range(DATA_RECEIVER *data_receive, RANGE*  del_range);

_int32 data_receiver_syn_data_buffer(DATA_RECEIVER *data_receive);

BOOL data_receiver_range_is_recv(DATA_RECEIVER *data_receive, const RANGE* r);

void data_receiver_cp_buffer(DATA_RECEIVER *data_receive, RANGE_DATA_BUFFER_LIST* need_flush_data);

_int32 data_receiver_get_releate_data_buffer(DATA_RECEIVER *data_receive, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer);

_int32 get_range_list_from_buffer_list(RANGE_DATA_BUFFER_LIST *buffer_list,  RANGE_LIST* ret_range_list);

_int32 get_releate_data_buffer_from_range_data_buffer_by_range(RANGE_DATA_BUFFER_LIST* src_range_buffer, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer);

#ifdef __cplusplus
}
#endif

#endif /* !defined(__DATA_RECEIVE_20080709)*/

