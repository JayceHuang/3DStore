#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/utility.h"
#include "utility/md5.h"
#include "utility/time.h"
#include "utility/logid.h"
#define  LOGID LOGID_RES_QUERY
#include "utility/logger.h"
#include "platform/sd_network.h"

#include "socket_proxy/socket_proxy_interface_imp.h"

#include "res_query_impl.h"
#include "res_query_setting.h"
#include "res_query_cmd_handler.h"
#include "res_query_cmd_define.h"
#include "res_query_interface_imp.h"
#include "res_query_dphub.h"
#include "download_task/download_task.h"

#define RES_QUERY_TIMEOUT 5000
#define RES_QUERY_CMD_MAX_TIME 50

_int32 res_query_commit_cmd(HUB_DEVICE* device, _u16 cmd_type, char* buffer, _u32 len, void* callback, 
                            void* user_data, _u32 cmd_seq,BOOL not_add_res, void *extra_data, _u8* aes_key)
{
	_int32 ret =  0;
	RES_QUERY_CMD* cmd = NULL;

	ret = sd_malloc(sizeof(RES_QUERY_CMD),(void**)&cmd);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("res_query_commit_cmd, but sd_malloc failed, errcode = %d.", ret);
		sd_free(buffer);		//记得把接管的buffer释放掉
		buffer = NULL;
		return ret;
	}
	sd_memset(cmd,0,sizeof(RES_QUERY_CMD));
	cmd->_cmd_ptr = buffer;
	cmd->_cmd_len = len;
	cmd->_callback = callback;
	cmd->_user_data = user_data;
	cmd->_cmd_type = cmd_type;
	cmd->_retry_times = 0;
	cmd->_cmd_seq = cmd_seq;
	cmd->_not_add_res = not_add_res;
    cmd->_extra_data = extra_data;
	if(aes_key != NULL)
	{
		cmd->_is_req_with_rsa = TRUE;
		sd_memcpy(cmd->_aes_key, aes_key, 16);
	}
	ret = list_push(&device->_cmd_list, cmd);
	LOG_DEBUG("res_query_commit_cmd cmd_type=%d, userdata:0x%x, cmd:0x%x", cmd_type,  user_data, cmd);
	if(ret != SUCCESS)
	{
		LOG_ERROR("res_query_commit_cmd, but list_push failed, errcode = %d.", ret);
		sd_free(buffer);
		sd_free(cmd);
		cmd = NULL;
		sd_assert(FALSE);
		return ret;
	}
	if(device->_socket == INVALID_SOCKET && list_size(&device->_cmd_list) == 1)
	{	//没有任何请求在执行
		ret = res_query_execute_cmd(device);
	}
	return ret;
}

