#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_server_impl.h"
#include "emule_server.h"
#include "emule_server_device.h"
#include "emule_server_cmd.h"
#include "../emule_utility/emule_memory_slab.h"
#include "../emule_utility/emule_tag_list.h"
#include "../emule_utility/emule_define.h"
#include "../emule_utility/emule_opcodes.h"
#include "../emule_utility/emule_setting.h"
#include "../emule_pipe/emule_pipe_device.h"
#include "../emule_impl.h"
#include "../emule_interface.h"
#include "platform/sd_fs.h"
#include "utility/bytebuffer.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/map.h"
#include "utility/list.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"
#include "utility/settings.h"

#define	MET_HEADER1				0x0E
#define	MET_HEADER2				0xE0

static	SET							g_emule_server_set;
static	LIST							g_priority_server_list;
static	EMULE_SERVER_DEVICE		g_server_device;
static	EMULE_SERVER_ENCRYPTOR		g_encryptor;
static	void*						g_emule_server_device_callback_table[6] = 
{
	emule_server_device_connect_callback,
	emule_server_device_send_callback,	
	emule_server_device_recv_callback,
	NULL,
	emule_server_device_close_callback,
	emule_server_device_notify_error
};

_int32 emule_server_comparator(void *E1, void *E2)
{
	EMULE_SERVER* left = (EMULE_SERVER*)E1;
	EMULE_SERVER* right = (EMULE_SERVER*)E2;
	return (_int32)(left->_ip - right->_ip);
}

_int32 emule_server_init(void)
{
	set_init(&g_emule_server_set, emule_server_comparator);
	list_init(&g_priority_server_list);
	sd_memset(&g_encryptor, 0, sizeof(EMULE_SERVER_ENCRYPTOR));
	sd_memset(&g_server_device, 0, sizeof(EMULE_SERVER_DEVICE));
	g_server_device._buffer = NULL;
	g_server_device._buffer_len = 0;
	g_server_device._buffer_offset = 0;
	g_server_device._can_query = FALSE;
	return SUCCESS;
}

_int32 emule_server_uninit(void)
{
	EMULE_SERVER* server = NULL;
	SET_ITERATOR cur_iter = NULL, next_iter = NULL;
    if(g_server_device._socket_device!= NULL)
	    emule_server_device_close(&g_server_device);
	cur_iter = SET_BEGIN(g_emule_server_set);
	while(cur_iter != SET_END(g_emule_server_set))
	{
		next_iter = SET_NEXT(g_emule_server_set, cur_iter);
		server = (EMULE_SERVER*)SET_DATA(cur_iter);
		emule_free_emule_server_slip(server);
		set_erase_iterator(&g_emule_server_set, cur_iter);
		cur_iter = next_iter;
		server = NULL;
	}
    list_clear(&g_priority_server_list);
	return SUCCESS;
}

char * emule_get_config_file_name(void)
{
    static char file_name[MAX_FILE_PATH_LEN]={0};
    static char system_path[MAX_FILE_PATH_LEN]={0};

    if(sd_strlen(file_name)==0)
    {
#ifdef LINUX

        settings_get_str_item("system.system_path", (char*)system_path);
        sd_strncpy(file_name, system_path, MAX_FILE_PATH_LEN-32);

        if(SUCCESS != sd_strcmp(file_name+sd_strlen(file_name)-1,"/"))
            sd_strcat(file_name,"/",1);
        sd_strcat(file_name,"server.met", sd_strlen("server.met"));

#else
        sd_strncpy(file_name,"server.met", sd_strlen("server.met"));
#endif
    }

    LOG_DEBUG("emule_get_config_file_name %s",file_name);
    return file_name;

}

