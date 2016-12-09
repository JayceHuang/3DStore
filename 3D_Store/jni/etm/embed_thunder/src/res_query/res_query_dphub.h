#ifndef _RES_QUERY_DPHUB_H_
#define _RES_QUERY_DPHUB_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "res_query_interface_imp.h"
#include "download_task/download_task.h"

#define DPHUB_PROTOCOL_VERSION      2

#define DPHUB_OWNER_QUERY_ID        21
#define DPHUB_OWNER_QUERY_RESP_ID   22

#define DPHUB_RC_QUERY_ID           71
#define DPHUB_RC_QUERY_RESP_ID      72

#define DPHUB_RC_NODE_QUERY_ID      73
#define DPHUB_RC_NODE_QUERY_RESP_ID 74

// 16+_partner_id_size+_thunder_version_size
typedef struct tagPRODUCT_INFO
{
    _u32    _partner_id_size;
    char	_partner_id[MAX_PARTNER_ID_LEN];
    _u32    _product_release_id;
    _u32    _product_flag;
    _u32    _thunder_version_size;
    char    _thunder_version[MAX_VERSION_LEN];
} PRODUCT_INFO;

// 29+_cid_size+_gcid_size+_file_idx_content_size
typedef struct tagFILE_RC
{
    _u32    _cid_size;
    char    _cid[CID_SIZE];
    _u32    _gcid_size;
    char    _gcid[CID_SIZE];
    _u64    _file_size;
    _u32    _block_size;
    _u16    _subnet_type;
    _u16    _file_src_type;
    _u8     _file_idx_desc_type;    // see dphub protocol doc
    _u32    _file_idx_content_size;
    char    *_file_idx_content;
} FILE_RC;

// 8+1+2+_node_host_len
typedef struct tagNODE_RC
{
    _u32    _node_id;
    _u8     _node_type;
    _u32    _node_host_len;
    char    _node_host[MAX_HOST_NAME_LEN];
    _u16    _node_port;
} NODE_RC;

typedef struct tagDPHUB_NODE_RC
{
    _u32    _intro_id;  // ��¼�¸�NODE_RC�ڵ��Ǳ�˭���ص�
    NODE_RC _node_rc;
} DPHUB_NODE_RC;

// 20+8+3+_peerid_size+_file_idx_content_size
typedef struct tagPEER_RC
{
    _u32    _peerid_size;
    char    _peerid[PEER_ID_SIZE];
    _u32    _peer_capacity;
    _u32    _ip;    // internal ip
    _u16    _tcp_port;
    _u16    _udp_port;
    _u8     _res_level;
    _u8     _res_priority;
    _u16    _file_subnet_type;  // ��Դ��������  ���dphub protocol doc.
    _u16    _file_src_type; // ��Դ����
    _u8     _file_idx_desc_type;
    _u32    _file_idx_content_size;
    char    *_file_idx_content;
    _u32    _cdn_type;
} PEER_RC;

// dphub(����/Ӧ��)����Ĺ���ͷ��
// 12 + 8
typedef struct tagDPHUB_CMD_HEADER
{
    _u32    _version;
    _u32    _seq;
    _u32    _cmd_len;   // ע�⣺���_cmd_len�ǳ�ȥͷ��֮�������ȣ�ͷ��������_u32 _version; _u32 _seq; _u32 _cmd_len;��
    _u16    _cmd_type;  // ע�⣺���_cmd_type�������������type���Ͳ�һ��
    _u32    _compress_len;
    _u16    _compress_flag;
} DPHUB_CMD_HEADER;

// dphub��������Ĺ���ͷ��
// 12 + 24 +_peerid_size+L(PRODUCT_INFO)
typedef struct tagDPHUB_CMD_REQUEST_HEADER
{
    DPHUB_CMD_HEADER    _header;    // common protocol header
    _u32                _peerid_size;
    _u8                 _peerid[PEER_ID_SIZE];
    _u32                _peer_capacity; // <==> peer_capability, according to protocol doc.
    _u32                _region_id;
    _u32                _parent_id;
    _u32                _pi_size;   // _product_info�ṹ�Ĵ�С
    PRODUCT_INFO        _product_info;
} DPHUB_CMD_REQUEST_HEADER;