_int32 res_query_execute_cmd(HUB_DEVICE* device)
{
	_int32 ret = SUCCESS;
	sd_assert(device->_last_cmd == NULL);
	if(list_size(&device->_cmd_list) == 0)
	{	/*no command to execute*/
		if(device->_socket != INVALID_SOCKET)
		{
#ifdef _DEBUG
			_u32 count = 0;
			LOG_DEBUG("res_query close socket, device = %d, socket = %u", device->_hub_type, device->_socket);
			socket_proxy_peek_op_count(device->_socket, DEVICE_SOCKET_TCP, &count);
			sd_assert(count == 0);
#endif /* _DEBUG */
			socket_proxy_close(device->_socket);
			device->_socket = INVALID_SOCKET;
		}
		SAFE_DELETE(device->_recv_buffer);
		device->_recv_buffer_len = 0;
		device->_had_recv_len = 0;

        if (device->_hub_type == DPHUB_NODE)
        {
            pop_device_from_dnode_list(device);
            sd_free(device);
            device = NULL;
        }

		return SUCCESS;		
	}
	list_pop(&device->_cmd_list, (void**)&device->_last_cmd);
	sd_assert(device->_last_cmd != NULL);
	LOG_DEBUG("res_query_execute_cmd, device = %d, task = 0x%x.", device->_hub_type, device->_last_cmd->_user_data);
	/*some command waiting for process in device->_cmd_queue*/
	if(device->_socket == INVALID_SOCKET)
	{	/*connect to hub*/
		RES_QUERY_SETTING* setting = get_res_query_setting();				/*can be optimized*/
		ret=socket_proxy_create(&device->_socket, SD_SOCK_STREAM);
		if(ret != SUCCESS)
		{	/*connect failed*/
			return res_query_handle_network_error(device, ret);
		}
		
		//CHECK_VALUE(ret);
		LOG_DEBUG("res_query create socket, device = %d, socket = %u", device->_hub_type, device->_socket);
		switch(device->_hub_type)
		{
		case SHUB:
			{
				LOG_DEBUG("SHUB(%s:%u)", setting->_shub_addr,  setting->_shub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_shub_addr, setting->_shub_port, 
					res_query_handle_connect_callback, device);
				break;
			}
		case PHUB:
			{
				LOG_DEBUG("PHUB(%s:%u)", setting->_phub_addr, setting->_phub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_phub_addr, setting->_phub_port, 
					res_query_handle_connect_callback, device);
				break;
			}
		case PARTNER_CDN:
			{
				LOG_DEBUG("PARTNER_CDN(%s:%u)", setting->_partner_cdn_addr, setting->_partner_cdn_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_partner_cdn_addr, setting->_partner_cdn_port, 
					res_query_handle_connect_callback, device);
				break;
			}
		case TRACKER:
			{
				LOG_DEBUG("TRACKER(%s:%u)", setting->_tracker_addr, setting->_tracker_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_tracker_addr, setting->_tracker_port, 
					res_query_handle_connect_callback, device);
				break;
			}
		case BT_HUB:
			{
				LOG_DEBUG("BT_HUB(%s:%u)", setting->_bt_hub_addr, setting->_bt_hub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_bt_hub_addr, setting->_bt_hub_port, 
					res_query_handle_connect_callback, device);
				break;
			}
#ifdef ENABLE_CDN
		case CDN_MANAGER:
			{
				LOG_DEBUG("CDN_MANAGER(%s:%u)", setting->_cdn_manager_addr, setting->_cdn_manager_port);
			      return socket_proxy_connect_by_domain(device->_socket, setting->_cdn_manager_addr, setting->_cdn_manager_port, 
					  res_query_handle_connect_callback, device);
			      break;
			}
		case NORMAL_CDN_MANAGER:
			{
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_normal_cdn_manager_addr, setting->_normal_cdn_manager_port, res_query_handle_connect_callback, device);
				break;
			}
#ifdef ENABLE_KANKAN_CDN
		case KANKAN_CDN_MANAGER:
			{
				LOG_DEBUG("KANKAN_CDN_MANAGER(%s:%u)", setting->_kankan_cdn_manager_addr, setting->_kankan_cdn_manager_port);
			      return socket_proxy_connect_by_domain(device->_socket, setting->_kankan_cdn_manager_addr, setting->_kankan_cdn_manager_port, 
					  res_query_handle_connect_callback, device);
			      break;				
			}
#endif
#ifdef ENABLE_HSC
		case VIP_HUB:
			{
				LOG_DEBUG("VIP_HUB(%s:%u)", setting->_vip_hub_addr, setting->_vip_hub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_vip_hub_addr, setting->_vip_hub_port, 
					res_query_handle_connect_callback, device);
				break;
			}
#endif /* ENABLE_HSC */
#endif /* ENABLE_CDN */
#ifdef ENABLE_EMULE
		case EMULE_HUB:
			{
				LOG_DEBUG("EMULE_HUB(%s:%u)", setting->_emule_hub_addr, setting->_emule_hub_port);
				return socket_proxy_connect_by_domain(device->_socket, setting->_emule_hub_addr, setting->_emule_hub_port, 
					res_query_handle_connect_callback, device);
				break;
			}
#endif /* ENABLE_EMULE */
        case DPHUB_ROOT:
            {
				LOG_DEBUG("DPHUB_ROOT(%s:%u)", setting->_dphub_root_addr, setting->_dphub_root_port);
                return socket_proxy_connect_by_domain(device->_socket, setting->_dphub_root_addr, setting->_dphub_root_port, 
					res_query_handle_connect_callback, device);
                break;
            }
        case DPHUB_NODE:
            {
				LOG_DEBUG("DPHUB_NODE(%s:%u)", device->_host, device->_port);
                return socket_proxy_connect_by_domain(device->_socket, device->_host, device->_port, 
					res_query_handle_connect_callback, device);
                break;
            }
		case CONFIG_HUB:
			{
				LOG_DEBUG("CONFIG_HUB(%s:%u)", setting->_config_hub_addr, setting->_config_hub_port);
				ret = socket_proxy_connect_by_domain(device->_socket, setting->_config_hub_addr, setting->_config_hub_port,
					res_query_handle_connect_callback, device);
				break;
			}
        case EMULE_TRACKER:
            {
				LOG_DEBUG("EMULE_TRACKER(%s:%u)", setting->_emule_trakcer_addr, setting->_emule_tracker_port);
                return socket_proxy_connect_by_domain(device->_socket, setting->_emule_trakcer_addr, setting->_emule_tracker_port,
					res_query_handle_connect_callback, device);
                break;
            }			
		default:
			{
				sd_assert(FALSE);
			}
		}
	}
	else
	{
		ret = socket_proxy_send(device->_socket, device->_last_cmd->_cmd_ptr, device->_last_cmd->_cmd_len,
			res_query_handle_send_callback, device);
	}
	if(ret != SUCCESS)
	{
		CHECK_VALUE(1);
		res_query_free_last_cmd(device);
	}
	else
	{
	    sd_time(&device->_last_cmd_born_time);
	}
	return ret;
}

#ifdef LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
static char* _get_server_ipstr(int fd, char* ipstr_out)
{
	struct sockaddr_in srvaddr;
	socklen_t len = sizeof srvaddr;
	int ret;

	static char ipstr[16];

	ret = getpeername(fd, (struct sockaddr*)&srvaddr, &len);
	if(ret == -1)
	{
		return NULL;
	}
	if(ipstr_out)
	{
		strcpy(ipstr_out, inet_ntoa(srvaddr.sin_addr));
		return ipstr_out;
	}

	strncpy(ipstr, inet_ntoa(srvaddr.sin_addr), sizeof ipstr);
	return ipstr;
}
#endif

/*connect event notify*/
_int32 res_query_handle_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data)
{
	HUB_DEVICE* device = (HUB_DEVICE*)user_data;
    
	if(errcode == MSG_CANCELLED)
	{
		sd_assert(pending_op_count == 0);
		if(device->_is_cmd_time_out)
		{
		    res_query_handle_network_error(device, ERR_RES_QUERY_CMD_TIME_OUT);
			device->_is_cmd_time_out = FALSE;
			return SUCCESS;
		}
		return res_query_handle_cancel_msg(device);

	}
	LOG_DEBUG("res_query_handle_connect_callback :errcode = %d,pending_op_count=%u,_hub_type=%u",
		errcode,pending_op_count,device->_hub_type);
    
	if(errcode != SUCCESS)
	{	/*connect failed*/
		return res_query_handle_network_error(device, errcode);
	}
	sd_assert(device->_last_cmd != NULL);
#ifdef LINUX
	do {
		char  h[] = "Host: res.res.res.res:";
		char* buf = device->_last_cmd->_cmd_ptr; // warning[为解决4G网络封杀，发包前需要修改host字段]
		char* p = strstr(buf, h);

		if(p)
		{
			int i = 0;
			int j = 0;
			char* q;
			char* ipstr = _get_server_ipstr(device->_socket, NULL);

			if(NULL == ipstr) break;

			p += 6;		/* "Host: res.res.res.res:" */
						/*       p^                 */

			q = p + 15;	/* "Host: res.res.res.res:" */
						/*                      q^  */

			while(ipstr[i] != 0) p[j++] = ipstr[i++];
			
			if(p + j < q)
			{
				i = 0;
				while(q[i] != '\r') p[j++] = q[i++];
				while(p + j < q + i) p[j++] =' ';
			}
		}
	} while(0);
#endif
	LOG_DEBUG("hub(%d) sending request: %s", device->_hub_type, device->_last_cmd->_cmd_ptr);
	return socket_proxy_send(device->_socket, device->_last_cmd->_cmd_ptr, device->_last_cmd->_cmd_len, res_query_handle_send_callback, device);
}


