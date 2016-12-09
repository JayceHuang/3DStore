#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define LOGID LOGID_RES_QUERY
#include "utility/logger.h"
#include "utility/version.h"
#include "utility/peer_capability.h"
#include "utility/utility.h"
#include "utility/peerid.h"
#include "utility/aes.h"
#include "res_query_dphub.h"
#include "res_query_impl.h"
#include "res_query_interface_imp.h"
#include "res_query_cmd_builder.h"
#include "download_task/download_task.h"
#include "p2sp_download/p2sp_task.h"

#ifdef ENABLE_BT
#include "bt_download/bt_task/bt_accelerate.h"
#include "bt_download/bt_task/bt_task.h"
#endif

#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#endif

#include "res_query_security.h"

extern _int32 get_version(char *buffer, _int32 bufsize);

static _u32 g_last_query_dphub_stamp = 0;
static _u32 g_region_id = 0;
static _u32 g_parent_id = 0;
static char g_parent_node_host[MAX_HOST_NAME_LEN] = {0};
static _u16 g_parent_node_port = 0;
static LIST g_to_query_node_list;   

static _u32 g_current_peer_rc_num = 0;  // 当前已经查到的所有peer rc的个数
static _u32 g_max_res = 0;  // 需要的最大资源数

_u32 init_dphub_module()
{
    list_init(&g_to_query_node_list);
    return 0;
}

_u32 uninit_dphub_module()
{
    g_last_query_dphub_stamp = 0;
    g_region_id = 0;
    g_parent_id = 0;
    sd_memset(&g_parent_node_host, 0, MAX_HOST_NAME_LEN);
    return 0;
}

_u32 get_g_last_query_dphub_stamp()
{
    return g_last_query_dphub_stamp;
}

_u32 set_g_last_query_dphub_stamp(_u32 stamp)
{
    g_last_query_dphub_stamp = stamp;
    return SUCCESS;
}

_u32 get_g_region_id()
{
    return g_region_id;
}

_u32 set_g_region_id(_u32 id)
{
    g_region_id = id;
    return SUCCESS;
}

_u32 get_g_parent_id()
{
    return g_parent_id;
}

_u32 set_g_parent_id(_u32 id)
{
    g_parent_id = id;
    return SUCCESS;
}

const char *get_g_parent_node_host()
{
    if (sd_strlen(g_parent_node_host) == 0)
        return DEFAULT_DPHUB_PARENT_NODE_HOST_NAME;
    else
        return g_parent_node_host;
}

_u16 get_g_parent_node_port()
{
    if (g_parent_node_port == 0)
        return DEFAULT_DPHUB_PARENT_NODE_PORT;
    else
        return g_parent_node_port;
}

_u32 get_g_to_query_node_list(LIST **query_node_list)
{
    *query_node_list = &g_to_query_node_list;
    return 0;
}

_u32 get_g_current_peer_rc_num()
{
    return g_current_peer_rc_num;
}

_u32 get_g_max_res()
{
    return g_max_res;
}

_int32 dphub_query_owner_node(HUB_DEVICE *device, void* user_data, res_query_notify_dphub_handler handler)
{
    char* buffer = NULL;
    _u32  len = 0;
    _int32 ret = SUCCESS;
    DPHUB_OWNER_QUERY cmd;
    
    LOG_DEBUG("dphub_query_owner_node, user_data = 0x%x.", user_data);
    sd_memset(&cmd, 0, sizeof(DPHUB_OWNER_QUERY));
    cmd._last_query_stamp = g_last_query_dphub_stamp;
    ret = build_dphub_query_owner_node_cmd(device, &buffer, &len, &cmd);
    CHECK_VALUE(ret);
    LOG_DEBUG("dphub_query_owner_node, cmd._last_query_stamp = %u.", cmd._last_query_stamp);

    return res_query_commit_cmd(device, DPHUB_OWNER_QUERY_ID, buffer, len, (void*)handler, user_data, cmd._req_header._header._seq, FALSE, NULL, NULL);
}

/*
    cmd_len: 特定命令的其他字段长度（除去_req_header的长度）
*/
_int32 build_dphub_cmd_req_header(DPHUB_CMD_REQUEST_HEADER *req_header, _u32 seq, _u32 cmd_len, _u16 cmd_type)
{
	req_header->_header._version = DPHUB_PROTOCOL_VERSION;
    req_header->_header._seq = seq;
    // 44: req_header中的固定字节长度（除去_version, _seq, _cmd_len三个字段组成的命令头部长度）
    req_header->_header._cmd_len = 44 + req_header->_peerid_size + 
        req_header->_product_info._partner_id_size + 
        req_header->_product_info._thunder_version_size + 
        cmd_len;
    req_header->_header._cmd_type = cmd_type;
    req_header->_header._compress_flag = 0; // 压缩暂时不实现
    req_header->_header._compress_len = 0;
    return 0;
}

_int32 build_dphub_query_owner_node_cmd(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_OWNER_QUERY *cmd)
{
    static _u32 seq = 0;
    _u32 query_owner_cmd_len = 0;
    _int32 ret;
    char *tmp_buf;
    _int32 tmp_len;
    char http_header[1024] = {0};
    _u32 http_header_len = 1024;
    _u32 encode_len = 0;

    query_owner_cmd_len = 4;
    build_dphub_cmd_req_header(&cmd->_req_header, seq++, query_owner_cmd_len, DPHUB_OWNER_QUERY_ID);    
    *len = HUB_CMD_HEADER_LEN + cmd->_req_header._header._cmd_len;
    encode_len = (cmd->_req_header._header._cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
    LOG_DEBUG("build_dphub_query_owner_node_cmd, hub_type=%d", device->_hub_type);
    ret = res_query_build_http_header(http_header, &http_header_len, encode_len, device->_hub_type, device->_host, device->_port);
    CHECK_VALUE(ret);
    ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
    if(ret != SUCCESS)
    {
        LOG_DEBUG("build_dphub_query_owner_node_cmd, malloc failed.");
        CHECK_VALUE(ret);
    }
    sd_memcpy(*buffer, http_header, http_header_len);
    tmp_buf = *buffer + http_header_len;
    tmp_len = (_int32)*len;

    // 填充req_header
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._version);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._seq);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._cmd_len);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_req_header._header._cmd_type);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._compress_len);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_req_header._header._compress_flag);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._peerid_size);
    sd_set_bytes(&tmp_buf, &tmp_len, (char*)&(cmd->_req_header._peerid), (_int32)cmd->_req_header._peerid_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._peer_capacity);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._region_id);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._parent_id);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._pi_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._partner_id_size);
    sd_set_bytes(&tmp_buf, &tmp_len, cmd->_req_header._product_info._partner_id, (_int32)cmd->_req_header._product_info._partner_id_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._product_release_id);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._product_flag);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._thunder_version_size);
    sd_set_bytes(&tmp_buf, &tmp_len, cmd->_req_header._product_info._thunder_version, (_int32)cmd->_req_header._product_info._thunder_version_size);

    // 填充cmd的特有字段
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_last_query_stamp);

#ifdef _LOGGER
    output_query_owner_node_for_debug(cmd);
#endif

    ret = xl_aes_encrypt(*buffer + http_header_len, len);
    if(ret != SUCCESS)
    {
        LOG_ERROR("build_dphub_query_owner_node_cmd, but aes_encrypt failed. errcode = %d.", ret);
        sd_free(*buffer);
        *buffer = NULL;
        sd_assert(FALSE);
        return ret;
    }
    sd_assert(tmp_len == 0);
    *len += http_header_len;
    return SUCCESS;
}

