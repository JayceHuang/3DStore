#ifndef _RES_QUERY_INTERFACE_H_
#define _RES_QUERY_INTERFACE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#define URL_MAX_LEN 2047
#define HUB_CMD_HEADER_LEN 12
#define HUB_ENCODE_PADDING_LEN 16

typedef enum tagHUB_TYPE 
{
    SHUB, 
    PHUB, 
    PARTNER_CDN, 
    TRACKER, 
    BT_HUB, 
    EMULE_HUB, 
    BT_TRACKER,
    BT_DHT, 
    EMULE_KAD, 
    CDN_MANAGER, 
    VIP_HUB, 
    KANKAN_CDN_MANAGER, 
    CONFIG_HUB, 
    DPHUB_ROOT, 
    DPHUB_NODE, 
    EMULE_TRACKER,
	NORMAL_CDN_MANAGER,
} HUB_TYPE;

typedef struct tagHUB_CMD_HEADER
{
    _u32 _version;
    _u32 _seq;
    _u32 _cmd_len;
}HUB_CMD_HEADER;

#ifdef _DK_QUERY
#include "res_query/dk_res_query/dk_manager_interface.h"
#endif

#ifdef	ENABLE_BT
#include "res_query/res_query_bt_tracker.h"
typedef _int32 (*res_query_bt_info_handler)(void* user_data,_int32 errcode, _u8 result, BOOL has_record, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size, _int32 control_flag);
_int32 res_query_bt_info(void* user_data, res_query_bt_info_handler callback_handler, _u8* info_id, _u32 file_index, BOOL need_bcid, _u32 query_times);
#ifdef ENABLE_BT_PROTOCOL
_int32 res_query_register_add_bt_res_handler(res_query_add_bt_res_handler bt_peer_handler);
_int32 res_query_bt_tracker(void* user_data, res_query_bt_tracker_handler callback, const char *url, _u8* info_hash);
#endif
#endif

#ifdef _DK_QUERY
_int32 res_query_dht(void* user_data, _u8 key[DHT_ID_LEN] );
_int32 res_query_kad(void* user_data, _u8 key[KAD_ID_LEN], _u64 file_size );

_int32 res_query_add_dht_nodes(void* user_data, LIST *p_nodes_list );
_int32 res_query_add_kad_node( _u32 ip, _u16 kad_port, _u8 version );

_int32 res_query_register_dht_res_handler(dht_res_handler res_handler);
_int32 res_query_register_kad_res_handler(kad_res_handler res_handler);
#endif

_int32 init_res_query_module(void);
_int32 uninit_res_query_module(void);
void res_query_update_hub( void);

_int32 force_close_res_query_module_res(void);

typedef _int32 (*res_query_add_server_res_handler)(void* user_data, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size, BOOL is_origin,_u8 resource_priority, BOOL relation_res, _u32 relation_index);
typedef _int32 (*res_query_add_peer_res_handler)(void* user_data, char *peer_id, _u8 *gcid, _u64 file_size, _u32 peer_capability, _u32 host_ip, _u16 tcp_port, _u16 udp_port,  _u8 res_level, _u8 res_from, _u8 res_priority);
typedef _int32 (*res_query_add_relation_fileinfo)(void* user_data,  _u8 gcid[CID_SIZE],  _u8 cid[CID_SIZE], _u64 file_size,  _u32 block_num,  RELATION_BLOCK_INFO** p_block_info_arr);
_int32 res_query_register_add_resource_handler(res_query_add_server_res_handler server_handler,res_query_add_peer_res_handler peer_handler, res_query_add_relation_fileinfo add_relation_fileinfo);

typedef _int32 (*res_query_notify_shub_handler)(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag);
_int32 res_query_shub_by_url(void* user_data, res_query_notify_shub_handler handler, const char* url, const char* refer_url,BOOL need_bcid,
							 _int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn,BOOL not_add_res);

_int32 res_query_shub_by_cid(void* user_data, res_query_notify_shub_handler handler, const _u8* cid, _u64 file_size, BOOL is_gcid, const char* url_or_gcid,
							 BOOL need_bcid, _int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn, _u8 cid_query_type);


_int32 res_query_shub_by_resinfo_newcmd(void* user_data, res_query_notify_shub_handler handler, const _u8* cid, _u64 file_size, BOOL is_gcid, const char* gcid,
							 BOOL need_bcid,_int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn, _u8 cid_query_type);

_int32 res_query_shub_by_url_newcmd(void* user_data, res_query_notify_shub_handler handler, const char* url, const char* refer_url,BOOL need_bcid,
							 _int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn,BOOL not_add_res);