#ifdef ENABLE_CDN

_int32 res_query_handle_cdn_manager_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
 	HUB_DEVICE* device = (HUB_DEVICE*)user_data;
	char* pos1;
	char* pos2;
	char* pos3;
	char   host[MAX_HOST_NAME_LEN];
	char   port[8];
	char*  start_pos;
	char*  end_pos;
	_int32 ret = SUCCESS,peer_count = 0;
	res_query_cdn_manager_handler callback = (res_query_cdn_manager_handler)device->_last_cmd->_callback;

       LOG_DEBUG("res_query_handle_cdn_manager_callback errcode =%d ....", errcode);
	if(errcode == MSG_CANCELLED)
	{	
		sd_assert(pending_op_count == 0);
              LOG_DEBUG("res_query_handle_cdn_manager_callback 1.... ret = %d", errcode);
		return res_query_handle_cancel_msg(device);
	}
	if(errcode != SUCCESS)
	{
              LOG_DEBUG("res_query_handle_cdn_manager_callback 2.... ret = %d", errcode);
		return res_query_handle_network_error(device, errcode);
	}
	/*recv success*/
	device->_had_recv_len += had_recv;
	/*handle CDN manager*/
	if(device->_hub_type == CDN_MANAGER)
	{
		/**/
		if( device->_had_recv_len > device->_recv_buffer_len )
		{
		          return res_query_handle_network_error(device, errcode);
		}
		/*device->_recv_buffer 已经在recv时被填充了 , by zhengzhe 20120416*/		
		//sd_memcpy(device->_recv_buffer+device->_had_recv_len, buffer, had_recv);
		//device->_had_recv_len += had_recv;
		device->_recv_buffer[device->_recv_buffer_len-1] = '\0';
              LOG_DEBUG("res_query_handle_cdn_manager_callback recv:[%s] ....", device->_recv_buffer);
	       end_pos = sd_strstr(device->_recv_buffer, "</script>", 0);
		if( NULL == sd_strstr(device->_recv_buffer, "<script>", 0) 
			|| NULL ==  end_pos)
		{
                        LOG_DEBUG("res_query_handle_cdn_manager_callback 3....");
	                 return socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer+ device->_had_recv_len , MAX_CDN_MANAGER_RESP_LEN-device->_had_recv_len , res_query_handle_recv_callback, device);
		}

		if(device->_last_cmd->_user_data!=NULL)
		{
			/*found <script></script> */
			start_pos = device->_recv_buffer;
			do 
			{
				pos1 = sd_strstr(start_pos, "{ip:\"", 0 );
				pos2 = sd_strstr(start_pos, "\",port:", 0 );
				pos3 = sd_strstr(start_pos, "}", 0 );
				if(NULL == pos1 || NULL == pos2 || NULL == pos3 
					|| ((_int32) (pos2-pos1) > (MAX_HOST_NAME_LEN+sd_strlen("{ip:\"")) ) || ((_int32) (pos3-pos2) > (8+sd_strlen("\",port:")) )
					|| pos1 > end_pos || pos2 > end_pos || pos3 > end_pos)
				{
			              LOG_DEBUG("res_query_handle_cdn_manager_callback 4.... ret = %d", errcode);
					break;
				}
				start_pos = pos3 + sd_strlen("}");
				sd_memset(host, 0, sizeof(host));
				sd_memset(port, 0, sizeof(port));
				sd_strncpy(host, pos1 + sd_strlen("{ip:\""), pos2 - pos1 - sd_strlen("{ip:\""));
				sd_strncpy(port, pos2 + sd_strlen("\",port:"), pos3 - pos2 - sd_strlen("\",port:"));
			       LOG_DEBUG("res_query_handle_cdn_manager_callback found a cdn host=%s port=%d", host, (_u16)sd_atoi(port));
#ifdef _DEBUG
#ifdef IPAD_KANKAN
		printf("\nres_query_handle_cdn_manager_callback found a cdn host=%s port=%d\n", host, (_u16)sd_atoi(port));
#endif /* IPAD_KANKAN */
#endif /* _DEBUG */
				ret = (*callback)(SUCCESS , device->_last_cmd->_user_data, SUCCESS ,FALSE, host,  (_u16)sd_atoi(port));
		              LOG_DEBUG("handle_res_query_cdn_manager_handler ret = %d...", ret);
				peer_count++;
		
			}while(1);

			LOG_ERROR("notify_res_query_cdn_manager_success, socket = %u, res_num = %u",device->_socket, peer_count);
#ifdef _DEBUG
#ifdef IPAD_KANKAN
		printf("\n notify_res_query_cdn_manager_success, socket = %u, res_num = %u\n",device->_socket, peer_count);
#endif /* IPAD_KANKAN */
#endif /* _DEBUG */
			ret = (*callback)(SUCCESS , device->_last_cmd->_user_data, SUCCESS ,TRUE, NULL,  (_u16)0);
		}
		/*the first request in queue had processed, remove*/
		res_query_free_last_cmd(device);
		return res_query_execute_cmd(device);
	}
	else
	{
	     sd_assert(FALSE);
	}

	/*parse cdn manager response */
	/*
	//<html>
	//	<head>
	//	<meta http-equiv=Content-Type content="text/html; charset=gb2312">
	//	<script>
	//	document.domain = "xunlei.com";
	//	var jsonObj = {cdnlist1:[
	//	{ip:"192.168.7.47",port:8080},
	//	{ip:"192.168.7.46",port:8080}
	//	]
	//	}
	//	</script>
	//	</head>
	//<body>
	//</body>
	//</html>

	*/
	
       return SUCCESS;
}

