/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/09/21
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_IMPL_H_
#define _EMULE_IMPL_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE

#include "emule/emule_utility/emule_define.h"
#include "emule/emule_utility/emule_peer.h"
#include "utility/define_const_num.h"
#include "asyn_frame/msg.h"

struct tagEMULE_TASK;
struct tagEMULE_PIPE_DEVICE;
struct tagEMULE_DATA_PIPE;
struct tagEMULE_TAG_LIST;

_int32 emule_init_local_peer(void);

_int32 emule_uninit_local_peer(void);

EMULE_PEER* emule_get_local_peer(void);

_u32 emule_get_local_client_id(void);

_int32 emule_notify_get_cid_info(struct tagEMULE_TASK* task, _u8 cid[CID_SIZE], _u8 gcid[CID_SIZE]);

_int32 emule_notify_get_part_hash(struct tagEMULE_TASK* task, const _u8* part_hash, _u32 part_hash_len);

_int32 emule_notify_get_aich_hash(struct tagEMULE_TASK* task, const _u8* aich_hash, _u32 aich_hash_len);

_int32 emule_notify_query_failed(struct tagEMULE_TASK* task);

_int32 emule_notify_query_phub_result(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);

_int32 emule_notify_task_failed(struct tagEMULE_TASK* task, _int32 errcode);

_int32 emule_notify_task_success(struct tagEMULE_TASK* task);

void emule_notify_task_delete(struct tagEMULE_TASK* task);

_int32 emule_notify_client_id_change(_u32 client_id);

_int32 emule_notify_find_source(_u8 file_id [ FILE_ID_SIZE ], _u32 client_id, _u16 client_port, _u32 server_ip, _u16 server_port);

#ifdef ENABLE_EMULE_PROTOCOL
_int32 emule_notify_passive_connect(struct tagEMULE_PIPE_DEVICE* pipe_device);
#endif

//emule download_queue 
_int32 emule_init_download_queue(void);

#ifdef ENABLE_EMULE_PROTOCOL

_int32 emule_download_queue_add(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_download_queue_remove(struct tagEMULE_DATA_PIPE* emule_pipe);

struct tagEMULE_DATA_PIPE* emule_download_queue_find(_u32 ip, _u16 udp_port);
#endif

_int32 emule_uninit_download_queue(void);

_int32 emule_kad_notify_find_sources(void* user_data, _u8 user_hash[USER_ID_SIZE], struct tagEMULE_TAG_LIST* tag_list);
void emule_task_shub_res_query(struct tagEMULE_TASK* task);
void emule_task_peer_res_query(struct tagEMULE_TASK* task);
void emule_task_query_emule_tracker(struct tagEMULE_TASK *task);
_int32 emule_notify_query_shub_callback(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag);
_int32 emule_notify_query_only_res_shub_callback(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag);
_int32 emule_notify_query_phub_result(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
_int32 emule_notify_query_vip_hub_result(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
_int32 emule_notify_query_tracker_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval,_u32 query_stamp);
_int32 emule_notify_query_partner_cdn_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
_int32 emule_notify_query_dphub_result(void* user_data,_int32 errcode, _u16 result, _u32 retry_interval, BOOL is_continued);
_int32 emule_handle_wait_dphub_root_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
#if defined(ENABLE_CDN)
_int32 emule_notify_query_cdn_manager_callback(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* host, _u16 port);
#endif /* ENABLE_CDN */

#endif /* ENABLE_EMULE */
#endif



