#include "utility/define.h"
#ifdef ENABLE_BT
#ifdef ENABLE_BT_PROTOCOL

#include "utility/url.h"
#include "utility/map.h"
#include "utility/utility.h"
#include "utility/settings.h"
#include "utility/mempool.h"
#include "utility/local_ip.h"
#include "utility/logid.h"
#define	LOGID	LOGID_RES_QUERY
#include "utility/logger.h"

#include "socket_proxy_interface.h"

#include "res_query_bt_tracker.h"
#include "bt_download/bt_utility/bt_utility.h"
#include "bt_download/torrent_parser/bencode.h"
#include "p2p_transfer_layer/ptl_utility.h"
#include "http_data_pipe/http_data_pipe_interface.h"
#include "data_manager/data_manager_interface.h"
#include "connect_manager/pipe_interface.h"

#define	BT_TRACKER_TIMEOUT_INSTANCE		1000

static res_query_add_bt_res_handler g_add_bt_res = NULL;
static void* g_bt_tracker_function_table[MAX_FUNC_NUM];
static PIPE_INTERFACE g_pipe_interface;
static HTTP_DATA_PIPE* g_http_pipe = NULL;
static RESOURCE* g_res = NULL;
static BT_TRACKER_CMD* g_cur_cmd = NULL;
static _u32	g_timeout_id = INVALID_MSG_ID;
static LIST	g_bt_tracker_cmd_list;		/*type is BT_TRACKER_CMD*/

_int32 init_bt_tracker()
{
	list_init(&g_bt_tracker_cmd_list);
	g_bt_tracker_function_table[0] = (void*)bt_tracker_change_range;
	g_bt_tracker_function_table[1] = (void*)bt_tracker_get_dispatcher_range;
	g_bt_tracker_function_table[2] = (void*)bt_tracker_set_dispatcher_range;
	g_bt_tracker_function_table[3] = (void*)bt_tracker_get_file_size;
	g_bt_tracker_function_table[4] = (void*)bt_tracker_set_file_size;
	g_bt_tracker_function_table[5] = (void*)bt_tracker_get_data_buffer;
	g_bt_tracker_function_table[6] = (void*)bt_tracker_free_data_buffer;
	g_bt_tracker_function_table[7] =(void*) bt_tracker_put_data_buffer;
	g_bt_tracker_function_table[8] = (void*)bt_tracker_read_data;
	g_bt_tracker_function_table[9] = (void*)bt_tracker_notify_dispatch_data_finish;
	g_bt_tracker_function_table[10] = (void*)bt_tracker_get_file_type;
	g_bt_tracker_function_table[11] = NULL;
	g_pipe_interface._file_index = 0;
	g_pipe_interface._range_switch_handler = NULL;
	g_pipe_interface._func_table_ptr = g_bt_tracker_function_table;

	g_http_pipe = NULL;
       g_res = NULL;
       g_cur_cmd = NULL;
       g_timeout_id = INVALID_MSG_ID;
	   
	return SUCCESS;
}

_int32 uninit_bt_tracker()
{
       BT_TRACKER_CMD* cmd = NULL;
	LIST_ITERATOR cmd_it = NULL;
	  
	sd_assert(g_http_pipe == NULL);
	sd_assert(g_timeout_id == INVALID_MSG_ID);

	 if(list_size(&g_bt_tracker_cmd_list) != 0)
	 {
	      LOG_DEBUG("uninit_bt_tracker  g_bt_tracker_cmd_list size = %u .", list_size(&g_bt_tracker_cmd_list));
		  
	      cmd_it = LIST_BEGIN(g_bt_tracker_cmd_list);
	      while(cmd_it != LIST_END(g_bt_tracker_cmd_list))	
	      {
	             cmd = (BT_TRACKER_CMD*)LIST_VALUE(cmd_it);
                    sd_free(cmd);
					cmd = NULL;
		      cmd_it = LIST_NEXT(cmd_it);	 
	      }
	 }
	   
	list_clear(&g_bt_tracker_cmd_list);   

	g_http_pipe = NULL;
       g_res = NULL;
       g_cur_cmd = NULL;
       g_timeout_id = INVALID_MSG_ID;
	   
	return SUCCESS;
}

