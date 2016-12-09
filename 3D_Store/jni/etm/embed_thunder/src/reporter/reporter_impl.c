#include "reporter_impl.h"
#include "reporter_setting.h"
#include "reporter_cmd_handler.h"
#include "reporter_cmd_define.h"
#include "socket_proxy_interface.h"
#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/utility.h"
#include "task_manager/task_manager.h"

#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif

#include "utility/logid.h"
#define  LOGID LOGID_REPORTER
#include "utility/logger.h"

#include "reporter_interface.h"

#ifdef EMBED_REPORT
static _u32	g_report_online_peer_time_id;
static BOOL	g_enable_file_report = TRUE;
#endif


_int32 reporter_commit_cmd(REPORTER_DEVICE* device, _u16 cmd_type, const char* buffer, _u32 len, void* user_data, _u32 cmd_seq)
{
	REPORTER_CMD * cmd=NULL;

	_int32 ret = sd_malloc(sizeof(REPORTER_CMD),(void**)&cmd);
	
	LOG_DEBUG("reporter_commit_cmd:cmd_type=%d,cmd_seq=0x%x,,list_size=%d",cmd_type,cmd_seq,list_size(&device->_cmd_list));

	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_commit_cmd, malloc failed");
		CHECK_VALUE(ret);
	}

	cmd->_cmd_ptr = buffer;
	cmd->_cmd_len = len;
	cmd->_user_data = (void*)user_data;
	cmd->_cmd_type = cmd_type;
	cmd->_retry_times = 0;
	cmd->_cmd_seq = cmd_seq;
	ret = list_push(&device->_cmd_list, cmd);
	if(ret!=SUCCESS)
	{
		SAFE_DELETE(cmd);
		return ret;
	}
	if(device->_last_cmd == NULL && list_size(&device->_cmd_list) == 1)		/*是否仅有本地一个请求*/
		return reporter_execute_cmd(device);
	else
		return SUCCESS;
}