_int32 build_dphub_request_header(DPHUB_CMD_REQUEST_HEADER *p_req_header)
{
    LOG_DEBUG("build_dphub_request_header, p_req_header(0x%x).", p_req_header);
    sd_assert(p_req_header != NULL);

    p_req_header->_peerid_size = PEER_ID_SIZE;
    get_peerid((char *)(p_req_header->_peerid), PEER_ID_SIZE);
    p_req_header->_peer_capacity = get_peer_capability();
    p_req_header->_region_id = get_g_region_id();
    p_req_header->_parent_id = get_g_parent_id();
    p_req_header->_product_info._partner_id_size = PARTNER_ID_LEN;
    get_partner_id(p_req_header->_product_info._partner_id, PARTNER_ID_LEN);
    p_req_header->_product_info._product_release_id = get_product_id();
    p_req_header->_product_info._product_flag = get_product_flag();
    p_req_header->_product_info._thunder_version_size = MAX_VERSION_LEN;
    get_version(p_req_header->_product_info._thunder_version, MAX_VERSION_LEN);
    p_req_header->_pi_size = 16 + p_req_header->_product_info._partner_id_size +
        p_req_header->_product_info._thunder_version_size;

    return SUCCESS;
}

_int32 build_dphub_rc_query_cmd(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_RC_QUERY *cmd, _u16 cmd_type)
{
    static _u32 seq = 0;
    _u32 rc_query_cmd_len = 0;  // 除去_req_header之后的大小
    _int32 ret;
    char *tmp_buf;
    _int32 tmp_len;
    char http_header[1024] = {0};
    _u32 http_header_len = 1024;
    _u32 encode_len = 0;

    rc_query_cmd_len = 72 + 
        cmd->_file_rc._cid_size + 
        cmd->_file_rc._gcid_size + 
        cmd->_file_rc._file_idx_content_size;
    build_dphub_cmd_req_header(&cmd->_req_header, seq++, rc_query_cmd_len, cmd_type);
    *len = HUB_CMD_HEADER_LEN + cmd->_req_header._header._cmd_len;
    encode_len = (cmd->_req_header._header._cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
    LOG_DEBUG("build_dphub_rc_query_cmd, hub_type=%d", device->_hub_type);
    ret = res_query_build_http_header(http_header, &http_header_len, encode_len, device->_hub_type, device->_host, device->_port);
    CHECK_VALUE(ret);
    ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
    if(ret != SUCCESS)
    {
        LOG_DEBUG("build_dphub_rc_query_cmd, malloc failed.");
        CHECK_VALUE(ret);
    }
    sd_memcpy(*buffer, http_header, http_header_len);
    tmp_buf = *buffer + http_header_len;
    tmp_len = (_int32)*len;

    // 填充req_header
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._version);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._seq);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._cmd_len);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_req_header._header._cmd_type);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._compress_len);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_req_header._header._compress_flag);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._peerid_size);
    sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_req_header._peerid, (_int32)cmd->_req_header._peerid_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._peer_capacity);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._region_id);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._parent_id);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._pi_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._partner_id_size);
    sd_set_bytes(&tmp_buf, &tmp_len, cmd->_req_header._product_info._partner_id, (_int32)cmd->_req_header._product_info._partner_id_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._product_release_id);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._product_flag);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._thunder_version_size);
    sd_set_bytes(&tmp_buf, &tmp_len, cmd->_req_header._product_info._thunder_version, (_int32)cmd->_req_header._product_info._thunder_version_size);

    // 填充cmd的特有字段
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_fr_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_rc._cid_size);
    sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_rc._cid, (_int32)cmd->_file_rc._cid_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_rc._gcid_size);
    sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_rc._gcid, (_int32)cmd->_file_rc._gcid_size);
    sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_rc._file_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_rc._block_size);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_file_rc._subnet_type);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_file_rc._file_src_type);
    sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_file_rc._file_idx_desc_type);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_rc._file_idx_content_size);
    sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_rc._file_idx_content, (_int32)cmd->_file_rc._file_idx_content_size);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_capacity);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_max_res);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_level_res);
    sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_speed_up);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capacity);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ip);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_upnp_ip);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_upnp_port);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_nat_type);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_intro_node_id);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cur_peer_num);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_total_query_times);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_query_times_on_node);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_request_from);    

#ifdef _LOGGER
    output_rc_query_for_debug(cmd);
#endif

    ret = xl_aes_encrypt(*buffer + http_header_len, len);

    // 这里发生了内存的拷贝，原来申请的内存理应释放掉，注意！
    if (cmd->_file_rc._file_idx_content != 0)
    {
        sd_free(cmd->_file_rc._file_idx_content);
        cmd->_file_rc._file_idx_content = 0;
    }

    if(ret != SUCCESS)
    {
        LOG_ERROR("build_dphub_rc_query_cmd, but aes_encrypt failed. errcode = %d.", ret);
        sd_free(*buffer);
        *buffer = NULL;
        sd_assert(FALSE);
        return ret;
    }
    sd_assert(tmp_len == 0);
    *len += http_header_len;
    return SUCCESS;
}

_int32 build_dphub_rc_node_query_cmd(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_RC_NODE_QUERY *cmd)
{
    return build_dphub_rc_query_cmd(device, buffer, len, (DPHUB_RC_QUERY *)cmd, DPHUB_RC_NODE_QUERY_ID);
}

_int32 extract_query_owner_node_resp_cmd(char *buffer, _u32 len, DPHUB_OWNER_QUERY_RESP *cmd)
{
    _int32 ret = SUCCESS;
    char* tmp_buf = buffer;
    _int32  tmp_len = (_int32)len;
    sd_memset(cmd, 0, sizeof(DPHUB_OWNER_QUERY_RESP));
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_header._cmd_type);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._compress_len);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_header._compress_flag);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_result);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_region_id);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_parent_id);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_parent_node_host_len);
    sd_get_bytes(&tmp_buf, &tmp_len, (char *)cmd->_parent_node_host, cmd->_parent_node_host_len);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_parent_node_port);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_query_stamp);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_query_interval);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_ping_interval);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_config_len);
    ret = sd_get_bytes(&tmp_buf, &tmp_len, (char *)cmd->_config_ptr, cmd->_config_len);
    if(ret != SUCCESS)
    {
        LOG_ERROR("[version = %u]extract_query_owner_node_resp_cmd failed, ret = %d.", cmd->_header._version, ret);
        return ERR_RES_QUERY_INVALID_COMMAND;
    }
    if(tmp_len > 0)
    {
        LOG_ERROR("[version = %u]extract_query_owner_node_resp_cmd, but last %d bytes is unknowned how to extract.", cmd->_header._version, tmp_len);
    }
    return SUCCESS;
}

#ifdef _LOGGER