BOOL   res_query_bt_tracker_exist(void* user_data)
{
	BT_TRACKER_CMD* cmd = NULL;
	LIST_ITERATOR iter;
	if(g_cur_cmd != NULL)
	{
		if(g_cur_cmd->_user_data == user_data)
			return TRUE;
	}
	for(iter = LIST_BEGIN(g_bt_tracker_cmd_list); iter != LIST_END(g_bt_tracker_cmd_list); iter = LIST_NEXT(iter))
	{
		cmd = (BT_TRACKER_CMD*)LIST_VALUE(iter);
		if(cmd->_user_data == user_data)
			return TRUE;
	}
	return FALSE;
}


_int32 res_query_register_add_bt_res_handler_impl(res_query_add_bt_res_handler bt_peer_handler)
{
	g_add_bt_res = bt_peer_handler;
	return SUCCESS;
}

_int32 bt_tracker_change_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *range, BOOL cancel_flag )
{
	return http_pipe_change_ranges(p_data_pipe, range);
}

_int32 bt_tracker_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_dispatcher_range, void *p_pipe_range )
{
	RANGE* range = (RANGE*)p_pipe_range;
	range->_index = p_dispatcher_range->_index;
	range->_num = p_dispatcher_range->_num;
	return SUCCESS;
}

_int32 bt_tracker_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, struct _tag_range *p_dispatcher_range )
{
	RANGE* range = (RANGE*)p_pipe_range;
	p_dispatcher_range->_index = range->_index;
	p_dispatcher_range->_num = range->_num;
	return SUCCESS;
}

_u64 bt_tracker_get_file_size( struct tagDATA_PIPE *p_data_pipe)
{
	return 0;
}

_int32 bt_tracker_set_file_size( struct tagDATA_PIPE *p_data_pipe, _u64 filesize)
{
	return SUCCESS;
}

_int32 bt_tracker_get_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 *p_data_len)
{
	LOG_DEBUG("bt_tracker_get_data_buffer.");
	return dm_get_data_buffer(NULL, pp_data_buffer, p_data_len);
}

_int32 bt_tracker_free_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 data_len)
{
	LOG_DEBUG("bt_tracker_free_data_buffer.");
	dm_free_data_buffer(NULL, pp_data_buffer, data_len);
	return SUCCESS;
}

