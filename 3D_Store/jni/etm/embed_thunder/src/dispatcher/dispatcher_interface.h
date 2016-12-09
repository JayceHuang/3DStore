//
//////////////////////////////////////////////////////////////////////

#if !defined(__DISPATCHER_INTERFACE_20080603)
#define __DISPATCHER_INTERFACE_20080603

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/range.h"
#include "asyn_frame/msg.h"
#include "connect_manager/resource.h"
#include "connect_manager/data_pipe.h"
#include "connect_manager/connect_manager_interface.h"
//#include "data_manager/data_manager_interface.h"
#include "dispatcher/dispatcher_interface_info.h"

typedef enum _tag_dispatcher_mode {NORMAL = 0, PLAY }DISPATCHER_MODE;

typedef struct   _tag_dispatcher 
{
       DS_DATA_INTEREFACE  _data_interface;
	CONNECT_MANAGER*  _p_connect_manager; 
	DISPATCHER_MODE _dispatch_mode;

	RANGE_LIST _tmp_range_list;   //  downing range list now
	RANGE_LIST _overalloc_range_list; // range list which  repeatly alloc times has over 3 times  

	RANGE_LIST _overalloc_in_once_dispatch;  //  range list which repeatly alloc in one dispitch period

	RANGE_LIST _cur_down_point;    // the dowing range point list now ()
	
	_u32 _cur_down_pipes;
	
    _u32 _start_index;
	_u32  _start_index_time_ms;   

	_u32  _dispatcher_timer;
	_u32  _immediate_dispatcher_timer;
    _u32 _last_dispatch_time_ms;
}DISPATCHER;

/* creat dispatch item node slab, must  be invoke in the initial state of the download program*/
_int32 init_dispatcher_module(void);
_int32 uninit_dispatcher_module(void);


/* platform of dispatcher */
_int32 get_dispatcher_cfg_parameter(void);
_int32 init_dispatch_item_slab(void);
_int32 destroy_dispatch_item_slab(void);

/* platform of dispatcher */

_int32 ds_init_dispatcher(DISPATCHER*  ptr_dispatch_interface, DS_DATA_INTEREFACE*  ptr_data_interface, CONNECT_MANAGER* ptr_connect_manager);

_int32 ds_unit_dispatcher(DISPATCHER*  ptr_dispatch_interface);

_int32 ds_start_dispatcher(DISPATCHER*  ptr_dispatch_interface);

_int32 ds_stop_dispatcher(DISPATCHER*  ptr_dispatch_interface);




/*先把所有在下载erased_blocks的pipe的任务块调整到不和erased_blocks重叠，然后进行重新调度*/

/* dispatcher  需要通过pipe的指针访问pipe的dispatch info 信息(连接状态，  已下载块，   未下载块 等信息)*/

_int32 ds_do_dispatch(DISPATCHER* ptr_dispatch_interface, BOOL force);

_int32 ds_active_new_connect(DISPATCHER*  ptr_dispatch_interface);

_int32 ds_dispatch_at_no_filesize(DISPATCHER*  ptr_dispatch_interface);

_int32 ds_dispatch_at_origin_res(DISPATCHER*  ptr_dispatch_interface);

_int32 ds_dispatch_at_muti_res(DISPATCHER*  ptr_dispatch_interface);




/*inter  function*/

_int32 ds_dispatcher_timer_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

void init_download_map(MAP* down_map);
void unit_download_map(MAP* down_map);
void clear_pipe_list(LIST*  p_pipe_list);
_int32 download_map_update_item(MAP* down_map, _u32 index, BOOL is_pipe_alloc, BOOL is_completed, DATA_PIPE* ptr_data_pipe, BOOL force);
_int32 download_map_erase_pipe(MAP* down_map, _u32 index, DATA_PIPE* ptr_data_pipe);
BOOL ds_dispatch_pipe_is_origin(DISPATCHER*  ptr_dispatch_interface, DATA_PIPE* p_pipe);
void ds_dispatch_get_origin_pipe_list(DISPATCHER*  ptr_dispatch_interface, PIPE_LIST* src_pipe_list, PIPE_LIST* origin_pipe_list);
BOOL ds_handle_correct_range(ERROR_BLOCK* p_error_block, PIPE_LIST* p_long_server_list,  PIPE_LIST* p_range_server_list, PIPE_LIST* p_last_server_list,
	                                           PIPE_LIST* p_peer_pipe_list,MAP* p_down_map);

