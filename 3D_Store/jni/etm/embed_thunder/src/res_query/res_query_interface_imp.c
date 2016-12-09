#include "utility/mempool.h"
#include "utility/peer_capability.h"
#include "utility/peerid.h"
#include "utility/local_ip.h"
#include "utility/utility.h"
#include "utility/version.h"
#include "utility/logid.h"
#include "utility/sd_iconv.h"
#include "utility/settings.h"
#define LOGID LOGID_RES_QUERY
#include "utility/logger.h"
#include "asyn_frame/device.h"
#include "platform/sd_network.h"

#include "res_query_interface_imp.h"
#include "embed_thunder_version.h"

#include "res_query_setting.h"
#include "res_query_impl.h"
#include "res_query_cmd_define.h"
#include "res_query_cmd_builder.h"
#include "res_query_cmd_handler.h"
#include "res_query_dphub.h"
#include "emule/emule_query/emule_query_emule_tracker.h"
#include "emule/emule_utility/emule_peer.h"
#include "emule/emule_impl.h"
#include "socket_proxy_interface.h"
#include "p2p_transfer_layer/ptl_utility.h"
#include "p2p_data_pipe/p2p_utility.h"
#include "download_task/download_task.h"
#include "res_query_cmd_builder.h"
#include "res_query_security.h"

#ifdef _DK_QUERY
#include "res_query/dk_res_query/dk_manager_interface.h"
#endif

#define DEFAULT_URL "http://127.0.0.1"

#define NODE_TYPE_RESOURCE_NODE 0
#define NODE_TYPE_CONTROL_NODE 1

static HUB_DEVICE g_shub;
static HUB_DEVICE g_phub;
static HUB_DEVICE g_partner_cdn;
static HUB_DEVICE g_tracker;
static HUB_DEVICE g_bt_hub;
static HUB_DEVICE g_dphub_root;       // root hub -- 查询根节点
static LIST g_dphub_node_list;  // 存放所有已经发出过请求的设备指针
static HUB_DEVICE g_emule_tracker;

#ifdef ENABLE_CDN
static HUB_DEVICE g_cdn_manager;
static HUB_DEVICE g_normal_cdn_manager; // 支持普通cdn的查询设备
#ifdef ENABLE_KANKAN_CDN
static HUB_DEVICE g_kankan_cdn_manager;
#endif
#ifdef ENABLE_HSC
static HUB_DEVICE g_vip_hub;
#endif /* ENABLE_HSC */
#endif
#ifdef ENABLE_EMULE
static HUB_DEVICE g_emule_hub;
#endif
static HUB_DEVICE g_config_hub;

void dump_buffer(char* buffer, _u32 length)
{
#if 0//def _DEBUG
	char tmp_buffer[100] = {0};
	char* ptr = tmp_buffer;
	_u32 i;
	for( i = 0; i < length;)
	{
		char hexbuff[4] = {0};
		char2hex(buffer[i], hexbuff, 4);

		sd_strcat(tmp_buffer, hexbuff, 2);
		sd_strcat(tmp_buffer, " ", 1);
		 i++;
		if(i % 16 ==0)
		{
			LOG_DEBUG("%s", tmp_buffer);
			memset(tmp_buffer, 0, sizeof(tmp_buffer));
		}
	}

	LOG_DEBUG("%s", tmp_buffer);
#endif
}

static _int32 res_query_dphub_node(void *user_data, res_query_notify_dphub_handler handler, 
                            _u64 file_size, _u32 block_size, _u16 file_src_type, 
                            const char *cid, const char *gcid, _u8 file_idx_desc_type,
                            _u32 file_idx_content_cnt, const char *file_idx_content, _u32 node_type, 
                            _u32 intro_node_id, _u16 req_from, HUB_DEVICE *hub_device);
                            
static _int32 force_close_hub_device_res(HUB_DEVICE* device)
{
#if 1
		return 0;
#else
    if (device->_socket != INVALID_SOCKET)//如果socket不为INVALID_SOCKET的话，那么一定有操作在上面
    {
#ifdef _DEBUG
        _u32 count = 0;
        socket_proxy_peek_op_count(device->_socket, DEVICE_SOCKET_TCP, &count);
        sd_assert(count == 1);		
#endif
        socket_proxy_cancel(device->_socket, DEVICE_SOCKET_TCP);
        device->_socket = INVALID_SOCKET;
	}

	return SUCCESS;
#endif
}

static _int32 init_hub_device(HUB_DEVICE* device, HUB_TYPE type)
{
#if 1
		return 0;
#else
	LOG_DEBUG("type = %d.", type);
	sd_memset(device, 0, sizeof(HUB_DEVICE));
	device->_hub_type = type;
	device->_socket = INVALID_SOCKET;
	list_init(&device->_cmd_list);
	device->_timeout_id = INVALID_MSG_ID;
	return SUCCESS;
#endif
}

static _int32 uninit_hub_device(HUB_DEVICE* device)
{
#if 1
		return 0;
#else
	LOG_DEBUG("type = %d.", device->_hub_type);

	force_close_hub_device_res(device);
	SAFE_DELETE(device->_recv_buffer);
	SAFE_DELETE(device->_last_cmd);
	
	//反初始化的时候，上层任务需要把所有的请求都取消
	while (list_size(&device->_cmd_list) > 0)
	{
		RES_QUERY_CMD* cmd = NULL;
		list_pop(&device->_cmd_list, (void**)&cmd);
		sd_free(cmd);
		cmd = NULL;
	}
	
	if (device->_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(device->_timeout_id);
		device->_timeout_id = INVALID_MSG_ID;
	}
	return SUCCESS;
#endif
}

static _int32 res_query_config()
{	
#if 1
		return 0;
#else
	QUERY_CONFIG_CMD cmd;
	sd_memset(&cmd, 0, sizeof(QUERY_CONFIG_CMD));
	cmd._local_peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._local_peerid, PEER_ID_SIZE + 1);	
	cmd._product_flag = get_product_flag();	

    sd_get_os((char*)cmd._os_details, MAX_OS_LEN);
	cmd._os_details_len = sd_strlen(cmd._os_details);

	settings_get_str_item("system.ui_version",  (char*)cmd._ui_version);
	cmd._ui_version_len = sd_strlen(cmd._ui_version);

	cmd._thunder_version_len = sd_strlen(ET_INNER_VERSION);
	sd_memcpy(cmd._thunder_version, ET_INNER_VERSION, cmd._thunder_version_len );
	
	cmd._network_type = sd_get_global_net_type();

#ifdef ENABLE_BT
    cmd._enable_bt = 1;
#endif

#ifdef ENABLE_EMULE
    cmd._enable_emule = 1;
#endif

	settings_get_int_item("query_config.timestamp", (_int32*)&cmd._timestamp);
	settings_get_str_item("query_config.section_filter", cmd._config_section_filter);
	cmd._config_section_filter_len = sd_strlen(cmd._config_section_filter);
	
	if ( 0==cmd._config_section_filter_len)
	{
#ifdef MOBILE_PHONE	
		sd_strncpy(cmd._config_section_filter, "upload_manager,relation_config", sizeof(cmd._config_section_filter)-1);
#else
		sd_strncpy(cmd._config_section_filter, "upload_manager_box,relation_config", sizeof(cmd._config_section_filter)-1);
#endif
		cmd._config_section_filter_len = sd_strlen(cmd._config_section_filter);
	}	

    _u32 local_ip = 0;
    char* buffer = NULL;
    _u32 len = 0;
    _int32 ret = build_query_config_cmd(&g_config_hub, &buffer, &len, &cmd);
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_config_hub, QUERY_CONFIG, buffer, len, NULL, NULL, cmd._header._seq, FALSE, NULL, NULL);
#endif
}

static _int32 res_query_register()
{	
#if 1
		return 0;
#else
	ENROLLSP1_CMD cmd;
	_u8 aes_key[16];

	sd_memset(&cmd, 0, sizeof(ENROLLSP1_CMD));
	cmd._local_peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._local_peerid, PEER_ID_SIZE + 1);
	_u32 local_ip = sd_get_local_ip();
	_int32 ret = sd_inet_ntoa(local_ip, cmd._internal_ip, 24);
	sd_assert(ret == SUCCESS);
	cmd._internal_ip_len = sd_strlen(cmd._internal_ip);
	cmd._product_flag = get_product_flag();
	cmd._thunder_version_len = 1;
	cmd._thunder_version = 0;
	cmd._network_type = -1;
	cmd._os_type = -1;
	cmd._os_details_len = 0;
	cmd._user_details_len = 0;
	cmd._plugin_size = 0;
	cmd._enable_login_control = 64472286;
	get_partner_id(cmd._partner_id, 32);
	cmd._partner_id_len = PARTNER_ID_LEN;
	char* buffer = NULL;
	_u32 len = 0;
	sd_srand(time(NULL));

#ifdef _RSA_RES_QUERY
	gen_aes_key_by_user_data((void*)sd_rand(), aes_key);
	ret = build_enrollsp1_cmd_rsa(&g_shub, &buffer, &len, &cmd, aes_key, RSA_PUBKEY_VERSION_SHUB);
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_shub, ENROLLSP1, buffer, len, NULL, NULL, cmd._header._seq, FALSE, NULL, aes_key);
#else
    ret = build_enrollsp1_cmd(&g_shub, &buffer, &len, &cmd);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_shub, ENROLLSP1, buffer, len, NULL, NULL, cmd._header._seq, FALSE, NULL, NULL);
#endif
#endif
}

_int32 init_res_query_module()
{
#if 1
		return 0;
#else
    LOG_DEBUG("init_res_query_module...");
	_int32 ret = init_res_query_setting();
	CHECK_VALUE(ret);
	
    list_init(&g_dphub_node_list);
	init_hub_device(&g_shub, SHUB);
	init_hub_device(&g_phub, PHUB);
	init_hub_device(&g_partner_cdn, PARTNER_CDN);
	init_hub_device(&g_tracker, TRACKER);
    init_hub_device(&g_dphub_root, DPHUB_ROOT);
	init_hub_device(&g_emule_tracker, EMULE_TRACKER);
	init_hub_device(&g_config_hub, CONFIG_HUB);
	
#ifdef ENABLE_CDN
	init_hub_device(&g_cdn_manager, CDN_MANAGER);
	init_hub_device(&g_normal_cdn_manager, NORMAL_CDN_MANAGER);
#ifdef ENABLE_KANKAN_CDN
	init_hub_device(&g_kankan_cdn_manager, KANKAN_CDN_MANAGER);	
#endif
#ifdef ENABLE_HSC
	init_hub_device(& g_vip_hub, VIP_HUB);
#endif /* ENABLE_HSC */
#endif

#ifdef ENABLE_BT
	init_hub_device(&g_bt_hub, BT_HUB);
#ifdef ENABLE_BT_PROTOCOL
	init_bt_tracker();
#endif
#endif

#ifdef ENABLE_EMULE
	init_hub_device(&g_emule_hub, EMULE_HUB);
#endif

#ifdef _DK_QUERY
	dk_init_module();
#endif

    res_query_config();
    
	init_dphub_module();

//NOTE:jieouy按照以前人写的代码，无线下载库不需要进行enroll，个人觉得值得商榷。	
#ifndef MOBILE_PHONE
    res_query_register();
#endif
	
	return res_query_dphub_root(handle_res_query_notify_dphub_root, get_g_region_id(), 
        get_g_parent_id(), get_g_last_query_dphub_stamp());
#endif
}