_int32 bt_tracker_put_data_buffer( struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len, struct tagRESOURCE *p_resource )
{
	_int32 ret = SUCCESS, i = 0;
	_u32 count = 0, ip = 0;
	_u16 port = 0;
	BC_DICT* dict = NULL, *peer_dict = NULL;
	BC_OBJ* peers = NULL, *obj = NULL, *p_bc_obj = NULL;
	BC_STR* peers_str = NULL;
	BC_LIST* peers_list = NULL;
	BC_STR* fail_str = NULL;
	LIST_ITERATOR iter;
	BC_PARSER *p_bc_parser = NULL;
	LOG_DEBUG("[tracker_pipe = 0x%x]bt_tracker_put_data_buffer, data_len = %u", p_data_pipe, data_len);

	ret = bc_parser_create(*pp_data_buffer, buffer_len, buffer_len, &p_bc_parser);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[tracker_pipe = 0x%x]bt_tracker_put_data_buffer, but bc_dict_malloc_wrap failed, errcode = %d.", p_data_pipe, ret);
		g_cur_cmd->_state = BT_TRACKER_FAILED;
		return dm_free_data_buffer(NULL, pp_data_buffer, buffer_len);
	}

	ret = bc_parser_str(p_bc_parser, &p_bc_obj);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[tracker_pipe = 0x%x]bt_tracker_put_data_buffer, but bc_dict_from_str failed, errcode = %d.", p_data_pipe, ret);
		g_cur_cmd->_state = BT_TRACKER_FAILED;
		bc_parser_destory(p_bc_parser);
		return dm_free_data_buffer(NULL, pp_data_buffer, buffer_len);
	}
	sd_assert( bc_get_type( p_bc_obj ) == DICT_TYPE );
	dict = (BC_DICT* )p_bc_obj;
	
	bc_parser_destory(p_bc_parser);
	
	bc_dict_get_value(dict, "failure reason", (BC_OBJ**)&fail_str);
	if(fail_str != NULL)
	{	//failed
		g_cur_cmd->_state = BT_TRACKER_FAILED;	
	}
	else
	{	//success
		bc_dict_get_value(dict, "peers", &peers);
		if(peers == NULL)
		{
			LOG_ERROR("bc_dict_get_value peers failed.");
			//sd_assert(FALSE);
			g_cur_cmd->_state = BT_TRACKER_FAILED;
			bc_dict_uninit((BC_OBJ*)dict);
			return dm_free_data_buffer(NULL, pp_data_buffer, buffer_len);
		}
		if(peers->_bc_type == STR_TYPE)
		{
			peers_str = (BC_STR*)peers;
			if(peers_str->_str_len % 6 == 0)		//这仅仅是compact模式，有肯还有list模式，这时要修改becode实现，暂未做
			{
				count = peers_str->_str_len / 6;
				for(i = 0; i < count; ++i) 
				{
					sd_memcpy(&ip, peers_str->_str_ptr + i * 6, sizeof(_u32));
					sd_memcpy(&port, peers_str->_str_ptr + i * 6 + 4, sizeof(_u16));
					port = sd_ntohs(port);
#ifdef _DEBUG
					{
						char	target_ip[24] = {0};
						settings_get_str_item("bt.download_from_ip", target_ip);
						if(sd_strlen(target_ip) != 0 && ip != sd_inet_addr(target_ip))		//这一句是为了调试才加上的
							continue;
					}

#endif
					LOG_DEBUG("add bt peer, ip = %u, port = %u.", ip, (_u32)port);
					g_add_bt_res(g_cur_cmd->_user_data, ip, port);
				}				
			}
		}
		else
		{
			sd_assert(peers->_bc_type == LIST_TYPE);
			peers_list = (BC_LIST*)peers;
			for(iter = LIST_BEGIN(peers_list->_list); iter != LIST_END(peers_list->_list); iter = LIST_NEXT(iter))
			{
				obj = (BC_OBJ*)LIST_VALUE(iter);
				if(obj->_bc_type == DICT_TYPE)
				{
					peer_dict = (BC_DICT*)obj;
					bc_dict_get_value(peer_dict, "ip", &obj);
					if(obj->_bc_type == STR_TYPE)
					{
						peers_str = (BC_STR*)obj;
						sd_memcpy(&ip, peers_str->_str_ptr, sizeof(_u32));
					}
					else 
					{
						continue;
					}
					bc_dict_get_value(peer_dict, "port", &obj);
					if(obj->_bc_type == INT_TYPE)
					{
						port = (_u16)((BC_INT*)obj)->_value;
					}
					else 
					{
						continue;
					}
#ifdef _DEBUG
					{
						char	target_ip[24] = {0};
						settings_get_str_item("bt.download_from_ip", target_ip);
						if(sd_strlen(target_ip) != 0 && ip != sd_inet_addr(target_ip))		//这一句是为了调试才加上的
							continue;
					}

#endif
					LOG_DEBUG("add bt peer, ip = %u, port = %u.", ip, (_u32)port);
					g_add_bt_res(g_cur_cmd->_user_data, ip, port);
				}
			}
		}
		g_cur_cmd->_state = BT_TRACKER_SUCCESS;
	}
	bc_dict_uninit((BC_OBJ*)dict);
	return dm_free_data_buffer(NULL, pp_data_buffer, buffer_len);
}

_int32  bt_tracker_read_data( struct tagDATA_PIPE *p_data_pipe, void *p_user, struct _tag_range_data_buffer* p_data_buffer, notify_read_result p_call_back )
{
	return SUCCESS;
}


_int32 bt_tracker_notify_dispatch_data_finish( struct tagDATA_PIPE *p_data_pipe)
{
	return SUCCESS;
}

_int32 bt_tracker_get_file_type( struct tagDATA_PIPE* p_data_pipe)
{
	return 0;
}