_int32 emule_server_save_impl(void)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buffer = NULL;
	_int32 buffer_len = 32 * 1024;
	_u32 file_id = 0, write_size = 0;

    // server.met
    char *file_name = emule_get_config_file_name();

    LOG_DEBUG("emule_server_save_impl, filename = %s", file_name);
	
	SET_ITERATOR iter;
	EMULE_SERVER* server = NULL;
	EMULE_TAG_LIST tag_list;
	if(set_size(&g_emule_server_set) == 0)
		return SUCCESS;			//处理没有启动ed2k服务器，退出时会把server.met置空的bug
	emule_tag_list_init(&tag_list);
	ret = sd_malloc(buffer_len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buffer = buffer;
	sd_set_int8(&tmp_buffer, &buffer_len, MET_HEADER1);
	sd_set_int32_to_lt(&tmp_buffer, &buffer_len, set_size(&g_emule_server_set));
	for(iter = SET_BEGIN(g_emule_server_set); iter != SET_END(g_emule_server_set); iter = SET_NEXT(g_emule_server_set, iter))
	{
		server = (EMULE_SERVER*)iter->_data;
		sd_set_int32_to_lt(&tmp_buffer, &buffer_len, (_int32)server->_ip);
		sd_set_int16_to_lt(&tmp_buffer, &buffer_len, (_int16)server->_port);
		ret = emule_tag_list_to_buffer(&tag_list, &tmp_buffer, &buffer_len);
	}
	sd_assert(ret == SUCCESS && buffer_len > 0);
	ret = sd_open_ex(file_name, O_FS_CREATE, &file_id);
	if( ret != SUCCESS )
	{
		sd_free(buffer);
		buffer = NULL;
		return ret;
	}
	sd_write(file_id, buffer, 32 * 1024 - buffer_len, &write_size);
	sd_assert(32 * 1024 - buffer_len == write_size);
	sd_close_ex(file_id);
	return sd_free(buffer);
}

_int32 emule_server_load_impl(void)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buffer = NULL;
	_int32 buffer_len = 0;
	_u32 file_id = 0, read_size = 0, i = 0;
	_u64 file_size = 0;
	_u8  version = 0;
	_u32 server_count = 0;
	EMULE_SERVER* server = NULL;
	const EMULE_TAG* tag = NULL;
	EMULE_TAG_LIST tag_list;
	//char file_name[32] = "server.met";

    // server.met
    char *file_name = emule_get_config_file_name();

    LOG_DEBUG("emule_server_load_impl, filename = %s", file_name);
	
	ret = sd_open_ex(file_name, O_FS_RDONLY, &file_id);
	if(ret != SUCCESS)
	{
	    LOG_DEBUG("file : %s, open fail", file_name);
		return ret;
	}
	sd_filesize(file_id, &file_size);
	if (file_size == 0)
	{
		sd_close_ex(file_id);
		return SUCCESS;
	}
	buffer_len = (_u32)file_size;
	ret = sd_malloc(buffer_len, (void**)&buffer);
	if(ret != SUCCESS)
	{
		sd_close_ex(file_id);
		return ret;
	}
	ret = sd_read(file_id, buffer, buffer_len, &read_size);
	sd_close_ex(file_id);
	if(ret != SUCCESS)
	{
		sd_free(buffer);
		buffer = NULL;
		return ret;
	}
	sd_assert(read_size == buffer_len);
	tmp_buffer = buffer;
	sd_get_int8(&tmp_buffer, &buffer_len, (_int8*)&version);
	sd_get_int32_from_lt(&tmp_buffer, &buffer_len, (_int32*)&server_count);
	if(version != MET_HEADER1 && version != MET_HEADER2)
	{
		sd_free(buffer);
		buffer = NULL;
		LOG_ERROR("emule_server_load, but version is wrong.");
		return -1;
	}
	for(i = 0; i < server_count; ++i)
	{
		emule_tag_list_init(&tag_list);
		ret = emule_get_emule_server_slip(&server);
		if(ret != SUCCESS)
			break;
		sd_get_int32_from_lt(&tmp_buffer, &buffer_len, (_int32*)&server->_ip);
		sd_get_int16_from_lt(&tmp_buffer, &buffer_len, (_int16*)&server->_port);
		emule_tag_list_from_buffer(&tag_list, &tmp_buffer, &buffer_len);
		tag = emule_tag_list_get(&tag_list, ST_SERVERNAME);
		if(tag != NULL && emule_tag_is_str(tag))
			sd_memcpy(server->_server_name, tag->_value._val_str, MIN(MAX_SERVER_NAME_SIZE, sd_strlen(tag->_value._val_str)));
		tag = emule_tag_list_get(&tag_list, ST_UDPFLAGS);
		if(tag != NULL && emule_tag_is_u32(tag))
			server->_udp_flags = tag->_value._val_u32;
//		tag = emule_tag_list_get(&tag_list, ST_TCPPORTOBFUSCATION);
//		if(tag != NULL && emule_tag_is_u32(const EMULE_TAG * tag)
		if(set_insert_node(&g_emule_server_set, server) != SUCCESS)
			emule_free_emule_server_slip(server);
		emule_tag_list_uninit(&tag_list, TRUE);
	}
	sd_free(buffer);
	buffer = NULL;
	return ret;
}