_int32 uninit_res_query_module()
{
#if 1
		return 0;
#else
	LOG_DEBUG("uninit_res_query_module...");
	uninit_hub_device(&g_shub);
	uninit_hub_device(&g_phub);
	uninit_hub_device(&g_partner_cdn);
	uninit_hub_device(&g_tracker);
    uninit_hub_device(&g_dphub_root);
    list_clear(&g_dphub_node_list);
	uninit_hub_device(&g_emule_tracker);
#ifdef ENABLE_CDN
	uninit_hub_device(&g_cdn_manager);
	uninit_hub_device(&g_normal_cdn_manager);
#ifdef ENABLE_KANKAN_CDN
	uninit_hub_device(&g_kankan_cdn_manager);
#endif
#ifdef ENABLE_HSC
	uninit_hub_device(& g_vip_hub);
#endif /* ENABLE_HSC */
#endif
#ifdef ENABLE_BT
	uninit_hub_device(&g_bt_hub);
#ifdef ENABLE_BT_PROTOCOL
	uninit_bt_tracker();
#endif

#endif
#ifdef ENABLE_EMULE
	uninit_hub_device(&g_emule_hub);
#endif

#ifdef _DK_QUERY
	dk_uninit_module();
#endif

    uninit_dphub_module();
    
	return SUCCESS;
#endif
}

_int32 force_close_res_query_module_res()
{
#if 1
		return 0;
#else
	LOG_DEBUG("force_close_res_query_module_res...");
	force_close_hub_device_res(&g_shub);
	force_close_hub_device_res(&g_phub);
	force_close_hub_device_res(&g_partner_cdn);
	force_close_hub_device_res(&g_tracker);
    force_close_hub_device_res(&g_dphub_root);
	force_close_hub_device_res(&g_emule_tracker);
#ifdef ENABLE_CDN
	force_close_hub_device_res(&g_cdn_manager);
	force_close_hub_device_res(&g_normal_cdn_manager);
#ifdef ENABLE_KANKAN_CDN
	force_close_hub_device_res(&g_kankan_cdn_manager);
#endif
#ifdef ENABLE_HSC
	force_close_hub_device_res(& g_vip_hub);
#endif /*ENABLE_HSC */
#endif
#ifdef ENABLE_BT
	force_close_hub_device_res(&g_bt_hub);
#endif
#ifdef ENABLE_EMULE
	force_close_hub_device_res(&g_emule_hub);
#endif
       return SUCCESS;
#endif
}

_int32 res_query_register_add_resource_handler(res_query_add_server_res_handler server_handler
    , res_query_add_peer_res_handler peer_handler
    , res_query_add_relation_fileinfo add_relation_fileinfo)
{
#if 1
		return 0;
#else
	return set_add_resource_func(server_handler, peer_handler, add_relation_fileinfo);
#endif
}

static _int32 res_query_shub_by_cid_ex(void* user_data, res_query_notify_shub_handler handler, const _u8* cid, _u64 file_size, BOOL is_gcid, const char* url_or_gcid,
							 BOOL need_bcid,_int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn, _u8 cid_query_type, BOOL relation_query_res, _u32 relation_index)
{	
#if 1
		return 0;
#else
	char* buffer = NULL;
	_u32 len = 0;
	_int32 ret;
	/*fill request package*/
	QUERY_SERVER_RES_CMD query_res_cmd;
	_u8 aes_key[16];
#ifdef _DEBUG
{
		char cid_t[41],gcid_t[41];
		sd_memset(cid_t,0x00,41);
		sd_memset(gcid_t,0x00,41);
		str2hex((char*)cid, CID_SIZE, cid_t, 40);
		cid_t[40] = '\0';
		if(is_gcid)
		{
			str2hex((char*)url_or_gcid, CID_SIZE, gcid_t, 40);
			gcid_t[40] = '\0';
		}
		LOG_ERROR("res_query_shub_by_cid, cid = %s, gcid = %s, is_gcid,=%d,file_size = %llu", cid_t, gcid_t,is_gcid,file_size);
#ifdef IPAD_KANKAN
		printf("\nres_query_shub_by_cid, cid = %s, gcid = %s, is_gcid,=%d,file_size = %llu\n", cid_t, gcid_t,is_gcid,file_size);
#endif
}
#endif
	LOG_DEBUG("res_query_shub_by_cid, user_data = 0x%x.",user_data);
	sd_memset(&query_res_cmd, 0, sizeof(QUERY_SERVER_RES_CMD));
	query_res_cmd._by_what = QUERY_BY_CID;
	if(!need_bcid)
		query_res_cmd._by_what |= QUERY_NO_BCID;
	if(is_gcid)
	{
		query_res_cmd._by_what |= QUERY_WITH_GCID;
		query_res_cmd._url_or_gcid_size = CID_SIZE;
		sd_memcpy(query_res_cmd._url_or_gcid, url_or_gcid, CID_SIZE);
	}
	else
	{
		query_res_cmd._url_or_gcid_size = sd_strlen(url_or_gcid);
		if(query_res_cmd._url_or_gcid_size > MAX_URL_LEN)
			return -1;
		sd_memcpy(query_res_cmd._url_or_gcid, url_or_gcid, query_res_cmd._url_or_gcid_size + 1);
	}
        if (!cid)
        {
	    LOG_ERROR("res_query_shub_by_cid, cid or gcid is null.");
           return -1;
        }
    
	query_res_cmd._cid_size = CID_SIZE;
	sd_memcpy(query_res_cmd._cid, cid, CID_SIZE);
	query_res_cmd._file_size = file_size;
	query_res_cmd._origin_url_size = sd_strlen(DEFAULT_URL);
	if(query_res_cmd._origin_url_size > MAX_URL_LEN)
		return -1;
	sd_memcpy(query_res_cmd._origin_url, DEFAULT_URL, query_res_cmd._origin_url_size + 1);
	query_res_cmd._max_server_res = max_server_res;
	query_res_cmd._bonus_res_num = bonus_res_num;
	query_res_cmd._peerid_size = PEER_ID_SIZE;
	get_peerid(query_res_cmd._peerid, PEER_ID_SIZE + 1);
	query_res_cmd._peer_report_ip = sd_get_local_ip();
	query_res_cmd._peer_capability = get_peer_capability();
	query_res_cmd._query_times_sn = query_times_sn;
	query_res_cmd._cid_query_type = cid_query_type;
	query_res_cmd._refer_url_size = sd_strlen(DEFAULT_URL);
	if(query_res_cmd._refer_url_size >= MAX_URL_LEN)
		return -1;
	sd_memcpy(query_res_cmd._refer_url, DEFAULT_URL, query_res_cmd._refer_url_size);
	query_res_cmd._partner_id_size = PARTNER_ID_LEN;
	get_partner_id(query_res_cmd._partner_id, PARTNER_ID_LEN);
	query_res_cmd._product_flag = get_product_flag();	
#ifdef _RSA_RES_QUERY
	/*build command*/
	gen_aes_key_by_user_data(user_data, aes_key);
	ret = build_query_server_res_cmd_rsa(&g_shub,&buffer, &len, &query_res_cmd, aes_key, RSA_PUBKEY_VERSION_SHUB);
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_shub, QUERY_SERVER_RES, buffer, len, (void*)handler, user_data, query_res_cmd._header._seq,FALSE, NULL, aes_key);			//commit a request
#else
    ret = build_query_server_res_cmd(&g_shub,&buffer, &len, &query_res_cmd);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_shub, QUERY_SERVER_RES, buffer, len, (void*)handler, user_data, query_res_cmd._header._seq,FALSE, NULL, NULL);			//commit a request
#endif

#endif
}

_int32 res_query_shub_by_cid(void* user_data, res_query_notify_shub_handler handler, const _u8* cid, _u64 file_size, BOOL is_gcid, const char* url_or_gcid,
							 BOOL need_bcid,_int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn, _u8 cid_query_type)
{
#if 1
		return 0;
#else
	return res_query_shub_by_cid_ex(user_data,  handler,  cid,  file_size,  is_gcid,   url_or_gcid,
							  need_bcid, max_server_res,  bonus_res_num,  query_times_sn,  cid_query_type,
							  FALSE, 0);
#endif
}

_int32 res_query_shub_by_resinfo_newcmd(void* user_data, res_query_notify_shub_handler handler, const _u8* cid, _u64 file_size, BOOL is_gcid, const char* gcid,
							 BOOL need_bcid,_int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn, _u8 cid_query_type)
{	
#if 1
		return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;
	_u8 aes_key[16];
	/*fill request package*/
	NEWQUERY_SERVER_RES_CMD query_res_cmd;
#ifdef _DEBUG
{
		char cid_t[41],gcid_t[41];
		sd_memset(cid_t,0x00,41);
		sd_memset(gcid_t,0x00,41);
		str2hex((char*)cid, CID_SIZE, cid_t, 40);
		cid_t[40] = '\0';
		if(is_gcid)
		{
			str2hex((char*)gcid, CID_SIZE, gcid_t, 40);
			gcid_t[40] = '\0';
		}
		
		LOG_ERROR("res_query_shub_by_resinfo_newcmd, cid = %s, gcid = %s, is_gcid,=%d,file_size = %llu", cid_t, gcid_t,is_gcid,file_size);
#ifdef IPAD_KANKAN
		printf("\res_query_shub_by_resinfo_newcmd, cid = %s, gcid = %s, is_gcid,=%d,file_size = %llu\n", cid_t, gcid_t,is_gcid,file_size);
#endif
}
#endif
	LOG_DEBUG("res_query_shub_by_resinfo_newcmd, user_data = 0x%x.",user_data);
	sd_memset(&query_res_cmd, 0, sizeof(NEWQUERY_SERVER_RES_CMD));
	build_reservce_60ver(&query_res_cmd._reserver_buffer, &query_res_cmd._reserver_length);

        if (!cid || !gcid)
        {
	    LOG_ERROR("res_query_shub_by_resinfo_newcmd, cid or gcid is null.");
           return -1;
        }

	query_res_cmd._cid_size = CID_SIZE;
	sd_memcpy(query_res_cmd._cid, cid, CID_SIZE);
	

	query_res_cmd._file_size = file_size;

	query_res_cmd._gcid_size = CID_SIZE;
	sd_memcpy(	query_res_cmd._gcid, gcid, CID_SIZE);

	query_res_cmd._gcid_level = 90;

	query_res_cmd._assist_url_size = 0;
	query_res_cmd._assist_url_code_page = -1;

	query_res_cmd._origion_url_size = 0;
	query_res_cmd._origion_url_code_page = -1;

	query_res_cmd._ref_url_size = 0;
	query_res_cmd._ref_url_code_page = -1;
	query_res_cmd._cid_query_type = cid_query_type;
	query_res_cmd._max_server_res = max_server_res;
	query_res_cmd._bonus_res_num = bonus_res_num;
	query_res_cmd._peerid_length = PEER_ID_SIZE;
	get_peerid(query_res_cmd._peerid, PEER_ID_SIZE + 1);
	query_res_cmd._peerid[PEER_ID_SIZE] = '\0';
	query_res_cmd._local_ip = sd_get_local_ip();
	query_res_cmd._query_sn = query_times_sn + 1;
	query_res_cmd._file_suffix_size = 0;
#ifdef _DEBUG
	LOG_DEBUG("res_query_shub_by_resinfo_newcmd gcid level:%d, _cid_query_type:%d, _max_server_res:%d, _bonus_res_num:%d, 	_peerid:%s, _local_ip:%d, _query_sn:%d, suffix:", 
		(_u32)query_res_cmd._gcid_level, (_u32)query_res_cmd._cid_query_type,
		(_u32)query_res_cmd._max_server_res,(_u32)query_res_cmd._bonus_res_num,query_res_cmd._peerid,(_u32)query_res_cmd._local_ip
		,(_u32)query_res_cmd._query_sn);
#endif
	
#ifdef _RSA_RES_QUERY
	/*build command*/
	gen_aes_key_by_user_data(user_data, aes_key);
	ret = build_query_newserver_res_cmd_rsa(&g_shub,&buffer, &len, &query_res_cmd, aes_key, RSA_PUBKEY_VERSION_SHUB);
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_shub, QUERY_NEW_RES_CMD_ID, buffer, len, (void*)handler, user_data, query_res_cmd._header._seq,FALSE, NULL, aes_key);			//commit a request
#else
    ret = build_query_newserver_res_cmd(&g_shub,&buffer, &len, &query_res_cmd);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_shub, QUERY_NEW_RES_CMD_ID, buffer, len, (void*)handler, user_data, query_res_cmd._header._seq,FALSE, NULL, NULL);			//commit a request