_int32 output_query_owner_node_for_debug(DPHUB_OWNER_QUERY *cmd)
{
    _int32 ret = SUCCESS;
    char thunder_version[MAX_VERSION_LEN + 1] = {0};
    char partner_id[MAX_PARTNER_ID_LEN + 1] = {0};
    char peerid[PEER_ID_SIZE + 1] = {0};
    
    ret = sd_memcpy((void *)peerid, (void *)(cmd->_req_header._peerid), PEER_ID_SIZE);
    CHECK_VALUE(ret);

    ret = sd_memcpy((void *)partner_id, (void **)(cmd->_req_header._product_info._partner_id),
        cmd->_req_header._product_info._partner_id_size);
    CHECK_VALUE(ret);

    ret = sd_memcpy((void *)thunder_version, (void **)(cmd->_req_header._product_info._thunder_version),
        cmd->_req_header._product_info._thunder_version_size);
    CHECK_VALUE(ret);

    LOG_DEBUG("DPHUB_OWNER_QUERY cmd description: \n"
        "_head_version = %u \n"
        "_seq = %u \n"
        "_cmd_len = %u \n"
        "_cmd_type = %hu \n"
        "_compress_len = %u \n"
        "_compress_flag = %hu \n"
        "_peerid_size = %u \n"
        "_peerid = %s \n"
        "_peer_capacity = %u \n"
        "_region_id = %u \n"
        "_parent_id = %u \n"
        "_pi_size = %u \n"
        "\t _partner_id_size = %u \n"
        "\t _partner_id = %u \n"
        "\t _product_release_id = %u \n"
        "\t _product_flag = %u \n"
        "\t _thunder_version_size = %u \n"
        "\t _thunder_version = %s \n"
        "_last_query_stamp = %u", 
        cmd->_req_header._header._version,
        cmd->_req_header._header._seq,
        cmd->_req_header._header._cmd_len,
        cmd->_req_header._header._cmd_type,
        cmd->_req_header._header._compress_len,
        cmd->_req_header._header._compress_flag,
        cmd->_req_header._peerid_size,
        peerid,
        cmd->_req_header._peer_capacity,
        cmd->_req_header._region_id,
        cmd->_req_header._parent_id,
        cmd->_req_header._pi_size,
        cmd->_req_header._product_info._partner_id_size,
        partner_id,
        cmd->_req_header._product_info._product_release_id,
        cmd->_req_header._product_info._product_flag,
        cmd->_req_header._product_info._thunder_version_size,
        thunder_version,
        cmd->_last_query_stamp);
    return ret;
}

_int32 output_rc_query_for_debug(DPHUB_RC_QUERY *cmd)
{
    _int32 ret = SUCCESS;
    char *file_idx_content = 0;
    char thunder_version[MAX_VERSION_LEN + 1] = {0};
    char partner_id[MAX_PARTNER_ID_LEN + 1] = {0};
    char peerid[PEER_ID_SIZE + 1] = {0};
    char cid[CID_SIZE*2+1] = {'\0'};
    char gcid[CID_SIZE*2+1] = {'\0'};
    sd_memset((void *)cid, 0, CID_SIZE*2+1);
    sd_memset((void *)gcid, 0, CID_SIZE*2+1);
    str2hex((char *)cmd->_file_rc._cid, CID_SIZE, cid, CID_SIZE*2);
    str2hex((char *)cmd->_file_rc._gcid, CID_SIZE, gcid, CID_SIZE*2);
    cid[CID_SIZE*2] = '\0';
    gcid[CID_SIZE*2] = '\0';

    ret = sd_memcpy((void *)peerid, (void *)(cmd->_req_header._peerid), PEER_ID_SIZE);
    CHECK_VALUE(ret);

    ret = sd_memcpy((void *)partner_id, (void **)(cmd->_req_header._product_info._partner_id),
        cmd->_req_header._product_info._partner_id_size);
    CHECK_VALUE(ret);

    ret = sd_memcpy((void *)thunder_version, (void **)(cmd->_req_header._product_info._thunder_version),
        cmd->_req_header._product_info._thunder_version_size);
    CHECK_VALUE(ret);

    if (cmd->_file_rc._file_idx_content != NULL)
    {
        ret = sd_malloc(cmd->_file_rc._file_idx_content_size, 
            (void **)&cmd->_file_rc._file_idx_content);
        CHECK_VALUE(ret);

        ret = sd_memcpy((void *)file_idx_content, (void *)cmd->_file_rc._file_idx_content, 
            cmd->_file_rc._file_idx_content_size);
        CHECK_VALUE(ret);
    }
    
    LOG_DEBUG("DPHUB_RC_QUERY cmd description: \n"
        "_head_version = %u \n"
        "_seq = %u \n"
        "_cmd_len = %u \n"
        "_cmd_type = %hu \n"
        "_compress_len = %u \n"
        "_compress_flag = %hu \n"
        "_peerid_size = %u \n"
        "_peerid = %s \n"
        "_peer_capacity = %u \n"
        "_region_id = %u \n"
        "_parent_id = %u \n"
        "_pi_size = %u \n"
        "\t _partner_id_size = %u \n"
        "\t _partner_id = %s \n"
        "\t _product_release_id = %u \n"
        "\t _product_flag = %u \n"
        "\t _thunder_version_size = %u \n"
        "\t _thunder_version = %s \n"
        "_fr_size = %u \n"
        "\t _cid = %s \n"
        "\t _gcid = %s \n"
        "\t _file_size = %llu \n"
        "\t _block_size = %u \n"
        "\t _subnet_type = %hu \n"
        "\t _file_src_type = %hu \n"
        "\t _file_idx_desc_type = %hhu \n"
        "\t _file_idx_content_size = %u \n"
        "\t _file_idx_content = %s \n"
        "_res_capacity = %u \n"
        "_max_res = %hu \n"
        "_level_res = %hu \n"
        "_speed_up = %hhu \n"
        "_ip = %u \n"
        "_upnp_ip = %u \n"
        "_upnp_port = %hu \n"
        "_nat_type = %u \n"
        "_intro_node_id = %u \n"
        "_cur_peer_num = %hu \n"
        "_total_query_times = %hu \n"
        "_query_times_on_node = %hu \n"
        "_request_from = %hu \n",
        cmd->_req_header._header._version,
        cmd->_req_header._header._seq,
        cmd->_req_header._header._cmd_len,
        cmd->_req_header._header._cmd_type,
        cmd->_req_header._header._compress_len,
        cmd->_req_header._header._compress_flag,
        cmd->_req_header._peerid_size,
        peerid,
        cmd->_req_header._peer_capacity,
        cmd->_req_header._region_id,
        cmd->_req_header._parent_id,
        cmd->_req_header._pi_size,
        cmd->_req_header._product_info._partner_id_size,
        partner_id,
        cmd->_req_header._product_info._product_release_id,
        cmd->_req_header._product_info._product_flag,
        cmd->_req_header._product_info._thunder_version_size,
        thunder_version,
        cmd->_fr_size,
        cid,
        gcid,
        cmd->_file_rc._file_size,
        cmd->_file_rc._block_size,
        cmd->_file_rc._subnet_type,
        cmd->_file_rc._file_src_type,
        cmd->_file_rc._file_idx_desc_type,
        cmd->_file_rc._file_idx_content_size,
        (file_idx_content == NULL) ? "NULL" : file_idx_content,
        cmd->_res_capacity,
        cmd->_max_res,
        cmd->_level_res,
        cmd->_speed_up,
        cmd->_ip,
        cmd->_upnp_ip,
        cmd->_upnp_port,
        cmd->_nat_type,
        cmd->_intro_node_id,
        cmd->_cur_peer_num,
        cmd->_total_query_times,
        cmd->_query_times_on_node,
        cmd->_request_from);

    if (file_idx_content != NULL)
    {
        sd_free(file_idx_content);
        file_idx_content = 0;
    }
    return ret;
}

