/*****************************************************************************
 *
 * Filename: 			dk_manager.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of dk_manager.
 *
 *****************************************************************************/



#if !defined(__dk_manager_20091015)
#define __dk_manager_20091015

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _DK_QUERY

#include "res_query/dk_res_query/routing_table.h"
#include "res_query/dk_res_query/dk_socket_handler.h"
#include "res_query/dk_res_query/ping_handler.h"

struct tagEMULE_TAG_LIST;

typedef struct tagDK_MANAGER
{
	LIST _task_list; //½ÚµãÎªdk_task
	_u32 _time_out_id;
	_u32 _dk_type;
    _u32 _idle_ticks;
}DK_MANAGER;

_int32 dk_manager_create( _u32 dk_type );
_int32 dk_manager_destroy( _u32 dk_type );
_int32 dk_manager_time_out(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid );
_int32 dht_res_nofity( void *p_user, _u32 ip, _u16 port );
_int32 kad_res_nofity( void *p_user, _u8 id[KAD_ID_LEN], struct tagEMULE_TAG_LIST *p_tag_list );
#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__dk_manager_20091015)