// ��ѯ�Լ��Ĺ����ڵ������(OwnerQuery)
typedef struct tagDPHUB_OWNER_QUERY
{
    DPHUB_CMD_REQUEST_HEADER    _req_header;    // common request header
    _u32                        _last_query_stamp;
} DPHUB_OWNER_QUERY;

// ��ѯ�����ڵ����Ӧ(OwnerQueryResp)
typedef struct tagDPHUB_OWNER_QUERY_RESP
{
    DPHUB_CMD_HEADER    _header;
    _u16                _result;
    _u32                _region_id;
    _u32                _parent_id;
    _u32                _parent_node_host_len;
    char                _parent_node_host[MAX_HOST_NAME_LEN];
    _u16                _parent_node_port;
    _u32                _query_stamp;   // ��ǰ�������Ĳ�ѯʱ��
    _u32                _query_interval;
    _u32                _ping_interval;
    _u32                _config_len;
    char                *_config_ptr;
} DPHUB_OWNER_QUERY_RESP;

// ��ѯ��Դ(RcQuery)
typedef struct tagDPHUB_RC_QUERY
{
    DPHUB_CMD_REQUEST_HEADER    _req_header;
    _u32                        _fr_size;   // _file_rc�Ľṹ���С
    FILE_RC                     _file_rc;
    _u32                        _res_capacity;
    _u16                        _max_res;
    _u16                        _level_res;
    _u8                         _speed_up;
    _u32                        _p2p_capacity;
    _u32                        _ip;
    _u32                        _upnp_ip;
    _u16                        _upnp_port;
    _u32                        _nat_type;
    _u32                        _intro_node_id; // ���ܽڵ��id��see dphub protocol doc.
    _u16                        _cur_peer_num;
    _u16                        _total_query_times;
    _u16                        _query_times_on_node; // �ڸýڵ�Ĳ�ѯ����
    _u16                        _request_from;
} DPHUB_RC_QUERY;

// ��Դ��Ӧ(RcQueryResp)
typedef struct tagDPHUB_RC_QUERY_RESP
{
    DPHUB_CMD_HEADER    _header;
    _u16                _result;
    _u32                _cid_size;
    char                _cid[CID_SIZE];
    _u32                _gcid_size;
    char                _gcid[CID_SIZE];
    _u64                _file_size;
    _u32                _block_size;
    _u16                _level_res;
    _u32                _total_res;
    _u32                _retry_interval;
    _u32                _nr_size;       // _uplevel_node�ṹ��Ĵ�С
    NODE_RC             _uplevel_node;
    _u32                _peerrc_num;
    _u32                _peer_rc_len;   // _peerrc_ptrָ��Ļ������ĳ���
    char                *_peerrc_ptr;
} DPHUB_RC_QUERY_RESP;

// ��ѯ��Դ�������ڵ�(RcNodeQuery)
#define DPHUB_RC_NODE_QUERY DPHUB_RC_QUERY

// ��Դ�����ڵ����Ӧ(RcNodeQueryResponse)
typedef struct tagDPHUB_RC_NODE_QUERY_RESP
{
    DPHUB_CMD_HEADER    _header;
    _u16                _result;
    _u32                _cid_size;
    char                _cid[CID_SIZE];
    _u32                _gcid_size;
    char                _gcid[CID_SIZE];
    _u64                _file_size;
    _u32                _block_size;
    _u16                _level_res;
    _u32                _total_res;
    _u32                _retry_interval;
    _u32                _nr_size;
    NODE_RC             _uplevel_node;
    _u32                _noderc_num;
    _u32                _node_rc_len;
    char                *_noderc_ptr;
} DPHUB_RC_NODE_QUERY_RESP;