_int32 output_query_owner_node_resp_for_debug(DPHUB_OWNER_QUERY_RESP *cmd)
{
    _int32 ret = SUCCESS;
    char *config = NULL;

    ret = sd_malloc(cmd->_config_len+1, (void **)&config);
    CHECK_VALUE(ret);
    sd_memset((void *)config, 0, cmd->_config_len);
    sd_memcpy((void *)config, (void *)cmd->_config_ptr, cmd->_config_len);
    *(config+cmd->_config_len) = '\0';

    LOG_DEBUG("DPHUB_OWNER_QUERY_RESP cmd description: \n _result = %u\n _region_id = %u\n _parent_id = %u\n"
              " _parent_node_host = %s\n _parent_node_port = %u\n _query_stamp = %u\n _query_interval = %u\n"
              " _ping_interval = %u\n _config = %s.\n", cmd->_result, cmd->_region_id, cmd->_parent_id,
              cmd->_parent_node_host, cmd->_parent_node_port, cmd->_query_stamp, cmd->_query_interval, 
              cmd->_ping_interval, config);

    sd_free(config);
    return SUCCESS;
}

void output_node_rc_for_debug(NODE_RC *node_rc, const char *head_descri_line)
{
    LOG_DEBUG("%s\n _node_id = %u\n _node_type = %hhu\n _node_host = %s\n _node_port = %hu", 
        head_descri_line, node_rc->_node_id, node_rc->_node_type, node_rc->_node_host, node_rc->_node_port);
}

void output_rc_query_resp_for_debug(DPHUB_RC_QUERY_RESP *cmd)
{
    char cid[CID_SIZE*2+1] = {'\0'};
    char gcid[CID_SIZE*2+1] = {'\0'};
    sd_memset((void *)cid, 0, CID_SIZE*2+1);
    sd_memset((void *)gcid, 0, CID_SIZE*2+1);
    str2hex((char *)cmd->_cid, CID_SIZE, cid, CID_SIZE*2);
    str2hex((char *)cmd->_gcid, CID_SIZE, gcid, CID_SIZE*2);
    cid[CID_SIZE*2] = '\0';
    gcid[CID_SIZE*2] = '\0';

    NODE_RC *node_rc = &cmd->_uplevel_node;
    LOG_DEBUG("DPHUB_RC_QUERY_RESP cmd description: \n _result = %u\n _cid = %s\n _gcid = %s\n"
              " _file_size = %llu\n _block_size = %u\n _level_res = %hu\n _total_res = %u\n"
              " _retry_interval = %u\n"
              " _uplevel_node:\n \t_node_id = %u\n \t_node_type = %hhu\n \t_node_host = %s\n \t_node_port = %hu"
              " _peerrc_num = %u",
              cmd->_result, cid, gcid, cmd->_file_size, 
              cmd->_block_size, cmd->_level_res, cmd->_total_res, cmd->_retry_interval,
              node_rc->_node_id, node_rc->_node_type, node_rc->_node_host, node_rc->_node_port, 
              cmd->_peerrc_num);
}

void output_rc_node_query_resp_for_debug(DPHUB_RC_NODE_QUERY_RESP *cmd)
{
    char cid[CID_SIZE*2+1] = {'\0'};
    char gcid[CID_SIZE*2+1] = {'\0'};
    sd_memset((void *)cid, 0, CID_SIZE*2+1);
    sd_memset((void *)gcid, 0, CID_SIZE*2+1);
    str2hex((char *)cmd->_cid, CID_SIZE, cid, CID_SIZE*2);
    str2hex((char *)cmd->_gcid, CID_SIZE, gcid, CID_SIZE*2);
    cid[CID_SIZE*2] = '\0';
    gcid[CID_SIZE*2] = '\0';

    NODE_RC *node_rc = &cmd->_uplevel_node;
    LOG_DEBUG("DPHUB_RC_NODE_QUERY_RESP cmd description: \n _result = %u\n _cid = %s\n _gcid = %s\n"
              " _file_size = %llu\n _block_size = %u\n _level_res = %hu\n _total_res = %u\n"
              " _retry_interval = %u\n"
              " _uplevel_node:\n \t_node_id = %u\n \t_node_type = %hhu\n \t_node_host = %s\n \t_node_port = %hu\n"
              " _noderc_num = %u",
              cmd->_result, cid, gcid, cmd->_file_size, 
              cmd->_block_size, cmd->_level_res, cmd->_total_res, cmd->_retry_interval,
              node_rc->_node_id, node_rc->_node_type, node_rc->_node_host, node_rc->_node_port,
              cmd->_noderc_num);
}

#endif

_int32 handle_query_owner_node_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{   
    _int32 ret = SUCCESS;
    DPHUB_OWNER_QUERY_RESP resp_cmd;
    DPHUB_NODE_RC *p_parent_node = NULL;

    LOG_DEBUG("handle_query_owner_node_resp.");

    sd_memset(&resp_cmd, 0, sizeof(DPHUB_OWNER_QUERY_RESP));
    if (extract_query_owner_node_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
    {
        ((res_query_notify_dphub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
    }
    else if (last_cmd->_cmd_seq == resp_cmd._header._seq)
    {

#ifdef _LOGGER
        output_query_owner_node_resp_for_debug(&resp_cmd);
#endif

        g_last_query_dphub_stamp = resp_cmd._query_stamp;
        g_region_id = resp_cmd._region_id;
        g_parent_id = resp_cmd._parent_id;
        sd_memcpy(g_parent_node_host, resp_cmd._parent_node_host, resp_cmd._parent_node_host_len);
        g_parent_node_port = resp_cmd._parent_node_port;
        LOG_DEBUG("handle_query_owner_node_resp success, g_last_query_dphub_stamp(%d), g_region_id(%d), g_parent_id(%d), g_parent_node_host(%s), g_parent_node_port(%d).", 
            g_last_query_dphub_stamp, g_region_id, g_parent_id, g_parent_node_host, g_parent_node_port);

        // 同时将父节点也作为一个DPHUB_NODE_RC插入到列表中
        ret = sd_malloc(sizeof(DPHUB_NODE_RC), (void **)&p_parent_node);
        CHECK_VALUE(ret);
        ret = sd_memset(p_parent_node, 0, sizeof(DPHUB_NODE_RC));
        CHECK_VALUE(ret);
        p_parent_node->_intro_id = 0; // 父节点的介绍节点是0，无介绍节点
        p_parent_node->_node_rc._node_id = g_parent_id;
        p_parent_node->_node_rc._node_type = 0;  // 父节点是资源节点
        p_parent_node->_node_rc._node_host_len = resp_cmd._parent_node_host_len;
        sd_memcpy(p_parent_node->_node_rc._node_host, resp_cmd._parent_node_host, resp_cmd._parent_node_host_len);
        p_parent_node->_node_rc._node_port = resp_cmd._parent_node_port;

        ret = list_push(&g_to_query_node_list, (void *)p_parent_node);
        CHECK_VALUE(ret);

        ((res_query_notify_dphub_root_handler)last_cmd->_callback)(SUCCESS, resp_cmd._result, 
            resp_cmd._query_interval);
    }
    else
    {
        sd_assert(FALSE);
        LOG_ERROR("handle_query_owner_node_resp failure.");
        ((res_query_notify_dphub_root_handler)last_cmd->_callback)(-1, 0, 0);
    }

    return SUCCESS;
}

_int32 extract_rc_query_resp_cmd(char *buffer, _u32 len, DPHUB_RC_QUERY_RESP *cmd)
{
    _int32 ret = SUCCESS;
    char* tmp_buf = buffer;
    _int32  tmp_len = (_int32)len;
    _int32 idx_peer_rc = 0, rc_len = 0;

    sd_memset(cmd, 0, sizeof(DPHUB_RC_QUERY_RESP));
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_header._cmd_type);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._compress_len);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_header._compress_flag);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_result);

    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cid_size);
    sd_get_bytes(&tmp_buf, &tmp_len, (char *)cmd->_cid, cmd->_cid_size);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_size);
    sd_get_bytes(&tmp_buf, &tmp_len, (char *)cmd->_gcid, cmd->_gcid_size);
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_size);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_block_size);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_level_res);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_total_res);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_retry_interval);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_nr_size);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_uplevel_node._node_id);
    sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_uplevel_node._node_type);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_uplevel_node._node_host_len);
    sd_get_bytes(&tmp_buf, &tmp_len, (char *)cmd->_uplevel_node._node_host, cmd->_uplevel_node._node_host_len);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_uplevel_node._node_port);
    ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerrc_num);
    cmd->_peerrc_ptr = tmp_buf;
    for (idx_peer_rc = 0; idx_peer_rc < cmd->_peerrc_num; ++idx_peer_rc)
    {
        sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&rc_len);
        cmd->_peer_rc_len += (rc_len + 4);
        tmp_len -= rc_len;
        tmp_buf += rc_len;

    }

    if(ret != SUCCESS)
    {
        LOG_ERROR("[version = %u]extract_rc_query_resp_cmd failed, ret = %d.", 
            cmd->_header._version, ret);
        return ERR_RES_QUERY_INVALID_COMMAND;
    }
    if(tmp_len > 0)
    {
        LOG_ERROR("[version = %u]extract_rc_query_resp_cmd, but last %d bytes is unknowned how to extract.", 
            cmd->_header._version, tmp_len);
    }
    return SUCCESS;
}