BOOL ds_assign_correct_range(ERROR_BLOCK* p_error_block, PIPE_LIST* p_pipe_list, MAP* p_down_map);

BOOL ds_handle_correct_range_using_origin_res(DISPATCHER*  ptr_dispatch_interface, ERROR_BLOCK* p_error_block, PIPE_LIST* p_long_server_list,  PIPE_LIST* p_range_server_list, PIPE_LIST* p_last_server_list,
	                                           PIPE_LIST* p_peer_pipe_list, MAP* p_down_map);

BOOL ds_assign_correct_range_using_origin_res(DISPATCHER*  ptr_dispatch_interface,ERROR_BLOCK* p_error_block, PIPE_LIST* p_pipe_list, MAP* p_down_map);

BOOL ds_res_is_include(LIST* res_list, RESOURCE* p_res);
DATA_PIPE* ds_get_data_pipe( PIPE_LIST* p_long_server_list,  PIPE_LIST* p_range_server_list, PIPE_LIST* p_last_server_list,
	                                           PIPE_LIST* p_peer_pipe_list);
void ds_put_data_pipe( DATA_PIPE* ptr_data_pipe,PIPE_LIST* p_long_server_list,  PIPE_LIST* p_range_server_list, PIPE_LIST* p_last_server_list,
	                                           PIPE_LIST* p_peer_pipe_list);
BOOL ds_assigned_range_to_pipe(DATA_PIPE* ptr_data_pipe, RANGE* ptr_range);
_int32 ds_build_uncomplete_map(MAP* down_map, RANGE_LIST* uncomplete_range_list);
_int32 ds_build_pri_range_map(MAP* down_map, RANGE_LIST* pri_range_list);
//_int32 ds_build_error_map(MAP* down_map, ERROR_BLOCKLIST* p_error_block_list);

void ds_adjust_pipe_list(PIPE_LIST* p_pipe_list);
_int32 ds_build_pipe_list_map(MAP* down_map,  MAP* partial_map,PIPE_LIST* p_pipe_list, RANGE_LIST* uncomplete_range_list, 
	        PIPE_LIST* p_long_pipe_list,  PIPE_LIST* p_range_pipe_list, PIPE_LIST* p_last_pipe_list,
	        PIPE_LIST* p_new_long_pipe_list,  PIPE_LIST* p_new_range_pipe_list, PIPE_LIST* p_new_last_pipe_list, 
	        PIPE_LIST* p_partial_pipe_list,RANGE_LIST*  correct_range_list, RANGE_LIST*  p_dowing_range_list,  _u32 now_time);

_int32 ds_dispatch_at_pipe(DISPATCHER*  ptr_dispatch_interface, DATA_PIPE*  ptr_data_pipe);

_int32 ds_start_dispatcher_immediate(DISPATCHER*  ptr_dispatch_interface);
    //过滤链表，从头删除超出block_num的range
    void ds_filter_range_list_from_begin(RANGE_LIST* range_list,int block_num);
#ifdef LITTLE_FILE_TEST
BOOL ds_is_super_pipe(DATA_PIPE * ptr_data_pipe);
BOOL ds_is_task_in_origin_mode(DATA_PIPE * ptr_data_pipe);
BOOL ds_should_use_little_file_assign_range(DATA_PIPE * ptr_data_pipe);
#endif /* LITTLE_FILE_TEST */

#ifdef __cplusplus
}
#endif

#endif /* !defined(__DISPATCHER_INTERFACE_20080603)*/




