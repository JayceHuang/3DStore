#ifndef _EMULE_QUERY_EMULE_TRACKER_H_
#define _EMULE_QUERY_EMULE_TRACKER_H_
#include "utility/define.h"

#ifdef ENABLE_EMULE

#include "asyn_frame/msg.h"
#include "emule/emule_utility/emule_define.h"
#include "res_query/res_query_interface.h"

#define QUERY_EMULE_TRACKER         161
#define QUERY_EMULE_TRACKER_RESP    162

// 4+16+4+16+4+2+1=47(peer的总长度)
typedef struct tagEMULE_TRACKER_PEER
{
    _u32    _user_id_len;
    _u8     _user_id[USER_ID_SIZE+1];   // 最后一个字符只是为了输出显示的便宜，不计入peer的总长度
    _u32    _peer_id_len;
    _u8     _peer_id[PEER_ID_SIZE+1];   // 同_user_id的说明
    _u32    _ip;
    _u16    _port;
    _u8     _capability;
} EMULE_TRACKER_PEER;

typedef struct tagQUERY_EMULE_TRACKER
{
    _u32	_protocol_version;
    _u32	_seq;
    _u32	_cmd_len;
    _u8		_cmd_type;
    _u32    _last_query_stamp;
    _u32    _file_hash_len;
    _u8     _file_hash[FILE_ID_SIZE];   // file_hash
    _u32    _user_id_len;
    _u8     _user_id[USER_ID_SIZE];     // emule_client_hash
    _u32    _local_peer_id_len;
    _u8     _local_peer_id[PEER_ID_SIZE];
    _u8     _capability;
    _u32    _ip;
    _u16    _port;
    _u32    _nat_type;
    _u32    _product_id;
    _u32    _upnp_ip;
    _u16    _upnp_port;
} QUERY_EMULE_TRACKER_CMD;

typedef struct tagQUERY_EMULE_TRACKER_RESP
{
    HUB_CMD_HEADER _header;
    _u8		_cmd_type;
    _u8		_result;
    _u32    _query_stamp;
    _u32    _query_interval;
    _u32    _file_hash_len;
    _u8     _file_hash[FILE_ID_SIZE];
    _u32    _super_node_ip;
    _u16    _super_node_port;
    _u32    _peer_res_num;
    _u32	_peer_res_buffer_len;
    char*	_peer_res_buffer;
} QUERY_EMULE_TRACKER_RESP_CMD;

_int32 emule_build_query_emule_tracker_cmd(QUERY_EMULE_TRACKER_CMD* cmd, char** buffer, _u32* len);

_int32 emule_extract_query_emule_tracker_resp_cmd(char* buffer, _u32 len, QUERY_EMULE_TRACKER_RESP_CMD* cmd);

_int32 emule_notify_query_emule_tracker_callback(void* user_data, _int32 errcode, _u8 result, 
                                                 _u32 retry_interval, _u32 query_stamp);

_int32 emule_handle_query_emule_tracker_timeout(const MSG_INFO *msg_info, _int32 errcode, 
                                                _u32 notice_count_left, _u32 expired, _u32 msgid);

// 根据协议，在emule_tracker_query和resp中返回的capability与通常所用的peer_capability并不一致
/*
第0位：1－Peer是内网的
第1位：1－与查询Peer位于相同局域网
第2位: 1－须使用加密协议连接
*/
_u32 build_emule_tracker_peer_capability();

_u32 emule_tracker_peer_capability_2_local_peer_capability(_u32 et_peer_capability);

#ifdef _RSA_RES_QUERY
_int32 emule_build_query_emule_tracker_cmd_rsa(QUERY_EMULE_TRACKER_CMD *cmd, char **buffer, _u32 *len, 
	_u8* aes_key, _int32 pubkey_version);

#endif

#endif 
#endif