#ifdef ENABLE_KANKAN_CDN
_int32 res_query_handle_kankan_cdn_manager_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
 	HUB_DEVICE* device = (HUB_DEVICE*)user_data;
	char* pos1 = NULL, *pos2 = NULL, *pos3 = NULL, *pos4 = NULL;
	char param1[16] = {0};
	char param2[16] = {0};
	char key[64] = {0};
	char md5_key[33] = {0};
	unsigned char hash[16] = {0};
	ctx_md5 md5;
	char host[MAX_HOST_NAME_LEN];
	char port[8];
	char path[MAX_URL_LEN];
	char url[MAX_URL_LEN];
	char*  start_pos;
	char*  end_pos;
	_int32 ret = SUCCESS,server_count = 0;
	res_query_kankan_cdn_manager_handler callback = (res_query_kankan_cdn_manager_handler)device->_last_cmd->_callback;

       LOG_DEBUG("res_query_handle_cdn_manager_callback errcode =%d ....", errcode);
	if(errcode == MSG_CANCELLED)
	{	
		sd_assert(pending_op_count == 0);
              LOG_DEBUG("res_query_handle_cdn_manager_callback 1.... ret = %d", errcode);
		return res_query_handle_cancel_msg(device);
	}
	if(errcode != SUCCESS)
	{
              LOG_DEBUG("res_query_handle_cdn_manager_callback 2.... ret = %d", errcode);
		return res_query_handle_network_error(device, errcode);
	}
	/*recv success*/
	device->_had_recv_len += had_recv;
	/*handle CDN manager*/
	if(device->_hub_type == KANKAN_CDN_MANAGER)
	{
		/**/
		if( device->_had_recv_len > device->_recv_buffer_len )
		{
		          return res_query_handle_network_error(device, errcode);
		}
		//sd_memcpy(device->_recv_buffer+device->_had_recv_len, buffer, had_recv);
		//device->_had_recv_len += had_recv;
		device->_recv_buffer[device->_recv_buffer_len-1] = '\0';
              LOG_DEBUG("res_query_handle_kankan_cdn_manager_callback recv:[%s] ....", device->_recv_buffer);
	       end_pos = sd_strstr(device->_recv_buffer, "</script>", 0);
		if( NULL == sd_strstr(device->_recv_buffer, "<script>", 0) 
			|| NULL ==  end_pos)
		{
                        LOG_DEBUG("res_query_handle_kankan_cdn_manager_callback 3....");
	                 return socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer+ device->_had_recv_len , MAX_CDN_MANAGER_RESP_LEN-device->_had_recv_len , res_query_handle_recv_callback, device);
		}

		if(device->_last_cmd->_user_data!=NULL)
		{
			/*found <script></script> */
			start_pos = device->_recv_buffer;

			pos1 = sd_strstr(start_pos, "{param1:", 0 );
			if( NULL == pos1)
			{
			          return res_query_handle_network_error(device, ERR_RES_QUERY_EXTRACT_CMD_FAIL);
			}
			pos2 = sd_strstr(pos1, ",param2:", 0 );
			if( NULL == pos2)
			{
			          return res_query_handle_network_error(device, ERR_RES_QUERY_EXTRACT_CMD_FAIL);
			}
			pos3 = sd_strstr(pos2, "}", 0 );
			if( NULL == pos3)
			{
			          return res_query_handle_network_error(device, ERR_RES_QUERY_EXTRACT_CMD_FAIL);
			}
			
			sd_strncpy(param1, pos1 + sd_strlen("{param1:"), pos2 - pos1 - sd_strlen("{param1:"));
			sd_strncpy(param2, pos2 + sd_strlen(",param2:"), pos3 - pos2 - sd_strlen(",param2:"));
			sd_snprintf(key, 64, "xl_mp43651%s%s", param1, param2);

			md5_initialize(&md5);
			md5_update(&md5, (const unsigned char*)key, sd_strlen(key));
			md5_finish(&md5, hash);
			str2hex((const char*)hash, 16, md5_key, 32);			
            sd_string_to_low_case(md5_key);
            
			do 
			{
				pos1 = sd_strstr(start_pos, "{ip:\"", 0 );
				pos2 = sd_strstr(start_pos, "\",port:", 0 );
				pos3 = sd_strstr(start_pos, ",path:\"", 0);
				pos4 = sd_strstr(start_pos, "\"}", 0 );
				if(NULL == pos1 || NULL == pos2 || NULL == pos3 || NULL == pos4
					|| ((_int32) (pos2-pos1) > (MAX_HOST_NAME_LEN+sd_strlen("{ip:\"")) ) || ((_int32) (pos3-pos2) > (8+sd_strlen("\",port:")) ) || ((_int32) (pos4-pos3) > (MAX_URL_LEN - MAX_HOST_NAME_LEN +sd_strlen(",path:\"")) )
					|| pos1 > end_pos || pos2 > end_pos || pos3 > end_pos || pos4 > end_pos)
				{
			              LOG_DEBUG("res_query_handle_kankan_cdn_manager_callback 4.... ret = %d", errcode);
					break;
				}
				start_pos = pos4 + sd_strlen("\"}");
				sd_memset(host, 0, sizeof(host));
				sd_memset(port, 0, sizeof(port));
				sd_memset(path, 0, sizeof(path));
				sd_memset(url, 0, sizeof(url));
				
				sd_strncpy(host, pos1 + sd_strlen("{ip:\""), pos2 - pos1 - sd_strlen("{ip:\""));
				sd_strncpy(port, pos2 + sd_strlen("\",port:"), pos3 - pos2 - sd_strlen("\",port:"));
				sd_strncpy(path, pos3 + sd_strlen(",path:\""), pos4 - pos3 - sd_strlen(",path:\""));

				/*
				http://${ip}/${path}?key=`md5("xl_mp43651"${param1}${param2})`&key1=${param2}[&start=${start_pos}][&restart=${restart_pos}][&end=end_pos]&
				*/
				sd_snprintf(url, MAX_URL_LEN , "http://%s%s?key=%s&key1=%s&\n", host, path, md5_key, param2);
                //sd_snprintf(url, MAX_URL_LEN , "http://%s%s", host, path);
				
			       LOG_DEBUG("res_query_handle_kankan_cdn_manager_callback found a cdn url=%s", url);
#ifdef _DEBUG
#ifdef IPAD_KANKAN
		printf("\res_query_handle_kankan_cdn_manager_callback found a cdn url=%s\n", url);
#endif /* IPAD_KANKAN */
#endif /* _DEBUG */

				ret = (*callback)(SUCCESS , device->_last_cmd->_user_data, SUCCESS ,FALSE, url, sd_strlen(url));
		              LOG_DEBUG("res_query_handle_kankan_cdn_manager_callback ret = %d...", ret);
				server_count++;
		
			}while(1);

			LOG_ERROR("notify_res_query_cdn_manager_success, socket = %u, res_num = %u",device->_socket, server_count);
#ifdef _DEBUG
#ifdef IPAD_KANKAN
		printf("\n notify_res_query_cdn_manager_success, socket = %u, res_num = %u\n",device->_socket, server_count);
#endif /* IPAD_KANKAN */
#endif /* _DEBUG */
			ret = (*callback)(SUCCESS , device->_last_cmd->_user_data, SUCCESS ,TRUE, NULL,  0);
		}
		/*the first request in queue had processed, remove*/
		res_query_free_last_cmd(device);
		return res_query_execute_cmd(device);
	}
	else
	{
	     sd_assert(FALSE);
	}

	/*parse cdn manager response */
	/*
	<html>
	<head>
	<meta http-equiv=Content-Type content="text/html; charset=gb2312">
	<script>
	document.domain = "xunlei.com";
	var jsonObj = {
	cdnlist1:[
	{ip:"114.80.189.70",port:8080,path:"/data3/cdn_transfer/CA/DB/ca8180b37970dd6d511d2ca2005ed94c52897adb_1.flv"},
	{ip:"61.139.103.132",port:8080,path:"/data3/cdn_transfer/CA/DB/ca8180b37970dd6d511d2ca2005ed94c52897adb_1.flv"}
	]
	}
	var jsCheckOutObj = {param1:2703790455,param2:1334544949}
	</script>
	</head>
	<body>
	</body>
	</html>

	*/
	
       return SUCCESS;
}
#endif /* ENABLE_KANKAN_CDN */
#endif /* ENABLE_CDN */