// 找到=0，未找到-1
_int32 find_node_ip_in_exist_node_list(_u32 to_be_find_ip, LIST *exist_node_list)
{
    _int32 ret = -1;
    LIST_ITERATOR it = LIST_BEGIN(*exist_node_list);

    for (; it!=LIST_END(*exist_node_list); it=LIST_NEXT(it))
    {
        if ((_u32)LIST_VALUE(it) == to_be_find_ip) 
        {
            LOG_DEBUG("find_node_ip_in_exist_node_list find node ip(%u).", to_be_find_ip);
            ret = 0;
            break;
        }
    }

    return ret;
}

_int32 get_dphub_query_context(TASK *task, RES_QUERY_PARA *query_para, DPHUB_QUERY_CONTEXT **dphub_context)
{
    _int32 result = SUCCESS;
    enum TASK_TYPE task_type = 0;
#ifdef ENABLE_BT
    BT_FILE_TASK_INFO *bt_file_task_info = 0;
    _u32 bt_file_index = 0;
#endif
    LOG_DEBUG("get_dphub_query_context, task(0x%x), query_para(0x%x)", task, query_para);
    if (!task || !query_para)
    {
        return INVALID_ARGUMENT;
    }

    task_type = task->_task_type;
    switch (task_type)
    {
    case P2SP_TASK_TYPE:
        *dphub_context = &(((P2SP_TASK *)task)->_dpub_query_context);
        break;
#ifdef ENABLE_BT
    case BT_TASK_TYPE:
        bt_file_index = query_para->_file_index;
        result = map_find_node(&(((BT_TASK *)task)->_file_task_info_map), (void *)bt_file_index, (void **)&bt_file_task_info);
        CHECK_VALUE(result);
        sd_assert(bt_file_task_info != 0);
        *dphub_context = &(bt_file_task_info->_dpub_query_context);
        break;
    case BT_MAGNET_TASK_TYPE:
        // 磁力链任务不应该加速
        *dphub_context = 0;
        sd_assert(FALSE);
        break;
#endif
#ifdef ENABLE_EMULE
    case EMULE_TASK_TYPE:
        *dphub_context = &(((EMULE_TASK *)task)->_dpub_query_context);
        break;
#endif
    default:
        *dphub_context = 0;
        result = -2;
        sd_assert(FALSE);
        break;
    }
    
    LOG_DEBUG("get_dphub_query_context, task(0x%x), type(%d), result(%d)", 
        task, task_type, result);

    return result;
}

_int32 push_dphub_node_rc_into_list(DPHUB_NODE_RC *p_node_rc, DPHUB_QUERY_CONTEXT *dphub_query_context)
{
    _int32 result = SUCCESS;
    _u32 hash_value = 0;
    LIST *exist_node_list = &dphub_query_context->_exist_node_list;
    LIST *to_query_node_list = &dphub_query_context->_to_query_node_list;

    // node节点列表中存放的是node节点的主机地址hash
    result = sd_get_url_hash_value(p_node_rc->_node_rc._node_host, p_node_rc->_node_rc._node_host_len, &hash_value);
    CHECK_VALUE(result);
    result = find_node_ip_in_exist_node_list(hash_value, exist_node_list);
    LOG_DEBUG("find_node_ip_in_exist_node_list ret(%d), ip(%u).", result, hash_value);
    if (result != 0)
    {
        list_push(to_query_node_list, (void *)p_node_rc);
        list_push(exist_node_list, (void *)hash_value);
		return SUCCESS;
	}
	else
	{//返回错误
		return -1;
	}

    return result;
}