const char *get_g_parent_node_host();

_u16 get_g_parent_node_port();

_u32 get_g_last_query_dphub_stamp();
_u32 set_g_last_query_dphub_stamp(_u32 stamp);

_u32 get_g_region_id();
_u32 set_g_region_id(_u32 id);

_u32 get_g_parent_id();
_u32 set_g_parent_id(_u32 id);

_u32 get_g_to_query_node_list(LIST **query_node_list);

_u32 get_g_current_peer_rc_num();

_u32 get_g_max_res();

_int32 get_dphub_query_context(TASK *task, RES_QUERY_PARA *query_para, DPHUB_QUERY_CONTEXT **dphub_context);

_int32 push_dphub_node_rc_into_list(DPHUB_NODE_RC *p_node_rc, DPHUB_QUERY_CONTEXT *dphub_query_context);

// ��ѯ�Լ��Ĺ����ڵ�(21)
_int32 dphub_query_owner_node(HUB_DEVICE *device, void* user_data, res_query_notify_dphub_handler handler);

// �����ѯ�����ڵ����Ӧ(22)
_int32 handle_query_owner_node_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);

// ��Դ��ѯ(71)
_int32 dphub_rc_query();

// ������Դ��ѯ��Ӧ(72)
_int32 handle_rc_query_resp(res_query_add_peer_res_handler add_peer_res_fun_ptr, char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);

// ��Դ�ڵ��ѯ(73)
_int32 dphub_rc_node_query();

// ������Դ�ڵ��ѯ��Ӧ(74)
_int32 handle_rc_node_query_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);


#ifdef _LOGGER

_int32 output_query_owner_node_for_debug(DPHUB_OWNER_QUERY *cmd);
_int32 output_rc_query_for_debug(DPHUB_RC_QUERY *cmd);
_int32 output_query_owner_node_resp_for_debug(DPHUB_OWNER_QUERY_RESP *cmd);
void output_node_rc_for_debug(NODE_RC *node_rc, const char *head_descri_line);
void output_rc_query_resp_for_debug(DPHUB_RC_QUERY_RESP *cmd);
void output_rc_node_query_resp_for_debug(DPHUB_RC_NODE_QUERY_RESP *cmd);

#endif


// �������ֶΰ��ֽ���䵽buffer��ȥ�������֮ǰ��Ȼ�Ѿ�֪���˸��ֶεľ���ȡֵ
_int32 build_dphub_query_owner_node_cmd(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_OWNER_QUERY *cmd);
_int32 extract_query_owner_node_resp_cmd(char *buffer, _u32 len, DPHUB_OWNER_QUERY_RESP *cmd);

_int32 build_dphub_request_header(DPHUB_CMD_REQUEST_HEADER *p_req_header);
_int32 build_dphub_rc_query_cmd(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_RC_QUERY *cmd, _u16 cmd_type);
_int32 extract_rc_query_resp_cmd(char *buffer, _u32 len, DPHUB_RC_QUERY_RESP *cmd);

_int32 build_dphub_rc_node_query_cmd(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_RC_NODE_QUERY *cmd);
_int32 extract_rc_node_query_resp_cmd(char *buffer, _u32 len, DPHUB_RC_NODE_QUERY_RESP *cmd);

#ifdef _RSA_RES_QUERY
_int32 build_dphub_query_owner_node_cmd_rsa(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_OWNER_QUERY *cmd, 
	_u8* aes_key, _int32 pubkey_version);
_int32 build_dphub_rc_query_cmd_rsa(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_RC_QUERY *cmd, _u16 cmd_type,
	_u8* aes_key, _int32 pubkey_version);

#endif

_u32 init_dphub_module();
_u32 uninit_dphub_module();

#ifdef __cplusplus
}
#endif
#endif