_int32 reporter_execute_cmd(REPORTER_DEVICE* device)
{
	_int32 ret = SUCCESS;
	REPORTER_SETTING* setting =NULL;
	LOG_DEBUG("reporter_execute_cmd, socket = %u,device_type=%d,_last_cmd=0x%X", device->_socket,device->_type,device->_last_cmd);
	sd_assert(device->_last_cmd == NULL);
	LOG_DEBUG("reporter_execute_cmd,list_size=%d",list_size(&device->_cmd_list));
	if(list_size(&device->_cmd_list) == 0)
	{	/*no command to execute*/
		if(device->_socket != INVALID_SOCKET)
		{
			LOG_DEBUG("reporter_execute_cmd close socket, socket = %u", device->_socket);
#ifdef REPORTER_TEST
			{
            _u32 count = 0;
            socket_proxy_peek_op_count(device->_socket, DEVICE_SOCKET_TCP, &count);
            sd_assert(count==0);
			}
#endif

			socket_proxy_close(device->_socket);
			device->_socket = INVALID_SOCKET;
		}
		return SUCCESS;		
	}
	
	list_pop(&device->_cmd_list, (void**)&device->_last_cmd);
	sd_assert(device->_last_cmd != NULL);
	/*some command waiting for process in device->_cmd_queue*/
	if(device->_socket == INVALID_SOCKET)
	{	/*connect to hub*/
		setting = get_reporter_setting();				/*can be optimized*/
		ret = socket_proxy_create(&device->_socket, SD_SOCK_STREAM);
		if(ret==SUCCESS)
		{
			LOG_DEBUG("reporter_execute_cmd create socket, socket = %u", device->_socket);
			if(device->_type == REPORTER_LICENSE)
			{
				LOG_DEBUG("license_server_addr = %s,port=%u", setting->_license_server_addr,setting->_license_server_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_license_server_addr, setting->_license_server_port, reporter_handle_connect_callback, device);
			}
			else
			if(device->_type == REPORTER_SHUB)
			{
				LOG_DEBUG("shub_addr = %s,port=%u", setting->_shub_addr,setting->_shub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_shub_addr, setting->_shub_port, reporter_handle_connect_callback, device);
			}
			else
			if(device->_type == REPORTER_STAT_HUB)
			{
				LOG_DEBUG("stat_hub_addr = %s,port=%u", setting->_stat_hub_addr,setting->_stat_hub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_stat_hub_addr, setting->_stat_hub_port, reporter_handle_connect_callback, device);
			}
#ifdef ENABLE_BT
			else
			if(device->_type == REPORTER_BT_HUB)
			{
				LOG_DEBUG("bt_hub_port = %s,port=%u", setting->_bt_hub_addr,setting->_bt_hub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_bt_hub_addr, setting->_bt_hub_port, reporter_handle_connect_callback, device);
			}
#endif
#ifdef ENABLE_EMULE
			else
			if(device->_type == REPORTER_EMULE_HUB)
			{
				LOG_DEBUG("emule_hub_port = %s,port=%u", setting->_emule_hub_addr,setting->_emule_hub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_emule_hub_addr, setting->_emule_hub_port, reporter_handle_connect_callback, device);
			}
#endif

#ifdef UPLOAD_ENABLE
			else
			if(device->_type == REPORTER_PHUB)
			{
				LOG_DEBUG("phub_addr = %s,port=%u", setting->_phub_addr,setting->_phub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_phub_addr, setting->_phub_port, reporter_handle_connect_callback, device);
			}
#endif

#ifdef EMBED_REPORT
			else
			if(device->_type == REPORTER_EMBED)
			{
				LOG_DEBUG("emb_hub_addr = %s,port=%u", setting->_emb_hub_addr,setting->_emb_hub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_emb_hub_addr, setting->_emb_hub_port, reporter_handle_connect_callback, device);
			}
#endif		

else
			{
#ifdef _DEBUG
						CHECK_VALUE(1);
#endif
				ret =  REPORTER_ERR_UNKNOWN_DEVICE_TYPE;
			}
		}
	}
	else
	{
#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(device->_last_cmd->_cmd_ptr), device->_last_cmd->_cmd_len, test_buffer, 1024);
			LOG_DEBUG( "socket_proxy_send1: _socket=%u, _cmd_type=%d,send buffer[%u]=%s" 
						,device->_socket,device->_last_cmd->_cmd_type, device->_last_cmd->_cmd_len, test_buffer);
		}
#endif /* _DEBUG */
	
		ret =  socket_proxy_send(device->_socket, device->_last_cmd->_cmd_ptr, device->_last_cmd->_cmd_len, reporter_handle_send_callback, device);
	}
	
	if(ret!=SUCCESS)
	{
		reporter_handle_network_error(device, ret);
//#ifdef _DEBUG
//		CHECK_VALUE(ret);
//#endif
	}
	
	return SUCCESS;
}

/*connect event notify*/
_int32 reporter_handle_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data)
{
	REPORTER_DEVICE* device = (REPORTER_DEVICE*)user_data;
	_int32 ret_val=SUCCESS;
	LOG_DEBUG("reporter_handle_connect_callback :errcode = %d,pending_op_count=%u,_socket=%u", errcode,pending_op_count,device->_socket);
	if(errcode == MSG_CANCELLED && pending_op_count == 0)
	{
		reporter_close_socket(device);
	}
	if(errcode != SUCCESS)
	{	/*connect failed*/
		reporter_handle_network_error(device, errcode);
	}
	else
	{
		sd_assert(device->_last_cmd != NULL);
#ifdef REPORTER_DEBUG
			{
				char test_buffer[1024];
				sd_memset(test_buffer,0,1024);
				str2hex( (char*)(device->_last_cmd->_cmd_ptr), device->_last_cmd->_cmd_len, test_buffer, 1024);
				LOG_DEBUG( "socket_proxy_send2: _socket=%u, _cmd_type=%d,send buffer[%u]=%s" 
							,device->_socket,device->_last_cmd->_cmd_type, device->_last_cmd->_cmd_len, test_buffer);
			}
#endif /* _DEBUG */
		
		ret_val =  socket_proxy_send(device->_socket, device->_last_cmd->_cmd_ptr, device->_last_cmd->_cmd_len, reporter_handle_send_callback, device);
		if(ret_val!=SUCCESS)
		{
			reporter_failure_exit(device, ret_val);	
		}
	}
	return SUCCESS;
}