_int32 emule_build_priority_server_list(void)
{
	SET_ITERATOR iter = NULL;
	EMULE_SERVER* server = NULL;
	_u32 ip = 0;
	_u16 port = 0;
	if(emule_designate_server(&ip, &port) == TRUE)
	{
		emule_get_emule_server_slip(&server);
		sd_memset(server, 0, sizeof(EMULE_SERVER));
		server->_ip = ip;
		server->_port = port;
		list_push(&g_priority_server_list, server);
		return SUCCESS;
	}	
	//目前算法比较简单，只是选用VeryCD的服务器进行连接
	for(iter = SET_BEGIN(g_emule_server_set); iter != SET_END(g_emule_server_set); iter = SET_NEXT(g_emule_server_set, iter))
	{
		server = (EMULE_SERVER*)iter->_data;
		list_push(&g_priority_server_list, server);
	}

	//sd_assert(list_size(&g_priority_server_list) > 0);
	LOG_DEBUG("emule_build_priority_server_list, list_size = %u.", list_size(&g_priority_server_list));
	return SUCCESS;
}

_int32 emule_try_connect_server(void)
{
	_int32 ret = SUCCESS;
#ifdef _DEBUG
	char ip[24] = {0};
#endif
	if(list_size(&g_priority_server_list) == 0)
		emule_build_priority_server_list();
    if(list_size(&g_priority_server_list) == 0) return SUCCESS;
	sd_assert(list_size(&g_priority_server_list) > 0);
	list_pop(&g_priority_server_list, (void**)&g_server_device._server);
	if(emule_enable_obfuscation() == TRUE)
		ret = emule_socket_device_create(&g_server_device._socket_device, EMULE_TCP_TYPE, &g_encryptor, 
                g_emule_server_device_callback_table, &g_server_device);
	else
		ret = emule_socket_device_create(&g_server_device._socket_device, EMULE_TCP_TYPE, NULL, 
                g_emule_server_device_callback_table, &g_server_device);
	CHECK_VALUE(ret);

#ifdef _DEBUG
	sd_inet_ntoa(g_server_device._server->_ip, ip, 24);
	LOG_DEBUG("[emule_server] connect to server_ip = %s, server_port = %u, use_obfuscation = %d.", 
        ip, (_u32)g_server_device._server->_port, emule_enable_obfuscation());
#endif

	return emule_server_device_connect(&g_server_device);
}

_int32 emule_server_notify_connect(EMULE_SERVER_DEVICE* device)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("[emule_server] notify connected.");
	ret = emule_send_login_req_cmd(device);
	CHECK_VALUE(ret);
	ret = emule_server_device_recv(device, EMULE_HEADER_SIZE);
	CHECK_VALUE(ret);
	return ret;
}

_int32 emule_server_notify_close(void)
{
	LOG_DEBUG("[emule_server] notify close.");
	return emule_try_connect_server();
}

_int32 emule_server_handle_error(_int32 errcode, EMULE_SERVER_DEVICE* server_device)
{
	LOG_DEBUG("[emule_server] handle_error, errcode = %d.", errcode);
	return emule_server_device_close(server_device);
}

extern _int32 emule_handle_callback_requested_cmd(EMULE_SERVER_DEVICE* device, char* buffer, _int32 len);

_int32 emule_server_recv_cmd(struct tagEMULE_SERVER_DEVICE* device, char* buffer, _int32 len)
{
	_u8 opcode = 0;
	sd_get_int8(&buffer, &len, (_int8*)&opcode);
	LOG_DEBUG("emule_server_recv_cmd, opcode = %u.", opcode);
	switch(opcode)
	{
		case OP_SERVERSTATUS:
			return emule_handle_server_status_cmd(device, buffer, len);
		case OP_CALLBACKREQUESTED:
			return emule_handle_callback_requested_cmd(device, buffer, len);
		case OP_SERVERMESSAGE:
			return emule_handle_server_message_cmd(device, buffer, len);
		case OP_IDCHANGE:
			return emule_handle_id_change_cmd(device, buffer, len);
		case OP_FOUNDSOURCES:
			return emule_handle_found_sources_cmd(device, buffer, len);
		case OP_FOUNDSOURCES_OBFU:
			return emule_handle_found_obfu_sources_cmd(device, buffer, len);
		default:
            return emule_server_handle_error(EMULE_UNKNOWN_SERVER_CMD, device);
			//sd_assert(FALSE);
	}
	return SUCCESS;
}