_int32 res_query_bt_tracker_impl(void* user_data, res_query_bt_tracker_handler callback, const char *url, _u8* info_hash)
{
	_int32 ret = SUCCESS;
	BT_TRACKER_CMD* cmd = NULL;
	LOG_DEBUG("res_query_bt_tracker, user_data = 0x%x, url = %s", user_data, url);

	//这个是测试用///////////////////////////////////////////////////////////////////////
//	return g_add_bt_res(user_data, sd_inet_addr("192.168.90.80"), 80);
	/////////////////////////////////////////////////////////////////////////////////////

	if(callback == NULL || sd_strlen(url) >= MAX_URL_LEN)
		return -1;
	ret = sd_malloc(sizeof(BT_TRACKER_CMD), (void**)&cmd);
	if(ret != SUCCESS)
		return ret;
	sd_memset(cmd, 0, sizeof(BT_TRACKER_CMD));
	cmd->_callback = (void*)callback;
	cmd->_user_data = user_data;
	sd_memcpy(cmd->_tracker_url, url, sd_strlen(url));
	sd_memcpy(cmd->_info_hash, info_hash, BT_INFO_HASH_LEN);
	cmd->_state = BT_TRACKER_RUNNING;
	ret = list_push(&g_bt_tracker_cmd_list, cmd);
	if(ret != SUCCESS)
	{
		sd_free(cmd);
		cmd = NULL;
		return ret;
	}
	if(g_timeout_id == INVALID_MSG_ID)
		start_timer(bt_tracker_handle_timeout, NOTICE_INFINITE, BT_TRACKER_TIMEOUT_INSTANCE, 0, NULL, &g_timeout_id);
	return SUCCESS;
}

_int32 bt_tracker_cancel_query(void* user_data)
{
	BOOL cancel = FALSE;
	BT_TRACKER_CMD* cmd = NULL;
	_u32 i = 0;
	//删除放在list里面的请求
	_u32 count = list_size(&g_bt_tracker_cmd_list);
	LOG_DEBUG("bt_tracker_cancel_query, user_data = 0x%x.", user_data);
	for(i = 0; i < count; ++i)
	{
		list_pop(&g_bt_tracker_cmd_list, (void**)&cmd);
		if(cmd->_user_data == user_data)
		{
			cancel = TRUE;
			sd_free(cmd);
			cmd = NULL;
		}
		else
		{
			list_push(&g_bt_tracker_cmd_list, cmd);
		}
	}
	//删除正在处理的请求
	if(g_cur_cmd != NULL && g_cur_cmd->_user_data == user_data)
	{
		cancel = TRUE;
		sd_free(g_cur_cmd);
		g_cur_cmd = NULL;
		sd_assert(g_http_pipe != NULL);
		http_pipe_close(&g_http_pipe->_data_pipe);
		g_http_pipe = NULL;
		sd_assert(g_res != NULL);
		http_resource_destroy(&g_res);
		g_res = NULL;
	}
//	sd_assert(cancel == TRUE);		记得以后打开
	//没有任何命令则关闭定时器
	if(list_size(&g_bt_tracker_cmd_list) == 0 && g_http_pipe == NULL && g_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(g_timeout_id);
		g_timeout_id = INVALID_MSG_ID;
	}
	return SUCCESS;
}


_int32 bt_tracker_query()
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	RANGE rg;
	LOG_DEBUG("bt_tracker_query.");
//	sd_assert(g_http_pipe == NULL);
//	sd_assert(g_cur_cmd == NULL);
	list_pop(&g_bt_tracker_cmd_list, (void**)&g_cur_cmd);
	sd_assert(g_cur_cmd != NULL);
	ret = http_resource_create(g_cur_cmd->_tracker_url, sd_strlen(g_cur_cmd->_tracker_url), NULL, 0, TRUE, &g_res);
	if(ret != SUCCESS)
		return bt_tracker_handle_result(ret);
	ret = http_pipe_create(NULL, g_res, (DATA_PIPE**)&g_http_pipe);
	if(ret != SUCCESS)
		return bt_tracker_handle_result(ret);
	dp_set_pipe_interface(&g_http_pipe->_data_pipe, &g_pipe_interface);
	ret = bt_tracker_build_query_cmd(&buffer, &len, g_cur_cmd);
	if(ret != SUCCESS)
	{
		return bt_tracker_handle_result(ret);
	}
	http_pipe_set_request(&g_http_pipe->_data_pipe, buffer, len);
	sd_free(buffer);
	buffer = NULL;
	ret = http_pipe_open(&g_http_pipe->_data_pipe);
	if(ret != SUCCESS)
	{
		return bt_tracker_handle_result(ret);
	}
	rg._index = 0;
	rg._num = MAX_U32;
	return pi_pipe_change_range(&g_http_pipe->_data_pipe, &rg, FALSE);
}