/*send event notify*/
_int32 reporter_handle_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	REPORTER_DEVICE* device = (REPORTER_DEVICE*)user_data;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("reporter_handle_send_callback :errcode = %d,pending_op_count=%u,had_send=%u,_socket=%u", errcode,pending_op_count,had_send,device->_socket);
	if(errcode == MSG_CANCELLED && pending_op_count == 0)
	{
		reporter_close_socket(device);
	}
	if(errcode != SUCCESS)
	{
		reporter_handle_network_error(device, errcode);
	}
	else
	{
		/*send success*/
		LOG_DEBUG("reporter_handle_send_callback had send cmd, command = %u,_socket=%u", device->_last_cmd->_cmd_type,device->_socket);
		device->_had_recv_len = 0;
        sd_memset( device->_recv_buffer, 0, device->_recv_buffer_len );
		ret_val =  socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer, device->_recv_buffer_len-1, reporter_handle_recv_callback, device);
		if(ret_val!=SUCCESS)
		{
			reporter_failure_exit(device, ret_val);	
		}
	}
	return SUCCESS;
}

/*recv event notify*/
_int32 reporter_handle_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	REPORTER_DEVICE* device = (REPORTER_DEVICE*)user_data;
	_int32 ret_val = SUCCESS;
	_u32 version = 0, sequence= 0 , body_len = 0, total_len = 0;
	char* recv_buffer = NULL;
	_int32 buffer_len = 0;
	char* http_header = NULL,*status_code = NULL;
	_u32 http_header_len = 0;

	LOG_DEBUG("reporter_handle_recv_callback :errcode = %d,pending_op_count=%u,had_recv=%u,_socket=%u, device->_type:%d, buffer:%s", 
		errcode,pending_op_count,had_recv,device->_socket,device->_type,buffer);
	
	if(errcode == MSG_CANCELLED && pending_op_count == 0)
	{
		reporter_close_socket(device);
	}
	if(errcode != SUCCESS)
	{
		reporter_handle_network_error(device, errcode);
	}
	else
	{
		/*recv success*/
		device->_had_recv_len += had_recv;
#ifndef ADD_HTTP_HEADER_TO_LICENSE_REPORT
        if( device->_type != REPORTER_LICENSE )
#endif			
        {//modified by xietao @20110428
        //因为license验证发送/应答升级为带http头，此处需要处理带http头和不带http头的应答
        //当为REPORTER_LICENSE 时，有http头就先处理http头，否则直接按照老的包来解析
            http_header = sd_strstr(device->_recv_buffer, "\r\n\r\n", 0);
            if(http_header == NULL)     //没找到，继续收
            {
                if(device->_recv_buffer_len <= device->_had_recv_len)
                {
                    //sd_assert( FALSE );
					LOG_ERROR("device->_recv_buffer_len <= device->_had_recv_len, (%d <= %d)", device->_recv_buffer_len, device->_had_recv_len);
                    reporter_handle_network_error(device, REPORTER_ERR_INVALID_TOTAL_LEN);
                    return SUCCESS;
                }
                return socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer + device->_had_recv_len - 1, device->_recv_buffer_len - device->_had_recv_len, reporter_handle_recv_callback, device);
            }
			status_code = sd_strstr(device->_recv_buffer, "HTTP/1.1 200 ", 0);
			if(status_code==NULL||status_code>http_header)
			{
				//sd_assert( FALSE );
				reporter_handle_network_error(device, REPORTER_ERR_INVALID_TOTAL_LEN);
				return SUCCESS;
			}
            //已经收到http头部了
            http_header_len = http_header - device->_recv_buffer + 4;
            if( http_header_len > device->_recv_buffer_len )
			{	/*length invalid, should be wrong protocol*/
                //sd_assert(FALSE);
				LOG_ERROR("http_header_len > device->_recv_buffer_len, (%d <= %d)", http_header_len, device->_recv_buffer_len);
				reporter_handle_network_error(device, REPORTER_ERR_INVALID_TOTAL_LEN);
                return SUCCESS;
            }
		}
        
		if(device->_had_recv_len - http_header_len >= REPORTER_CMD_HEADER_LEN)
		{	/*have received header, (protocol version 5.0)*/
			recv_buffer = device->_recv_buffer+http_header_len;
			buffer_len = device->_had_recv_len - http_header_len;
			sd_get_int32_from_lt(&recv_buffer, &buffer_len, (_int32*)&version);
			sd_get_int32_from_lt(&recv_buffer, &buffer_len, (_int32*)&sequence);
			sd_get_int32_from_lt(&recv_buffer, &buffer_len, (_int32*)&body_len);

            total_len = body_len + REPORTER_CMD_HEADER_LEN + http_header_len;
            if( total_len > REPORTER_MAX_REC_DATA_LEN + REPORTER_MAX_REC_BUFFER_LEN )
			{	/*length invalid, should be wrong protocol*/
                //sd_assert(FALSE);
				LOG_ERROR("total_len > REPORTER_MAX_REC_DATA_LEN + REPORTER_MAX_REC_BUFFER_LEN, (%d <= %d)", 
					total_len, REPORTER_MAX_REC_DATA_LEN + REPORTER_MAX_REC_BUFFER_LEN);
				reporter_handle_network_error(device, REPORTER_ERR_INVALID_TOTAL_LEN);
			}
			else
			{
				/*judge receive data's integrity*/
				if(total_len == device->_had_recv_len)
				{	/*receive a complete protocol package*/
					ret_val =reporter_handle_recv_resp_cmd(device->_recv_buffer+http_header_len, device->_had_recv_len - http_header_len, device);	/*process protocol package*/
					if(ret_val ==SUCCESS)
					{

#if defined(MOBILE_PHONE)&& defined(EMBED_REPORT)
					
						if (device->_last_cmd->_cmd_type == EMB_REPORT_MOBILE_USER_ACTION_CMD_TYPE)
						{
							ret_val = reporter_from_file_callback();
						}
#endif

						/*the first request in queue had processed, remove*/
						sd_free((char*)device->_last_cmd->_cmd_ptr);
						device->_last_cmd->_cmd_ptr = NULL;

						sd_free(device->_last_cmd);
						device->_last_cmd = NULL;
						LOG_DEBUG("reporter_handle_recv_callback :ok,ready to execute the next command");
						ret_val =   reporter_execute_cmd(device);
					}
				}
				else if(total_len > device->_had_recv_len)
				{	/*receive a uncomplete package*/
					if(total_len > device->_recv_buffer_len)
					{	/*extend receive buffer*/
						ret_val =   reporter_extend_recv_buffer(device, total_len);
					}
					
					if(ret_val ==SUCCESS)
					{
						ret_val =   socket_proxy_recv(device->_socket, device->_recv_buffer + device->_had_recv_len, total_len - device->_had_recv_len, reporter_handle_recv_callback, device);
					}
				}

				if(ret_val!=SUCCESS)
				{
					reporter_failure_exit(device, ret_val);	
				}
			}
		}
		else		/*hub->_had_recv_len < REPORTER_CMD_HEADER_LEN*/
		{
            //sd_assert(device->_recv_buffer_len > device->_had_recv_len);
            if (device->_recv_buffer_len <= device->_had_recv_len)
            {
				LOG_ERROR("device->_recv_buffer_len > device->_had_recv_len, 2nd, (%d <= %d)", 
					device->_recv_buffer_len, device->_had_recv_len);
				reporter_handle_network_error(device, REPORTER_ERR_INVALID_TOTAL_LEN);
                return SUCCESS;
            }
			
            return socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer + device->_had_recv_len, device->_recv_buffer_len - device->_had_recv_len, reporter_handle_recv_callback, device);
			//sd_assert(FALSE);	/*logical error*/
			//reporter_failure_exit(device, REPORTER_ERR_INVALID_TOTAL_LEN);	
		}
	}
	return SUCCESS;
}