_int32 handle_rc_query_resp(res_query_add_peer_res_handler add_peer_res_fun_ptr, char* buffer, _u32 len, 
                            RES_QUERY_CMD *last_cmd)
{
    DPHUB_RC_QUERY_RESP resp_cmd;
    _int32 ret = SUCCESS;
    _int32 tmp_len = 0, i = 0, res_len = 0;
    char *tmp_buf = NULL;
    PEER_RC peer_rc;
    DPHUB_NODE_RC *p_node_rc = NULL;
    BOOL is_continue_query = FALSE;
    TASK *p_task = 0;
    enum TASK_STATUS task_statu = 0;
    DPHUB_QUERY_CONTEXT *dphub_query_context = 0;
    char to_be_add_peer_id[PEER_ID_SIZE + 1] = {'\0'};

    LOG_DEBUG("handle_rc_query_resp.");
    if (last_cmd->_user_data == NULL)
    {
        return SUCCESS;
    }
    
    p_task = ((RES_QUERY_PARA *)last_cmd->_user_data)->_task_ptr;
    if (p_task != 0) 
    {
        task_statu = p_task->task_status;
        ret = get_dphub_query_context(p_task, (RES_QUERY_PARA *)last_cmd->_user_data, &dphub_query_context);
        CHECK_VALUE(ret);
    }
    
    // 如果任务已经暂停或结束了，不再处理查询回来的结果
    if (task_statu != TASK_S_RUNNING)
    {
        LOG_ERROR("TASK IS STOPED!! status(%d).", task_statu != TASK_S_RUNNING);
        return SUCCESS;
    }

    sd_memset(&resp_cmd, 0, sizeof(DPHUB_RC_QUERY_RESP));
    if (extract_rc_query_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
    {
        ((res_query_notify_dphub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, FALSE);
    }
    else if (last_cmd->_cmd_seq == resp_cmd._header._seq)
    {
        tmp_buf = resp_cmd._peerrc_ptr;
        tmp_len = (_int32)resp_cmd._peer_rc_len;
        
#ifdef _LOGGER
        LOG_DEBUG("handle_rc_query_resp, peer_rc_num(%d).", resp_cmd._peerrc_num);
        output_rc_query_resp_for_debug(&resp_cmd);
#endif

        for (i = 0; i < resp_cmd._peerrc_num; ++i )
        {
            sd_memset(&peer_rc, 0, sizeof(PEER_RC));
            sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&res_len);
            if (tmp_len < res_len) break;
            tmp_len -= res_len; // 注意：这里跳过了一个PEER_RC结构
            sd_get_int32_from_lt(&tmp_buf, &res_len, (_int32*)&peer_rc._peerid_size);
            sd_get_bytes(&tmp_buf, &res_len, (char *)peer_rc._peerid, peer_rc._peerid_size);
            sd_get_int32_from_lt(&tmp_buf, &res_len, (_int32*)&peer_rc._peer_capacity);
            sd_get_int32_from_lt(&tmp_buf, &res_len, (_int32*)&peer_rc._ip);
            sd_get_int16_from_lt(&tmp_buf, &res_len, (_int16*)&peer_rc._tcp_port);
            sd_get_int16_from_lt(&tmp_buf, &res_len, (_int16*)&peer_rc._udp_port);
            sd_get_int8(&tmp_buf, &res_len, (_int8*)&peer_rc._res_level);
            sd_get_int8(&tmp_buf, &res_len, (_int8*)&peer_rc._res_priority);
            sd_get_int16_from_lt(&tmp_buf, &res_len, (_int16*)&peer_rc._file_subnet_type);
            sd_get_int16_from_lt(&tmp_buf, &res_len, (_int16*)&peer_rc._file_src_type);
            sd_get_int8(&tmp_buf, &res_len, (_int8*)&peer_rc._file_idx_desc_type);
            sd_get_int32_from_lt(&tmp_buf, &res_len, (_int32*)&peer_rc._file_idx_content_size);
            // 跳过file_idx_content的解析
            tmp_buf += peer_rc._file_idx_content_size;
            res_len -= peer_rc._file_idx_content_size;
            //sd_get_bytes(&tmp_buf, &res_len, (char *)peer_rc._file_idx_content, peer_rc._file_idx_content_size);
            ret = sd_get_int32_from_lt(&tmp_buf, &res_len, (_int32*)&peer_rc._cdn_type);

            if(ret != SUCCESS)
            {
                LOG_ERROR("[version = %u]extract_rc_query_resp_cmd %s failed, ret = %d.", 
                    resp_cmd._header._version, ret);
                sd_assert(FALSE);
                return ((res_query_notify_dphub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, FALSE);
            }

            if(res_len != 0)
            {
                LOG_ERROR("[version = %u]extract_rc_query_resp_cmd %s warnning, get more %d bytes data!", 
                    resp_cmd._header._version, P2P_FROM_DPHUB, res_len);
                tmp_buf += res_len;
            }

            if ((peer_rc._udp_port == 0) || (peer_rc._tcp_port == 0))
            {
                LOG_DEBUG("invalide peer resource, tcp or udt port is 0.");
                continue;
            }
            else
            {
                sd_memset((void *)to_be_add_peer_id, '\0', PEER_ID_SIZE+1);
                sd_memcpy(to_be_add_peer_id, peer_rc._peerid, PEER_ID_SIZE);
                to_be_add_peer_id[PEER_ID_SIZE] = '\0';
                LOG_DEBUG("add peer res from dphub %d, peerid = %s, peer_capability = 0x%X, ip = %u, tcp_port = %u, udp_port = %u, res_level = %u, res_priority = %u", P2P_FROM_DPHUB, 
                    to_be_add_peer_id, peer_rc._peer_capacity, peer_rc._ip, peer_rc._tcp_port, peer_rc._udp_port, peer_rc._res_level, peer_rc._res_priority);
                // 插入peer资源
                ret = add_peer_res_fun_ptr(last_cmd->_user_data, to_be_add_peer_id, (_u8*)(resp_cmd._gcid), resp_cmd._file_size, 
                    (_u8)peer_rc._peer_capacity, peer_rc._ip, peer_rc._tcp_port, peer_rc._udp_port, peer_rc._res_level
                    , P2P_FROM_DPHUB, peer_rc._res_priority);
                if (task_statu == TASK_S_RUNNING && ret == SUCCESS)
                {
                    dphub_query_context->_current_peer_rc_num++;
                }
            }
        }

        // 记录上一级节点（控制节点）
        if ((resp_cmd._uplevel_node._node_port != 0) && (resp_cmd._uplevel_node._node_host_len != 0))
        {
            LOG_DEBUG("push uplevel node to list, host(%s), port(%u).", 
                resp_cmd._uplevel_node._node_host, resp_cmd._uplevel_node._node_port);
            
            // 该节点占用的内存在查询完后释放
            ret = sd_malloc(sizeof(DPHUB_NODE_RC), (void **)&p_node_rc);
            CHECK_VALUE(ret);

            sd_memset((void *)p_node_rc, 0, sizeof(DPHUB_NODE_RC));

            // 注意：在rc_query的命令中，这个私货是_intro_id
            p_node_rc->_intro_id = (_u32)last_cmd->_extra_data;
            p_node_rc->_node_rc._node_id = resp_cmd._uplevel_node._node_id;
            p_node_rc->_node_rc._node_type = resp_cmd._uplevel_node._node_type;
            p_node_rc->_node_rc._node_host_len = resp_cmd._uplevel_node._node_host_len;
            sd_memcpy(p_node_rc->_node_rc._node_host, resp_cmd._uplevel_node._node_host, 
                resp_cmd._uplevel_node._node_host_len);
            p_node_rc->_node_rc._node_port = resp_cmd._uplevel_node._node_port;

            if (task_statu == TASK_S_RUNNING)
            {
				int ret1 = push_dphub_node_rc_into_list(p_node_rc, dphub_query_context);
				if (ret1 != SUCCESS)
				{//add by tlm
					SAFE_DELETE(p_node_rc);
				}
            }
            else
            {
                LOG_ERROR("0 Task isn't running(0x%x).", last_cmd->_user_data);
				//add by tlm
				SAFE_DELETE(p_node_rc);
            }
        }
        else
        {
            LOG_DEBUG("resp_cmd._uplevel_node._node_port(%u), resp_cmd._uplevel_node._node_host(%s).",
                resp_cmd._uplevel_node._node_port, resp_cmd._uplevel_node._node_host);
        }


        // 记录下dphub返回的，下次查询需要带上的“期望最大资源数”
        dphub_query_context->_max_res = resp_cmd._level_res;
        // 资源总数决定是否还要继续查询
        is_continue_query = (dphub_query_context->_current_peer_rc_num < resp_cmd._total_res) ? TRUE : FALSE;

        LOG_DEBUG("handle rc query cmd successful, task(0x%x), _current_peer_rc_num(%u), total_res(%u).",
            p_task, dphub_query_context->_current_peer_rc_num, resp_cmd._total_res);

        ((res_query_notify_dphub_handler)last_cmd->_callback)(last_cmd->_user_data, SUCCESS, 
            resp_cmd._result, resp_cmd._retry_interval, is_continue_query);
    }
    else
    {
        LOG_ERROR("handle_rc_query_resp failure.");
        ((res_query_notify_dphub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, FALSE);
        sd_assert(FALSE);
    }

    return SUCCESS;
}

_int32 extract_rc_node_query_resp_cmd(char *buffer, _u32 len, DPHUB_RC_NODE_QUERY_RESP *cmd)
{
    _int32 ret = SUCCESS;
    char* tmp_buf = buffer;
    _int32  tmp_len = (_int32)len;
    _int32 idx_node_rc = 0, rc_len = 0;

    sd_memset(cmd, 0, sizeof(DPHUB_RC_NODE_QUERY_RESP));
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_header._cmd_type);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._compress_len);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_header._compress_flag);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_result);

    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cid_size);
    sd_get_bytes(&tmp_buf, &tmp_len, (char *)cmd->_cid, cmd->_cid_size);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_size);
    sd_get_bytes(&tmp_buf, &tmp_len, (char *)cmd->_gcid, cmd->_gcid_size);
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_size);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_block_size);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_level_res);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_total_res);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_retry_interval);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_nr_size);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_uplevel_node._node_id);
    sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_uplevel_node._node_type);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_uplevel_node._node_host_len);
    sd_get_bytes(&tmp_buf, &tmp_len, (char *)cmd->_uplevel_node._node_host, cmd->_uplevel_node._node_host_len);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_uplevel_node._node_port);
    ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_noderc_num);
    cmd->_noderc_ptr = tmp_buf;
    for (idx_node_rc = 0; idx_node_rc < cmd->_noderc_num; ++idx_node_rc)
    {
        sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&rc_len);
        cmd->_node_rc_len += (rc_len + 4);
        tmp_len -= rc_len;
        tmp_buf += rc_len;
    }

    if(ret != SUCCESS)
    {
        LOG_ERROR("[version = %u]extract_rc_node_query_resp_cmd failed, ret = %d.", 
            cmd->_header._version, ret);
        return ERR_RES_QUERY_INVALID_COMMAND;
    }
    if(tmp_len > 0)
    {
        LOG_ERROR("[version = %u]extract_rc_node_query_resp_cmd, but last %d bytes is unknowned how to extract.", 
            cmd->_header._version, tmp_len);
    }
    return SUCCESS;
}

