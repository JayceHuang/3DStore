

#if !defined(__BT_QUERY_H_20090319)
#define __BT_QUERY_H_20090319
/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : bt_query.h                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                        */
/*     Module     : bt_download													*/
/*     Version    : 1.3  													*/
/*--------------------------------------------------------------------------*/
/*                  Shenzhen XunLei Networks			                    */
/*--------------------------------------------------------------------------*/
/*                  - C (copyright) - www.xunlei.com -		     		    */
/*                                                                          */
/*      This document is the proprietary of XunLei                          */
/*                                                                          */
/*      All rights reserved. Integral or partial reproduction               */
/*      prohibited without a written authorization from the                 */
/*      permanent of the author rights.                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*                         FUNCTIONAL DESCRIPTION                           */
/*--------------------------------------------------------------------------*/
/* This file contains the platform of bt info query and resource query                      */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2009.03.19 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT
#include "bt_download/bt_task/bt_task.h"
#include "asyn_frame/msg.h"


_int32 bt_start_query_hub( BT_TASK *p_bt_task );
_int32 bt_start_query_hub_for_single_file( BT_TASK *p_bt_task,BT_FILE_INFO *p_file_info,_u32 file_index );
_int32 bt_stop_query_hub( BT_TASK *p_bt_task );
_int32 bt_query_info_callback( void* user_data,_int32 errcode,  _u8 result,BOOL has_record, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_int32 control_flag );
_int32 bt_query_info_success_callback( BT_TASK *p_bt_task, _u32 file_index,BT_FILE_INFO * p_file_info,BOOL has_record, _u64 file_size,
	_u8 *cid,_u8* gcid, _u32 gcid_level,_u8 *bcid, _u32 bcid_size, _u32 block_size );

_int32 bt_add_bt_peer_resource(void* user_data, _u32 host_ip, _u16 port);

_int32 bt_start_res_query_bt_tracker(BT_TASK * p_bt_task );
_int32 bt_stop_res_query_bt_tracker(BT_TASK * p_bt_task );
_int32 bt_res_query_bt_tracker_callback(void* user_data,_int32 errcode );


_int32 bt_start_res_query_dht(BT_TASK * p_bt_task );
_int32 bt_stop_res_query_dht(BT_TASK * p_bt_task );
_int32 bt_res_query_dht_callback(void* user_data,_int32 errcode );

_int32 bt_handle_query_info_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 bt_handle_query_bt_tracker_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 bt_handle_query_dht_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);

#endif /* #ifdef ENABLE_BT */

#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_QUERY_H_20090319)