/*send event notify*/
_int32 res_query_handle_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	HUB_DEVICE* device = (HUB_DEVICE*)user_data;
	_int32 ret = SUCCESS;

	LOG_DEBUG("res_query_handle_send_callback errcode=%d...", errcode);
	if(errcode == MSG_CANCELLED)
	{
		sd_assert(pending_op_count == 0);
		if(device->_is_cmd_time_out)
		{
		    res_query_handle_network_error(device, ERR_RES_QUERY_CMD_TIME_OUT);
			device->_is_cmd_time_out = FALSE;
			return SUCCESS;
		}
		return res_query_handle_cancel_msg(device);
	}
	if(errcode != SUCCESS)
	{
		return res_query_handle_network_error(device, errcode);
	}
#ifdef 	RES_QUERY_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		str2hex( buffer, had_send, test_buffer, 1024);
		LOG_DEBUG( "res_query_handle_send_callback:buffer[%u]=%s" ,had_send, buffer);
		printf("\n res_query_handle_send_callback:buffer[%u]=%s\n" ,had_send, buffer);
		LOG_DEBUG( "res_query_handle_send_callback:buffer in hex[%u]=%s" ,had_send, test_buffer);
		printf("\n res_query_handle_send_callback:buffer in hex[%u]=%s\n" ,had_send, test_buffer);
	}
#endif 
	/*send success*/
	LOG_DEBUG("res_query had send cmd, command = %u, device = %d", device->_last_cmd->_cmd_type, device->_hub_type);
	device->_had_recv_len = 0;
	if(device->_recv_buffer == NULL)
	{
		ret = sd_malloc(3096, (void**)&device->_recv_buffer);
		device->_recv_buffer_len = 3096;
        sd_memset(device->_recv_buffer, 0, device->_recv_buffer_len);
	}
	if(ret != SUCCESS)
	{	//sd_malloc failed
		return res_query_handle_network_error(device, ret);
	}
	
#ifdef ENABLE_CDN
	if(device->_hub_type == CDN_MANAGER)
	{
	     ret =  socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer, MAX_CDN_MANAGER_RESP_LEN, res_query_handle_cdn_manager_callback, device);
	}
#ifdef ENABLE_KANKAN_CDN
	else if(device->_hub_type == KANKAN_CDN_MANAGER)
	{
	     ret =  socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer, MAX_CDN_MANAGER_RESP_LEN, res_query_handle_kankan_cdn_manager_callback, device);
	
	}
#endif    
	else
	{
#endif	
	     ret =  socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer, device->_recv_buffer_len, res_query_handle_recv_callback, device);