_int32 reporter_close_socket(REPORTER_DEVICE* device)
{
	LOG_DEBUG("reporter_close_socket :device->type = %d,device->socket=%u", device->_type,device->_socket);
	if(device->_last_cmd != NULL)
	{
		sd_free((char*)device->_last_cmd->_cmd_ptr);
		sd_free(device->_last_cmd);
		device->_last_cmd = NULL;
	}
    
#ifdef REPORTER_TEST
	{
                _u32 count = 0;
                socket_proxy_peek_op_count(device->_socket, DEVICE_SOCKET_TCP, &count);
                sd_assert(count==0);
	}
#endif
    
	socket_proxy_close(device->_socket);
	device->_socket = INVALID_SOCKET;
	sd_free(device->_recv_buffer);
	device->_recv_buffer = NULL;
	device->_recv_buffer_len = 0;
	sd_assert(list_size(&device->_cmd_list) == 0);
	return SUCCESS;
}

/*handle network error*/
_int32 reporter_handle_network_error(REPORTER_DEVICE* device, _int32 errcode)
{
	_int32 ret_val = SUCCESS;
	static REPORTER_SETTING* setting = NULL;
	_u16 last_cmd_type = 0;
	LOG_DEBUG("reporter_handle_network_error,  device_type:%d, errcode is %d", device->_type, errcode);
#ifdef REPORTER_TEST
	{
                _u32 count = 0;
                socket_proxy_peek_op_count(device->_socket, DEVICE_SOCKET_TCP, &count);
                sd_assert(count==0);
	}
#endif

	socket_proxy_close(device->_socket);

	device->_socket = INVALID_SOCKET;
	if(device->_last_cmd == NULL)
	{
		sd_assert(FALSE);
		ret_val = REPORTER_ERR_INVALID_LAST_CMD;
 		 goto ErrHandler;
 	}
	last_cmd_type = device->_last_cmd->_cmd_type;
	++device->_last_cmd->_retry_times;
	setting = get_reporter_setting();
	if(device->_last_cmd->_retry_times >= setting->_cmd_retry_times)
	{
		reporter_notify_execute_cmd_failed(device);
		ret_val =   REPORTER_ERR_RETRY_TOO_MANY_TIMES;
		goto ErrHandler;
 	}
	else
	{
		list_push(&device->_cmd_list, device->_last_cmd);
		device->_last_cmd = NULL;
		ret_val =   start_timer(reporter_retry_handler, NOTICE_ONCE, REPORTER_TIMEOUT, 0, device, &device->_timeout_id);
		if(ret_val!=SUCCESS) goto ErrHandler;
	}
	
	return SUCCESS;

ErrHandler:
	
		if(device->_type==REPORTER_LICENSE)
		{
			tm_notify_license_report_result(-1, errcode, ret_val );
		}
		else
		if((device->_type == REPORTER_SHUB)||(device->_type == REPORTER_STAT_HUB)
#ifdef ENABLE_BT
			||(device->_type == REPORTER_BT_HUB)
#endif
#ifdef ENABLE_EMULE
            ||(device->_type == REPORTER_EMULE_HUB)
#endif


		)
		{
			switch(last_cmd_type)
			{
				case REPORT_DW_STAT_CMD_TYPE:
					LOG_DEBUG("REPORT_DW_STAT_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
				break;	
				case REPORT_DW_FAIL_CMD_TYPE:
					LOG_DEBUG("REPORT_DW_FAIL_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
				break;	
				case REPORT_INSERTSRES_CMD_TYPE:
					LOG_DEBUG("REPORT_INSERTSRES_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
				break;
#ifdef ENABLE_BT
				case REPORT_BT_TASK_DL_STAT_CMD_TYPE:
					LOG_DEBUG("REPORT_BT_TASK_DL_STAT_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
				break;	
				case REPORT_BT_DL_STAT_CMD_TYPE:
					LOG_DEBUG("REPORT_BT_DL_STAT_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
				break;	
				case REPORT_BT_INSERT_RES_CMD_TYPE:
					LOG_DEBUG("REPORT_BT_INSERT_RES_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
				break;
#endif /* ENABLE_BT */
#ifdef ENABLE_EMULE
                case REPORT_EMULE_INSERTSRES_CMD_TYPE:
                    LOG_DEBUG("REPORT_EMULE_INSERTSRES_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
                break;
#endif 


				default:
					LOG_DEBUG("UNKNOWN shub command type error![_cmd_type=%d,errcode=%d,ret_val=%d]!",last_cmd_type,errcode,ret_val);
			}
		}
#ifdef UPLOAD_ENABLE
		else
		if(device->_type == REPORTER_PHUB)
		{
			switch(last_cmd_type)
			{
				case REPORT_ISRC_ONLINE_CMD_TYPE:
					LOG_DEBUG("REPORT_ISRC_ONLINE_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
					ulm_isrc_online_resp(FALSE, errcode);
				break;	
				case REPORT_RC_LIST_CMD_TYPE:
					LOG_DEBUG("REPORT_RC_LIST_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
					ulm_report_rclist_resp( FALSE);
				break;	
				case REPORT_INSERT_RC_CMD_TYPE:
					LOG_DEBUG("REPORT_INSERT_RC_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
					ulm_insert_rc_resp(FALSE);
				break;	
				case REPORT_DELETE_RC_CMD_TYPE:
					LOG_DEBUG("REPORT_DELETE_RC_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
					ulm_delete_rc_resp(FALSE);
				break;
				default:
					LOG_DEBUG("UNKNOWN rc command type error![_cmd_type=%d,errcode=%d,ret_val=%d]!",last_cmd_type,errcode,ret_val);
			}
		}
#endif

	LOG_DEBUG("reporter_handle_network_error,list_size=%d",list_size(&device->_cmd_list));
	if(device->_last_cmd == NULL && list_size(&device->_cmd_list) != 0)		/*是否仅有本地还有请求*/
	{
		ret_val = reporter_execute_cmd(device);
		if(ret_val!=SUCCESS)
		{
			reporter_failure_exit(device, ret_val);
		}
	}

		
	return SUCCESS;
	
}

_int32 reporter_retry_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	REPORTER_DEVICE* device = (REPORTER_DEVICE*)msg_info->_user_data;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("reporter_retry_handler:errcode=%d",errcode);
	if((device==NULL)||(msgid != device->_timeout_id)) return -1;
		
	//sd_assert(msgid == device->_timeout_id);
	device->_timeout_id = INVALID_MSG_ID;
	if(errcode == MSG_CANCELLED)
	{
		return SUCCESS;
	}
	if(errcode == MSG_TIMEOUT)
		ret_val =  reporter_execute_cmd(device);
	else
		ret_val = errcode;
	
	if(ret_val!=SUCCESS)
	{
		reporter_failure_exit(device, ret_val);	
	}

	return SUCCESS;
}

_int32 reporter_notify_execute_cmd_failed(REPORTER_DEVICE* device)
{
	//	notify_res_query_shub_failed_event((DOWNLOAD_TASK*)device->_last_cmd->_task);
      LOG_DEBUG("reporter_notify_execute_cmd_failed, _retry_times=%d",device->_last_cmd->_retry_times);

	sd_free((char*)device->_last_cmd->_cmd_ptr);
	sd_free(device->_last_cmd);
	device->_last_cmd = NULL;
	return SUCCESS;
}

_int32 reporter_extend_recv_buffer(REPORTER_DEVICE* device, _u32 total_len)
{
	char* tmp = NULL;
	_int32 ret = sd_malloc(total_len, (void**)&tmp);
	LOG_DEBUG("reporter_extend_recv_buffer:total_len=%u",total_len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("res_query_extend_recv_buffer, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(tmp, device->_recv_buffer, device->_had_recv_len);
	sd_free(device->_recv_buffer);
	device->_recv_buffer = tmp;
	device->_recv_buffer_len = total_len;
	return SUCCESS;
}

/*handle reporter error*/
_int32 reporter_failure_exit(REPORTER_DEVICE* device, _int32 errcode)
{
	_int32 ret_val = SUCCESS;
	_u16 last_cmd_type = 0;
	LOG_DEBUG("reporter_failure_exit:0x%x,  errcode is %d", device, errcode);

	if(device->_socket != INVALID_SOCKET)
	{
#ifdef REPORTER_TEST
                    _u32 count = 0;
                    socket_proxy_peek_op_count(device->_socket, DEVICE_SOCKET_TCP, &count);
                    sd_assert(count==0);
#endif

		socket_proxy_close(device->_socket);
		device->_socket = INVALID_SOCKET;
	}
	if(device->_last_cmd == NULL)
	{
		//sd_assert(FALSE);
		return SUCCESS;
 	}
	
	last_cmd_type = device->_last_cmd->_cmd_type;
	reporter_notify_execute_cmd_failed(device);
	
	//return ret_val;

//ErrHandler:
	
		if(device->_type==REPORTER_LICENSE)
		{
			tm_notify_license_report_result(-1, errcode, ret_val );
		}
#ifdef UPLOAD_ENABLE
		else
		if(device->_type == REPORTER_PHUB)
		{
			switch(last_cmd_type)
			{
				case REPORT_ISRC_ONLINE_CMD_TYPE:
					LOG_DEBUG("REPORT_ISRC_ONLINE_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
					ulm_isrc_online_resp(FALSE, errcode);
				break;	
				case REPORT_RC_LIST_CMD_TYPE:
					LOG_DEBUG("REPORT_RC_LIST_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
					ulm_report_rclist_resp( FALSE);
				break;	
				case REPORT_INSERT_RC_CMD_TYPE:
					LOG_DEBUG("REPORT_INSERT_RC_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
					ulm_insert_rc_resp(FALSE);
				break;	
				case REPORT_DELETE_RC_CMD_TYPE:
					LOG_DEBUG("REPORT_DELETE_RC_CMD_TYPE Error:errcode = %d,ret_val=%d !",errcode,ret_val);
					ulm_delete_rc_resp(FALSE);
				break;
				default:
					LOG_DEBUG("UNKNOWN rc command type error![_cmd_type=%d,errcode=%d,ret_val=%d]!",last_cmd_type,errcode,ret_val);
			}
		}
#endif

	LOG_DEBUG("reporter_failure_exit,list_size=%d",list_size(&device->_cmd_list));
	if(device->_last_cmd == NULL && list_size(&device->_cmd_list) != 0)		/*是否仅有本地还有请求*/
	{
		ret_val = reporter_execute_cmd(device);
		if(ret_val!=SUCCESS)
		{
			reporter_failure_exit(device, ret_val);
		}
	}

		
	return SUCCESS;
	
}

#ifdef EMBED_REPORT

_int32 reporter_init_timeout_event(BOOL from_server)
{
	REPORTER_SETTING* setting =NULL;
	setting = get_reporter_setting();				
	g_report_online_peer_time_id = INVALID_MSG_ID;
	sd_assert( setting->_online_peer_report_interval > 0 );

	LOG_DEBUG("reporter_handle_timeout,interval:%u", setting->_online_peer_report_interval );

	if(from_server)
	{
		/* 根据服务器返回的时间间隔上报 */
		start_timer(reporter_handle_timeout, NOTICE_ONCE, setting->_online_peer_report_interval *1000, 0,NULL,&g_report_online_peer_time_id); 
	}
	else
	{
		/* 程序初始化,延时5秒上报 */
		start_timer(reporter_handle_timeout, NOTICE_ONCE, 5000, 0,NULL,&g_report_online_peer_time_id); 
	}

	return SUCCESS;
}

_int32 reporter_uninit_timeout_event()
{
	if( g_report_online_peer_time_id != INVALID_MSG_ID )
	{
		cancel_timer(g_report_online_peer_time_id);
		g_report_online_peer_time_id = INVALID_MSG_ID;
	}

	return SUCCESS;
}

_int32 reporter_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("reporter_handle_timeout");
	
#if defined(MOBILE_PHONE)

	// 手雷
	if(errcode == MSG_TIMEOUT
		&& msgid == g_report_online_peer_time_id )
	{
		char peer_id[PEER_ID_SIZE];
		g_report_online_peer_time_id = INVALID_MSG_ID;

		ret_val = reporter_get_peerid(peer_id, PEER_ID_SIZE);
		if ((ret_val != SUCCESS) || (!sd_is_net_ok()))
		{
			// peerid未就绪或网络没连接好。
			ret_val = start_timer(reporter_handle_timeout, NOTICE_ONCE, 2000, 0,NULL,&g_report_online_peer_time_id);	
			CHECK_VALUE(ret_val);
		}
		else
		{
			static _int32 timeout_count = 0;
			if (timeout_count % 10 == 0)
			{
				/* 每隔(get_reporter_setting()->_online_peer_report_interval * 1000)秒钟(10分钟)上报一次 */
				ret_val = reporter_mobile_network();
			}
			timeout_count ++;
			/* 增加开关控制文件上报 */
			if(g_enable_file_report)
			{
				ret_val = reporter_from_file();
			}
			
			if(ret_val!=SUCCESS)
			{
				// 上报不成功，可能是正在写文件。无所谓，下次再试。
			}

			/* 每10分钟定时循环 */
			ret_val = start_timer(reporter_handle_timeout, NOTICE_ONCE,
				get_reporter_setting()->_online_peer_report_interval * 100, 0,NULL,&g_report_online_peer_time_id);	//故意乘以100而不是1000是因为reporter_from_file需要快点上报
			CHECK_VALUE(ret_val);
		}
	}

#else

	if(errcode == MSG_TIMEOUT
		&& msgid == g_report_online_peer_time_id )
	{
		g_report_online_peer_time_id = INVALID_MSG_ID;
		ret_val = emb_reporter_online_peer_state();
		if(ret_val!=SUCCESS)
		{
			/* 本次上报失败,延时1分钟再尝试上报 */
			ret_val = start_timer(reporter_handle_timeout, NOTICE_ONCE,60* 1000, 0,NULL,&g_report_online_peer_time_id);  
			if(ret_val!=SUCCESS)
			{
				LOG_ERROR("ERROR!ret_val= %d",ret_val);
				CHECK_VALUE(ret_val);
			}
		}
	}
	
#endif
	
	return SUCCESS;

}

_int32 reporter_set_enable_file_report(BOOL is_enable)
{
	g_enable_file_report = is_enable;
	return SUCCESS;	
}

#endif