#endif
#endif
}


_int32 relation_res_query_shub(void * user_data, res_query_notify_shub_handler handler, const char * url, const char * refer_url, char cid [ CID_SIZE ], char gcid [ CID_SIZE ], _u64 file_size, _int32 max_res, _int32 query_times_sn)
{
#if 1
		return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;

	
	/*fill request package*/
	QUERY_FILE_RELATION_RES_CMD  cmd;
	LOG_DEBUG("relation_res_query_shub, user_data = 0x%x", user_data);
	sd_memset(&cmd, 0, sizeof(QUERY_FILE_RELATION_RES_CMD));
	cmd._assist_url_code_page = -1;
	cmd._original_url_code_page = -1;
	cmd._ref_url_code_page = -1;

	if(url != NULL)
	{
		cmd._original_url_length = sd_strlen(url);
		if(cmd._original_url_length >= URL_MAX_LEN)
		{
			LOG_DEBUG("relation_res_query_shub,  url length not valid...");
			return -1;
		}
	
		sd_strncpy(cmd._original_url, url, URL_MAX_LEN - 1);	
	}

	if(refer_url != NULL)
	{
		cmd._ref_url_length = sd_strlen(refer_url);
		if(cmd._ref_url_length >= URL_MAX_LEN)
		{
			LOG_DEBUG("relation_res_query_shub,  _ref_url_length length not valid..."); 
			return -1;
		}
	
		sd_strncpy(cmd._ref_url, refer_url, URL_MAX_LEN - 1);
	}


#ifdef _DEBUG
	if(url != NULL)
	{
		LOG_DEBUG("relation_res_query_shub,  origion url:%s", url);
	}

	if(refer_url != NULL)
	{
		LOG_DEBUG("relation_res_query_shub,  refer_url url:%s", refer_url);
	}
#endif

        if (!cid || !gcid)
        {
	    LOG_ERROR("relation_res_query_shub, cid or gcid is null.");
           return -1;
        }

	 cmd._cid_size = CID_SIZE;
	 sd_memcpy(cmd._cid, cid, CID_SIZE);

	 cmd._gcid_size = CID_SIZE;
	 sd_memcpy(cmd._gcid, gcid, CID_SIZE);

	 cmd._file_size = file_size;

	 cmd._max_relation_resource_num = max_res;
	 cmd._gcid_level =  90 ;

	 cmd._query_sn = query_times_sn;
	 cmd._peerid_length= PEER_ID_SIZE;
	 get_peerid(cmd._peerid, PEER_ID_SIZE + 1);

	 cmd._local_ip =  sd_get_local_ip();

#ifdef _DEBUG
	char cidbuff[41], gcidbuff[41];
	sd_memset(cidbuff, 0, sizeof(cidbuff));
	sd_memset(gcidbuff, 0, sizeof(gcidbuff));
	str2hex((char*)cid, CID_SIZE, cidbuff, 40);
	str2hex((char*)gcid, CID_SIZE, gcidbuff, 40);	

	LOG_DEBUG("relation_res_query_shub cid:%s, gcid:%s filesize:%llu", cidbuff, gcidbuff, file_size);

	LOG_DEBUG("relation_res_query_shub _assist_url:%s", cmd._assist_url);
	LOG_DEBUG("relation_res_query_shub _original_url:%s", cmd._original_url);
	LOG_DEBUG("relation_res_query_shub _ref_url:%s", cmd._ref_url);
	LOG_DEBUG("relation_res_query_shub _max_relation_resource_num:%u", cmd._max_relation_resource_num);
	LOG_DEBUG("relation_res_query_shub _peerid:%s", cmd._peerid);
	LOG_DEBUG("relation_res_query_shub _local_ip:%d", cmd._local_ip);
	LOG_DEBUG("relation_res_query_shub _query_sn:%d", cmd._query_sn);
	LOG_DEBUG("relation_res_query_shub _file_suffilx:%s", cmd._file_suffilx);
	LOG_DEBUG("relation_res_query_shub _gcid_level:%d", cmd._gcid_level);
	LOG_DEBUG("relation_res_query_shub _original_url_code_page:%d", cmd._original_url_code_page);
	LOG_DEBUG("relation_res_query_shub _ref_url_code_page:%d", cmd._ref_url_code_page);
	LOG_DEBUG("relation_res_query_shub _assist_url_code_page:%d", cmd._assist_url_code_page);
#endif


	ret = build_relation_query_server_res_cmd(&g_shub,&buffer, &len, &cmd);
	CHECK_VALUE(ret);

#ifdef _DEBUG
	LOG_DEBUG("relation_res_query_shub cid:send buffer content:");
	dump_buffer(buffer, len);	
#endif

	
	return res_query_commit_cmd(&g_shub, QUERY_FILE_RELATION, buffer, len, (void*)handler, user_data, cmd._header._seq,FALSE, NULL, NULL);
	
#endif
}

_int32 res_query_shub_by_url(void* user_data, res_query_notify_shub_handler handler, const char* url, const char* refer_url,BOOL need_bcid,
							 _int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn,BOOL not_add_res)
{
#if 1
		return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;
	/*fill request package*/
	QUERY_SERVER_RES_CMD query_res_cmd;
	LOG_DEBUG("res_query_shub_by_url, user_data = 0x%x", user_data);
	sd_memset(&query_res_cmd, 0, sizeof(QUERY_SERVER_RES_CMD));
	query_res_cmd._by_what = 0x00;
	if(!need_bcid)
		query_res_cmd._by_what |= QUERY_NO_BCID;
	query_res_cmd._url_or_gcid_size = sd_strlen(url);
	if(query_res_cmd._url_or_gcid_size >= MAX_URL_LEN)
		return -1;
	sd_memcpy(query_res_cmd._url_or_gcid, url, query_res_cmd._url_or_gcid_size);
	query_res_cmd._origin_url_size = sd_strlen(url);
	if(query_res_cmd._origin_url_size >= MAX_URL_LEN)
		return -1;
	sd_memcpy(query_res_cmd._origin_url, url, query_res_cmd._origin_url_size);
	query_res_cmd._max_server_res = max_server_res;
	query_res_cmd._bonus_res_num = bonus_res_num;
	query_res_cmd._peerid_size = PEER_ID_SIZE;
	get_peerid(query_res_cmd._peerid, PEER_ID_SIZE + 1);
	query_res_cmd._peer_report_ip = sd_get_local_ip();
	query_res_cmd._peer_capability = get_peer_capability();
	query_res_cmd._query_times_sn = query_times_sn;
	query_res_cmd._refer_url_size = sd_strlen(refer_url);
	if(query_res_cmd._refer_url_size >= MAX_URL_LEN)
		return -1;
	sd_memcpy(query_res_cmd._refer_url, refer_url, query_res_cmd._refer_url_size);
	query_res_cmd._partner_id_size = PARTNER_ID_LEN;
	get_partner_id(query_res_cmd._partner_id, PARTNER_ID_LEN);
	query_res_cmd._product_flag = get_product_flag();	
	/*build command*/
	ret = build_query_server_res_cmd(&g_shub,&buffer, &len, &query_res_cmd);
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_shub, QUERY_SERVER_RES, buffer, len, (void*)handler, user_data, query_res_cmd._header._seq,FALSE, NULL, NULL);
#endif
}



_int32 res_query_shub_by_url_newcmd(void* user_data, res_query_notify_shub_handler handler, const char* url, const char* refer_url,BOOL need_bcid,
							 _int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn,BOOL not_add_res)
{
#if 1
		return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;
	/*fill request package*/
	QUERY_RES_INFO_CMD  query_info_cmd;
	_u8 aes_key[16];
	LOG_DEBUG("res_query_shub_by_url_newcmd, user_data = 0x%x", user_data);
	sd_memset(&query_info_cmd, 0, sizeof(QUERY_RES_INFO_CMD));
	query_info_cmd._by_what = 0x00;

	build_reservce_60ver(&query_info_cmd._reserver_buffer, &query_info_cmd._reserver_length);

	_u32 utf8_output_len = MAX_URL_LEN - 1;

	if(url != NULL)
	{
		sd_any_format_to_utf8(url, sd_strlen(url), query_info_cmd._query_url, &utf8_output_len);
		query_info_cmd._query_url_size = utf8_output_len;
	
	}
	else
	{
		query_info_cmd._query_url_size = 0;
		utf8_output_len = 0;
	}

	
	query_info_cmd._query_url_code_page = -1;


	
	query_info_cmd._origion_url_size = 0;
	//if(utf8_output_len > 0)
	{
		//sd_memcpy(query_info_cmd._origion_url, query_info_cmd._query_url, utf8_output_len);
	}
	query_info_cmd._origion_url_code_page = -1;

	if(refer_url != NULL)
	{
		utf8_output_len = MAX_URL_LEN - 1;
		sd_any_format_to_utf8(refer_url, sd_strlen(refer_url), query_info_cmd._ref_url, &utf8_output_len);
		query_info_cmd._ref_url_size = utf8_output_len;
	}
	else
	{
		query_info_cmd._ref_url_size = 0;
	}
	query_info_cmd._ref_url_code_page = -1;
	query_info_cmd._cid_query_type = 1;
	
	query_info_cmd._peerid_length = PEER_ID_SIZE;
	get_peerid(query_info_cmd._peerid, PEER_ID_SIZE + 1);
	query_info_cmd._peerid[PEER_ID_SIZE] = '\0';
	query_info_cmd._local_ip = sd_get_local_ip();
	query_info_cmd._query_sn = query_times_sn + 1;
	query_info_cmd._bcid_range = -1;

	if(!need_bcid)
	{
		query_info_cmd._by_what = 2;
		query_info_cmd._bcid_range = 0;
	}

#ifdef _DEBUG
	LOG_DEBUG("res_query_shub_by_url_newcmd _query_url:%s, _query_url_code_page:%d,origionurl:origion_codepage:-1, _ref_url:%s, _ref_url_code_page:%d _cid_query_type:%d, _peerid:%s, _local_ip:%d, _query_sn:%d, _bcid_range:%d,by_what:%d", 
		query_info_cmd._query_url, 	(_u32)query_info_cmd._ref_url_code_page,
		query_info_cmd._ref_url,(_u32)query_info_cmd._ref_url_code_page,(_u32)query_info_cmd._cid_query_type
		,query_info_cmd._peerid,(_u32)query_info_cmd._local_ip,(_u32)query_info_cmd._query_sn,(_u32)query_info_cmd._bcid_range,(_u32)query_info_cmd._by_what);
#endif
	
	/*build command*/
	

#ifdef _RSA_RES_QUERY
    gen_aes_key_by_user_data(user_data, aes_key);
    ret = build_query_info_cmd_rsa(&g_shub,&buffer, &len, &query_info_cmd, aes_key, RSA_PUBKEY_VERSION_SHUB);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_shub, QUERY_RES_INFO_CMD_ID, buffer, len, (void*)handler, user_data, query_info_cmd._header._seq,FALSE, NULL, aes_key);