_int32 handle_rc_node_query_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{
    DPHUB_RC_NODE_QUERY_RESP resp_cmd;
    _int32 ret = SUCCESS;
    _int32 tmp_len = 0, i = 0, res_len = 0;
    char *tmp_buf = NULL;
    NODE_RC node_rc;
    DPHUB_NODE_RC *p_node_rc = NULL;
    TASK *p_task = 0;
    enum TASK_STATUS task_statu = 0;
    DPHUB_QUERY_CONTEXT *dphub_query_context = 0;

    LOG_DEBUG("handle_rc_node_query_resp.");
    if (last_cmd->_user_data == NULL)
    {
        return SUCCESS;
    }

    p_task = ((RES_QUERY_PARA *)last_cmd->_user_data)->_task_ptr;
    if (p_task != 0) 
    {
        task_statu = p_task->task_status;
        ret = get_dphub_query_context(p_task, (RES_QUERY_PARA *)last_cmd->_user_data, &dphub_query_context);
        CHECK_VALUE(ret);
    }

    // 如果任务已经暂停或结束了，不再处理查询回来的结果
    if (task_statu != TASK_S_RUNNING)
    {
        LOG_ERROR("TASK IS STOPED!! status(%d).", task_statu != TASK_S_RUNNING);
        return SUCCESS;
    }

    sd_memset(&resp_cmd, 0, sizeof(DPHUB_RC_NODE_QUERY_RESP));
    if (extract_rc_node_query_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
    {
        ((res_query_notify_dphub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, FALSE);
    }
    else if ((last_cmd->_cmd_seq == resp_cmd._header._seq) && (resp_cmd._result == 0))
    {
        // 先插入本一级的控制节点，供后续查询
        tmp_buf = resp_cmd._noderc_ptr;
        tmp_len = (_int32)resp_cmd._node_rc_len;

#ifdef _LOGGER
        LOG_DEBUG("handle_rc_node_query_resp, node_rc_num(%d).", resp_cmd._noderc_num);
        output_rc_node_query_resp_for_debug(&resp_cmd);
#endif

        for (i = 0; i < resp_cmd._noderc_num; ++i )
        {
            sd_memset(&node_rc, 0, sizeof(NODE_RC));
            sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&res_len);
            if (tmp_len < res_len) break;
            tmp_len -= res_len; // 注意：这里跳过了一个NODE_RC结构
            sd_get_int32_from_lt(&tmp_buf, &res_len, (_int32*)&node_rc._node_id);
            sd_get_int8(&tmp_buf, &res_len, (_int8*)&node_rc._node_type);
            sd_get_int32_from_lt(&tmp_buf, &res_len, (_int32*)&node_rc._node_host_len);
            sd_get_bytes(&tmp_buf, &res_len, (char *)node_rc._node_host, node_rc._node_host_len);
            ret = sd_get_int16_from_lt(&tmp_buf, &res_len, (_int16*)&node_rc._node_port);

            if(ret != SUCCESS)
            {
                LOG_ERROR("[version = %u]handle_rc_node_query_resp %s failed, ret = %d.", 
                    resp_cmd._header._version, ret);
                sd_assert(FALSE);
                return ((res_query_notify_dphub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, FALSE);
            }

            if(res_len != 0)
            {
                LOG_ERROR("[version = %u]handle_rc_node_query_resp %s warnning, get more %d bytes data!", 
                    resp_cmd._header._version, P2P_FROM_DPHUB, res_len);
                tmp_buf += res_len;
            }

            if (node_rc._node_port == 0)
            {
                LOG_DEBUG("invalide node resource, port is 0.");
                continue;
            }
            else
            {
                // 将这些个节点再次插入到node集合中
                // 还有上级控制节点，需要再次保存起来，以备后查
                LOG_DEBUG("push node rc into list.");

                // 该节点占用的内存在查询完后释放
                ret = sd_malloc(sizeof(DPHUB_NODE_RC), (void **)&p_node_rc);
                CHECK_VALUE(ret);

                sd_memset((void *)p_node_rc, 0, sizeof(DPHUB_NODE_RC));

                // 注意：在rc_query(rc_node_query)的命令中，这个私货是_intro_id
                p_node_rc->_intro_id = (_u32)last_cmd->_extra_data;
                p_node_rc->_node_rc._node_id = node_rc._node_id;
                p_node_rc->_node_rc._node_type = node_rc._node_type;
                p_node_rc->_node_rc._node_host_len = node_rc._node_host_len;
                sd_memcpy(p_node_rc->_node_rc._node_host, node_rc._node_host, 
                    node_rc._node_host_len);
                p_node_rc->_node_rc._node_port = node_rc._node_port;

                if (task_statu == TASK_S_RUNNING)
                {
					int ret1 = push_dphub_node_rc_into_list(p_node_rc, dphub_query_context);
					if (ret1 != SUCCESS)
					{//add by tlm
						SAFE_DELETE(p_node_rc);
					}
				}
                else
                {
                    LOG_ERROR("1 Task isn't running(0x%x).", last_cmd->_user_data);
					//add by tlm
					SAFE_DELETE(p_node_rc);

				}            
            }
        }

        // 再插入上一级的控制节点，供以后查询
        if (resp_cmd._uplevel_node._node_port != 0)
        {
            // 还有上级控制节点，需要再次保存起来，以备后查
            LOG_DEBUG("push uplevel node to list.");

            // 该节点占用的内存在查询完后释放
            ret = sd_malloc(sizeof(DPHUB_NODE_RC), (void **)&p_node_rc);
            CHECK_VALUE(ret);

            sd_memset((void *)p_node_rc, 0, sizeof(DPHUB_NODE_RC));

            // 注意：在rc_query(rc_node_query)的命令中，这个私货是_intro_id
            p_node_rc->_intro_id = (_u32)last_cmd->_extra_data;
            p_node_rc->_node_rc._node_id = resp_cmd._uplevel_node._node_id;
            p_node_rc->_node_rc._node_type = resp_cmd._uplevel_node._node_type;
            p_node_rc->_node_rc._node_host_len = resp_cmd._uplevel_node._node_host_len;
            sd_memcpy(p_node_rc->_node_rc._node_host, resp_cmd._uplevel_node._node_host, 
                resp_cmd._uplevel_node._node_host_len);
            p_node_rc->_node_rc._node_port = resp_cmd._uplevel_node._node_port;

            if (task_statu == TASK_S_RUNNING)
            {
				int ret1 = push_dphub_node_rc_into_list(p_node_rc, dphub_query_context);
				if (ret1 != SUCCESS)
				{//add by tlm
					SAFE_DELETE(p_node_rc);
				}
            }
            else
            {
                LOG_ERROR("2 Task isn't running(0x%x).", last_cmd->_user_data);
				//add by tlm
				SAFE_DELETE(p_node_rc);

			}
        }
        else
        {
            LOG_DEBUG("resp_cmd._uplevel_node._node_port(%u), resp_cmd._uplevel_node._node_host(%s).",
                resp_cmd._uplevel_node._node_port, resp_cmd._uplevel_node._node_host);
        }

        LOG_DEBUG("handle rc node query cmd successful, continue to query peer rc.");
        ((res_query_notify_dphub_handler)last_cmd->_callback)(last_cmd->_user_data, SUCCESS, 
            resp_cmd._result, resp_cmd._retry_interval, TRUE);
    }
    else
    {
        sd_assert(FALSE);
        LOG_ERROR("handle_rc_node_query_resp failure.");
        ((res_query_notify_dphub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, FALSE);
    }

    return SUCCESS;
}

///////////////////////////////////////////////// RSA /////////////////////////////////////////////
#ifdef _RSA_RES_QUERY

_int32 build_dphub_query_owner_node_cmd_rsa(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_OWNER_QUERY *cmd, 
	_u8* aes_key, _int32 pubkey_version)
{
	static _u32 seq = 0;
	_u32 query_owner_cmd_len = 0;
	_int32 ret;
	char *tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;

	query_owner_cmd_len = 4;
	build_dphub_cmd_req_header(&cmd->_req_header, seq++, query_owner_cmd_len, DPHUB_OWNER_QUERY_ID);    
	*len = HUB_CMD_HEADER_LEN + cmd->_req_header._header._cmd_len;
	encode_len = (cmd->_req_header._header._cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	LOG_DEBUG("build_dphub_query_owner_node_cmd, hub_type=%d", device->_hub_type);
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len, device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	ret = sd_malloc(http_header_len + encode_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_dphub_query_owner_node_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN;
	tmp_len = (_int32)*len;

	// 填充req_header
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._cmd_len);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_req_header._header._cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._compress_len);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_req_header._header._compress_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)&(cmd->_req_header._peerid), (_int32)cmd->_req_header._peerid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._peer_capacity);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._region_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._parent_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._pi_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._partner_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_req_header._product_info._partner_id, (_int32)cmd->_req_header._product_info._partner_id_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._product_release_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._product_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._thunder_version_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_req_header._product_info._thunder_version, (_int32)cmd->_req_header._product_info._thunder_version_size);

	// 填充cmd的特有字段
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_last_query_stamp);