_int32 bt_tracker_handle_result(_int32 errcode)
{
	res_query_bt_tracker_handler callback_fun = (res_query_bt_tracker_handler)g_cur_cmd->_callback;
	callback_fun(g_cur_cmd->_user_data,errcode );
	sd_free(g_cur_cmd);
	g_cur_cmd = NULL;
	if(g_http_pipe != NULL)
	{
		http_pipe_close(&g_http_pipe->_data_pipe);
		g_http_pipe = NULL;
	}
	if(g_res != NULL)
	{
		http_resource_destroy(&g_res);
		g_res = NULL;
	}
	//这里紧接着判断是否还有命令需要请求
	if(list_size(&g_bt_tracker_cmd_list) != 0)
	{
		return bt_tracker_query();
	}
	//没有任何命令则关闭定时器
	if(g_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(g_timeout_id);
		g_timeout_id = INVALID_MSG_ID;
	}
	return SUCCESS;
}

_int32 bt_tracker_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	if(g_http_pipe == NULL && list_size(&g_bt_tracker_cmd_list) > 0)
		return bt_tracker_query();
	sd_assert(g_cur_cmd != NULL);
	LOG_DEBUG("bt_tracker_handle_timeout, pipe_state = %d.", g_http_pipe->_data_pipe._dispatch_info._pipe_state);
	if(g_http_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
	{
		g_cur_cmd->_state = BT_TRACKER_FAILED;
	}
	switch(g_cur_cmd->_state)
	{
	case BT_TRACKER_RUNNING:
		return SUCCESS;
	case BT_TRACKER_SUCCESS:
		return bt_tracker_handle_result(SUCCESS);
	case BT_TRACKER_FAILED:
		return bt_tracker_handle_result(-1);
	}
	sd_assert(FALSE);
	return SUCCESS;
}

_int32 bt_tracker_build_query_cmd(char** buffer, _u32* len, BT_TRACKER_CMD* cmd)
{
	_int32 ret = SUCCESS;
	URL_OBJECT url_obj;
	char bt_peerid[BT_PEERID_LEN];
	char hash_str[61] = {0};
	_int32 hash_str_len = 61;
	char peerid_str[61] = {0};
	_int32 peerid_str_len = 61;
	char ip_str[16] = {0};
//	char state_str[10] = {0};
	ret = bt_get_local_peerid(bt_peerid, BT_PEERID_LEN);
	CHECK_VALUE(ret);
	ret = bt_escape_string((const char*)cmd->_info_hash, BT_INFO_HASH_LEN, hash_str, &hash_str_len);
	CHECK_VALUE(ret);
	hash_str[hash_str_len] = 0;
	ret = bt_escape_string(bt_peerid, BT_PEERID_LEN, peerid_str, &peerid_str_len);
	CHECK_VALUE(ret);
	peerid_str[peerid_str_len] = 0;
	sd_inet_ntoa(sd_get_local_ip(), ip_str, 15);
	ret = sd_url_to_object(cmd->_tracker_url, sd_strlen(cmd->_tracker_url), &url_obj);
	CHECK_VALUE(ret);
	*len = 1024;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	sd_memset(*buffer, 0, *len);
	//switch(cmd->_state)
	//{
	//case BT_TRACKER_STARTED:
	//	sd_memcpy(state_str, "started", sd_strlen("started"));
	//	break;
	//case BT_TRACKER_STOPPED:
	//	sd_memcpy(state_str, "stopped", sd_strlen("stopped"));
	//	break;
	//case BT_TRACKER_COMPLETED:
	//	sd_memcpy(state_str, "completed", sd_strlen("completed"));
	//	break;
	//}
	/*这里特别注意：目前仅仅实现在tracker服务器上注册，即仅实现started,没实现stop,complete。以后再改进*/
	*len = sd_snprintf(*buffer, *len, "GET %s?info_hash=%s&peer_id=%s&ip=%s&port=%u&uploaded=0&downloaded=0&left=289742100&numwant=200&key=%d&compact=1&event=%s \
HTTP/1.0\r\nHost: %s\r\nUser-Agent: Bittorrent\r\nAccept: */*\r\nAccept-Encoding: gzip\r\nConnection: closed\r\n\r\n",
								url_obj._full_path,hash_str, peerid_str, ip_str, ptl_get_local_tcp_port(), sd_rand(), "started", url_obj._host);
	LOG_DEBUG("request_buffer, len = %u, content : %s", *len, *buffer);
	if((_int32)*len < 0)
	{
		sd_free(*buffer);
		*buffer = NULL;
		return -1;
	}
	return SUCCESS;
}

#endif
#endif