#else
    ret = build_query_info_cmd(&g_shub, &buffer, &len, &query_info_cmd);
    return res_query_commit_cmd(&g_shub, QUERY_RES_INFO_CMD_ID, buffer, len, (void*)handler, user_data, query_info_cmd._header._seq,FALSE, NULL, NULL);
#endif
#endif
}


_int32 res_query_shub_by_cid_newcmd(void* user_data, res_query_notify_shub_handler handler, const _u8* cid, _u64 file_size, BOOL is_gcid, const char* url_or_gcid,
							 BOOL need_bcid,_int32 max_server_res, _int32 bonus_res_num, _int32 query_times_sn, _u8 cid_query_type)
{	
#if 1
		return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;
	/*fill request package*/
	QUERY_RES_INFO_CMD query_info_cmd;
	_u8 aes_key[16];

    sd_memset(&query_info_cmd, 0, sizeof(QUERY_RES_INFO_CMD));
#ifdef _DEBUG
{
		char cid_t[41],gcid_t[41];
		sd_memset(cid_t,0x00,41);
		sd_memset(gcid_t,0x00,41);
		str2hex((char*)cid, CID_SIZE, cid_t, 40);
		cid_t[40] = '\0';
		if(is_gcid)
		{
			str2hex((char*)url_or_gcid, CID_SIZE, gcid_t, 40);
			gcid_t[40] = '\0';
		}
		LOG_ERROR("res_query_shub_by_cid_newcmd, cid = %s, gcid = %s, is_gcid,=%d,file_size = %llu", cid_t, gcid_t,is_gcid,file_size);
#ifdef IPAD_KANKAN
		printf("\res_query_shub_by_cid_newcmd, cid = %s, gcid = %s, is_gcid,=%d,file_size = %llu\n", cid_t, gcid_t,is_gcid,file_size);
#endif
}
#endif

	sd_memset((void *)&query_info_cmd, 0, sizeof(query_info_cmd));

	query_info_cmd._by_what = 0x01;

	
	build_reservce_60ver(&query_info_cmd._reserver_buffer, &query_info_cmd._reserver_length);
    
        if (!cid )
        {
	    LOG_ERROR("res_query_shub_by_cid_newcmd, cid or gcid is null.");
           return -1;
        }
	query_info_cmd._cid_size = CID_SIZE;
	sd_memcpy(query_info_cmd._cid, cid, CID_SIZE);
	query_info_cmd._file_size = file_size;

	query_info_cmd._cid_assist_url_size = 0;
	query_info_cmd._cid_assist_url_code_page = -1;
	query_info_cmd._cid_origion_url_size = 0;
	query_info_cmd._cid_origion_url_code_page = -1;
	query_info_cmd._cid_ref_url_size = 0;
	query_info_cmd._cid_ref_url_code_page = -1;
	query_info_cmd._cid_query_type = cid_query_type;
	
	
	query_info_cmd._peerid_length = PEER_ID_SIZE;
	get_peerid(query_info_cmd._peerid, PEER_ID_SIZE + 1);
	query_info_cmd._peerid[PEER_ID_SIZE] = '\0';
	query_info_cmd._local_ip = sd_get_local_ip();
	query_info_cmd._query_sn = query_times_sn + 1;
	query_info_cmd._bcid_range = -1;

	if(!need_bcid)
	{
		query_info_cmd._by_what = 3;
		query_info_cmd._bcid_range = 0;
	}

#ifdef _DEBUG
	LOG_DEBUG("res_query_shub_by_cid_newcmd  _cid_query_type:%d, _peerid:%s, _local_ip:%d, _query_sn:%d, _bcid_range:%d,by_what:%d", 
			(_u32)query_info_cmd._cid_query_type
		,query_info_cmd._peerid,(_u32)query_info_cmd._local_ip,(_u32)query_info_cmd._query_sn,(_u32)query_info_cmd._bcid_range,(_u32)query_info_cmd._by_what);

#endif

#ifdef _RSA_RES_QUERY
	/*build command*/
	gen_aes_key_by_user_data(user_data, aes_key);
	ret = build_query_info_cmd_rsa(&g_shub,&buffer, &len, &query_info_cmd, aes_key, RSA_PUBKEY_VERSION_SHUB);
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_shub, QUERY_RES_INFO_CMD_ID, buffer, len, (void*)handler, user_data, query_info_cmd._header._seq,FALSE, NULL, aes_key);
#else
    ret = build_query_info_cmd(&g_shub,&buffer, &len, &query_info_cmd);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_shub, QUERY_RES_INFO_CMD_ID, buffer, len, (void*)handler, user_data, query_info_cmd._header._seq,FALSE, NULL, NULL);
#endif
#endif
}

static _int32 _res_query_phub_helper(void* user_data, 
									 res_query_notify_phub_handler handler, 
									 const _u8* cid, 
									 const _u8* gcid, 
									 _u64 file_size, 
									 _int32 bonus_res_num, 
									 _u8 query_type,
									 QUERY_PEER_RES_CMD *cmd)
{
#if 1
		return 0;
#else
	static BOOL wan_res = FALSE;
	char cid_str[41] = {0};
	char gcid_str[41] = {0};

	if (!cid || !gcid)
	{
		LOG_ERROR("_res_query_phub_helper, cid or gcid is null.");
		return -1;
	}

	cmd->_peerid_size = PEER_ID_SIZE;
	get_peerid(cmd->_peerid, PEER_ID_SIZE + 1);
	
	cmd->_cid_size = CID_SIZE;
	if (sd_is_cid_valid(cid))
	{
		sd_memcpy(cmd->_cid, cid, CID_SIZE);
	}

	cmd->_file_size = file_size;

	cmd->_gcid_size = CID_SIZE;
	if (sd_is_cid_valid(gcid))
	{
		sd_memcpy(cmd->_gcid, gcid, CID_SIZE);
	}

	cmd->_peer_capability = get_peer_capability();
	cmd->_internal_ip = sd_get_local_ip();
	cmd->_nat_type = 0;	/*nat type unknown*/
	cmd->_level_res_num = bonus_res_num;
	// 不知道这里的用意！暂时保持原样~
	if(wan_res == TRUE)
	{
		cmd->_resource_capability = 2;
		wan_res = FALSE;
	}
	else
	{
		cmd->_resource_capability = query_type;
	}
#if defined(MOBILE_PHONE)
	if(NT_GPRS_WAP & sd_get_net_type())
	{
		cmd->_resource_capability = 7;  // Only return the peers which support p2p protocol with 'HTTP' header
	}
#endif /* MOBILE_PHONE  */	
	cmd->_server_res_num = 0;
	cmd->_query_times = 0;
	cmd->_p2p_capability = get_p2p_capability();
	cmd->_upnp_ip = 0;
	cmd->_upnp_port = 0;
	cmd->_rsc_type = 1;
	cmd->_partner_id_size = PARTNER_ID_LEN;	
	get_partner_id(cmd->_partner_id, PARTNER_ID_LEN);
	cmd->_product_flag = get_product_flag();

#ifdef _LOGGER
	cid_str[0] = '\0';
	if (sd_is_cid_valid(cid))
	{
		str2hex((char*)cid, CID_SIZE, cid_str, 40);
		cid_str[40] = '\0';
	}
	gcid_str[0] = '\0';
	if (sd_is_cid_valid(gcid))
	{
		str2hex((char*)gcid, CID_SIZE, gcid_str, 40);
		gcid_str[40] = '\0';
	}    
	LOG_DEBUG("_res_query_phub_helper, cid = %s, gcid = %s, file_size = %llu", 
		cid_str, gcid_str, file_size);
#endif

	return SUCCESS;
#endif
}


/* query_type 0:普通查询	1:仅需要发行Server	 2:仅需要外网资源	3:仅需要内网资源  4. 需要所有种类资源（与0的差别在于如果有cdn资源必须返回）5：仅需要upnp 资源6：仅需要同子网资源7：仅需要支持http的资源*/
_int32 res_query_phub(void* user_data, 
					  res_query_notify_phub_handler handler, 
					  const _u8* cid, 
					  const _u8* gcid, 
					  _u64 file_size, 
					  _int32 bonus_res_num, 
					  _u8 query_type)
{
#if 1
		return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret = -1;
	QUERY_PEER_RES_CMD	cmd;
	_u8 aes_key[16];

	LOG_DEBUG("res_query_phub, user_data = 0x%x.", user_data);
	
	sd_memset(&cmd, 0, sizeof(QUERY_PEER_RES_CMD));
	ret = _res_query_phub_helper(user_data, 
								 handler, 
								 cid, gcid, 
								 file_size, 
								 bonus_res_num, 
								 query_type, 
								 &cmd);
	CHECK_VALUE(ret);

#ifdef _RSA_RES_QUERY
	/*build command*/
	gen_aes_key_by_user_data(user_data, aes_key);
	ret = build_query_peer_res_cmd_rsa(&g_phub,&buffer, &len, &cmd, aes_key, RSA_PUBKEY_VERSION_PHUB);
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_phub, QUERY_PEER_RES, buffer, len, (void*)handler, user_data, cmd._seq,FALSE, NULL, aes_key);
#else
    ret = build_query_peer_res_cmd(&g_phub,&buffer, &len, &cmd);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_phub, QUERY_PEER_RES, buffer, len, (void*)handler, user_data, cmd._seq,FALSE, NULL, NULL);
#endif
#endif
}