_int32 res_query_shub_by_cid_newcmd(void* user_data, res_query_notify_shub_handler handler, const _u8* cid, _u64 file_size, BOOL is_gcid, const char* url_or_gcid,
							 BOOL need_bcid,_int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn, _u8 cid_query_type);
							 
_int32 relation_res_query_shub(void* user_data, res_query_notify_shub_handler handler, const char* url, const char* refer_url, char cid[CID_SIZE], char gcid[CID_SIZE], _u64 file_size, _int32 max_res,  _int32 query_times_sn);

/* query_type 0:普通查询	1:仅需要发行Server	 2:仅需要外网资源	3:仅需要内网资源  4. 需要所有种类资源（与0的差别在于如果有cdn资源必须返回）5：仅需要upnp 资源6：仅需要同子网资源7：仅需要支持http的资源*/
typedef _int32 (*res_query_notify_phub_handler)(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
_int32 res_query_phub(void* user_data, res_query_notify_phub_handler handler, const _u8* cid, const _u8* gcid, 
					  _u64 file_size, _int32 bonus_res_num, _u8 query_type);

_int32 res_query_partner_cdn(void* user_data, res_query_notify_phub_handler handler, const _u8* cid, const _u8* gcid, _u64 file_size);

typedef _int32 (*res_query_notify_tracker_handler)(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval,_u32 query_stamp);
_int32 res_query_tracker(void* user_data, res_query_notify_tracker_handler handler, _u32 last_query_stamp, const _u8* gcid, _u64 file_size,
						 _u8 max_res, _int32 bonus_res_num, _u32 upnp_ip, _u32 upnp_port);

// req_from: 
// [0 - P2SP_TASK - p2p 查询]
typedef _int32 (*res_query_notify_dphub_handler)(void *user_data, _int32 errcode, _u16 result, _u32 retry_interval, BOOL is_continued);
_int32 res_query_dphub(void *user_data, res_query_notify_dphub_handler handler, 
                       _u16 file_src_type, _u8 file_idx_desc_type, _u32 file_idx_content_cnt,
                       char *file_idx_content, _u64 file_size, _u32 block_size, 
                       const char *cid, const char *gcid, _u16 req_from);


#ifdef ENABLE_CDN
typedef _int32 (*res_query_cdn_manager_handler)(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* host, _u16 port);
_int32 res_query_cdn_manager(_int32 version, _u8* gcid, _u64 file_size, res_query_cdn_manager_handler callback_handler, void* user_data);

// 针对普通用户提供的cdn支持
// 只有当任务下载速度为0时才启用的cdn资源
_int32 res_query_normal_cdn_manager(void* user_data,
									res_query_notify_phub_handler handler,
									const _u8 *cid,
									const _u8 *gcid,
									_u64 file_size,
									_int32 bonus_res_num, 
									_u8 query_type);

#ifdef ENABLE_KANKAN_CDN
typedef _int32 (*res_query_kankan_cdn_manager_handler)(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* url, _u32 url_len);
_int32 res_query_kankan_cdn_manager(_int32 version, 
									_u8* gcid, 
									_u64 file_size, 
									res_query_cdn_manager_handler callback_handler, 
									void* user_data);
#endif
#ifdef ENABLE_HSC
typedef _int32 (*res_query_notify_vip_hub_handler)(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
_int32 res_query_vip_hub(void* user_data, res_query_notify_vip_hub_handler handler, const _u8* cid, const _u8* gcid, _u64 file_size, _int32 bonus_res_num, _u8 query_type);
#endif /* ENABLE_HSC */
#endif
/*
function : cancel all resource query on task, no notify any more.
*/
_int32 res_query_cancel(void* user_data, HUB_TYPE type);

typedef _int32 (*res_query_notify_recv_data)(_int32 errcode, char* buffer, _u32 len, void* user_data);

#ifdef ENABLE_EMULE
_int32 res_query_commit_request(HUB_TYPE device_type, _u32 seq, char** buffer, _u32 len, res_query_notify_recv_data callback, void* user_data);
#endif /* ENABLE_EMULE */

// query emule tracker
_int32 res_query_emule_tracker(void* user_data, res_query_notify_tracker_handler handler, _u32 last_query_stamp, const char *file_hash, 
                               _u32 upnp_ip, _u16 upnp_port);

_int32 res_query_aes_encrypt(char* buffer, _u32* len);

void dump_buffer(char* buffer, _u32 length);


#ifdef __cplusplus
}
#endif

#endif