#ifdef ENABLE_CDN
	}
#endif	
	return ret;
}

/*recv event notify*/
_int32 res_query_handle_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	_int32 ret = SUCCESS;
	HUB_DEVICE* device = (HUB_DEVICE*)user_data;
	char* http_header = NULL;
	_u32 http_header_len = 0;
    LOG_DEBUG("res_query_handle_recv_callback  errcode=%d,had_recv=%u...device->_had_recv_len=%u, hub_type=%d, user_data=0x%x", 
		errcode,had_recv, device->_had_recv_len, device->_hub_type, user_data);

#ifdef _DEBUG
	if((had_recv!=0)&&(device->_recv_buffer_len!=0)&&(device->_recv_buffer!=NULL))
	{
       	LOG_DEBUG("[%u]%s", device->_had_recv_len + had_recv, device->_recv_buffer);
	}
#endif /* _DEBUG */
	if(errcode == MSG_CANCELLED)
	{	
		sd_assert(pending_op_count == 0);
		if(device->_is_cmd_time_out)
		{
		    res_query_handle_network_error(device, ERR_RES_QUERY_CMD_TIME_OUT);
			device->_is_cmd_time_out = FALSE;
			return SUCCESS;
		}

		return res_query_handle_cancel_msg(device);
	}
	if((errcode != SUCCESS)&&(had_recv==0))
	{
		return res_query_handle_network_error(device, errcode);
	}
	/*recv success*/
	device->_had_recv_len += had_recv;
#ifdef ENABLE_CDN
	/*handle CDN manager*/
	if(device->_hub_type == CDN_MANAGER)
	{
		/*在里面已经加了had_recv, by zhengzhe 20120416*/		
		device->_had_recv_len -= had_recv;
	       LOG_DEBUG("res_query_handle_recv_callback  res_query_handle_cdn_manager_callback...");
		return res_query_handle_cdn_manager_callback(errcode, pending_op_count,  buffer, had_recv,  user_data);
	}
#ifdef ENABLE_KANKAN_CDN
	else if(device->_hub_type == KANKAN_CDN_MANAGER)
	{
		/*在里面已经加了*/
		device->_had_recv_len -= had_recv;
	       LOG_DEBUG("res_query_handle_recv_callback  res_query_handle_kankan_cdn_manager_callback...");
		return res_query_handle_kankan_cdn_manager_callback(errcode, pending_op_count,  buffer, had_recv,  user_data);		
	}
#endif
#endif	/* ENABLE_CDN */
	http_header = sd_strstr(device->_recv_buffer, "\r\n\r\n", 0);
	if(http_header == NULL)		//没找到，继续收
	{
		sd_assert(device->_recv_buffer_len > device->_had_recv_len);
		return socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer + device->_had_recv_len, device->_recv_buffer_len - device->_had_recv_len, res_query_handle_recv_callback, device);
	}

	//已经收到http头部了
	http_header_len = http_header - device->_recv_buffer + 4;

#if defined(MOBILE_PHONE)
	if(NT_GPRS_WAP & sd_get_net_type())
	{
		if(is_cmwap_prompt_page(device->_recv_buffer,http_header_len) )
		{
			ret = socket_proxy_send(device->_socket, device->_last_cmd->_cmd_ptr, device->_last_cmd->_cmd_len, res_query_handle_send_callback, device);
			if(ret != SUCCESS)
			{
				return res_query_handle_network_error(device, ret);
			}

			return SUCCESS;
		}
	}
#endif /* MOBILE_PHONE */			
	if(device->_last_cmd->_is_req_with_rsa)
	{
		 return res_query_handle_rsa_encountered_resp(device, http_header_len);
	}
	else if(device->_had_recv_len - http_header_len >= HUB_CMD_HEADER_LEN)
	{	/*have received header, (protocol version 5.0)*/
		_u32 version, sequence, body_len, total_len;
		char* buffer = device->_recv_buffer + http_header_len;
		_int32 buffer_len = device->_had_recv_len - http_header_len;
		sd_get_int32_from_lt(&buffer, &buffer_len, (_int32*)&version);
		sd_get_int32_from_lt(&buffer, &buffer_len, (_int32*)&sequence);
		sd_get_int32_from_lt(&buffer, &buffer_len, (_int32*)&body_len);
		total_len = body_len + HUB_CMD_HEADER_LEN + http_header_len;
		if( total_len > 1024 * 1024 )
		{	/*length invalid, should be wrong protocol*/
			return res_query_handle_network_error(device, ERR_RES_QUERY_EXTRACT_CMD_FAIL);
		}
		/*judge receive data's integrity*/
		if(total_len == device->_had_recv_len)
		{	/*receive a complete protocol package*/
			handle_recv_resp_cmd(device->_recv_buffer + http_header_len, device->_had_recv_len - http_header_len, device);	/*process protocol package*/
            res_query_free_last_cmd(device);
            return res_query_execute_cmd(device);
        }
		else if(total_len > device->_had_recv_len)
		{	/*receive a uncomplete package*/
			if(total_len > device->_recv_buffer_len)
			{	/*extend receive buffer*/
				ret = res_query_extend_recv_buffer(device, total_len);
				if(ret != SUCCESS)
					return res_query_handle_network_error(device, ret);
			}
			return socket_proxy_recv(device->_socket, device->_recv_buffer + device->_had_recv_len, total_len - device->_had_recv_len, res_query_handle_recv_callback, device);
		}
	}
	else		/*hub->_had_recv_len < HUB_CMD_HEADER_LEN*/
	{
		sd_assert(device->_recv_buffer_len > device->_had_recv_len);
		return socket_proxy_uncomplete_recv(device->_socket, device->_recv_buffer + device->_had_recv_len, device->_recv_buffer_len - device->_had_recv_len, res_query_handle_recv_callback, device);
	}
	return SUCCESS;
}