_int32 res_query_partner_cdn(void* user_data, res_query_notify_phub_handler handler, const _u8* cid, const _u8* gcid, _u64 file_size)
{
#if 1
		return 0;
#else
	static BOOL wan_res = FALSE;
	char  cid_str[41], gcid_str[41];
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;
	QUERY_PEER_RES_CMD	cmd;
	_u8 aes_key[16];

	sd_memset(&cmd, 0, sizeof(QUERY_PEER_RES_CMD));
	LOG_DEBUG("res_query_partner_cdn, user_data = 0x%x.", user_data);
	cmd._peerid_size = PEER_ID_SIZE;
	get_peerid(cmd._peerid, PEER_ID_SIZE + 1);

        if (!cid || !gcid)
        {
	    LOG_ERROR("res_query_partner_cdn, cid or gcid is null.");
           return -1;
        }
    
	cmd._cid_size = CID_SIZE;
	sd_memcpy(cmd._cid, cid, CID_SIZE);
	cmd._file_size = file_size;
	cmd._gcid_size = CID_SIZE;
	sd_memcpy(cmd._gcid, gcid, CID_SIZE);
	cmd._peer_capability = get_peer_capability();
	cmd._internal_ip = sd_get_local_ip();
	cmd._nat_type = 0;	/*nat type unknown*/
	cmd._level_res_num = 20;
	if(wan_res == TRUE)
	{
		cmd._resource_capability = 2;
		wan_res = FALSE;
	}
	else
	{
		cmd._resource_capability = 4;
	}
	
#if defined(MOBILE_PHONE)
	if(NT_GPRS_WAP & sd_get_net_type())
	{
		cmd._resource_capability = 7;  // Only return the peers which support p2p protocol with 'HTTP' header
	}
#endif /* MOBILE_PHONE  */	

	cmd._server_res_num = 0;
	cmd._query_times = 0;
	cmd._p2p_capability = get_p2p_capability();
	cmd._upnp_ip = 0;
	cmd._upnp_port = 0;
	cmd._rsc_type = 1;
	cmd._partner_id_size = PARTNER_ID_LEN;	
	get_partner_id(cmd._partner_id, PARTNER_ID_LEN);
	cmd._product_flag = get_product_flag();	

#ifdef _RSA_RES_QUERY
	/*build command*/
	gen_aes_key_by_user_data(user_data, aes_key);
	ret = build_query_peer_res_cmd_rsa(&g_partner_cdn,&buffer, &len, &cmd, aes_key, RSA_PUBKEY_VERSION_PARTNER_CDN);
	CHECK_VALUE(ret);
	str2hex((char*)cid, CID_SIZE, cid_str, 40);
	cid_str[40] = '\0';
	str2hex((char*)gcid, CID_SIZE, gcid_str, 40);
	gcid_str[40] = '\0';
	LOG_DEBUG("res_query_partner_cdn, cid = %s, gcid = %s, file_size = %llu", cid_str, gcid_str, file_size);
	return res_query_commit_cmd(&g_partner_cdn, QUERY_PEER_RES, buffer, len, (void*)handler, user_data, cmd._seq,FALSE, NULL, aes_key);
#else
    ret = build_query_peer_res_cmd(&g_partner_cdn,&buffer, &len, &cmd);
    CHECK_VALUE(ret);
    str2hex((char*)cid, CID_SIZE, cid_str, 40);
    cid_str[40] = '\0';
    str2hex((char*)gcid, CID_SIZE, gcid_str, 40);
    gcid_str[40] = '\0';
    LOG_DEBUG("res_query_partner_cdn, cid = %s, gcid = %s, file_size = %llu", cid_str, gcid_str, file_size);
    return res_query_commit_cmd(&g_partner_cdn, QUERY_PEER_RES, buffer, len, (void*)handler, user_data, cmd._seq,FALSE, NULL, NULL);
#endif
#endif
}

_int32 res_query_tracker(void* user_data, res_query_notify_tracker_handler handler, _u32 last_query_stamp, const _u8* gcid, _u64 file_size, 
						 _u8 max_res, _int32 bonus_res_num, _u32 upnp_ip, _u32 upnp_port)
{
#if 1
		return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;
	QUERY_TRACKER_RES_CMD cmd;
	_u8 aes_key[16];

	LOG_DEBUG("res_query_tracker, user_data = 0x%x.", user_data);
	sd_memset(&cmd, 0, sizeof(QUERY_TRACKER_RES_CMD));
	cmd._last_query_stamp = last_query_stamp;
	cmd._by_what = 0x01;
    
        if ( !gcid)
        {
	    LOG_ERROR("res_query_tracker, cid or gcid is null.");
           return -1;
        }
	cmd._gcid_size = CID_SIZE;
	sd_memcpy(cmd._gcid, gcid, CID_SIZE);
	cmd._file_size = file_size;
	cmd._peerid_size = PEER_ID_SIZE;
	get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	cmd._local_ip = sd_get_local_ip();
	cmd._tcp_port = ptl_get_local_tcp_port();
	cmd._max_res = max_res;
	cmd._nat_type =  0;					/*nat type unknown*/
	cmd._download_ratio = 0;			/*not use*/
	cmd._download_map = 0;				/*not use*/
	cmd._peer_capability = get_peer_capability();
	cmd._release_id = 0;				/*must be get_product_release_id, but this version just fill zero, tracker didn't use this value*/
	cmd._upnp_ip = upnp_ip;
	cmd._upnp_port = (_u16)upnp_port;
	cmd._p2p_capability = get_p2p_capability();
	cmd._udp_port = ptl_get_local_udp_port();
	cmd._rsc_type = 1;
	cmd._partner_id_size = PARTNER_ID_LEN;
	get_partner_id(cmd._partner_id, PARTNER_ID_LEN);
	cmd._product_flag = get_product_flag();

#ifdef _RSA_RES_QUERY
	/*build command*/
	gen_aes_key_by_user_data(user_data, aes_key);
	ret = build_query_tracker_res_cmd_rsa(&g_tracker,&buffer, &len, &cmd, aes_key, RSA_PUBKEY_VERSION_TRACKER);
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_tracker, QUERY_TRACKER_RES, buffer, len, (void*)handler, user_data, cmd._seq,FALSE, NULL, aes_key);
#else
    ret = build_query_tracker_res_cmd(&g_tracker,&buffer, &len, &cmd);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_tracker, QUERY_TRACKER_RES, buffer, len, (void*)handler, user_data, cmd._seq,FALSE, NULL, NULL);
#endif
#endif
}

_int32 res_query_dphub(void *user_data, res_query_notify_dphub_handler handler, 
                       _u16 file_src_type, _u8 file_idx_desc_type, _u32 file_idx_content_cnt,
                       char *file_idx_content, _u64 file_size, _u32 block_size, 
                       const char *cid, const char *gcid, _u16 req_from)
{
#if 1
	return 0;
#else
	_int32 ret = SUCCESS;
    DPHUB_NODE_RC *p_dphub_node_rc = NULL, *p_dphub_parent_node_rc;
    LIST *p_to_query_node_list = NULL;
    TASK *p_task = NULL;
    DPHUB_QUERY_CONTEXT *dphub_query_context = 0;

    LOG_DEBUG("res_query_dphub, user_data = 0x%x, req_from = %u.", user_data, 
        (_u32)req_from);

    /*
        需要先判断父节点是否查询回来，如果没有，需要先查询父节点
    */
    if (get_g_parent_id() == 0)
    {
        LOG_DEBUG("parent node is NIL, so query root firstly.");
        return ERR_RES_QUERY_NO_ROOT;
    }
    else
    {
        // 不同任务的查询，都需要首先向父节点查询
        // 如果未查询过父节点，则需要先插入到该任务待查队列中
        p_task = (((RES_QUERY_PARA *)user_data)->_task_ptr);
        sd_assert(p_task != NULL);

        if (p_task)
        {
            ret = get_dphub_query_context(p_task, (RES_QUERY_PARA *)user_data, &dphub_query_context);
            CHECK_VALUE(ret);
        }

        if (dphub_query_context->_is_query_root_finish == FALSE)
        {
            ret = sd_malloc(sizeof(DPHUB_NODE_RC), (void **)&p_dphub_parent_node_rc);
            CHECK_VALUE(ret);
            ret = sd_memset(p_dphub_parent_node_rc, 0, sizeof(DPHUB_NODE_RC));
			if (ret != SUCCESS)
			{//add by tlm
				SAFE_DELETE(p_dphub_parent_node_rc);
			}
            CHECK_VALUE(ret);
            p_dphub_parent_node_rc->_intro_id = 0;      // 父节点的介绍节点是0，无介绍节点
            p_dphub_parent_node_rc->_node_rc._node_id = get_g_parent_id();
            p_dphub_parent_node_rc->_node_rc._node_type = 0;     // 父节点是资源节点
            sd_memcpy(p_dphub_parent_node_rc->_node_rc._node_host, 
                get_g_parent_node_host(), MAX_HOST_NAME_LEN);
            p_dphub_parent_node_rc->_node_rc._node_port = get_g_parent_node_port();
            p_dphub_parent_node_rc->_node_rc._node_host_len = MAX_HOST_NAME_LEN;

			int ret1 = push_dphub_node_rc_into_list(p_dphub_parent_node_rc, dphub_query_context);
			if (ret1 != SUCCESS)
			{
				sd_assert(FALSE);
				SAFE_DELETE(p_dphub_parent_node_rc);
			}
			else
			{
				dphub_query_context->_is_query_root_finish = TRUE;
			}
        }
    }

    /*遍历所有的查询节点查询，第一个节点一定是父节点，查询之后，弹出、释放！*/
    p_to_query_node_list = &dphub_query_context->_to_query_node_list;
    LOG_DEBUG("list_size(p_to_query_node_list) 1 = %d.", list_size(p_to_query_node_list));
    while (list_size(p_to_query_node_list) > 0)
    {
        //LOG_DEBUG("list_size(p_to_query_node_list) 2 = %d.", list_size(p_to_query_node_list));

        list_pop(p_to_query_node_list, (void **)&p_dphub_node_rc);
        sd_assert(p_dphub_node_rc != NULL);
        
        HUB_DEVICE *p_dphub_node_svr = NULL;
        // 这块内存在查询成功回来之后释放
        ret = sd_malloc(sizeof(HUB_DEVICE), (void **)&p_dphub_node_svr);
		if (ret != SUCCESS)
		{
			SAFE_DELETE(p_dphub_node_rc);
			CHECK_VALUE(ret);
		}
        sd_memset((void*)p_dphub_node_svr, 0, sizeof(HUB_DEVICE));
        init_hub_device(p_dphub_node_svr, DPHUB_NODE);

        sd_memset((void *)p_dphub_node_svr->_host, 0, MAX_HOST_NAME_LEN);
        ret = sd_memcpy((void *)p_dphub_node_svr->_host, p_dphub_node_rc->_node_rc._node_host, 
            p_dphub_node_rc->_node_rc._node_host_len);
		if (ret != SUCCESS)
		{//add by tlm
			SAFE_DELETE(p_dphub_node_svr);
			SAFE_DELETE(p_dphub_node_rc);
			CHECK_VALUE(ret);
		}

        p_dphub_node_svr->_port = p_dphub_node_rc->_node_rc._node_port;
        if (p_dphub_node_svr->_port == 0)
        {
            LOG_DEBUG("since dphub node port is 0, so drop to query dphub node.");
            sd_free(p_dphub_node_rc);
            p_dphub_node_rc = NULL;
            continue;
        }

        // 保存当前的node服务器地址，供取消时使用
		ret = list_push(&g_dphub_node_list, (void *)p_dphub_node_svr);
		if (ret != SUCCESS)
		{//add by tlm
			SAFE_DELETE(p_dphub_node_svr);
			SAFE_DELETE(p_dphub_node_rc);
			CHECK_VALUE(ret);
		}

        ret = res_query_dphub_node(user_data, handler, file_size, block_size, 
            file_src_type, cid, gcid, file_idx_desc_type, file_idx_content_cnt,
            file_idx_content, p_dphub_node_rc->_node_rc._node_type, 
            p_dphub_node_rc->_node_rc._node_id, req_from, p_dphub_node_svr);

		SAFE_DELETE(p_dphub_node_rc);

    }

    return ret;
#endif
}
                            