_int32 emule_server_recv_udp_cmd(char* buffer, _int32 len, _u32 ip, _u16 port)
{
	_u8 opcode = 0; 
	sd_get_int8(&buffer, &len, (_int8*)&opcode);
	switch(opcode)
	{
	case OP_GLOBFOUNDSOURCES:
		{
			emule_handle_udp_found_sources_cmd(buffer, len);
			break;
		}
	default:
		{
			sd_assert(FALSE);
		}
	}
	return SUCCESS;
}

_int32 emule_handle_server_status_cmd(EMULE_SERVER_DEVICE* device, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u32 user_count = 0, file_count = 0;
	sd_get_int32_from_lt(&buffer, &len, (_int32*)&user_count);
	ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&file_count);
	sd_assert(ret == SUCCESS && len == 0);
	LOG_DEBUG("[emule_server] recv OP_SERVERSTATUS cmd, user_count = %u, file_count = %u.", user_count, file_count);
	return ret;
}

_int32 emule_handle_callback_requested_cmd(EMULE_SERVER_DEVICE* device, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u32 ip = 0;
	_u16 port = 0;
	EMULE_PIPE_DEVICE* pipe_device = NULL;
	EMULE_RESOURCE res;
	sd_assert(len == 6);
	sd_get_int32_from_lt(&buffer, &len, (_int32*)&ip);
	ret = sd_get_int16_from_lt(&buffer, &len, (_int16*)&port);
	sd_assert(ret == SUCCESS && len == 0);
	ret = emule_pipe_device_create(&pipe_device);
	CHECK_VALUE(ret);
	pipe_device->_passive = TRUE;
	sd_memset(&res, 0, sizeof(EMULE_RESOURCE));
	res._client_id = ip;
	res._client_port = port;
	LOG_DEBUG("emule_handle_callback_requested_cmd, connect to ip = %u, port = %u.", res._client_id, (_u32)res._client_port);
	return emule_pipe_device_connect(pipe_device, &res);
}

//获得client_id后，表示已经登陆成功
_int32 emule_handle_id_change_cmd(EMULE_SERVER_DEVICE* device, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u32 client_id = 0, tcp_flags = 0, server_report_ip = 0, obf_tcp_port = 0;
	ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&client_id);
	LOG_DEBUG("[emule_server] recv OP_IDCHANGE cmd, client_id = %u.", client_id);
	if(len >= 4)
	{
		ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&tcp_flags);
		sd_assert(tcp_flags != 0);
		device->_server->_tcp_flags = tcp_flags;
	}
	if(len >= 8)
	{
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&server_report_ip);
		ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&obf_tcp_port);
		device->_server->_obf_tcp_port = (_u16)obf_tcp_port;
	}
	CHECK_VALUE(ret);
	device->_can_query = TRUE;
	emule_notify_client_id_change(client_id);
	device->_server->_tcp_flags = tcp_flags;
	return ret;
}

_int32 emule_handle_server_message_cmd(EMULE_SERVER_DEVICE* device, char* buffer, _int32 len)
{
	LOG_DEBUG("[emule_server] recv OP_SERVERMESSAGE cmd.");
	//return SUCCESS;
	_int32 ret = SUCCESS;
	_u16 message_len = 0;
	char message[2048] = {0};
	sd_get_int16_from_lt(&buffer, &len, (_int16*)&message_len);
	ret = sd_get_bytes(&buffer, &len, message, MIN(message_len, 2048));
	LOG_DEBUG("[emule_server] recv OP_SERVERMESSAGE cmd, message is : %s.", message);
	return SUCCESS;
}