#ifdef _LOGGER
	output_query_owner_node_for_debug(cmd);
#endif

	ret = xl_aes_encrypt(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_dphub_query_owner_node_cmd, but aes_encrypt failed. errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	ret = aes_encrypt_with_known_key(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len, aes_key);
	if(ret != SUCCESS)
	{
		LOG_ERROR("aes_encrypt failed. errcode = %d.", ret);
		sd_free(*buffer);
		sd_assert(FALSE);
		return ret;
	}
	sd_assert(tmp_len == 0);
	tmp_buf = *buffer + http_header_len;
	tmp_len = RSA_ENCRYPT_HEADER_LEN;
	ret = build_rsa_encrypt_header(&tmp_buf, &tmp_len, pubkey_version, aes_key, *len);
	if(ret != SUCCESS)
	{
		sd_free(*buffer);
		return ret;
	}
	sd_assert(tmp_len == 0);
	*len += (http_header_len + RSA_ENCRYPT_HEADER_LEN);
	LOG_DEBUG("tmp_len=%u", tmp_len);
	return SUCCESS;
}

_int32 build_dphub_rc_query_cmd_rsa(HUB_DEVICE *device, char **buffer, _u32 *len, DPHUB_RC_QUERY *cmd, _u16 cmd_type,
	_u8* aes_key, _int32 pubkey_version)
{
	static _u32 seq = 0;
	_u32 rc_query_cmd_len = 0;  // 除去_req_header之后的大小
	_int32 ret;
	char *tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;

	rc_query_cmd_len = 72 + 
		cmd->_file_rc._cid_size + 
		cmd->_file_rc._gcid_size + 
		cmd->_file_rc._file_idx_content_size;
	build_dphub_cmd_req_header(&cmd->_req_header, seq++, rc_query_cmd_len, cmd_type);
	*len = HUB_CMD_HEADER_LEN + cmd->_req_header._header._cmd_len;
	encode_len = (cmd->_req_header._header._cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	LOG_DEBUG("build_dphub_rc_query_cmd, hub_type=%d", device->_hub_type);
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len, device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	ret = sd_malloc(http_header_len + encode_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_dphub_rc_query_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN;
	tmp_len = (_int32)*len;

	// 填充req_header
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._cmd_len);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_req_header._header._cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._header._compress_len);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_req_header._header._compress_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_req_header._peerid, (_int32)cmd->_req_header._peerid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._peer_capacity);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._region_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._parent_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._pi_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._partner_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_req_header._product_info._partner_id, (_int32)cmd->_req_header._product_info._partner_id_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._product_release_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._product_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_req_header._product_info._thunder_version_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_req_header._product_info._thunder_version, (_int32)cmd->_req_header._product_info._thunder_version_size);

	// 填充cmd的特有字段
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_fr_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_rc._cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_rc._cid, (_int32)cmd->_file_rc._cid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_rc._gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_rc._gcid, (_int32)cmd->_file_rc._gcid_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_rc._file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_rc._block_size);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_file_rc._subnet_type);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_file_rc._file_src_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_file_rc._file_idx_desc_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_rc._file_idx_content_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_rc._file_idx_content, (_int32)cmd->_file_rc._file_idx_content_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_capacity);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_max_res);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_level_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_speed_up);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capacity);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ip);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_upnp_ip);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_upnp_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_nat_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_intro_node_id);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cur_peer_num);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_total_query_times);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_query_times_on_node);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_request_from);    

#ifdef _LOGGER
	output_rc_query_for_debug(cmd);
#endif

	ret = xl_aes_encrypt(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len);

	// 这里发生了内存的拷贝，原来申请的内存理应释放掉，注意！
	if (cmd->_file_rc._file_idx_content != 0)
	{
		sd_free(cmd->_file_rc._file_idx_content);
		cmd->_file_rc._file_idx_content = 0;
	}

	if(ret != SUCCESS)
	{
		LOG_ERROR("build_dphub_rc_query_cmd, but aes_encrypt failed. errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	ret = aes_encrypt_with_known_key(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len, aes_key);
	if(ret != SUCCESS)
	{
		LOG_ERROR("aes_encrypt failed. errcode = %d.", ret);
		sd_free(*buffer);
		sd_assert(FALSE);
		return ret;
	}
	sd_assert(tmp_len == 0);
	tmp_buf = *buffer + http_header_len;
	tmp_len = RSA_ENCRYPT_HEADER_LEN;
	ret = build_rsa_encrypt_header(&tmp_buf, &tmp_len, pubkey_version, aes_key, *len);
	if(ret != SUCCESS)
	{
		sd_free(*buffer);
		return ret;
	}
	sd_assert(tmp_len == 0);
	*len += (http_header_len + RSA_ENCRYPT_HEADER_LEN);
	LOG_DEBUG("tmp_len=%u", tmp_len);
	return SUCCESS;
}
#endif