static _int32 fill_rc_query_cmd(DPHUB_RC_QUERY *cmd, const char *cid, const char *gcid, 
                         _u64 file_size, _u32 block_size, _u16 file_src_type,
                         _u32 intro_node_id, _u32 file_idx_cnt, 
                         const char *file_idx_content, _u16 req_from)
{
#if 1
			return 0;
#else
    cmd->_file_rc._cid_size = CID_SIZE;
    if (!cid || !gcid)
    {
	    LOG_ERROR("fill_rc_query_cmd, cid or gcid is null.");
           return -1;
    }
    
    sd_memcpy(cmd->_file_rc._cid, cid, CID_SIZE);
    cmd->_file_rc._gcid_size = CID_SIZE;
    sd_memcpy(cmd->_file_rc._gcid, gcid, CID_SIZE);

    cmd->_file_rc._file_size = file_size;
    cmd->_file_rc._block_size = block_size;
    cmd->_file_rc._subnet_type = 0;     // 0: 全网 1: gvod
    cmd->_file_rc._file_src_type = file_src_type;    //1: 普通完整文件 2: 普通不完整文件
    cmd->_file_rc._file_idx_content_size = file_idx_cnt;
    cmd->_file_rc._file_idx_content = (char*)file_idx_content;
    cmd->_fr_size = 29 + cmd->_file_rc._cid_size + cmd->_file_rc._gcid_size +
        cmd->_file_rc._file_idx_content_size;
    cmd->_res_capacity = 0;
    cmd->_max_res = 10000; //get_g_max_res();  // 这个值暂时先设置成尽可能的多
    cmd->_level_res = 0;    // 期望得到等级资源数
    cmd->_speed_up = 1;     // 0： 不加速 1：加速
    cmd->_p2p_capacity = get_p2p_capability();
    cmd->_ip = sd_get_local_ip();
    cmd->_upnp_ip = 0;
    cmd->_upnp_port = 0;
    cmd->_nat_type = 0;
    cmd->_intro_node_id = intro_node_id;
    cmd->_cur_peer_num = get_g_current_peer_rc_num();   // 暂时没什么用
    cmd->_total_query_times = 0;    // 同上
    cmd->_request_from = req_from;

    return SUCCESS;
#endif
}

// node_type:[0: parent节点][1: 其他node节点]
static _int32 res_query_dphub_node(void *user_data, res_query_notify_dphub_handler handler, 
                            _u64 file_size, _u32 block_size, _u16 file_src_type, 
                            const char *cid, const char *gcid, _u8 file_idx_desc_type,
                            _u32 file_idx_content_cnt, const char *file_idx_content, _u32 node_type, 
                            _u32 intro_node_id, _u16 req_from, HUB_DEVICE *hub_device)
{
#if 1
			return 0;
#else
    char* buffer = NULL;
    _u32  len = 0;
    _int32 ret;
    DPHUB_RC_QUERY rc_query_cmd;
    DPHUB_RC_NODE_QUERY rc_node_query_cmd;
    _u16 cmd_type;
    _u32 seq = 0;
	_u8 aes_key[16];

    LOG_DEBUG("res_query_dphub_node, user_data = 0x%x, node_type = %u, req_from = %u.", 
        user_data, node_type, (_u32)req_from);
#ifdef _RSA_RES_QUERY
	gen_aes_key_by_user_data(user_data, aes_key);
#endif
    if (node_type == NODE_TYPE_RESOURCE_NODE)
    {
        sd_memset(&rc_query_cmd, 0, sizeof(DPHUB_RC_QUERY));
        
        build_dphub_request_header(&rc_query_cmd._req_header);

        fill_rc_query_cmd(&rc_query_cmd, cid, gcid, file_size, block_size, file_src_type, 
            intro_node_id, file_idx_content_cnt, file_idx_content, req_from);

#ifdef _RSA_RES_QUERY
        ret = build_dphub_rc_query_cmd_rsa(hub_device, &buffer, &len, &rc_query_cmd, DPHUB_RC_QUERY_ID, aes_key, RSA_PUBKEY_VERSION_DPHUB);
        CHECK_VALUE(ret);
#else
        ret = build_dphub_rc_query_cmd(hub_device, &buffer, &len, &rc_query_cmd, DPHUB_RC_QUERY_ID);
        CHECK_VALUE(ret);
#endif

        cmd_type = DPHUB_RC_QUERY_ID;
        seq = rc_query_cmd._req_header._header._seq;
    }
    // rc_query查询回来后，再次发起的资源节点查询，发起的是rc_node_query命令
    else if (node_type == NODE_TYPE_CONTROL_NODE)
    {
        sd_memset(&rc_node_query_cmd, 0, sizeof(DPHUB_RC_NODE_QUERY));

        build_dphub_request_header(&rc_node_query_cmd._req_header);

        fill_rc_query_cmd((DPHUB_RC_QUERY *)&rc_node_query_cmd, cid, gcid, file_size, 
            block_size, file_src_type, intro_node_id, file_idx_content_cnt, 
            file_idx_content, req_from);
#ifdef _RSA_RES_QUERY
        // NOTE: RC_QUERY 和 RC_NODE_QUERY 命令内容是一样的
        ret = build_dphub_rc_query_cmd_rsa(hub_device, &buffer, &len, &rc_node_query_cmd, DPHUB_RC_NODE_QUERY_ID, aes_key, RSA_PUBKEY_VERSION_DPHUB);
        CHECK_VALUE(ret);
#else
        ret = build_dphub_rc_query_cmd(hub_device, &buffer, &len, &rc_node_query_cmd, DPHUB_RC_NODE_QUERY_ID);
        CHECK_VALUE(ret);
#endif

        cmd_type = DPHUB_RC_NODE_QUERY_ID;
        seq = rc_node_query_cmd._req_header._header._seq;
    }

    LOG_DEBUG("res_query_dphub_node, hub_device.port(%u), hub_device.host(%s).",
        hub_device->_port, hub_device->_host);

#ifdef _RSA_RES_QUERY
    return res_query_commit_cmd(hub_device, cmd_type, buffer, len, 
        (void *)handler, user_data, seq, FALSE, 
        (void *)intro_node_id, aes_key);
#else
    return res_query_commit_cmd(hub_device, cmd_type, buffer, len, 
        (void *)handler, user_data, seq, FALSE, 
        (void *)intro_node_id, NULL);
#endif
#endif
}

_int32 res_query_dphub_root(res_query_notify_dphub_root_handler handler, _u32 region_id,
                            _u32 parent_id, _u32 last_query_stamp)
{
#if 1
			return 0;
#else
    char* buffer = NULL;
    _u32  len = 0;
    _int32 ret;
    DPHUB_OWNER_QUERY cmd;
	_u8 aes_key[16];
    
    LOG_DEBUG("res_query_dphub_root, region_id = %u, parent_id = %u, last_query_stamp = %u.", 
        region_id, parent_id, last_query_stamp);

    sd_memset(&cmd, 0, sizeof(DPHUB_OWNER_QUERY));
    cmd._req_header._peerid_size = PEER_ID_SIZE;
    get_peerid((char*)(cmd._req_header._peerid), PEER_ID_SIZE);
    cmd._req_header._peer_capacity = get_peer_capability();
    cmd._req_header._region_id = region_id; 
    cmd._req_header._parent_id = parent_id; 
    cmd._req_header._product_info._partner_id_size = PARTNER_ID_LEN;
    get_partner_id(cmd._req_header._product_info._partner_id, PARTNER_ID_LEN);
    cmd._req_header._product_info._product_release_id = get_product_id();
    cmd._req_header._product_info._product_flag = get_product_flag();
    cmd._req_header._product_info._thunder_version_size = MAX_VERSION_LEN;
    get_version(cmd._req_header._product_info._thunder_version, MAX_VERSION_LEN);
    cmd._req_header._pi_size = 16 + cmd._req_header._product_info._partner_id_size +
        cmd._req_header._product_info._thunder_version_size;

    cmd._last_query_stamp = last_query_stamp;    

#ifdef _RSA_RES_QUERY
	gen_aes_key_by_user_data((void*)sd_rand(), aes_key);
    ret = build_dphub_query_owner_node_cmd_rsa(&g_dphub_root, &buffer, &len, &cmd, aes_key, RSA_PUBKEY_VERSION_DPHUB);
    CHECK_VALUE(ret);

    return res_query_commit_cmd(&g_dphub_root, DPHUB_OWNER_QUERY_ID, buffer, len, 
        (void *)handler, NULL, cmd._req_header._header._seq, FALSE, NULL, aes_key);
#else
    ret = build_dphub_query_owner_node_cmd(&g_dphub_root, &buffer, &len, &cmd);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_dphub_root, DPHUB_OWNER_QUERY_ID, buffer, len, 
        (void *)handler, NULL, cmd._req_header._header._seq, FALSE, NULL, NULL);
#endif
#endif
}

_int32 handle_res_query_notify_dphub_root(_int32 errcode, _u16 result, _u32 retry_interval)
{
#if 1
			return 0;
#else
    LOG_DEBUG("handle_res_query_notify_dphub_root, errcode = %d, result = %u, retry_inteval = %u.", 
        errcode, (_u32)result, retry_interval);
    return SUCCESS;
#endif
}


#ifdef ENABLE_CDN
_int32 res_query_cdn_manager(_int32 version, _u8* gcid, _u64 file_size, res_query_cdn_manager_handler callback_handler, void* user_data)
{
#if 1
			return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;
	CDNMANAGERQUERY	cmd;
	LOG_DEBUG("res_query_cdn_manager, task = 0x%x.", user_data);
	if(callback_handler == NULL || gcid == NULL)
		return ERR_RES_QUERY_INVALID_PARAM;
	sd_memset(&cmd, 0, sizeof(CDNMANAGERQUERY));

        if ( !gcid)
        {
	    LOG_ERROR("res_query_cdn_manager, cid or gcid is null.");
           return -1;
        }
	cmd._version = version;
	sd_memcpy(cmd._gcid, gcid, CID_SIZE);
	ret = build_query_cdn_manager_info_cmd(&g_cdn_manager, &buffer, &len, &cmd); 
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_cdn_manager, QUERY_CDN_MANAGER_INFO, buffer, len, (void*)callback_handler, user_data, cmd._header._seq,FALSE, NULL, NULL);
#endif
}