_int32 emule_server_query_source_impl(_u8 file_id[FILE_ID_SIZE], _u64 file_size)
{
	_int32 ret = SUCCESS;
	EMULE_SERVER* server = NULL;
	SET_ITERATOR iter = NULL;
	if(g_server_device._can_query == TRUE)
	{
		ret = emule_send_tcp_query_source_cmd(&g_server_device, file_id, file_size);
		CHECK_VALUE(ret);
	}
	//使用udp去查询
	for(iter = SET_BEGIN(g_emule_server_set); iter != SET_END(g_emule_server_set); iter = SET_NEXT(g_emule_server_set, iter))
	{
		server = (EMULE_SERVER*)iter->_data;
		ret = emule_send_udp_query_source_cmd(server, file_id, file_size);
		sd_assert(ret == SUCCESS);
	}
	return ret;
}

_int32 emule_handle_found_sources_cmd(EMULE_SERVER_DEVICE* device, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u8 file_id[FILE_ID_SIZE];
	_u8 source_count = 0, i = 0;
	_u32 client_id = 0;
	_u16 client_port = 0;
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	sd_get_int8(&buffer, &len, (_int8*)&source_count);
	LOG_DEBUG("[emule_server] recv OP_FOUNDSOURCES cmd, source_count = %u.", source_count);
	for(i = 0; i < source_count; ++i)
	{
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&client_id);
		ret = sd_get_int16_from_lt(&buffer, &len, (_int16*)&client_port);
        LOG_DEBUG("found source client_id = %u, client_port = %hu.", client_id, client_port);
		emule_notify_find_source(file_id, client_id, client_port, device->_server->_ip, device->_server->_port);
	}
	sd_assert(ret == SUCCESS && len == 0);
	return ret;
}

_int32 emule_handle_found_obfu_sources_cmd(EMULE_SERVER_DEVICE* device, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u8 file_id[FILE_ID_SIZE];
	_u8 ach_user_hash[16];
	_u8 source_count = 0, i = 0, crypt_options = 0;
	_u32 client_id = 0;
	_u16 client_port = 0;
	LOG_DEBUG("emule_handle_found_obfu_sources_cmd.");
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	sd_get_int8(&buffer, &len, (_int8*)&source_count);
	for(i = 0; i < source_count; ++i)
	{
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&client_id);
		sd_get_int16_from_lt(&buffer, &len, (_int16*)&client_port);
		ret = sd_get_int8(&buffer, &len, (_int8*)&crypt_options);
		if((crypt_options & 0x80) > 0)
			ret = sd_get_bytes(&buffer, &len, (char*)ach_user_hash, 16);
		emule_notify_find_source(file_id, client_id, client_port, device->_server->_ip, device->_server->_port);
	}
	sd_assert(ret == SUCCESS && len == 0);
	return ret;
}

_int32 emule_handle_udp_found_sources_cmd(char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u8	file_id[FILE_ID_SIZE];
	_u8 count = 0, i = 0;
	_u32 client_id = 0;
	_u16 client_port = 0;

    LOG_DEBUG("emule_handle_udp_found_sources_cmd, len = %d.", len);

	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	sd_get_int8(&buffer, &len, (_int8*)&count);
	for(i = 0; i < count; ++i)
	{
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&client_id);
		ret = sd_get_int16_from_lt(&buffer, &len, (_int16*)&client_port);
		emule_notify_find_source(file_id, client_id, client_port, 0, 0);
	}
	sd_assert(ret == SUCCESS && len == 0);
	return ret;
}

_int32 emule_server_request_callback(_u32 client_id)
{
    LOG_DEBUG("emule_server_request_callback, client_id = %u.", client_id);
	return emule_send_request_callback_cmd(&g_server_device, client_id);
}

BOOL emule_is_local_server_impl(_u32 ip, _u16 port)
{
    if(g_server_device._server==NULL) return FALSE;
	if(g_server_device._server->_ip == ip && g_server_device._server->_port == port)
		return TRUE;
	else
		return FALSE;
}

_int32 emule_get_local_server(_u32* server_ip, _u16* server_port)
{
	if(emule_enable_server() == TRUE && g_server_device._server!=NULL)
	{
		*server_ip = g_server_device._server->_ip;
		*server_port = g_server_device._server->_port;
	}
	else
	{
		*server_ip = 0;
		*server_port = 0;
	}
	return SUCCESS;
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