_int32 res_query_handle_cancel_msg(HUB_DEVICE* device)
{
#ifdef _DEBUG
	_u32 count = 0;
	socket_proxy_peek_op_count(device->_socket, DEVICE_SOCKET_TCP, &count);
	LOG_DEBUG("res_query_handle_cancel_msg, op_count = %u.", count);
	sd_assert(count == 0);
#endif /* _DEBUG */
	sd_assert(device->_last_cmd != NULL);
	if(device->_last_cmd != NULL)
	{
		res_query_free_last_cmd(device);
	}
	socket_proxy_close(device->_socket);
	device->_socket = INVALID_SOCKET;
	SAFE_DELETE(device->_recv_buffer);
	device->_recv_buffer = NULL;
	device->_recv_buffer_len = 0;
	sd_assert(list_size(&device->_cmd_list) == 0);
	return SUCCESS;
}

/*handle network error*/
_int32 res_query_handle_network_error(HUB_DEVICE* device, _int32 errcode)
{
	static RES_QUERY_SETTING* setting = NULL;	
#ifdef _DEBUG
	_u32 count = 0;
	socket_proxy_peek_op_count(device->_socket, DEVICE_SOCKET_TCP, &count);
	LOG_DEBUG("res_query_handle_cancel_msg, op_count = %u.", count);
	sd_assert(count == 0);
#endif
	LOG_DEBUG("res_query_handle_network_error, device type is %d, errcode is %d", device->_hub_type, errcode);
	socket_proxy_close(device->_socket);
	device->_socket = INVALID_SOCKET;
	SAFE_DELETE(device->_recv_buffer);
	device->_recv_buffer_len = 0;
	device->_had_recv_len = 0;
	if(device->_last_cmd == NULL)
	{
		sd_assert(FALSE);
		return errcode;
	}
	if((device->_last_cmd->_user_data == NULL) 
       && (device->_last_cmd->_cmd_type != ENROLLSP1) 
       && (device->_last_cmd->_cmd_type != DPHUB_OWNER_QUERY_ID))
	{	//该命令已经被cancel了，因为上层cancel一个正在执行的命令时，只是把device->_last_cmd->_user_data置为NULL
		//删除该命令，不需要重试该命令
		return res_query_free_last_cmd(device);
	}
	++device->_last_cmd->_retry_times;
	setting = get_res_query_setting();
	if(device->_last_cmd->_retry_times > setting->_cmd_retry_times)
	{
		return res_query_notify_execute_cmd_failed(device);		
	}
	else
	{
		LOG_DEBUG("res_query failed, errcode = %d, device_type = %d, task = 0x%x, start timer to retry query.", errcode, device->_hub_type, device->_last_cmd->_user_data);
		list_push(&device->_cmd_list, device->_last_cmd);
		device->_last_cmd = NULL;
		return start_timer(res_query_retry_handler, NOTICE_ONCE, RES_QUERY_TIMEOUT, 0, device, &device->_timeout_id);
	}
}

_int32 res_query_retry_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	HUB_DEVICE* device = (HUB_DEVICE*)msg_info->_user_data;
	sd_assert(notice_count_left == 0);
	device->_timeout_id = INVALID_MSG_ID;
	if(errcode == MSG_CANCELLED)
	{
		return SUCCESS;
	}
	else
	{
		return res_query_execute_cmd(device);
	}
}