_int32 res_query_normal_cdn_manager(void* user_data,
									res_query_notify_phub_handler handler,
									const _u8 *cid,
									const _u8 *gcid,
									_u64 file_size,
									_int32 bonus_res_num, 
									_u8 query_type)
{
#if 1
			return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret = -1;
	QUERY_PEER_RES_CMD cmd;
	
	LOG_DEBUG("res_query_normal_cdn_manager, user_data = 0x%x.", user_data);

	sd_memset(&cmd, 0, sizeof(QUERY_PEER_RES_CMD));
	ret = _res_query_phub_helper(user_data, 
								 handler, 
								 cid, gcid, 
								 file_size, 
								 bonus_res_num, 
								 query_type, 
								 &cmd);
	CHECK_VALUE(ret);

	ret = build_query_peer_res_cmd(&g_normal_cdn_manager, &buffer, &len, &cmd);
	CHECK_VALUE(ret);

	return res_query_commit_cmd(&g_normal_cdn_manager, QUERY_PEER_RES, buffer, len, 
		(void*)handler, user_data, cmd._seq, FALSE, NULL, NULL);
#endif
}

#ifdef ENABLE_KANKAN_CDN

_int32 res_query_kankan_cdn_manager(_int32 version, _u8* gcid, _u64 file_size, res_query_cdn_manager_handler callback_handler, void* user_data)
{
#if 1
			return 0;
#else
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;
	CDNMANAGERQUERY	cmd;
	LOG_DEBUG("res_query_kankan_cdn_manager, task = 0x%x.", user_data);
	if(callback_handler == NULL || gcid == NULL)
		return ERR_RES_QUERY_INVALID_PARAM;
	sd_memset(&cmd, 0, sizeof(CDNMANAGERQUERY));

	cmd._version = version;
        if ( !gcid)
        {
	    LOG_ERROR("res_query_kankan_cdn_manager, cid or gcid is null.");
           return -1;
        }
	sd_memcpy(cmd._gcid, gcid, CID_SIZE);
	ret = build_query_kankan_cdn_manager_info_cmd(&g_kankan_cdn_manager, &buffer, &len, &cmd); 
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_kankan_cdn_manager, QUERY_CDN_MANAGER_INFO, buffer, len, (void*)callback_handler, user_data, cmd._header._seq,FALSE, NULL, NULL);
#endif
}

#endif

#ifdef ENABLE_HSC
_int32 res_query_vip_hub(void* user_data, res_query_notify_vip_hub_handler handler, const _u8* cid, const _u8* gcid, _u64 file_size, _int32 bonus_res_num, _u8 query_type)
{
#if 1
			return 0;
#else
	static BOOL wan_res = TRUE;
	char  cid_str[41], gcid_str[41];
	char* buffer = NULL;
	_u32  len = 0;
	_int32 ret;
	QUERY_PEER_RES_CMD	cmd;
	_u8 aes_key[16];

	sd_memset(&cmd, 0, sizeof(QUERY_PEER_RES_CMD));
	LOG_DEBUG("res_query_vip_hub, user_data = 0x%x.", user_data);
	cmd._peerid_size = PEER_ID_SIZE;
	get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	cmd._cid_size = CID_SIZE;
        if (!cid || !gcid)
        {
	    LOG_ERROR("res_query_vip_hub, cid or gcid is null.");
           return -1;
        }
       if (sd_is_cid_valid(cid))
       {
	    sd_memcpy(cmd._cid, cid, CID_SIZE);
       }
	cmd._file_size = file_size;
	cmd._gcid_size = CID_SIZE;
       if (sd_is_cid_valid(gcid))
       {
	    sd_memcpy(cmd._gcid, gcid, CID_SIZE);
       }
	cmd._peer_capability = get_peer_capability();
	cmd._internal_ip = sd_get_local_ip();
	cmd._nat_type = 0;	/*nat type unknown*/
	cmd._level_res_num = bonus_res_num;
	if(wan_res == TRUE)
	{
		cmd._resource_capability = 2;
		wan_res = FALSE;
	}
	else
	{
		cmd._resource_capability = query_type;
	}
	
#if defined(MOBILE_PHONE)
	if(NT_GPRS_WAP & sd_get_net_type())
	{
		cmd._resource_capability = 7;  // Only return the peers which support p2p protocol with 'HTTP' header
	}
#endif /* MOBILE_PHONE  */	

	cmd._server_res_num = 0;
	cmd._query_times = 0;
	cmd._p2p_capability = get_p2p_capability();
	cmd._upnp_ip = 0;
	cmd._upnp_port = 0;
	cmd._rsc_type = 1;
	cmd._partner_id_size = PARTNER_ID_LEN;	
	get_partner_id(cmd._partner_id, PARTNER_ID_LEN);
	cmd._product_flag = get_product_flag();	

#ifdef _RSA_RES_QUERY
	/*build command*/
	gen_aes_key_by_user_data(user_data, aes_key);
	ret = build_query_peer_res_cmd_rsa(&g_vip_hub,&buffer, &len, &cmd, aes_key, RSA_PUBKEY_VERSION_VIPHUB);
	CHECK_VALUE(ret);
       
	cid_str[0] = '\0';
       if (sd_is_cid_valid(cid))
       {
	    str2hex((char*)cid, CID_SIZE, cid_str, 40);
	    cid_str[40] = '\0';
       }
	gcid_str[0] = '\0';
       if (sd_is_cid_valid(gcid))
       {
	    str2hex((char*)gcid, CID_SIZE, gcid_str, 40);
	    gcid_str[40] = '\0';
       }
	LOG_DEBUG("res_query_vip_hub, cid = %s, gcid = %s, file_size = %llu", cid_str, gcid_str, file_size);
	return res_query_commit_cmd(&g_vip_hub, QUERY_PEER_RES, buffer, len, (void*)handler, user_data, cmd._seq,FALSE, NULL, aes_key);
#else
    ret = build_query_peer_res_cmd(&g_vip_hub,&buffer, &len, &cmd);
    CHECK_VALUE(ret);

    cid_str[0] = '\0';
    if (sd_is_cid_valid(cid))
    {
        str2hex((char*)cid, CID_SIZE, cid_str, 40);
        cid_str[40] = '\0';
    }
    gcid_str[0] = '\0';
    if (sd_is_cid_valid(gcid))
    {
        str2hex((char*)gcid, CID_SIZE, gcid_str, 40);
        gcid_str[40] = '\0';
    }
    LOG_DEBUG("res_query_vip_hub, cid = %s, gcid = %s, file_size = %llu", cid_str, gcid_str, file_size);
    return res_query_commit_cmd(&g_vip_hub, QUERY_PEER_RES, buffer, len, (void*)handler, user_data, cmd._seq,FALSE, NULL, NULL);
#endif
#endif
}
#endif /* ENABLE_HSC */
#endif /* ENABLE_CDN */

static _int32 res_query_remove_cmd(HUB_DEVICE* device, void* task)
{	
#if 1
			return 0;
#else
	_int32 ret = -1;
	LIST_ITERATOR cur_iter = NULL;
	LIST_ITERATOR next_iter = NULL;
	RES_QUERY_CMD* cmd = NULL;

    if (!device) return SUCCESS;

	if(device->_last_cmd != NULL && device->_last_cmd->_user_data == task)
	{	//cancel了一个正在执行的命令
		device->_last_cmd->_user_data = NULL;
		ret = SUCCESS;
	}
	else if (device->_last_cmd == NULL)
	{
		ret = SUCCESS;
	}

	cur_iter = LIST_BEGIN(device->_cmd_list);
	while(cur_iter != LIST_END(device->_cmd_list))
	{
		next_iter = LIST_NEXT(cur_iter);
		cmd = (RES_QUERY_CMD*)cur_iter->_data;
		if(cmd->_user_data == task)
		{
			sd_free((char*)cmd->_cmd_ptr);
			sd_free(cmd);
			cmd = NULL;
			list_erase(&device->_cmd_list, cur_iter);
			ret = SUCCESS;	//取消了一个命令
		}
		cur_iter = next_iter;
	}
	//如果队列内没有任何命令了，但定时器还没返回的话，需要CANCEL定时器
	if(device->_timeout_id != INVALID_MSG_ID && list_size(&device->_cmd_list) == 0 && device->_last_cmd == NULL)
	{
		cancel_timer(device->_timeout_id);
		device->_timeout_id = INVALID_MSG_ID;
		ret = SUCCESS;	
	}

    LOG_DEBUG("res_query_remove_cmd, device(0x%x), _timeout_id(%u), device->_last_cmd(0x%x), "
        "device->_last_cmd->_user_data(0x%x), task(0x%x).", device, device->_timeout_id, device->_last_cmd, 
        device->_last_cmd == NULL ? 0 : device->_last_cmd->_user_data, task);

	return ret;
#endif
}

void pop_device_from_dnode_list(HUB_DEVICE *device)
{
#if 1
			return ;
#else
    LIST_ITERATOR it = LIST_BEGIN(g_dphub_node_list);
    for (; it!=LIST_END(g_dphub_node_list); it=LIST_NEXT(it))
    {
        if (LIST_VALUE(it) == device)
        {
            list_erase(&g_dphub_node_list, it);
            return;
        }
    }

    LOG_ERROR("no device in dphub node list.");
#endif
}

_int32 res_query_cancel(void* task, HUB_TYPE type)
{
#if 1
			return 0;
#else
	_int32 ret = SUCCESS;
    HUB_DEVICE *p_node = NULL;
    LIST_ITERATOR it;
    LIST_ITERATOR it_tmp;
	LOG_DEBUG("res_query_cancel, task = 0x%x, hub_type = %d", task, type);
	switch(type)
	{
	case SHUB:
		{
			ret = res_query_remove_cmd(&g_shub, task);
			break;
		}
	case PHUB:
		{
			ret = res_query_remove_cmd(&g_phub, task);
			break;
		}
	case PARTNER_CDN:
		{
			ret = res_query_remove_cmd(&g_partner_cdn, task);
			break;
		}
	case TRACKER:
		{
			ret = res_query_remove_cmd(&g_tracker, task);
			break;
		}
	case BT_HUB:
		{
			ret = res_query_remove_cmd(&g_bt_hub, task);
			break;
		}
#ifdef ENABLE_CDN
	case CDN_MANAGER:
		{
			ret = res_query_remove_cmd(&g_cdn_manager, task);
			break;
		}
	case NORMAL_CDN_MANAGER:
		{
			ret = res_query_remove_cmd(&g_normal_cdn_manager, task);
			break;
		}
#ifdef ENABLE_KANKAN_CDN            
    case KANKAN_CDN_MANAGER:
        {
            ret = res_query_remove_cmd(&g_kankan_cdn_manager, task);
            break;
        }
#endif
#ifdef ENABLE_HSC
	case VIP_HUB:
		{
			ret = res_query_remove_cmd(& g_vip_hub, task);
			break;
		}
#endif /* ENABLE_HSC */
#endif
#ifdef ENABLE_BT
#ifdef ENABLE_BT_PROTOCOL

	case BT_TRACKER:
		{
			ret = bt_tracker_cancel_query(task);
			break;
		}
#endif
#endif

#ifdef ENABLE_EMULE
	case EMULE_HUB:
		{
			ret = res_query_remove_cmd(&g_emule_hub, task);
			break;
		}
#endif	
#ifdef _DK_QUERY
	case BT_DHT:
		{
			ret = dk_unregister_qeury( task, DHT_TYPE );
			break;
		}
 	case EMULE_KAD:
		{
			ret = dk_unregister_qeury( task, KAD_TYPE );
			break;
		}
#endif
    case DPHUB_ROOT:
        {
            //ret = res_query_remove_cmd(&g_dphub_root, task);
            break;
        }
    case DPHUB_NODE:
        {
            it = LIST_BEGIN(g_dphub_node_list);
            while (it != LIST_END(g_dphub_node_list))
            {
                p_node = (HUB_DEVICE *)(LIST_VALUE(it));
                if (p_node)
                {
                    LOG_DEBUG("p_node(0x%x), p_node->_last_cmd(0x%x).", p_node, p_node->_last_cmd);
                    res_query_remove_cmd(p_node, task);
                }
                it = LIST_NEXT(it);
            }
            break;
        }
    case EMULE_TRACKER:
        {
            ret = res_query_remove_cmd(&g_emule_tracker, task);
            break;
        }
	default:
		{
			sd_assert(FALSE);
		}
	}
	sd_assert(ret == SUCCESS);
	return ret;
#endif
}

#ifdef	ENABLE_BT

#ifdef ENABLE_BT_PROTOCOL

_int32 res_query_register_add_bt_res_handler(res_query_add_bt_res_handler bt_peer_handler)
{
	return res_query_register_add_bt_res_handler_impl(bt_peer_handler);
}
#endif

_int32 res_query_bt_info(void* user_data, res_query_bt_info_handler callback_handler, _u8* info_id, _u32 file_index, BOOL need_bcid, _u32 query_times)
{
	_u8 aes_key[16];
    QUERY_BT_INFO_CMD cmd;

    LOG_DEBUG("task = 0x%x.", user_data);
    if ((callback_handler == NULL) || (info_id == NULL))
    {
        return ERR_RES_QUERY_INVALID_PARAM;
    }
	sd_memset(&cmd, 0, sizeof(QUERY_BT_INFO_CMD));
	cmd._peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	cmd._info_id_len = CID_SIZE;
	sd_memcpy(cmd._info_id, info_id, CID_SIZE);
	cmd._file_index = file_index;
	cmd._query_times = query_times;
	if (need_bcid)
	{
	    cmd._need_bcid = 0x01;
	}
	
	char* buffer = NULL;
	_u32 len = 0;
#ifdef _RSA_RES_QUERY
	gen_aes_key_by_user_data(user_data, aes_key);
	_int32 ret = build_query_bt_info_cmd_rsa(&g_bt_hub, &buffer, &len, &cmd, aes_key, RSA_PUBKEY_VERSION_BTHUB);
	CHECK_VALUE(ret);
	return res_query_commit_cmd(&g_bt_hub, QUERY_BT_INFO, buffer, len, (void*)callback_handler, user_data, cmd._header._seq, FALSE, NULL, aes_key);
#else
    _int32 ret = build_query_bt_info_cmd(&g_bt_hub, &buffer, &len, &cmd);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_bt_hub, QUERY_BT_INFO, buffer, len, (void*)callback_handler, user_data, cmd._header._seq, FALSE, NULL, NULL);
#endif
}

#ifdef ENABLE_BT_PROTOCOL
_int32 res_query_bt_tracker(void* user_data, res_query_bt_tracker_handler callback, const char *url, _u8* info_hash)
{
	return res_query_bt_tracker_impl(user_data, callback, url, info_hash);
    
}
#endif

#endif

#ifdef ENABLE_EMULE
_int32 res_query_commit_request(HUB_TYPE device_type, _u32 seq, char** buffer, _u32 len, res_query_notify_recv_data callback, void* user_data)
{
	_int32 ret = SUCCESS;
	RES_QUERY_CMD*	cmd = NULL;
	HUB_DEVICE* device = NULL;
	ret = sd_malloc(sizeof(RES_QUERY_CMD),(void**)&cmd);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("res_query_commit_cmd, but sd_malloc failed, errcode = %d.", ret);
		sd_free(buffer);		//记得把接管的buffer释放掉
		buffer = NULL;
		return ret;
	}
	sd_memset(cmd,0,sizeof(RES_QUERY_CMD));
	cmd->_cmd_ptr = *buffer;
	cmd->_cmd_len = len;
	cmd->_callback = callback;
	cmd->_user_data = user_data;
	cmd->_cmd_type = 0;
	cmd->_retry_times = 0;
	cmd->_cmd_seq = seq;
	cmd->_not_add_res = FALSE;
	switch(device_type)
	{
		case EMULE_HUB:
		{
			device = &g_emule_hub;
			break;
		}
		default:
		{
			sd_assert(FALSE);
		}
	}
	ret = list_push(&device->_cmd_list, cmd);
	if(ret != SUCCESS)
	{
		LOG_ERROR("res_query_commit_request, but list_push failed, errcode = %d.", ret);
		sd_free(cmd);
		cmd = NULL;
		sd_assert(FALSE);
		return ret;
	}
	*buffer = NULL;		//已经被接管了
	if(device->_socket == INVALID_SOCKET && list_size(&device->_cmd_list) == 1)
	{	//没有任何请求在执行
		ret = res_query_execute_cmd(device);
	}
	return ret;
}

_int32 res_query_emule_tracker(void* user_data, res_query_notify_tracker_handler handler, _u32 last_query_stamp, 
                               const char *file_hash, _u32 upnp_ip, _u16 upnp_port)
{
    char *buffer = NULL;
    _u32 len = 0;
    _int32 ret = SUCCESS;
    QUERY_EMULE_TRACKER_CMD cmd;
    EMULE_PEER *local_peer = (EMULE_PEER*)emule_get_local_peer();
	_u8 aes_key[16];
    
    LOG_DEBUG("res_query_emule_tracker, user_data = 0x%x.", user_data);
    sd_memset(&cmd, 0, sizeof(QUERY_EMULE_TRACKER_CMD));
    cmd._last_query_stamp = last_query_stamp;
    cmd._file_hash_len = FILE_ID_SIZE;
    sd_memcpy(cmd._file_hash, file_hash, FILE_ID_SIZE);
    cmd._user_id_len = USER_ID_SIZE;
    sd_memcpy(cmd._user_id, (char *)local_peer->_user_id, USER_ID_SIZE);
    cmd._local_peer_id_len = PEER_ID_SIZE;
    get_peerid((char *)(cmd._local_peer_id), PEER_ID_SIZE + 1);
    cmd._capability = build_emule_tracker_peer_capability();
    cmd._ip = sd_get_local_ip();
    LOG_DEBUG("local_peer->_tcp_port = %hu.", local_peer->_tcp_port);
    cmd._port = local_peer->_tcp_port;
    cmd._nat_type = 0;
    cmd._product_id = get_product_id();
    cmd._upnp_ip = upnp_ip;
    cmd._upnp_port = upnp_port;

#ifdef _RSA_RES_QUERY
	gen_aes_key_by_user_data(user_data, aes_key);
    ret = emule_build_query_emule_tracker_cmd_rsa(&cmd, &buffer, &len, aes_key, RSA_PUBKEY_VERSION_EMULEHUB);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_emule_tracker, QUERY_EMULE_TRACKER, buffer, len, (void *)handler, user_data, 
        cmd._seq, FALSE, NULL, aes_key);
#else
    ret = emule_build_query_emule_tracker_cmd(&cmd, &buffer, &len);
    CHECK_VALUE(ret);
    return res_query_commit_cmd(&g_emule_tracker, QUERY_EMULE_TRACKER, buffer, len, (void *)handler, user_data, 
        cmd._seq, FALSE, NULL, NULL);
#endif
}
#endif /* ENABLE_EMULE */

_int32 res_query_aes_encrypt(char* buffer, _u32* len)
{
#if 1
			return 0;
#else
	return xl_aes_encrypt(buffer, len);
#endif
}

#ifdef _DK_QUERY
_int32 res_query_dht(void* user_data, _u8 key[DHT_ID_LEN] )
{
#ifndef _DK_TEST
	return dk_register_qeury( key, DHT_ID_LEN, user_data, 0, DHT_TYPE );//test
#else
   _u64 file_size = 734760960;
    
    //_u64 file_size_1 = 730185728;
    _u8 id[KAD_ID_LEN] = { 0x6C, 0xC1, 0x5E, 0x83, 0x50, 0x05, 0x4C, 0xF2, 0xFA, 0xF2, 0xEB, 0x0C, 0x12, 0x4E, 0x5D, 0x75 };
    //_u8 id_1[KAD_ID_LEN] = {  0x39, 0xA2, 0xD1, 0x3F, 0x72, 0x72, 0x32, 0x91, 0xED, 0xFD, 0xA1, 0xB1, 0xE3, 0x47, 0xD8, 0xC7 };
    return res_query_kad( user_data, id, file_size );
    ///////////////////////////
#endif    
}

_int32 res_query_kad(void* user_data, _u8 key[KAD_ID_LEN], _u64 file_size )
{
    LOG_DEBUG("res_query_kad, emule_task(0x%x), key(%s), file_size(%llu).", user_data, 
        (const char *)key, file_size);
    return dk_register_qeury( key, KAD_ID_LEN, user_data, file_size, KAD_TYPE );
}

_int32 res_query_add_dht_nodes(void* user_data, LIST *p_nodes_list )
{
#ifndef _DK_TEST
	return dht_add_routing_nodes( user_data, p_nodes_list );
#else
    _u32 ip1 = 0, ip2 = 0;
    _u16 port1 = 12345;
    _u16 port2 = 7125;
    sd_inet_aton( "192.168.90.155", &ip1);
    sd_inet_aton( "113.87.79.113", &ip2);

	res_query_add_kad_node( ip1, port1, 0 );
    res_query_add_kad_node( ip2, port2, 0 );
   
    return SUCCESS;
#endif  
}

_int32 res_query_add_kad_node( _u32 ip, _u16 kad_port, _u8 version )
{
	return kad_add_routing_node( ip, kad_port, version );
}

_int32 res_query_register_dht_res_handler(dht_res_handler res_handler)
{
	return dht_register_res_handler(res_handler);
}

_int32 res_query_register_kad_res_handler(kad_res_handler res_handler)
{
	return kad_register_res_handler(res_handler);
}

#endif

void res_query_update_hub( void)
{
#if 1
			return ;
#else
	res_query_update_last_cmd(&g_shub);
	res_query_update_last_cmd(&g_phub);
	res_query_update_last_cmd(&g_tracker);
	res_query_update_last_cmd(&g_partner_cdn);
	#ifdef ENABLE_HSC
	res_query_update_last_cmd(& g_vip_hub);
	#endif /* ENABLE_HSC  */
//     res_query_update_last_cmd(&g_dphub_root);
//     res_query_update_last_cmd(&g_dphub_node);
#endif
}