_int32 res_query_notify_execute_cmd_failed(HUB_DEVICE* device)
{
	//因为在通知上层查询结果的调用中，上层可能继续投递查询请求，所以首先需要把device->_last_cmd设为NULL
	RES_QUERY_CMD* query_cmd = device->_last_cmd;
	device->_last_cmd = NULL;
	if(query_cmd->_user_data != NULL)
	{	//命令没有被cancel掉
		if(device->_hub_type == SHUB && ( query_cmd->_cmd_type == QUERY_SERVER_RES || query_cmd->_cmd_type == QUERY_FILE_RELATION || query_cmd->_cmd_type == QUERY_RES_INFO_CMD_ID || query_cmd->_cmd_type == QUERY_NEW_RES_CMD_ID))
			((res_query_notify_shub_handler)query_cmd->_callback)(query_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
		else if(device->_hub_type == PHUB)
			((res_query_notify_phub_handler)query_cmd->_callback)(query_cmd->_user_data, -1, 0, 0);
		else if(device->_hub_type == TRACKER)
			((res_query_notify_tracker_handler)query_cmd->_callback)(query_cmd->_user_data, -1, 0, 0, 0);

		else if(device->_hub_type == PARTNER_CDN)
			((res_query_notify_phub_handler)query_cmd->_callback)(query_cmd->_user_data, -1, 0, 0);

#ifdef ENABLE_CDN
		else if(device->_hub_type == CDN_MANAGER)
		{
			res_query_cdn_manager_handler callback = (res_query_cdn_manager_handler)query_cmd->_callback;
			 callback(ERR_RES_QUERY_FAILED , query_cmd->_user_data, -1 ,TRUE, NULL,  (_u16)0);
		}
#ifdef ENABLE_HSC
		else if(device->_hub_type == VIP_HUB)
			((res_query_notify_phub_handler)query_cmd->_callback)(query_cmd->_user_data, -1, 0, 0);
#endif /* ENABLE_HSC */
#endif /* ENABLE_CDN */
#ifdef ENABLE_BT
		else if(device->_hub_type == BT_HUB)
		{
			res_query_bt_info_handler callback = (res_query_bt_info_handler)query_cmd->_callback;
			callback(query_cmd->_user_data, ERR_RES_QUERY_FAILED, 0, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0);
		}
#endif
#ifdef ENABLE_EMULE
		else if(device->_hub_type == EMULE_HUB)
		{
			((res_query_notify_recv_data)query_cmd->_callback)(-1, NULL, 0, query_cmd->_user_data);
		}
#endif
        else if (device->_hub_type == DPHUB_ROOT)
        {
            ((res_query_notify_dphub_root_handler)query_cmd->_callback)(-1, -1, 0);
        }
        else if (device->_hub_type == DPHUB_NODE)
        {
            ((res_query_notify_dphub_handler)query_cmd->_callback)(query_cmd->_user_data, -1, -1, 0, FALSE);
        }
        else if (device->_hub_type == EMULE_TRACKER)
        {
            ((res_query_notify_tracker_handler)query_cmd->_callback)(query_cmd->_user_data, -1, 0, 0, 0);
        }
		else if (device->_hub_type == NORMAL_CDN_MANAGER)
		{
			((res_query_notify_phub_handler)query_cmd->_callback)(query_cmd->_user_data, -1, 0, 0);
		}
	}
	sd_free((char*)query_cmd->_cmd_ptr);
	sd_free(query_cmd);
	query_cmd = NULL;
	if(device->_last_cmd != NULL)
		return SUCCESS;
	return res_query_execute_cmd(device);			//应该继续处理后面的命令
}

_int32 res_query_extend_recv_buffer(HUB_DEVICE* device, _u32 total_len)
{
	char* tmp = NULL;
	_int32 ret = sd_malloc(total_len, (void**)&tmp);
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

_int32 res_query_free_last_cmd(HUB_DEVICE* device)
{
    LOG_DEBUG("res_query_free_last_cmd... hub_type:%d", device->_hub_type);
    if ((device != NULL) && (device->_last_cmd != NULL))
    {
        sd_free((char*)device->_last_cmd->_cmd_ptr);
	    sd_free(device->_last_cmd);
	    device->_last_cmd = NULL;	
    }
	return SUCCESS;
}

void res_query_update_last_cmd(HUB_DEVICE* device)
{
	_u32 cur_time = 0;

	sd_time(&cur_time);
	
	//LOG_DEBUG("res_query_update_last_cmd, last_time:%u, cur_time:%u, _is_cmd_time_out:%d",
	//	device->_last_cmd_born_time, cur_time, device->_is_cmd_time_out );
	if( device->_last_cmd == NULL ) return;
	
	if(cur_time < device->_last_cmd_born_time )
	{
		//sd_assert(FALSE);
		return;
	}
	if( (cur_time - device->_last_cmd_born_time) > RES_QUERY_CMD_MAX_TIME
		&& !device->_is_cmd_time_out )
	{
	    LOG_DEBUG("res_query_update_last_cmd,socket_proxy_cancel!!, last_time:%u, cur_time:%u.",device->_last_cmd_born_time, cur_time );
		socket_proxy_cancel(device->_socket, DEVICE_SOCKET_TCP);
		device->_is_cmd_time_out = TRUE;
	}
}


/* handle RSA encountered response */
_int32 res_query_handle_rsa_encountered_resp(HUB_DEVICE *p_device, _u32 http_header_len)
{
	_int32 ret_val = SUCCESS;
	char *buffer = p_device->_recv_buffer + http_header_len;
	_int32 buff_len = p_device->_had_recv_len - http_header_len;
	_u32 total_len = 0, body_len = 0;
	_u8* aeskey = p_device->_last_cmd->_aes_key;
#ifdef _DEBUG
	char aeskey_str[40] = {0};
#endif//_DEBUG

	if( p_device->_had_recv_len - http_header_len >= 4)
	{
		sd_get_int32_from_lt(&buffer, &buff_len, (_int32*)&body_len);
		total_len = 4 + body_len + http_header_len;
		LOG_DEBUG("res_query_handle_rsa_enct_resp, RSA encountered response, body_len=%u, total_len=%u, had_recv_len=%u, hub_type=%u",
			body_len, total_len, p_device->_had_recv_len, p_device->_hub_type);

		if (total_len > 1024 * 1024)
			return res_query_handle_network_error(p_device, ERR_RES_QUERY_EXTRACT_CMD_FAIL);

		if (total_len == p_device->_had_recv_len)
		{
			/* received a complete package */
			if (p_device->_socket == INVALID_SOCKET
				|| p_device->_last_cmd == NULL )
			{
				if (p_device->_last_cmd)
				{
					ret_val = res_query_free_last_cmd(p_device);
					CHECK_VALUE(ret_val);
				}
				return res_query_execute_cmd(p_device);
			}

			ret_val = handle_recv_resp_cmd_with_aes_key(buffer, body_len, p_device, aeskey);
			CHECK_VALUE(ret_val);
			ret_val = res_query_free_last_cmd(p_device);
			CHECK_VALUE(ret_val);
			return res_query_execute_cmd(p_device);
		}
		else if (total_len > p_device->_had_recv_len)
		{
			/* received an uncompelet package */
			if(total_len > p_device->_recv_buffer_len)
			{
				/*extend receive buffer*/
				ret_val = res_query_extend_recv_buffer(p_device, total_len);
				if(ret_val != SUCCESS)
					return res_query_handle_network_error(p_device, ret_val);
			}
			return socket_proxy_recv(
				p_device->_socket,
				p_device->_recv_buffer + p_device->_had_recv_len,
				total_len - p_device->_had_recv_len,
				res_query_handle_recv_callback, p_device);
		}
		else
		{
			return res_query_handle_network_error(p_device, ERR_RES_QUERY_EXTRACT_CMD_FAIL);
		}
	}
	else
	{
		sd_assert(p_device->_recv_buffer_len > p_device->_had_recv_len);
		return socket_proxy_uncomplete_recv(p_device->_socket, p_device->_recv_buffer + p_device->_had_recv_len, p_device->_recv_buffer_len - p_device->_had_recv_len, res_query_handle_recv_callback, p_device);
	}
	return SUCCESS;
}
