#include "http_server_interface.h"

#include "utility/string.h"
#include "utility/utility.h"
#include "utility/url.h"
#include "utility/map.h"
#include "utility/errcode.h"
#include "platform/sd_socket.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/notice.h"
#include "data_manager/data_manager_interface.h"
#include "data_manager/data_receive.h"
#include "utility/range.h"
#include "task_manager/task_manager.h"
#include "download_task/download_task.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/version.h"
#include "utility/peerid.h"
#include "utility/local_ip.h"
#include "p2sp_download/p2sp_task.h"
#include "bt_download/bt_task/bt_task.h"

#include "utility/define_const_num.h"
#include "socket_proxy_interface.h"
#include "platform/sd_fs.h"

#ifdef ENABLE_XV_FILE_DEC
#include "index_parser/xv_dec.h"
#endif /* ENABLE_XV_FILE_DEC */

#include "utility/logid.h"
#define	 LOGID	LOGID_HTTP_SERVER
#include "utility/logger.h"

#include "vod_data_manager/vod_data_manager_interface.h"
#include "http_server_interface.h"
#if defined(MACOS)&&defined(MOBILE_PHONE)
#include "platform/ios_interface/sd_ios_device_info.h"
#endif


#include "index_parser/flv_parser.h"
#define FLASH_PLAYER_FILE_PATH "/data/data/com.xunlei.kankan/Thunder/resource/FlvPlayer/" //"./etm_system/"

#define FLASH_PLAYER_HTML_FILE_NAME "index.html"
#define FLASH_PLAYER_HTML_FILE_TASK_ID (MAX_U32-1)

#define FLASH_PLAYER_SWF_FILE_NAME "AndroidPlayer.swf"
#define FLASH_PLAYER_SWF_FILE_TASK_ID (MAX_U32-2)

#define FLASH_PLAYER_ICO_FILE_NAME "favicon.ico"
#define FLASH_PLAYER_ICO_FILE_TASK_ID (MAX_U32-3)

#define FLASH_PLAYER_LOCAL_FILE_MARK "localfile?fullpath="
#define FLASH_PLAYER_LOCAL_FILE_TASK_ID (MAX_U32-4)

#define DEFAULT_LOCAL_FLV_HEADER_TO_FIRST_TAG_BUFFER_LEN (500*1024)
static _u64 g_flv_header_to_first_tag_len = 0;
static char * gp_flv_header_to_first_tag = NULL;
static FLVFormatContext g_flv_context;

static BOOL g_is_xv_file = FALSE;
static char * gp_xv_file_decoded_start_data = NULL;			/* 解密后的那段数据 */
static _u32 g_xv_file_decoded_start_data_len = 0;			/* 解密后的那段数据的长度 */
static _u64 g_xv_file_decoded_file_size = 0;				/* 解密后去掉加密头(结构体)的实际有效文件大小 */
static _u32 g_xv_file_context_header_len = 0;				/* 加密头(结构体)的大小 */
////////////////////////////////////////
static char g_local_file_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};
#define HTTP_LOCAL_FILE_TASK_ID (MAX_U32-5)

static _u32 g_current_task_id = 0;
static _u32 g_http_server_timer_id = 0;
static HTTP_SERVER_ACCEPT_SOCKET_DATA* gp_http_data = NULL;

//#define DEFAULT_HTTP_SERVER_TIMEOUT (30*1000)  // 10s
#define DEFAULT_HTTP_SERVER_TIMEOUT (120*1000)  // 超时时间暂时改成2 分钟

#ifdef ENABLE_ASSISTANT	

static ASSISTANT_INCOM_REQ_CALLBACK g_assistant_incom_req_callback = NULL;
static ASSISTANT_SEND_RESP_CALLBACK g_assistant_send_resp_callback = NULL;
#endif /* ENABLE_ASSISTANT */
//////////////////////////////////////////////////////////////
static SET	g_HTTP_SERVER_ACCEPT_SOCKET_DATA_set;

static SOCKET g_http_server_tcp_accept_sock = INVALID_SOCKET;

static SEARCH_INFO g_search_info;
static BOOL g_search_need_restart = FALSE;
static CACHE_ADDRESS g_search_cache_address[SEARCH_CACHE_ADDRESS_MAX_NUM];

_int32 http_server_handle_send( _int32 _errcode, _u32 notice_count_left,const char* buffer, _u32 had_send,void *user_data);

_int32 http_server_accept_socket_data_comparator(void* E1, void* E2)
{
	HTTP_SERVER_ACCEPT_SOCKET_DATA* left = (HTTP_SERVER_ACCEPT_SOCKET_DATA*)E1;
	HTTP_SERVER_ACCEPT_SOCKET_DATA* right = (HTTP_SERVER_ACCEPT_SOCKET_DATA*)E2;
	if(left->_sock == right->_sock)
		return 0;
	else if(left->_sock > right->_sock)
		return 1;
	else
		return -1;
}
_int32 http_server_safe_erase_accept_socket_data(HTTP_SERVER_ACCEPT_SOCKET_DATA* data)
{
	LOG_DEBUG("http_server_safe_erase_accept_socket_data:0x%X,gp_http_data=0x%X,g_current_task_id=%X,data->_task_id=%X,is_canceled=%d",data,gp_http_data,g_current_task_id,data->_task_id,data->_is_canceled);
	if((g_current_task_id==data->_task_id)&&(data->_is_canceled!=TRUE))
	{
		g_current_task_id = 0;
	}
	if(gp_http_data==data)
	{
		gp_http_data = NULL;
	}
	data->_post_type = -1;
	return sd_free(data);
}
_int32 http_server_erase_accept_socket_data(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, BOOL close_socket)
{
	_int32 ret = SUCCESS,socket_err = SUCCESS;
	_u32 op_count = 0;
	LOG_DEBUG("http_server_erase_accept_socket_data:sock=%u,fetch_file_pos=%llu,fetch_file_length=%llu,buffer_len=%u,task_id=%u,state=%d,close_socket=%d" , data->_sock,data->_fetch_file_pos, data->_fetch_file_length, data->_buffer_len ,data->_task_id, data->_state,close_socket);
	ret = set_erase_node(&g_HTTP_SERVER_ACCEPT_SOCKET_DATA_set, data);
	CHECK_VALUE(ret);
	if(close_socket == TRUE)
	{
		socket_err = get_socket_error(data->_sock);
		ret = socket_proxy_peek_op_count(data->_sock, DEVICE_SOCKET_TCP,&op_count);
		sd_assert(ret==SUCCESS);
					
		LOG_DEBUG("http_server_erase_accept_socket_data:sock=%u,socket_err=%d,op_ret=%d,op_count=%u" , data->_sock,socket_err,ret,op_count);
		if(op_count!=0)
		{
			ret = socket_proxy_cancel(data->_sock,DEVICE_SOCKET_TCP);
			sd_assert(ret==SUCCESS);
		}
		
		ret = socket_proxy_close(data->_sock);
		CHECK_VALUE(ret);
	}
	ret = sd_free(data->_buffer);
	data->_buffer = NULL;
	CHECK_VALUE(ret);
	if((FLASH_PLAYER_HTML_FILE_TASK_ID == data->_task_id||FLASH_PLAYER_SWF_FILE_TASK_ID == data->_task_id
		||FLASH_PLAYER_ICO_FILE_TASK_ID == data->_task_id||FLASH_PLAYER_LOCAL_FILE_TASK_ID == data->_task_id
		||HTTP_LOCAL_FILE_TASK_ID == data->_task_id) 
		&& 0!= data->_file_index)
	{
		sd_close_ex((_u32)data->_file_index);
	}

#ifdef ENABLE_ASSISTANT	
	if(HTTP_LOCAL_FILE_TASK_ID == data->_task_id&&data->_post_type == 4)
	{
		/* 将发送响应xml文件的操作结果回调通知界面 */
		MA_RESP_RET ma_resp = {0};
		ma_resp._ma_id = (_u32)data;
		ma_resp._user_data = data->_user_data;
		ma_resp._result = data->_errcode;
		
		if(gp_http_data==data)
		{
			gp_http_data = NULL;
		}
		
		g_assistant_send_resp_callback(&ma_resp);
	}
#endif /*  #ifdef ENABLE_ASSISTANT	*/

	return http_server_safe_erase_accept_socket_data(data);
}

_int32 init_vod_http_server_module(_u16 * port)
{
    _int32 ret = http_vod_server_start(port);

	sd_memset(g_local_file_path,0x00,MAX_LONG_FULL_PATH_BUFFER_LEN);
	sd_memset(&g_flv_context,0x00,sizeof(FLVFormatContext ));
	g_current_task_id = 0;
	g_flv_header_to_first_tag_len = 0;
	gp_flv_header_to_first_tag = NULL;
	g_http_server_timer_id = 0;
	gp_http_data = NULL;

	return ret;
}

_int32 uninit_vod_http_server_module()
{

	SAFE_DELETE(gp_flv_header_to_first_tag);
	SAFE_DELETE(g_flv_context.tav_flv_keyframe);
	SAFE_DELETE(g_flv_context.tav_flv_filepos);
	sd_memset(&g_flv_context,0x00,sizeof(FLVFormatContext ));
	if(g_http_server_timer_id!=0)
	{
		cancel_timer(g_http_server_timer_id);
		g_http_server_timer_id = 0;
	}

   return http_vod_server_stop();
}

_int32 force_close_http_server_module_res()
{
      if(g_http_server_tcp_accept_sock != INVALID_SOCKET)
      {
		g_http_server_tcp_accept_sock = INVALID_SOCKET;
      }
      return SUCCESS;
}

_int32 http_server_parse_get_request(char* buffer, _u32 length, char* file_name, _u64* file_pos,BOOL * is_flv_seek,BOOL * is_local_file)
{
	char *pos = NULL,*http_header = NULL;
	char* pos_space;
	char * ptr_header;
	char * ptr_header_end;
	char * ptr_param_end;
	char * ptr_param_value;
	_int32          ret;
	char* pos_startByte = NULL;
	char file_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};

	*file_pos = 0;
	*is_flv_seek = FALSE;
	ptr_header = buffer;
	ptr_header_end = sd_strstr( (char*)ptr_header,(const char*) "\r\n\r\n" ,0);
	LOG_DEBUG("http_server_parse_get_request..");
	if(NULL == ptr_header_end || (ptr_header_end - ptr_header) > length )
	{
	       LOG_DEBUG("http_server_parse_get_request..NULL == ptr_header_end || (ptr_header_end - ptr_header) > length ");
		return -1;
	}
	/*add extra 2 bytes for \r\n*/
	ptr_header_end+=2; 

	pos = ptr_header;
	if(sd_strncmp((const char*)pos, (const char*)"GET /", sd_strlen("GET /")) != 0 )
	{
	       LOG_DEBUG("http_server_parse_get_request..not start with GET / ");
		return -1;
	}
	pos = pos + sd_strlen("GET ");
	pos_space = sd_strstr(pos, " ", 0);
	if(NULL == pos_space || pos_space > ptr_header_end)
	{
	       LOG_DEBUG("http_server_parse_get_request..not found space after GET/ ");
		return -1;
	}
	
	http_header = sd_strstr(pos, "http://", 0);
	if(http_header!=NULL&&(http_header<pos_space))
	{
		pos = http_header+sd_strlen( "http://");
	}
	pos = sd_strchr(pos, '/', 0);
	if(pos>pos_space)
	{
	       LOG_DEBUG("http_server_parse_get_request..not found path / after GET/ ");
		return -1;
	}
	sd_strncpy(file_path, pos, pos_space-pos);
	file_path[pos_space-pos] = '\0'; /*Got file name*/
	pos = pos_space+1;

	if(NULL==sd_strchr(file_path, '?',0)&&http_server_is_file_exist(file_path))
	{
		*is_local_file = TRUE;
		sd_strncpy(file_name, file_path, sd_strlen(file_path));
	}
	else
	{
		sd_strncpy(file_name, file_path+1, sd_strlen(file_path+1));
	}
	
	 LOG_DEBUG("http_server_parse_get_request.Got file name:%s",file_name);

	/* 增加在URL中带位置偏移参数的数据请求方式20110930 by zyq */
	pos_startByte = sd_strstr(file_name, "startByte=", 0);
	
	 LOG_DEBUG("http_server_parse_get_request.is_local_file:%d,pos_startByte:%s",*is_local_file,pos_startByte);

	if((*is_local_file==FALSE)&&(NULL != pos_startByte))
	{
		/* http://192.168.90.103/1.flv?startByte={字节点} */
	       LOG_DEBUG("http_server_parse_get_request..find startByte in URL:file_name=%s ",file_name);
		pos_startByte--;
		*pos_startByte = '\0'; /* 在file_name中去掉 "?startByte=..." */
		pos_startByte+=sd_strlen("?startByte=");
		ret = sd_str_to_u64( pos_startByte, sd_strlen(pos_startByte) , file_pos );
		CHECK_VALUE(ret);

		if(*file_pos!=0)
		{
			*is_flv_seek = TRUE;
		}
		/* 不需要再解析后续的字段 */
		return SUCCESS;
	}
	
	 LOG_DEBUG("http_server_parse_get_request.to find rn:%s",pos);

	ptr_param_end = sd_strstr(pos, "\r\n", 0);
	if(NULL == ptr_param_end || ptr_param_end > ptr_header_end)
	{
	       LOG_DEBUG("http_server_parse_get_request..not found \r\n after GET/ ");
		return -1;
	}
	pos = ptr_param_end + sd_strlen("\r\n");
	/*skip first line now*/

	/*头部移动到参数列表*/
	ptr_param_end = sd_strstr( pos, "\r\n" ,0);
	while(ptr_param_end)
	{
		ptr_param_value = sd_strchr(pos, ':', 0 );
		if( !ptr_param_end || (ptr_param_end - pos > 255 )
			|| ptr_param_value > ptr_param_end ) {
			/*参数不合法 ，退出解析*/
	              LOG_DEBUG("http_server_parse_get_request..parameter parse fail");
			break;
		}

		/*
		Range:bytes=39397251-
		strncpy( name, pos, ptr_param_end-pos );
		name[ ptr_param_end - pos] = 0;
		name[ptr_param_value - pos] = 0;
		*/
		//name是参数名  ptr_param_value 是参数值现在
		//LOG_DEBUG("http_server_parse_get_request Param: %s = %s", name, (name+(paramvalue+1-pHeader)));
              //LOG_DEBUG("http_server_parse_get_request..parameter list %s", pos);
		if( sd_strncmp(pos, "Range:", sd_strlen("Range:")) == 0 )
		{
			/*should trim first*/
			/*Got Range:bytes here*/
			ptr_param_value = sd_strstr(ptr_param_value,"=", 0 );
			if(NULL != ptr_param_value && ptr_param_value<ptr_param_end)
			{
  			     ptr_param_value = ptr_param_value+sd_strlen("=");
			     while(*ptr_param_value == ' ')
			     {
			          ptr_param_value++;
			     }
			     ret = sd_str_to_u64( ptr_param_value, ptr_param_end - ptr_param_value-1 , file_pos );
		             LOG_DEBUG("http_server_parse_get_request..parameter parse file_pos=%llu, ret=%d", *file_pos, ret);
			}
		}

		pos = ptr_param_end+sd_strlen("\r\n");
		ptr_param_end = sd_strstr( pos, "\r\n",0);
		if( ptr_param_end >= ptr_header_end)
		{
			//已经超出字符串长度
			break;
		}
	}
	return SUCCESS;
}
_int32 http_server_parse_post_request(char* buffer, _u32 length, char* file_name, _u64* content_len,BOOL * is_local_file)
{
	char *pos = NULL,*http_header = NULL;
	char* pos_space;
	char * ptr_header;
	_int32          ret;
	char* pos_content_len = NULL;
	//char file_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};

	*content_len = 0;
	*is_local_file = FALSE;
	ptr_header = buffer;
	pos = ptr_header;
	if(sd_strncmp((const char*)pos, (const char*)"POST /", sd_strlen("POST /")) != 0 )
	{
	       LOG_DEBUG("http_server_parse_post_request..not start with POST / ");
		return -1;
	}
	pos = pos + sd_strlen("POST ");
	pos_space = sd_strstr(pos, " ", 0);
	if(NULL == pos_space || pos_space-buffer > length)
	{
	       LOG_DEBUG("http_server_parse_post_request..not found space after POST / ");
		return -1;
	}
	
	http_header = sd_strstr(pos, "http://", 0);
	if(http_header!=NULL&&(http_header<pos_space))
	{
		pos = http_header+sd_strlen( "http://");
	}
	pos = sd_strchr(pos, '/', 0);
	if(pos>pos_space)
	{
	       LOG_DEBUG("http_server_parse_post_request..not found path / after POST / ");
		return -1;
	}

	sd_strncpy(file_name, pos, pos_space-pos);
	file_name[pos_space-pos] = '\0'; /*Got file name*/
	pos = pos_space+1;

	if(NULL==sd_strchr(file_name, '?',0))
	{
		*is_local_file = TRUE;
	}

	pos_content_len = sd_strstr(pos,"Content-Length: ",0);
	if(NULL == pos_content_len || pos_content_len-ptr_header > length)
	{
	       LOG_DEBUG("http_server_parse_post_request..not found Content-Length after POST / ");
		return -1;
	}
	pos_content_len+=sd_strlen("Content-Length: ");
	pos = sd_strstr(pos_content_len,"\r\n",0);
	if(NULL == pos || pos-pos_content_len > 20)
	{
	       LOG_DEBUG("http_server_parse_post_request..Bad Content-Length after POST / ");
		return -1;
	}
	ret=sd_str_to_u64(pos_content_len,pos-pos_content_len,content_len);
	if(ret != SUCCESS || *content_len == 0)
	{
	       LOG_DEBUG("http_server_parse_post_request..Bad Content-Length after POST / 2");
		return -1;
	}
	return SUCCESS;
}

_int32 http_server_response_header(_int32 err_code, char* header_buffer, _u64 start_pos, _u64 content_length)
{
	char _Tbuffer[MAX_URL_LEN];
	char http_header_206[] = "HTTP/1.1 206 Partial Content\r\nServer: thunder/5.0.0.72\r\nContent-Type: application/octet-stream\r\nAccept-Ranges: bytes\r\n";
	char http_header_200[] = "HTTP/1.1 200 OK\r\nServer: thunder/5.0.0.72\r\nContent-Type: application/octet-stream\r\nAccept-Ranges: bytes\r\n";
	char http_header_404[] = "HTTP/1.1 404 File Not Found\r\nServer: thunder/5.0.0.72\r\nContent-Type: application/octet-stream\r\nAccept-Ranges: bytes\r\n";
	char http_header_503[] = "HTTP/1.1 503 Service Unavailable\r\nServer: thunder/5.0.0.72\r\nContent-Type: application/octet-stream\r\nAccept-Ranges: bytes\r\n";
	char range[256] = "Content-Length: ";
	char http_header_tail[] = "Connection: close\r\n\r\n";

	if(err_code == SUCCESS)
	{
		sd_memset(_Tbuffer, 0, sizeof(_Tbuffer));
	       sd_snprintf(_Tbuffer,128,"%llu\r\n", content_length);
		sd_strcat(range, _Tbuffer,sd_strlen(_Tbuffer)); 

		if(start_pos!=0)
		{
		       sd_memset(_Tbuffer, 0, sizeof(_Tbuffer));
			sd_snprintf(_Tbuffer,128,"Content-Range: bytes %llu-%llu/%llu\r\n",start_pos, start_pos+content_length-1, start_pos+content_length);
			sd_strcat(range, _Tbuffer,sd_strlen(_Tbuffer)); 
			
			sd_strncpy(header_buffer, http_header_206, sd_strlen(http_header_206));
			header_buffer[sd_strlen(http_header_206)] = '\0';
		}
		else
		{
			sd_strncpy(header_buffer, http_header_200, sd_strlen(http_header_200));
			header_buffer[sd_strlen(http_header_200)] = '\0';
		}
	      sd_strcat(header_buffer, range, sd_strlen(range));
	}
	else
	{
		if(err_code == 503)
		{
			sd_strncpy(header_buffer, http_header_503, sd_strlen(http_header_503));
			header_buffer[sd_strlen(http_header_503)] = '\0';
		}
		else
		{
			sd_strncpy(header_buffer, http_header_404, sd_strlen(http_header_404));
			header_buffer[sd_strlen(http_header_404)] = '\0';
		}
	}
	
	sd_strcat(header_buffer, http_header_tail, sd_strlen(http_header_tail));
	return SUCCESS;
}


_int32 http_server_vdm_async_recv_handler(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	_int32 ret;
	_u32  now;
	RANGE range;
	HTTP_SERVER_ACCEPT_SOCKET_DATA* data = (HTTP_SERVER_ACCEPT_SOCKET_DATA*)user_data;
	if(NULL ==  data)
	{
	    LOG_DEBUG("http_server_vdm_async_recv_handler..., errcode=%d, had_recv =%d", errcode, had_recv );
	    return SUCCESS;
	}
	LOG_DEBUG("http_server_vdm_async_recv_handler..., errcode=%d, had_recv =%d, sock=%d", errcode, had_recv, data->_sock );
	if(errcode != SUCCESS  || had_recv == 0||data->_is_canceled)
	{
		data->_errcode = errcode;
		return http_server_erase_accept_socket_data(data, TRUE);
	}

	ret = sd_time_ms(&now);
	range = pos_length_to_range( data->_fetch_file_pos, MIN(data->_fetch_file_length, data->_buffer_len) , data->_fetch_file_pos+data->_fetch_file_length);
	if( (now - data->_fetch_time_ms) > 5*1000 )
	{
	          LOG_DEBUG("http_server_vdm_async_recv_handler, async read time too long pos=%llu, length=%llu, range(%d,%d),  time is too long =%d..." ,   data->_fetch_file_pos, MIN(data->_fetch_file_length, data->_buffer_len) , range._index, range._num, now - data->_fetch_time_ms);
	}

	ret =  socket_proxy_send(data->_sock, buffer, had_recv, http_server_handle_send,(void*)data);
	
    return ret;
}
_int32 http_server_get_real_file_pos(HTTP_SERVER_ACCEPT_SOCKET_DATA*data,_u64 virtual_file_pos,_u64  * real_file_pos)
{
	_int32 ret_val = SUCCESS;
	#ifdef ENABLE_FLASH_PLAYER
	LOG_DEBUG("http_server_get_real_file_pos, data->_task_id=%X,g_current_task_id=%X,virtual_file_pos=%llu,g_flv_header_to_first_tag_len=%llu,data->_real_file_start_pos=%llu" ,   data->_task_id,g_current_task_id,virtual_file_pos,g_flv_header_to_first_tag_len,data->_real_file_start_pos);
	if(data->_task_id == FLASH_PLAYER_LOCAL_FILE_TASK_ID)
	{
		//*dl_position = data->_virtual_file_size;
	}
	else
	{
		if(data->_task_id!=g_current_task_id)
		{
			*real_file_pos = 0;
			CHECK_VALUE(-1);
		}

	}
	
	if(virtual_file_pos<g_flv_header_to_first_tag_len)
	{
		*real_file_pos = 0;
		CHECK_VALUE(-1);
	}
	
	*real_file_pos = virtual_file_pos-g_flv_header_to_first_tag_len+data->_real_file_start_pos;
	#endif /* ENABLE_FLASH_PLAYER */
	return ret_val;
}

_int32 http_server_correct_dl_position(HTTP_SERVER_ACCEPT_SOCKET_DATA* data,_u64 * dl_position)
{
	_int32 ret_val = SUCCESS;
	#ifdef ENABLE_FLASH_PLAYER
	LOG_DEBUG("http_server_correct_dl_position, data->_task_id=%X,g_current_task_id=%X,g_flv_header_to_first_tag_len=%llu,data->_real_file_start_pos=%llu" ,   data->_task_id,g_current_task_id,g_flv_header_to_first_tag_len,data->_real_file_start_pos);
	if(data->_task_id == FLASH_PLAYER_LOCAL_FILE_TASK_ID)
	{
		*dl_position = data->_virtual_file_size;
	}
	else
	{
		if(data->_task_id!=g_current_task_id)
		{
			*dl_position = 0;
			CHECK_VALUE(-1);
		}

		if(*dl_position<g_flv_header_to_first_tag_len)
		{
			*dl_position = 0;
			CHECK_VALUE(-1);
		}

		if(*dl_position<=data->_real_file_start_pos)
		{
			*dl_position = g_flv_header_to_first_tag_len;
		}
		else
		{
			*dl_position = *dl_position-data->_real_file_start_pos+g_flv_header_to_first_tag_len;
		}
	}
	#endif /* ENABLE_FLASH_PLAYER */
	return ret_val;
}
_int32 http_server_get_local_flv_header_to_end_of_first_tag_len(char* file_head, _int32 file_head_length,_u64 * flv_header_to_end_of_first_tag_len )
{
	_int32 ret_val = SUCCESS;
	#ifdef ENABLE_FLASH_PLAYER
	FLVFileInfo flvfile;
	LOG_DEBUG("http_server_get_local_flv_header_to_end_of_first_tag_len, file_head_length=%d" ,   file_head_length);
	ret_val = flv_read_headers((_u8*)file_head, file_head_length,  &flvfile);
	CHECK_VALUE(ret_val);
	
	*flv_header_to_end_of_first_tag_len = (flvfile.end_of_first_audio_tag>flvfile.end_of_first_video_tag)?flvfile.end_of_first_audio_tag:flvfile.end_of_first_video_tag;
	*flv_header_to_end_of_first_tag_len+=4; /* 加上后续的四位tag长度 */
	
	#endif /* ENABLE_FLASH_PLAYER */
	return ret_val;
}
_int32 http_server_get_virtual_flv_file_size(HTTP_SERVER_ACCEPT_SOCKET_DATA* data,_u64 * file_size,_u64 * file_pos)
{
	_int32 ret_val = SUCCESS;
	#ifdef ENABLE_FLASH_PLAYER
	_u32 readsize = 0;
	LOG_DEBUG("http_server_get_virtual_flv_file_size, data->_task_id=%X,g_current_task_id=%X,g_flv_header_to_first_tag_len=%llu,data->_real_file_start_pos=%llu" ,   data->_task_id,g_current_task_id,g_flv_header_to_first_tag_len,data->_real_file_start_pos);
	if(data->_task_id == FLASH_PLAYER_LOCAL_FILE_TASK_ID)
	{
		if(data->_task_id!=g_current_task_id)
		{
			SAFE_DELETE(gp_flv_header_to_first_tag);
			SAFE_DELETE(g_flv_context.tav_flv_keyframe);
			SAFE_DELETE(g_flv_context.tav_flv_filepos);
			sd_memset(&g_flv_context,0x00,sizeof(FLVFormatContext ));

			if(data->_file_index == 0)
			{
				ret_val = sd_open_ex(g_local_file_path, O_FS_RDONLY, (_u32 *)&data->_file_index);
				CHECK_VALUE(ret_val);
			}

			ret_val = sd_malloc(DEFAULT_LOCAL_FLV_HEADER_TO_FIRST_TAG_BUFFER_LEN, (void**)&gp_flv_header_to_first_tag);
			CHECK_VALUE(ret_val);
			if(g_is_xv_file)
			{
				sd_assert(g_xv_file_decoded_start_data_len!=0);
				sd_assert(g_xv_file_decoded_start_data_len<DEFAULT_LOCAL_FLV_HEADER_TO_FIRST_TAG_BUFFER_LEN);
				if(g_xv_file_decoded_start_data_len>=DEFAULT_LOCAL_FLV_HEADER_TO_FIRST_TAG_BUFFER_LEN)
				{
					CHECK_VALUE(-1);
				}
				
				sd_assert(gp_xv_file_decoded_start_data!=NULL);
				sd_memcpy(gp_flv_header_to_first_tag, gp_xv_file_decoded_start_data, g_xv_file_decoded_start_data_len);
				
				ret_val = sd_pread((_u32 )data->_file_index,gp_flv_header_to_first_tag+g_xv_file_decoded_start_data_len, 
					DEFAULT_LOCAL_FLV_HEADER_TO_FIRST_TAG_BUFFER_LEN-g_xv_file_decoded_start_data_len, 
					g_xv_file_context_header_len+g_xv_file_decoded_start_data_len, &readsize);
				
				readsize+=g_xv_file_decoded_start_data_len;
			}
			else
			{
				ret_val = sd_pread((_u32 )data->_file_index,gp_flv_header_to_first_tag, DEFAULT_LOCAL_FLV_HEADER_TO_FIRST_TAG_BUFFER_LEN, 0, &readsize);
			}
			if(ret_val!=SUCCESS)
			{
				SAFE_DELETE(gp_flv_header_to_first_tag);
				CHECK_VALUE(ret_val);
			}

			if(readsize<24)
			{
				SAFE_DELETE(gp_flv_header_to_first_tag);
				CHECK_VALUE(-1);
			}

			if(gp_flv_header_to_first_tag[0]!='F'||gp_flv_header_to_first_tag[1]!='L'||gp_flv_header_to_first_tag[2]!='V')
			{
				SAFE_DELETE(gp_flv_header_to_first_tag);
				CHECK_VALUE(-1);
			}

			ret_val = http_server_get_local_flv_header_to_end_of_first_tag_len(gp_flv_header_to_first_tag,( _int32 )readsize,&g_flv_header_to_first_tag_len );
			if(ret_val!=SUCCESS)
			{
				SAFE_DELETE(gp_flv_header_to_first_tag);
				CHECK_VALUE(ret_val);
			}

			g_current_task_id = data->_task_id;

			ret_val = flv_analyze_metadata(&g_flv_context,(_u8*)gp_flv_header_to_first_tag,g_flv_header_to_first_tag_len);
			if(ret_val!=SUCCESS)
			{
				SAFE_DELETE(gp_flv_header_to_first_tag);
				CHECK_VALUE(ret_val);
			}
		}
		
		//data->_real_file_size = *file_size;
		
		ret_val = flv_get_next_tag_start_pos(&g_flv_context,*file_pos,file_pos);
		if(ret_val!=SUCCESS)
		{
			SAFE_DELETE(gp_flv_header_to_first_tag);
			CHECK_VALUE(ret_val);
		}

	}
	else
	{
		if(data->_task_id!=g_current_task_id)
		{
			SAFE_DELETE(gp_flv_header_to_first_tag);
			SAFE_DELETE(g_flv_context.tav_flv_keyframe);
			SAFE_DELETE(g_flv_context.tav_flv_filepos);
			sd_memset(&g_flv_context,0x00,sizeof(FLVFormatContext ));

			ret_val = vdm_get_flv_header_to_first_tag(data->_task_id,data->_file_index,&gp_flv_header_to_first_tag,&g_flv_header_to_first_tag_len);
			CHECK_VALUE(ret_val);

			g_current_task_id = data->_task_id;

			ret_val = flv_analyze_metadata(&g_flv_context,(_u8*)gp_flv_header_to_first_tag,g_flv_header_to_first_tag_len);
			CHECK_VALUE(ret_val);
		}
		
		//data->_real_file_size = *file_size;
		
		ret_val = flv_get_next_tag_start_pos(&g_flv_context,*file_pos,file_pos);
		CHECK_VALUE(ret_val);

	}
	
	data->_real_file_size = *file_size;

	*file_size = (*file_size)-(*file_pos)+g_flv_header_to_first_tag_len;

	data->_real_file_start_pos = *file_pos;
	data->_real_file_pos = *file_pos;
	data->_virtual_file_size = *file_size;
	data->_virtual_file_pos = 0;
	//data->_is_header_sent = FALSE;

	*file_pos = 0;
	#endif /* ENABLE_FLASH_PLAYER */
	return ret_val;
}
_int32 http_server_send_flv_header_and_first_tag(HTTP_SERVER_ACCEPT_SOCKET_DATA* data)
{
	_int32 ret_val = SUCCESS;
	#ifdef ENABLE_FLASH_PLAYER
	data->_state = HTTP_SERVER_STATE_SENDING_BODY;
	ret_val = http_server_vdm_async_recv_handler(ret_val,0, gp_flv_header_to_first_tag, g_flv_header_to_first_tag_len, (void*)data);
	#endif /* ENABLE_FLASH_PLAYER */
	return ret_val;
}

_int32 http_server_send_response_header(HTTP_SERVER_ACCEPT_SOCKET_DATA* data,_u32 task_id,_u32 file_index,_u64 file_size,_u64 file_pos)
{
	_int32 ret = SUCCESS;

	data->_task_id = task_id;
	data->_file_index = file_index;
	
	if(data->_is_flv_seek)
	{
		ret = http_server_get_virtual_flv_file_size(data,&file_size,&file_pos);
		CHECK_VALUE(ret);
	}
	
       LOG_DEBUG("http_server_send_respon_header. task_id = %u, file_pos=%llu, file_size=%llu,is_flv_seek=%d", task_id, file_pos, file_size,data->_is_flv_seek);
	/*fetch the filename's buffer here*/
	ret = http_server_response_header(SUCCESS, data->_buffer, file_pos, (_u64)file_size-file_pos);

	data->_state = HTTP_SERVER_STATE_SENDING_HEADER;
	data->_fetch_file_pos = file_pos;
	data->_fetch_file_length = (_u64)file_size-file_pos;

       LOG_DEBUG("http_server_response_header(sock=%u)[%d]:%s",data->_sock,sd_strlen(data->_buffer), data->_buffer);

       ret =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
	CHECK_VALUE(ret);
	return SUCCESS;
}
_u32 http_server_get_buffer_len(_u64 str_pos,_u64 dl_pos)
{
	if(str_pos+SEND_MOVIE_BUFFER_LENGTH_LOOG<dl_pos)
	{
		return SEND_MOVIE_BUFFER_LENGTH_LOOG;
	}
	else
	{
		return SEND_MOVIE_BUFFER_LENGTH;
	}
}
_int32 http_server_get_file_size(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, _u64 * file_size)
{
	_int32 ret_val = SUCCESS;
	char file_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};

	*file_size = 0;
	
	if(FLASH_PLAYER_HTML_FILE_TASK_ID == data->_task_id)
	{
		sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s%s",FLASH_PLAYER_FILE_PATH,FLASH_PLAYER_HTML_FILE_NAME);
	}
	else
	if(FLASH_PLAYER_SWF_FILE_TASK_ID == data->_task_id)
	{
		sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s%s",FLASH_PLAYER_FILE_PATH,FLASH_PLAYER_SWF_FILE_NAME);
	}
	else
	if(FLASH_PLAYER_ICO_FILE_TASK_ID == data->_task_id)
	{
		sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s%s",FLASH_PLAYER_FILE_PATH,FLASH_PLAYER_ICO_FILE_NAME);
	}
	else
	if(FLASH_PLAYER_LOCAL_FILE_TASK_ID == data->_task_id)
	{
		sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s",g_local_file_path);
		if(g_is_xv_file)
		{
			*file_size = g_xv_file_decoded_file_size;
			return SUCCESS;
		}
	}
	else
	if(HTTP_LOCAL_FILE_TASK_ID == data->_task_id)
	{
		sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s",g_local_file_path);
	}
	else
	{
		return -1;
	}

	ret_val =sd_get_file_size_and_modified_time(file_path,file_size,NULL);
	return ret_val;
}
_int32 http_server_decode_xv_file(char * file_path)
{
#ifdef ENABLE_XV_FILE_DEC
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0,readsize = 0,decode_data_offset = 0;
	XV_DEC_CONTEXT* xv_context = NULL;
	unsigned char * read_buffer = NULL;
	_u64 file_size = 0;
	
	LOG_DEBUG("http_server_decode_xv_file:%s", file_path);
	ret_val = sd_open_ex(file_path,O_FS_RDONLY, &file_id);
	CHECK_VALUE(ret_val);

	ret_val = sd_malloc(4*1024, (void**)&read_buffer);
	if(ret_val!=SUCCESS)
	{
		sd_close_ex( file_id);
		return ret_val;
	}

	ret_val = sd_filesize(file_id,&file_size);
	sd_assert(ret_val==SUCCESS);
	sd_assert(file_size>1024);
	
	ret_val = sd_read( file_id, (char*)read_buffer, 4*1024, &readsize);
	if(ret_val!=SUCCESS||readsize<1024)
	{
		LOG_ERROR("http_server_decode_xv_file:sd_read Error 1:ret_val=%d,readsize=%u", ret_val,readsize);
		sd_close_ex( file_id);
		SAFE_DELETE(read_buffer);
		return END_OF_FILE;
	}
	

	xv_context = create_xv_decoder();
	if(xv_context==NULL)
	{
		sd_close_ex( file_id);
		SAFE_DELETE(read_buffer);
		return -1;
	}
	
	ret_val = analyze_xv_head(xv_context,read_buffer,readsize,&decode_data_offset);
	LOG_DEBUG("http_server_decode_xv_file:analyze_xv_head:ret_val=%d,encoded_data_offset=%u,encoded_data_len=%u", ret_val,xv_context->encoded_data_offset,xv_context->encoded_data_len);
	if(ret_val!=SUCCESS)
	{
		sd_close_ex( file_id);
		SAFE_DELETE(read_buffer);
		close_xv_decoder(xv_context);
		return ret_val;
	}

	sd_assert(4*1024>xv_context->encoded_data_len);
	if(4*1024<xv_context->encoded_data_len)
	{
		sd_close_ex( file_id);
		SAFE_DELETE(read_buffer);
		close_xv_decoder(xv_context);
		return -1;
	}
	
	sd_memset(read_buffer,0x00,4*1024);
	readsize = 0;
	ret_val = sd_pread( file_id, (char*)read_buffer,xv_context->encoded_data_len,decode_data_offset, &readsize);
	sd_close_ex( file_id);
	if(ret_val!=SUCCESS||readsize!=xv_context->encoded_data_len)
	{
		LOG_ERROR("http_server_decode_xv_file:sd_pread Error 2:ret_val=%d,readsize=%u", ret_val,readsize);
		SAFE_DELETE(read_buffer);
		close_xv_decoder(xv_context);
		return -1;
	}
	
	ret_val = decode_xv_data(xv_context, decode_data_offset,read_buffer,readsize);
	LOG_DEBUG("http_server_decode_xv_file:decode_xv_data:ret_val=%d,data after decoded=%s", ret_val,read_buffer);
	close_xv_decoder(xv_context);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(read_buffer);
		return ret_val;
	}

	if(sd_strncmp((const char *)read_buffer, "FLV", 3)!=0)
	{
		SAFE_DELETE(read_buffer);
		LOG_ERROR("http_server_decode_xv_file:this is not a flv file!");
		return -1;
	}
	
	SAFE_DELETE(gp_xv_file_decoded_start_data);
	
	g_xv_file_decoded_start_data_len = readsize;
	gp_xv_file_decoded_start_data = (char*)read_buffer;
	
	g_xv_file_decoded_file_size = file_size-decode_data_offset;
	g_xv_file_context_header_len = decode_data_offset;
#else
	return NOT_IMPLEMENT;
#endif /* ENABLE_XV_FILE_DEC */	
	return SUCCESS;
}
_int32 http_server_handle_player_file(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, char * file_name,_u64 file_pos)
{
	#ifdef ENABLE_FLASH_PLAYER
	_int32 ret_val = SUCCESS;
	_u64 file_size = 0;
	char * path_pos = NULL;
	
       LOG_DEBUG("http_server_handle_player_file. file_name = %s, file_pos=%llu", file_name, file_pos);
	/* 增加对android下FLV播放的支持 */
	if(NULL!=sd_strstr(file_name,FLASH_PLAYER_HTML_FILE_NAME,0))
	{
		data->_task_id = FLASH_PLAYER_HTML_FILE_TASK_ID;
		ret_val = http_server_get_file_size(data,&file_size);
		if(ret_val!=SUCCESS) goto ErrHandler;
		file_pos = 0;
	       LOG_DEBUG("http_server_response_header. FLASH_PLAYER_HTML_FILE_NAME = %s, file_pos=%llu, file_size=%llu", FLASH_PLAYER_HTML_FILE_NAME, file_pos, file_size);
	}
	else
	if(NULL!=sd_strstr(file_name,FLASH_PLAYER_SWF_FILE_NAME,0))
	{
		data->_task_id = FLASH_PLAYER_SWF_FILE_TASK_ID;
		ret_val = http_server_get_file_size(data,&file_size);
		if(ret_val!=SUCCESS) goto ErrHandler;
		file_pos = 0;
	       LOG_DEBUG("http_server_response_header. FLASH_PLAYER_SWF_FILE_NAME = %s, file_pos=%llu, file_size=%llu", FLASH_PLAYER_SWF_FILE_NAME, file_pos, file_size);
	}
	else
	if(NULL!=sd_strstr(file_name,FLASH_PLAYER_ICO_FILE_NAME,0))
	{
		data->_task_id = FLASH_PLAYER_ICO_FILE_TASK_ID;
		ret_val = http_server_get_file_size(data,&file_size);
		if(ret_val!=SUCCESS) goto ErrHandler;
		file_pos = 0;
	       LOG_DEBUG("http_server_response_header. FLASH_PLAYER_ICO_FILE_NAME = %s, file_pos=%llu, file_size=%llu", FLASH_PLAYER_ICO_FILE_NAME, file_pos, file_size);
	}
	else
	if(NULL!=sd_strstr(file_name,FLASH_PLAYER_LOCAL_FILE_MARK,0))
	{
		char local_full_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};
		path_pos = sd_strstr(file_name,FLASH_PLAYER_LOCAL_FILE_MARK,0);
		path_pos += sd_strlen(FLASH_PLAYER_LOCAL_FILE_MARK);
		/* 由于Adobe的移动版本FlashPlayer不支持文件路径中带'%'的编码方式,所以这里用了把路径名直接转成16进制字符串的山寨编码方式 */
		ret_val = sd_string_to_hex(path_pos,(_u8 * )local_full_path);
		if(ret_val!=SUCCESS)
		{
			/* 不是山寨编码方式,改用常规编码方式 */
			url_object_decode_ex(path_pos, local_full_path ,MAX_LONG_FULL_PATH_BUFFER_LEN);
		}
		if(sd_strcmp(local_full_path, g_local_file_path)!=0)
		{
			g_current_task_id = 0;
		}
		sd_strncpy(g_local_file_path,local_full_path, MAX_LONG_FULL_PATH_BUFFER_LEN);
		
		data->_task_id = FLASH_PLAYER_LOCAL_FILE_TASK_ID;

		/* 是否xv 文件 */
		path_pos = g_local_file_path + sd_strlen(g_local_file_path)-3;
		if(sd_stricmp(path_pos, ".xv")==0)
		{
			ret_val = http_server_decode_xv_file(g_local_file_path);
			if(ret_val==SUCCESS)
			{
				g_is_xv_file = TRUE;	
				file_size = g_xv_file_decoded_file_size;
			}
		}
		else
		{
			g_is_xv_file = FALSE;
			ret_val = http_server_get_file_size(data,&file_size);
		}
		
	       LOG_DEBUG("http_server_response_header. FLASH_PLAYER_LOCAL_FILE_MARK ret_val= %d,g_local_file_path=%s, file_pos=%llu, file_size=%llu,g_is_xv_file=%d", ret_val,g_local_file_path, file_pos, file_size,g_is_xv_file);
		if(ret_val!=SUCCESS) goto ErrHandler;
	}
	else
	{
		CHECK_VALUE(-1);
	}
	
	ret_val =  http_server_send_response_header( data,data->_task_id,0, file_size, file_pos);
	if(ret_val!=SUCCESS) goto ErrHandler;
	
	return SUCCESS;
	
ErrHandler:
	LOG_ERROR("http_server_handle_player_file .ret_val = %d", ret_val); 
	/*fetch the filename's buffer here*/
	if(ret_val==503)
	{
		ret_val = http_server_response_header(503, data->_buffer, file_pos, (_u64)0);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	}
	else
	{
		ret_val = http_server_response_header(404, data->_buffer, file_pos, (_u64)0);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	}
       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
	CHECK_VALUE(ret_val);
	#endif /* ENABLE_FLASH_PLAYER */
	return SUCCESS;
}
BOOL http_server_is_file_exist(char * file_name)
{
	char local_full_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};

	url_object_decode_ex(file_name, local_full_path ,MAX_LONG_FULL_PATH_BUFFER_LEN);
	if(sd_file_exist(local_full_path))
	{
		return TRUE;
	}
	return FALSE;
}
_int32 http_server_handle_get_local_file(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, char * file_name,_u64 file_pos)
{
	_int32 ret_val = SUCCESS;
	_u64 file_size = 0;
	//char * path_pos = NULL;
	char local_full_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};

	url_object_decode_ex(file_name, local_full_path ,MAX_LONG_FULL_PATH_BUFFER_LEN);
	if(sd_strcmp(local_full_path, g_local_file_path)!=0)
	{
		g_current_task_id = 0;
	}
	sd_strncpy(g_local_file_path,local_full_path, MAX_LONG_FULL_PATH_BUFFER_LEN);

	/* 暂不支持分段下载 */
	data->_is_flv_seek = FALSE;
	file_pos = 0;
	
	data->_task_id = HTTP_LOCAL_FILE_TASK_ID;
	ret_val = http_server_get_file_size(data,&file_size);
	if(ret_val!=SUCCESS) goto ErrHandler;
       LOG_DEBUG("http_server_handle_local_file. %s, file_pos=%llu, file_size=%llu", g_local_file_path, file_pos, file_size);

	
	ret_val =  http_server_send_response_header( data,data->_task_id,0, file_size, file_pos);
	if(ret_val!=SUCCESS) goto ErrHandler;
	
	return SUCCESS;
	
ErrHandler:
	LOG_ERROR("http_server_handle_player_file .ret_val = %d", ret_val); 
	/*fetch the filename's buffer here*/
	if(ret_val==503)
	{
		ret_val = http_server_response_header(503, data->_buffer, file_pos, (_u64)0);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	}
	else
	{
		ret_val = http_server_response_header(404, data->_buffer, file_pos, (_u64)0);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	}
       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 http_server_handle_post_local_file(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, char * file_name,_u64 content_len,char *ptr_data,_u32 data_len)
{
	_int32 ret_val = SUCCESS;
	//_u64 file_size = 0;
	char * path_pos = NULL;
	char local_full_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};
	_u32 writesize = 0;

	url_object_decode_ex(file_name, local_full_path ,MAX_LONG_FULL_PATH_BUFFER_LEN);
	if(sd_strcmp(local_full_path, g_local_file_path)!=0)
	{
		g_current_task_id = 0;
	}
	sd_strncpy(g_local_file_path,local_full_path, MAX_LONG_FULL_PATH_BUFFER_LEN);

	path_pos = sd_strrchr(local_full_path, DIR_SPLIT_CHAR);
	sd_assert(path_pos!=NULL);
	if(path_pos==NULL) goto ErrHandler;

	*path_pos = '\0';

	if(!sd_dir_exist(local_full_path))
	{
		/* 文件存放路径不存在 */
		goto ErrHandler;
	}

	sd_delete_file(g_local_file_path);

	ret_val = sd_open_ex(g_local_file_path, O_FS_CREATE, &data->_file_index);
	if(ret_val!=SUCCESS) goto ErrHandler;

	data->_post_type= 1;
	data->_task_id = HTTP_LOCAL_FILE_TASK_ID;
	g_current_task_id = data->_task_id;

	if(data_len!=0)
	{
		ret_val = sd_write(data->_file_index, ptr_data,data_len, &writesize);
		if(ret_val!=SUCCESS||writesize!=data_len) goto ErrHandler;
	}
	
	if(writesize<content_len)
	{
		/* 接收后续数据*/
		data->_fetch_file_length = content_len;
		data->_fetch_file_pos = writesize;
		
		//data->_task_id = FLASH_PLAYER_LOCAL_FILE_TASK_ID;
		data->_buffer_len = MIN(data->_buffer_len,data->_fetch_file_length-data->_fetch_file_pos);

		ret_val =  socket_proxy_recv(data->_sock, data->_buffer , data->_buffer_len, http_server_handle_http_complete_recv_callback, data);
		if(ret_val!=SUCCESS) goto ErrHandler;
	}
	else
	{
		/* 数据接收完毕 */
		sd_close_ex(data->_file_index);
		data->_file_index = 0;
		
		ret_val =  http_server_response_header(SUCCESS,  data->_buffer, 0,0);
		CHECK_VALUE(ret_val);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
		CHECK_VALUE(ret_val);
	}
	return SUCCESS;
	
ErrHandler:
	LOG_ERROR("http_server_handle_post_local_file .ret_val = %d", ret_val); 
	/*fetch the filename's buffer here*/
	if(ret_val==503)
	{
		ret_val = http_server_response_header(503, data->_buffer, 0,0);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	}
	else
	{
		ret_val = http_server_response_header(404, data->_buffer, 0, 0);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	}
       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

_int32  http_server_get_xml_file_store_path( char * xml_file_path_buffer)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_ASSISTANT
	static _u32 count = 0;
	char system_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};
	_u32 cur_time_stamp = 0;

	settings_get_str_item("system.system_path",system_path);
	sd_assert(sd_strlen(system_path)!=0);
	sd_time_ms(&cur_time_stamp);
	sd_snprintf(xml_file_path_buffer, MAX_FULL_PATH_BUFFER_LEN-1, "%s/%u_%u.xml",system_path,count++,cur_time_stamp);
#endif /* #ifdef ENABLE_ASSISTANT	 */
	return ret_val;
}

_int32 http_server_handle_post_command(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, char * file_name,_u64 content_len,char *ptr_data,_u32 data_len)
{
#ifdef ENABLE_ASSISTANT	

	_int32 ret_val = SUCCESS;
	//_u64 file_size = 0;
	char * path_pos = NULL;
	char local_full_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};
	_u32 writesize = 0;

	http_server_get_xml_file_store_path(local_full_path);
	
	LOG_DEBUG("http_server_handle_post_command:g_local_file_path=%s,g_current_task_id=%u,local_full_path=%s",g_local_file_path,g_current_task_id,local_full_path);

	//sd_assert(g_current_task_id==0);
	if(sd_strcmp(local_full_path, g_local_file_path)!=0)
	{
		g_current_task_id = 0;
	}
	sd_strncpy(g_local_file_path,local_full_path, MAX_LONG_FULL_PATH_BUFFER_LEN);

	path_pos = sd_strrchr(local_full_path, DIR_SPLIT_CHAR);
	sd_assert(path_pos!=NULL);
	if(path_pos==NULL) goto ErrHandler;

	*path_pos = '\0';

	if(!sd_dir_exist(local_full_path))
	{
		/* 文件存放路径不存在 */
		goto ErrHandler;
	}

	sd_delete_file(g_local_file_path);

	ret_val = sd_open_ex(g_local_file_path, O_FS_CREATE, &data->_file_index);
	if(ret_val!=SUCCESS) goto ErrHandler;

	data->_post_type = 2;
	data->_task_id = HTTP_LOCAL_FILE_TASK_ID;
	g_current_task_id = data->_task_id;

	if(data_len!=0)
	{
		ret_val = sd_write(data->_file_index, ptr_data,data_len, &writesize);
		if(ret_val!=SUCCESS||writesize!=data_len) goto ErrHandler;
	}
	
	if(writesize<content_len)
	{
		/* 接收后续数据*/
		data->_fetch_file_length = content_len;
		data->_fetch_file_pos = writesize;
		
		//data->_task_id = FLASH_PLAYER_LOCAL_FILE_TASK_ID;
		data->_buffer_len = MIN(data->_buffer_len,data->_fetch_file_length-data->_fetch_file_pos);

		ret_val =  socket_proxy_recv(data->_sock, data->_buffer , data->_buffer_len, http_server_handle_http_complete_recv_callback, data);
		if(ret_val!=SUCCESS) goto ErrHandler;
	}
	else
	{
		MA_REQ ma_req = {0};
		/* 数据接收完毕 */
		sd_close_ex(data->_file_index);
		data->_file_index = 0;

		ma_req._ma_id = (_u32)data;
		sd_strncpy(ma_req._req_file, g_local_file_path, 1023);

		/* 回调通知UI */
		LOG_DEBUG("http_server_handle_post_command:g_assistant_incom_req_callback:ma_id=0x%X,ma_req._req_file=%s",ma_req._ma_id,ma_req._req_file);
		ret_val = g_assistant_incom_req_callback(&ma_req);
		if(ret_val!=SUCCESS) goto ErrHandler;

		sd_assert(g_http_server_timer_id==0);
		ret_val = start_timer(http_server_timeout, NOTICE_ONCE, DEFAULT_HTTP_SERVER_TIMEOUT, 0, data, &g_http_server_timer_id);
		sd_assert(ret_val==SUCCESS);
		sd_assert(g_http_server_timer_id!=0);

		sd_assert(gp_http_data==NULL);
		gp_http_data = data;
		data->_post_type = 3;
	}
	return SUCCESS;
	
ErrHandler:
	LOG_ERROR("http_server_handle_post_local_file .ret_val = %d", ret_val); 
	ret_val = http_server_response_header(503, data->_buffer, 0,0);
	sd_assert(ret_val==SUCCESS);
	g_current_task_id = 0;
	gp_http_data = NULL;
	sd_delete_file(g_local_file_path);
	g_local_file_path[0]='\0';
	data->_state = HTTP_SERVER_STATE_SENDING_ERR;
       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
	CHECK_VALUE(ret_val);
#endif /* #ifdef ENABLE_ASSISTANT	 */
	return SUCCESS;
}

_int32 http_server_sync_read_file(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, _u64 start_pos, _u64 len, char *buf)
{
	_int32 ret_val = SUCCESS;
	_u32 readsize = 0;
	char file_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};

	LOG_DEBUG("http_server_sync_read_file:task_id=%u,file_index=%u,start_pos=%llu,len=%llu,g_is_xv_file=%d",data->_task_id,data->_file_index,start_pos,len,g_is_xv_file);
	if(data->_file_index==0)
	{
		if(FLASH_PLAYER_HTML_FILE_TASK_ID == data->_task_id)
		{
			sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s%s",FLASH_PLAYER_FILE_PATH,FLASH_PLAYER_HTML_FILE_NAME);
		}
		else
		if(FLASH_PLAYER_SWF_FILE_TASK_ID == data->_task_id)
		{
			sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s%s",FLASH_PLAYER_FILE_PATH,FLASH_PLAYER_SWF_FILE_NAME);
		}
		else
		if(FLASH_PLAYER_ICO_FILE_TASK_ID == data->_task_id)
		{
			sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s%s",FLASH_PLAYER_FILE_PATH,FLASH_PLAYER_ICO_FILE_NAME);
		}
		else
		if(FLASH_PLAYER_LOCAL_FILE_TASK_ID == data->_task_id)
		{
			sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s",g_local_file_path);
		}
		else
		if(HTTP_LOCAL_FILE_TASK_ID == data->_task_id)
		{
			sd_snprintf(file_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s",g_local_file_path);
		}
		else
		{
			return -1;
		}
		ret_val = sd_open_ex(file_path, O_FS_RDONLY, (_u32 *)&data->_file_index);
		if(ret_val!=SUCCESS) goto ErrHandler;
	}

	if((FLASH_PLAYER_LOCAL_FILE_TASK_ID == data->_task_id)&&g_is_xv_file)
	{
		/* xv文件 */
		if(start_pos==0)
		{
			sd_assert(g_xv_file_decoded_start_data_len!=0);
			sd_assert(g_xv_file_decoded_start_data_len<len);
			if(g_xv_file_decoded_start_data_len>=len)
			{
				ret_val = -1;
				goto ErrHandler;
			}
			
			sd_assert(gp_xv_file_decoded_start_data!=NULL);
			sd_memcpy(buf, gp_xv_file_decoded_start_data, g_xv_file_decoded_start_data_len);
			ret_val = sd_pread((_u32 )data->_file_index,buf+g_xv_file_decoded_start_data_len, len-g_xv_file_decoded_start_data_len, start_pos+g_xv_file_context_header_len+g_xv_file_decoded_start_data_len, &readsize);
			readsize+=g_xv_file_decoded_start_data_len;
		}
		else
		{
			ret_val = sd_pread((_u32 )data->_file_index,buf, len, start_pos+g_xv_file_context_header_len, &readsize);
		}
	}
	else
	{
		ret_val = sd_pread((_u32 )data->_file_index,buf, len, start_pos, &readsize);
	}
	LOG_DEBUG("http_server_sync_read_file:ret_val=%d,readsize=%u",ret_val,readsize);
	if(ret_val!=SUCCESS) goto ErrHandler;

	sd_assert(readsize == len);
	if(readsize != len) 
	{
		ret_val = -1;
		goto ErrHandler;
	}

ErrHandler:
	ret_val = http_server_vdm_async_recv_handler(ret_val,0, buf, len, (void*)data);
	CHECK_VALUE(ret_val);
	return ret_val;
}
_int32 http_server_handle_send( _int32 _errcode, _u32 notice_count_left,const char* buffer, _u32 had_send,void *user_data)
{
	_int32 task_id;
	_int32 ret;
	_u64  request_length = 0,dl_position = 0,real_file_pos = 0;
	HTTP_SERVER_ACCEPT_SOCKET_DATA* data = (HTTP_SERVER_ACCEPT_SOCKET_DATA*)user_data;
	LOG_DEBUG("http_server_handle_send...%d bytes, _errcode=%d, sock=%d,state=%d", had_send, _errcode, data->_sock,data->_state);
	ret = SUCCESS;

	if(_errcode != SUCCESS  || had_send == 0||data->_is_canceled)
	{
		data->_errcode = _errcode;
		return http_server_erase_accept_socket_data(data, TRUE);
	}

	if(data->_state == HTTP_SERVER_STATE_SENDING_ERR)
	{
		return http_server_erase_accept_socket_data(data, TRUE);
	}

	task_id = data->_task_id;
	sd_assert(had_send<=data->_fetch_file_length);
       had_send = MIN(had_send, data->_fetch_file_length);

	if(FLASH_PLAYER_HTML_FILE_TASK_ID == task_id||FLASH_PLAYER_SWF_FILE_TASK_ID == task_id
		||FLASH_PLAYER_ICO_FILE_TASK_ID == task_id||FLASH_PLAYER_LOCAL_FILE_TASK_ID == task_id
		||HTTP_LOCAL_FILE_TASK_ID == task_id)
	{
		ret = http_server_get_file_size(data,&dl_position);
	}
	else
	{
		ret = vdm_vod_get_download_position(task_id,&dl_position);
	}

	if(data->_is_flv_seek)
	{
		ret = http_server_correct_dl_position(data,&dl_position);
	}
	//sd_assert(ret == SUCCESS);
	LOG_DEBUG("http_get_download_position: ret=%d, task_id=%u,dl_position=%llu,_is_flv_seek=%d", ret, task_id, dl_position,data->_is_flv_seek);

	if(HTTP_SERVER_STATE_SENDING_HEADER == data->_state)
	{
		//sd_free(data->_buffer);
		//sd_malloc(SEND_MOVIE_BUFFER_LENGTH, (void**)&data->_buffer);
		if(data->_is_flv_seek)
		{
			return http_server_send_flv_header_and_first_tag(data);
		}
		
		data->_buffer_offset = 0;
		data->_buffer_len = http_server_get_buffer_len(data->_fetch_file_pos,dl_position);
		data->_state = HTTP_SERVER_STATE_SENDING_BODY;
	       request_length = MIN(data->_fetch_file_length, (_u64)data->_buffer_len);
		LOG_DEBUG("http_server_handle_send HTTP_SERVER_STATE_SENDING_HEADER, vdm_vod_async_read_file, pos = %llu, length= %llu", data->_fetch_file_pos,data->_fetch_file_length);

		if(FLASH_PLAYER_HTML_FILE_TASK_ID == task_id||FLASH_PLAYER_SWF_FILE_TASK_ID == task_id
			||FLASH_PLAYER_ICO_FILE_TASK_ID == task_id||FLASH_PLAYER_LOCAL_FILE_TASK_ID == task_id
			||HTTP_LOCAL_FILE_TASK_ID == task_id)
		{
			ret = http_server_sync_read_file(data,  data->_fetch_file_pos, request_length , data->_buffer );
		}
		else
		{
			ret = vdm_vod_async_read_file(task_id, data->_file_index,  data->_fetch_file_pos, request_length , data->_buffer, 0 , http_server_vdm_async_recv_handler, (void*)data);
		}
	}
	else if(HTTP_SERVER_STATE_SENDING_BODY == data->_state)
	{
		data->_buffer_offset = 0;
		data->_fetch_file_pos += had_send; 
		data->_fetch_file_length  -= had_send;
		data->_buffer_len = http_server_get_buffer_len(data->_fetch_file_pos,dl_position);
		data->_state = HTTP_SERVER_STATE_SENDING_BODY;
		sd_time_ms(&data->_fetch_time_ms);
	       request_length = MIN(data->_fetch_file_length, (_u64)data->_buffer_len);
		if(request_length >0)
		{
			LOG_DEBUG("http_server_handle_send HTTP_SERVER_STATE_SENDING_BODY, vdm_vod_async_read_file, pos = %llu, length= %llu", data->_fetch_file_pos,data->_fetch_file_length);
			real_file_pos = data->_fetch_file_pos;
			if(data->_is_flv_seek)
			{
				ret = http_server_get_real_file_pos(data,data->_fetch_file_pos,&real_file_pos);
				if(ret!=SUCCESS)
				{
					http_server_erase_accept_socket_data(data, TRUE);
					return SUCCESS;
				}
			}

			if(FLASH_PLAYER_HTML_FILE_TASK_ID == task_id||FLASH_PLAYER_SWF_FILE_TASK_ID == task_id
			||FLASH_PLAYER_ICO_FILE_TASK_ID == task_id||FLASH_PLAYER_LOCAL_FILE_TASK_ID == task_id
			||HTTP_LOCAL_FILE_TASK_ID == task_id)
			{
				ret = http_server_sync_read_file(data,  real_file_pos, request_length , data->_buffer);
			}
			else
			{
				ret = vdm_vod_async_read_file(task_id, data->_file_index, real_file_pos,request_length , data->_buffer, 0 , http_server_vdm_async_recv_handler, (void*)data);
			}
		}
		else
		{
			LOG_DEBUG("http_server_handle_send request to file end, then stop");
			ret = http_server_erase_accept_socket_data(data, TRUE);
		}
	}

	return ret;
}

_int32 http_server_handle_get(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, _u32 header_len)
{
	_int32 ret = SUCCESS;
	char file_name[MAX_URL_LEN]={0};
	_u64 file_pos = 0;
	char str[MAX_URL_LEN]={0};
	_int32 i = 0;
	_int32 task_id = 0;
	TASK* taskinfo = NULL;
	TASK* ptr_task = NULL;
	_u64 file_size = 0;
	_int32  file_index = 0;
	_u32   pos = 0,current_vod_id = 0;
	char * p_cookie = NULL;
	BOOL  is_local_file = FALSE;

	*(data->_buffer+header_len) ='\0'; 
	p_cookie = sd_strstr(data->_buffer, "Cookie: ", 0);

	LOG_DEBUG("http_server_handle_get,sock=%u. got a HTTP request[%u]:%s",data->_sock,header_len,data->_buffer);
	
	//已经收到整个GET请求
	ret = http_server_parse_get_request(data->_buffer, data->_buffer_offset, file_name, &file_pos,&data->_is_flv_seek,&is_local_file);
    	if(SUCCESS != ret)
	{
	       LOG_DEBUG("http_server_parse_get_request. error ret=%d", ret);
		return http_server_erase_accept_socket_data(data, TRUE);
	}

	LOG_DEBUG("http_server_parse_get_request end,file_name=%s,file_pos=%llu,is_flv_seek=%d,is_local_file=%d",file_name,file_pos,data->_is_flv_seek,is_local_file);
	/* 增加对android下FLV播放的支持 */
	#ifdef ENABLE_FLASH_PLAYER
	#ifndef _DEBUG
	if(is_local_file==FALSE&&NULL!=sd_strstr(file_name,FLASH_PLAYER_LOCAL_FILE_MARK,0))
	{
		if(NULL==sd_strstr(data->_buffer,"Host: 127.0.0.1",0))
		{
			LOG_DEBUG("http_server_response_header :NOT Allowed to visit this file:%s",file_name); 
			/* 不允许非本机的文件访问*/
			ret = http_server_response_header(404, data->_buffer, file_pos, (_u64)0);
			data->_state = HTTP_SERVER_STATE_SENDING_ERR;
		       ret =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
			return SUCCESS;
		}
	}
	#endif /* _DEBUG */
	if(is_local_file==FALSE)
	{
		if((NULL!=sd_strstr(file_name,FLASH_PLAYER_HTML_FILE_NAME,0))||(NULL!=sd_strstr(file_name,FLASH_PLAYER_SWF_FILE_NAME,0))
			||(NULL!=sd_strstr(file_name,FLASH_PLAYER_ICO_FILE_NAME,0))||(NULL!=sd_strstr(file_name,FLASH_PLAYER_LOCAL_FILE_MARK,0)))
		{
		       ret =  http_server_handle_player_file(data, file_name,file_pos);
			CHECK_VALUE(ret);
			return SUCCESS;
		}
	}
	#endif /* ENABLE_FLASH_PLAYER */

	#ifdef ENABLE_ASSISTANT
	if(is_local_file)
	{
		if(p_cookie==NULL ||NULL==sd_strstr(p_cookie,"key=",0))
		{
			/* 这里需要检验key的合法性 */
			
			LOG_DEBUG("http_server_response_header :NOT Allowed to visit this file:%s,because no key!",file_name); 
			
			/* 不允许的文件访问*/
			ret = http_server_response_header(404, data->_buffer, file_pos, (_u64)0);
			data->_state = HTTP_SERVER_STATE_SENDING_ERR;
		       ret =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
			return SUCCESS;
		}
		
	       ret =  http_server_handle_get_local_file(data, file_name,file_pos);
		CHECK_VALUE(ret);
		return SUCCESS;
	}
	#endif /* ENABLE_ASSISTANT */
	
	/*在这里增加处理传进来URL ，创建相应任务，涉及原始URL or 看看URL*/

       i=0;
	sd_memset((void*)str, 0, sizeof(str));
	while(file_name[i]<='9' && file_name[i]>='0' && i<MAX_URL_LEN)
	{
	     str[i] = file_name[i];
	     i++;
	}
	str[i+1] = '\0';
	
	/* 获取task_id */
	if(i >0 && i < MAX_URL_LEN ){
	    task_id =  sd_atoi(str);
	}else{
	    task_id = 0;
	}

	/* 获取file_index */
	i++;
	pos = i;
	while(file_name[i]<='9' && file_name[i]>='0' && i<MAX_URL_LEN)
	{
	     str[i-pos] = file_name[i];
	     i++;
	}
	str[i-pos+1] = '\0';
	if(i >pos && i < MAX_URL_LEN ){
	    file_index =  sd_atoi(str);
	}else{
	    file_index = 0;
	}
	
	LOG_DEBUG("handle_http_recv_callback. task_id = %d , file_index = %d,file_pos=%llu", task_id, file_index,file_pos);
	ret = tm_get_task_by_id(task_id,&taskinfo);
	if(SUCCESS != ret)
	{
		LOG_DEBUG("http_server_response_header .tm_get_task_by_id return = %d", ret); 
		/*fetch the filename's buffer here*/
		ret = http_server_response_header(404, data->_buffer, file_pos, (_u64)0);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	       ret =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
	}
	else
	{
		/*get DOWNLOAD_TASK ptr here*/
		ptr_task = (TASK*)taskinfo;
		current_vod_id = vdm_get_current_vod_task_id();
		LOG_DEBUG("http_server_response_header .current_vod_id=%X,task_id=%X ",current_vod_id,ptr_task->_task_id); 
		if(current_vod_id!=0 && current_vod_id!=ptr_task->_task_id)
		{
			/* 不能同时点播两个任务 */
			LOG_DEBUG("http_server_response_header .cannot play 2 tasks,current vod task=%u ",current_vod_id); 
			ret = http_server_response_header(404, data->_buffer, file_pos, (_u64)0);
			data->_state = HTTP_SERVER_STATE_SENDING_ERR;
			return socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
		}
		
		if(P2SP_TASK_TYPE == ptr_task->_task_type)
		{
		     file_size = dm_get_file_size(&((P2SP_TASK*)ptr_task)->_data_manager);
		}
#ifdef ENABLE_BT
		else if(BT_TASK_TYPE == ptr_task->_task_type)
		{
		     file_size = bdm_get_sub_file_size(&((BT_TASK*)ptr_task)->_data_manager, file_index);
		}
#endif
		else
		{
		     LOG_DEBUG("http_server_response_header .unknown task type "); 
		     file_size = 0;
		}
		//file_size = file_size==0?((_u64)1024*1024*100):file_size;
		if( file_size == 0 ) //return ERR_VOD_DATA_UNKNOWN_TASK;
		{
			LOG_DEBUG("http_server_response_header .cannot get filesize "); 
			ret = http_server_response_header(404, data->_buffer, file_pos, (_u64)0);
			data->_state = HTTP_SERVER_STATE_SENDING_ERR;
			return socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
		}
		
		LOG_DEBUG("http_server_response_header .task_id=%X,file_index=%u ,file_size=%llu, file_pos=%llu",task_id,file_index, file_size, file_pos); 
	   	ret =  http_server_send_response_header( data,task_id,file_index, file_size, file_pos);
		if(ret!=SUCCESS)
		{
			ret = http_server_response_header(404, data->_buffer, file_pos, (_u64)0);
			data->_state = HTTP_SERVER_STATE_SENDING_ERR;
		       ret =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
		}
	}

	return ret;

}
_int32 http_server_handle_post(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, _u32 header_len)
{
#ifdef ENABLE_ASSISTANT
	_int32 ret = SUCCESS;
	char*  ptr_data = NULL;
	char file_name[MAX_URL_LEN]={0};
	_u64 content_len = 0;
	char * p_cookie = NULL;
	BOOL  is_local_file = FALSE;
	_u32 data_len = 0;


	ptr_data=data->_buffer+header_len;
	data_len = data->_buffer_offset-header_len;
	
	*(ptr_data-1) = '\0';
	p_cookie = sd_strstr(data->_buffer, "Cookie: ", 0);

	LOG_DEBUG("http_server_handle_post,sock=%u. got a HTTP request[%u]:%s",data->_sock,header_len,data->_buffer);

	//已经收到整个POST请求
	ret = http_server_parse_post_request(data->_buffer, header_len-1, file_name, &content_len,&is_local_file);
    	if(SUCCESS != ret)
	{
	       LOG_DEBUG("http_server_parse_post_request. error ret=%d", ret);
		return http_server_erase_accept_socket_data(data, TRUE);
	}

	if(p_cookie==NULL ||NULL==sd_strstr(p_cookie,"key=",0))
	{
		/* 这里需要检验key的合法性 */
		
		LOG_DEBUG("http_server_handle_post :NOT Allowed to visit this file:%s,because no key!",file_name); 
		
		/* 不允许的文件访问*/
		ret = http_server_response_header(404, data->_buffer, 0, (_u64)0);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	       ret =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
		return SUCCESS;
	}

	if(is_local_file)
	{
	       http_server_handle_post_local_file(data, file_name,content_len,ptr_data,data_len);
	}
	else
	{
	      http_server_handle_post_command(data, file_name,content_len,ptr_data,data_len);
	}

	return SUCCESS;
#else
	return http_server_erase_accept_socket_data(data, TRUE);
#endif /* ENABLE_ASSISTANT */
}
_int32 http_server_handle_http_complete_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
#ifdef ENABLE_ASSISTANT	
	_int32 ret_val = SUCCESS;
	_u32 writesize = 0;

	HTTP_SERVER_ACCEPT_SOCKET_DATA* data = (HTTP_SERVER_ACCEPT_SOCKET_DATA*)user_data;
	LOG_DEBUG("http_server_handle_http_complete_recv_callback..., sock=%d,errcode=%d,had_recv=%u", data->_sock,errcode,had_recv);
	if(errcode != SUCCESS)
	{
		return http_server_erase_accept_socket_data(data, TRUE);
	}
	//接收成功了
	sd_assert(had_recv > 0);
	//data->_buffer_offset += had_recv;

	ret_val = sd_pwrite(data->_file_index, buffer,had_recv, data->_fetch_file_pos,&writesize);
	if(ret_val!=SUCCESS||writesize!=had_recv) goto ErrHandler;

	
	data->_fetch_file_pos += writesize;
	if(data->_fetch_file_pos<data->_fetch_file_length)
	{
		/* 接收后续数据*/
		
		data->_buffer_len = MIN(data->_buffer_len,data->_fetch_file_length-data->_fetch_file_pos);

		ret_val =  socket_proxy_recv(data->_sock, data->_buffer , data->_buffer_len, http_server_handle_http_complete_recv_callback, data);
		if(ret_val!=SUCCESS) goto ErrHandler;
	}
	else
	{
		/* 数据接收完毕 */
		sd_close_ex(data->_file_index);
		data->_file_index = 0;
		if(data->_post_type ==1)
		{
			ret_val =  http_server_response_header(SUCCESS,  data->_buffer, 0,0);
			sd_assert(ret_val==SUCCESS);
			g_current_task_id = 0;
			gp_http_data = NULL;
			g_local_file_path[0]='\0';
			data->_state = HTTP_SERVER_STATE_SENDING_ERR;
		       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
			sd_assert(ret_val==SUCCESS);
		}
		else
		if(data->_post_type ==2)
		{
			MA_REQ ma_req = {0};

			ma_req._ma_id = (_u32)data;
			sd_strncpy(ma_req._req_file, g_local_file_path, 1023);

			/* 回调通知UI */
			LOG_DEBUG("http_server_handle_http_complete_recv_callback:g_assistant_incom_req_callback:ma_id=0x%X,ma_req._req_file=%s",ma_req._ma_id,ma_req._req_file);
			ret_val = g_assistant_incom_req_callback(&ma_req);
			if(ret_val!=SUCCESS) goto ErrHandler;

			sd_assert(g_http_server_timer_id==0);
			ret_val = start_timer(http_server_timeout, NOTICE_ONCE, DEFAULT_HTTP_SERVER_TIMEOUT, 0, data, &g_http_server_timer_id);
			sd_assert(ret_val==SUCCESS);
			sd_assert(g_http_server_timer_id!=0);

			sd_assert(gp_http_data==NULL);
			gp_http_data = data;
			data->_post_type = 3;
		}
		else
		{
			sd_assert(FALSE);
		}
	}
	return SUCCESS;
	
ErrHandler:
	LOG_ERROR("http_server_handle_post_local_file .ret_val = %d", ret_val); 
	ret_val = http_server_response_header(503, data->_buffer, 0,0);
	sd_assert(ret_val==SUCCESS);
	g_current_task_id = 0;
	gp_http_data = NULL;
	sd_delete_file(g_local_file_path);
	g_local_file_path[0]='\0';
	data->_state = HTTP_SERVER_STATE_SENDING_ERR;
       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
	CHECK_VALUE(ret_val);
#endif /* #ifdef ENABLE_ASSISTANT	*/
	return SUCCESS;
}

_int32 http_server_handle_http_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	_int32 ret = SUCCESS;
	char*  ptr_crlf = NULL;

	HTTP_SERVER_ACCEPT_SOCKET_DATA* data = (HTTP_SERVER_ACCEPT_SOCKET_DATA*)user_data;
	LOG_DEBUG("handle_http_recv_callback..., sock=%d,errcode=%d,had_recv=%u", data->_sock,errcode,had_recv);
	if(errcode != SUCCESS||data->_is_canceled)
	{
		return http_server_erase_accept_socket_data(data, TRUE);
	}
	//接收成功了
	sd_assert(had_recv > 0);
	data->_buffer_offset += had_recv;
	/*wait for \r\n\r\n here*/
	ptr_crlf = sd_strstr(data->_buffer, "\r\n\r\n", 0);

	if(NULL == ptr_crlf)
		return socket_proxy_uncomplete_recv(data->_sock, data->_buffer + data->_buffer_offset, data->_buffer_len - data->_buffer_offset, http_server_handle_http_recv_callback, data);

	if(sd_strncmp(data->_buffer, "GET ", 4)==0)
	{
		http_server_handle_get(data, ptr_crlf+4-data->_buffer);
	}
	else
	{
		http_server_handle_post(data, ptr_crlf+4-data->_buffer);
	}
	
	return ret;

}

_int32 http_server_handle_http_accept_callback(_int32 errcode, _u32 pending_op_count, SOCKET conn_sock, void* user_data)
{
	_int32 ret = SUCCESS;
	HTTP_SERVER_ACCEPT_SOCKET_DATA*	data = NULL;
       LOG_DEBUG("http_server_handle_http_accept_callback errcode = %d, conn_sock=%d", errcode, conn_sock); 
	if(errcode == MSG_CANCELLED)
	{
		if(pending_op_count == 0)
		{
			ret =  socket_proxy_close(g_http_server_tcp_accept_sock);
			g_http_server_tcp_accept_sock = INVALID_SOCKET;
			return SUCCESS;
		}
		else 
			return SUCCESS;
	}
	if(errcode == SUCCESS)
	{	//accept success

		/*******************************/
		if(set_size(&g_HTTP_SERVER_ACCEPT_SOCKET_DATA_set)>0)
		{
			SET_NODE * node = SET_BEGIN(g_HTTP_SERVER_ACCEPT_SOCKET_DATA_set);
			data = (HTTP_SERVER_ACCEPT_SOCKET_DATA*)node->_data;
       		LOG_DEBUG("http_server_handle_http_accept_callback:set_size=%u,data=0x%X,data->_task_id=0x%X", set_size(&g_HTTP_SERVER_ACCEPT_SOCKET_DATA_set),data,data->_task_id); 
			if(data->_task_id == HTTP_LOCAL_FILE_TASK_ID)
			{
       			LOG_ERROR("http_server_handle_http_accept_callback:ERROR:g_current_task_id=0x%X,g_http_server_timer_id=%u,gp_http_data=0x%X,g_local_file_path=%s",g_current_task_id,g_http_server_timer_id, gp_http_data,g_local_file_path); 
				sd_assert(FALSE);
				socket_proxy_close(conn_sock);
				goto ErrHandler;
			}
			else
			if(data->_task_id <=FLASH_PLAYER_LOCAL_FILE_TASK_ID)
			{
				/* 把之前的请求停掉 */
				data->_is_canceled = TRUE;
	       		LOG_ERROR("http_server_handle_http_accept_callback:ERROR:g_current_task_id=0x%X,cancel the old request!",g_current_task_id); 
			}
		}
		/*******************************/
		ret = sd_malloc((_u32)sizeof(HTTP_SERVER_ACCEPT_SOCKET_DATA), (void**)(&data) );
		CHECK_VALUE(ret);
		sd_memset(data,0x00,sizeof(HTTP_SERVER_ACCEPT_SOCKET_DATA));
       	LOG_DEBUG("http_server_handle_http_accept_callback malloc HTTP_SERVER_ACCEPT_SOCKET_DATA=0x%X", data); 
		ret = sd_malloc(SEND_MOVIE_BUFFER_LENGTH_LOOG, (void**)&data->_buffer);
		CHECK_VALUE(ret);
		data->_sock = conn_sock;
		data->_buffer_len = SEND_MOVIE_BUFFER_LENGTH_LOOG;
		data->_buffer_offset = 0;
		sd_time_ms(&data->_fetch_time_ms);
		if(data->_buffer != NULL)
		{
			socket_proxy_uncomplete_recv(data->_sock, data->_buffer, RECV_HTTP_HEAD_BUFFER_LEN, http_server_handle_http_recv_callback, data);
			ret = set_insert_node(&g_HTTP_SERVER_ACCEPT_SOCKET_DATA_set, data);
			CHECK_VALUE(ret);
		}
		else
		{
			sd_assert(FALSE);
			socket_proxy_close(conn_sock);
			http_server_safe_erase_accept_socket_data(data);
		}
	}
	
ErrHandler:
       if(g_http_server_tcp_accept_sock != INVALID_SOCKET)
       {
	      ret =  socket_proxy_accept(g_http_server_tcp_accept_sock, http_server_handle_http_accept_callback, NULL);
       }
       return ret;
}


_int32 http_vod_server_stop()
{
	_u32 count = 0;
	_int32 ret = SUCCESS;
	LOG_DEBUG("http_vod_server_stop...");
	if(g_http_server_tcp_accept_sock == INVALID_SOCKET)
		return SUCCESS;
	ret = socket_proxy_peek_op_count(g_http_server_tcp_accept_sock, DEVICE_SOCKET_TCP, &count);
	if(SUCCESS != ret )
	{
	  LOG_DEBUG("http_vod_server_stop. http_server is not running, ret = %d", ret);
	  g_http_server_tcp_accept_sock = INVALID_SOCKET;
	   return ret;
	}
	//sd_assert(count == 1);
	if(count > 0)
	{
		ret =  socket_proxy_cancel(g_http_server_tcp_accept_sock, DEVICE_SOCKET_TCP);
	}
	else
	{
		ret  =  socket_proxy_close(g_http_server_tcp_accept_sock);
	       g_http_server_tcp_accept_sock = INVALID_SOCKET;
	}
	return ret;

}

_int32 http_vod_server_start(_u16 * port)
{
	/*create tcp accept*/
	SD_SOCKADDR	addr;
	_int32 ret = socket_proxy_create(&g_http_server_tcp_accept_sock, SD_SOCK_STREAM);

	LOG_DEBUG("http_vod_server_start...");
	
	CHECK_VALUE(ret);
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ANY_ADDRESS;		/*any address*/
	addr._sin_port = sd_htons(*port);
	ret = socket_proxy_bind(g_http_server_tcp_accept_sock, &addr);
	if(ret != SUCCESS)
	{
	        LOG_DEBUG("http_vod_server_start...socket_proxy_bind ret=%d", ret);
		socket_proxy_close(g_http_server_tcp_accept_sock);
		g_http_server_tcp_accept_sock = INVALID_SOCKET;
		return ret;
	}
	*port = sd_ntohs(addr._sin_port);
	LOG_DEBUG("http_vod_server_start bind port %d...", *port);
	ret = socket_proxy_listen(g_http_server_tcp_accept_sock, *port);
	if(ret != SUCCESS)
	{
	        LOG_DEBUG("http_vod_server_start...socket_proxy_listen ret=%d", ret);
		socket_proxy_close(g_http_server_tcp_accept_sock);
		g_http_server_tcp_accept_sock = INVALID_SOCKET;
		return ret;
	}
	ret = socket_proxy_accept(g_http_server_tcp_accept_sock, http_server_handle_http_accept_callback, NULL);
	if(ret != SUCCESS)
	{
	        LOG_DEBUG("http_vod_server_start...socket_proxy_accept ret=%d", ret);
		socket_proxy_close(g_http_server_tcp_accept_sock);
		g_http_server_tcp_accept_sock = INVALID_SOCKET;
	}
	set_init(&g_HTTP_SERVER_ACCEPT_SOCKET_DATA_set, http_server_accept_socket_data_comparator);

	return ret;
}

_int32 http_server_start_handle(_u16 *port)
{
#ifdef ENABLE_HTTP_SERVER
  	return init_http_server_module(port);
#else
  	return init_vod_http_server_module(port);
#endif /* ENABLE_HTTP_SERVER */	
}

_int32 http_server_stop_handle()
{
#ifdef ENABLE_HTTP_SERVER
  	return uninit_http_server_module();
#else
  	return uninit_vod_http_server_module();
#endif /* ENABLE_HTTP_SERVER */	
}

//////////////////////////////////////////////////////
_int32 http_server_start_search_server( void * _param )
{
	TM_POST_PARA_1* p_param = (TM_POST_PARA_1*)_param;
	SEARCH_SERVER * p_search = (SEARCH_SERVER *)p_param->_para1;

	_u32 local_ip = 0;
	_u32 netmask = 0;
	_u32 n_start_ip = 0;
	_u32 n_end_ip = 0;
	_int32 ret_val = SUCCESS;
#if defined(MACOS)&&defined(MOBILE_PHONE)
	_u32 time = 0;
#endif

	if( (p_search->_find_server_callback_fun == NULL) || (p_search->_search_finished_callback_fun == NULL))
	{
		p_param->_result = HS_SEARCH_INVALID_PARAMETER;
		
		LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
		return signal_sevent_handle(&(p_param->_handle));			
	}
	if(g_search_info._search_state == SEARCH_RUNNING||g_search_info._search_state == SEARCH_STOPPING)
	{
		p_param->_result = HS_SEARCH_INVALID_STATE;
		
		LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
		return signal_sevent_handle(&(p_param->_handle));	
	}
#if defined(_DEBUG) && defined(MACOS)&&defined(MOBILE_PHONE)
	sd_time_ms(&time);
	printf("start time:%u=================\n", time);
#endif	
	sd_memset(g_search_cache_address, 0, sizeof(CACHE_ADDRESS) * SEARCH_CACHE_ADDRESS_MAX_NUM);
	
	sd_memset(&g_search_info, 0, sizeof(SEARCH_INFO));
	
	g_search_info._find_server_callback = p_search->_find_server_callback_fun;
	g_search_info._search_finished_callback = p_search->_search_finished_callback_fun;
	g_search_info._search_state = SEARCH_RUNNING;

	if((p_search->_ip_from != 0) && (p_search->_ip_to != 0))
	{
		g_search_info._start_ip = p_search->_ip_from;
		g_search_info._end_ip = p_search->_ip_to;
	}
	else
	{
		sd_set_local_ip(0);
		local_ip = sd_get_local_ip();
		netmask = sd_get_local_netmask();

		if(local_ip == 0)
		{
			p_param->_result = HS_SEARCH_INVALID_LOCAL_IP;
			LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
			return signal_sevent_handle(&(p_param->_handle));	
		}

		if(netmask == 0)
		{
			p_param->_result = HS_SEARCH_INVALID_NETMASK;
			LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
			return signal_sevent_handle(&(p_param->_handle));	
		}
		
		n_start_ip = local_ip & netmask;
		n_end_ip = local_ip | (~netmask);
		
		g_search_info._start_ip = sd_ntohl(n_start_ip) + 1;
		g_search_info._end_ip = sd_ntohl(n_end_ip) - 1;
	}

	g_search_info._start_port = p_search->_port_from;
	g_search_info._end_port = p_search->_port_to;

	//http_server_start_ssdp_find();
	
	ret_val = http_server_start_local_poll(g_search_info._start_ip, g_search_info._end_ip, g_search_info._start_port);
		
	p_param->_result = ret_val;
	LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}

_int32 http_server_restart_search_server( void * _param )
{
	TM_POST_PARA_0* p_param = (TM_POST_PARA_0*)_param;
	p_param->_result = SUCCESS;
	_u32 local_ip = 0;
	_u32 netmask = 0;
	_u32 n_start_ip = 0;
	_u32 n_end_ip = 0;

	if((g_search_info._find_server_callback == NULL) ||( g_search_info._search_finished_callback == NULL))
	{
		p_param->_result = HS_SEARCH_HAVE_NOT_STARTED;
		LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
		return signal_sevent_handle(&(p_param->_handle));			
	}
	
	if((g_search_info._search_state == SEARCH_IDLE) || (g_search_info._search_state == SEARCH_FINISHED))
	{
		// 如果是空闲状态直接start
		sd_set_local_ip(0);
		local_ip = sd_get_local_ip();
		netmask = sd_get_local_netmask();
		if(local_ip == 0)
		{
			p_param->_result = HS_SEARCH_INVALID_LOCAL_IP;
			LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
			return signal_sevent_handle(&(p_param->_handle));	
		}

		if(netmask == 0)
		{
			p_param->_result = HS_SEARCH_INVALID_NETMASK;
			LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
			return signal_sevent_handle(&(p_param->_handle));	
		}

		n_start_ip = local_ip & netmask;
		n_end_ip = local_ip | (~netmask);
		
		g_search_info._start_ip = sd_ntohl(n_start_ip) + 1;
		g_search_info._end_ip = sd_ntohl(n_end_ip) - 1;		
		g_search_info._used_fd = 0;
		g_search_info._connected_num = 0;
		g_search_info._search_state = SEARCH_RUNNING;
		g_search_info._current_ip = g_search_info._start_ip - 1;
		// 先搜索缓存地址
		if (http_server_start_cache_poll() != SUCCESS) {
			p_param->_result = http_server_start_local_poll(g_search_info._start_ip, g_search_info._end_ip, g_search_info._start_port);
		} 
		//p_param->_result = http_server_start_local_poll(g_search_info._start_ip, g_search_info._end_ip, g_search_info._start_port);
	}
	else
	{
		g_search_info._search_state = SEARCH_STOPPING;
		g_search_need_restart = TRUE;
	}

	LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}

_int32 http_server_stop_search_server( void * _param )
{
	TM_POST_PARA_0* p_param = (TM_POST_PARA_0*)_param;	
	p_param->_result = SUCCESS;

	if (g_search_info._search_state == SEARCH_RUNNING) 
	{
		g_search_info._search_state = SEARCH_STOPPING;	
		g_search_need_restart = FALSE;
	}
	else 
	if (g_search_info._search_state == SEARCH_FINISHED) 
	{
		g_search_info._search_state = SEARCH_IDLE;
	}
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}
/*
_int32 http_server_start_ssdp_find()
{
	SD_SOCKADDR addr;
	SOCKET sockfd = 0;
	_int32 ret_val = SUCCESS;
	SINGLE_CONNECTION *p_connect;
	
	ret_val = sd_malloc(sizeof(SINGLE_CONNECTION), (void**)&p_connect);
	CHECK_VALUE(ret_val);
	sd_memset(p_connect, 0, sizeof(SINGLE_CONNECTION));

	char* sendbuf = "M-SEARCH * HTTP/1.1\r\nHost: 239.255.255.250:1900\r\nMan: \"ssdp:discover\"\r\nMX: 5\r\nST: urn:schemas-upnp-org:service:filestorage\r\n\r\n";

	ret_val = socket_proxy_create(&sockfd, SD_SOCK_DGRAM);
	if(ret_val != SUCCESS)
	{
		return ret_val;
	}
	
	p_connect->_sockfd = sockfd;
	
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = sd_inet_addr("239.255.255.250");
	addr._sin_port = sd_htons(1900);

	ret_val = socket_proxy_sendto(sockfd, sendbuf, sd_strlen(sendbuf), &addr, http_server_ssdp_send_callback, (void*)p_connect);
	
	return ret_val;
}

_int32 http_server_ssdp_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	SINGLE_CONNECTION *p_connect = (SOCKET*)user_data;
	char *recv_buf = NULL;
	_int32 ret_val = SUCCESS;
	

	if(errcode == SUCCESS)
	{
		ret_val = sd_malloc(1024, (void**)&recv_buf);
		
		CHECK_VALUE(ret_val);
		sd_memset(recv_buf, 0, 1024);
		ret_val = socket_proxy_recvfrom(p_connect->_sockfd, recv_buf, 1024, http_server_ssdp_recv_callback, user_data);
	}
	return ret_val;
}

_int32 http_server_ssdp_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	if(errcode == SUCCESS)
	{
#if defined(MACOS)&&defined(MOBILE_PHONE)
		printf("====================%s", buffer);
#endif
	}
	g_search_info._search_state = SEARCH_FINISHED;
	return SUCCESS;
}

*/

_int32 http_server_start_cache_poll(void)
{
	char ip_addr[24] = {0};
	_int32 i;
	_int32 ret_val = -1;
	
	for(i = 0; i < SEARCH_CACHE_ADDRESS_MAX_NUM; i++)
	{
		if(g_search_cache_address[i]._ip != 0)
		{
			sd_inet_ntoa(g_search_cache_address[i]._ip, ip_addr, 24);
			g_search_cache_address[i]._notified = FALSE;
			ret_val  = http_server_single_connect(ip_addr, (short)g_search_cache_address[i]._port, g_search_info._find_server_callback);
			if(ret_val != SUCCESS)
			{
				break;			
			}
			g_search_info._connected_num++;
		}
		else {
			break;
		}
	}
	return ret_val;
}

_int32 http_server_start_local_poll(_u32 start_ip, _u32 end_ip, _u32 port)
{
	char ip_addr[24] = {0};
	_u32 num;
	_int32 ret_val = SUCCESS;
	_u32 current_ip = start_ip;
 	short port_connect = (short)port; 
	
	for (num = 0; num < SEARCH_CONNECT_MAX_NUM; num++)
	{
		sd_inet_ntoa(sd_htonl(current_ip), ip_addr, 24);
		ret_val  = http_server_single_connect(ip_addr, port_connect, g_search_info._find_server_callback);
		if((ret_val != SUCCESS))
		{
			if(num == 0)
			{
				return ret_val;	
			}
			else
			{
				break;
			}
		}
		g_search_info._connected_num++;
		g_search_info._current_ip = current_ip;
		
		if(current_ip++ == end_ip) break;
	}
	
	return ret_val;

}

_int32 http_server_single_connect(char* ip_addr, _u16 port, FIND_SERVER_CALL_BACK_FUNCTION callback_fun)
{
	SD_SOCKADDR addr;
	SINGLE_CONNECTION *p_connect;
	SOCKET sockfd = 0;
	_int32 ret_val = SUCCESS;

	ret_val = socket_proxy_create(&sockfd, SD_SOCK_STREAM);

	if(ret_val != SUCCESS)
	{
		return ret_val;
	}
	
	g_search_info._used_fd++;

	//printf("http_server_single_connect: ip:%s : %d\n", ip_addr, port);

	ret_val = sd_malloc(sizeof(SINGLE_CONNECTION), (void**)&p_connect);
	
	if(ret_val != SUCCESS)
	{
		socket_proxy_close(sockfd);
		g_search_info._used_fd--;
		//sd_free(p_connect);
		return ret_val;
	}
	sd_memset(p_connect, 0, sizeof(SINGLE_CONNECTION));

	p_connect->_sockfd = sockfd;
	p_connect->_ip = sd_inet_addr(ip_addr);
	p_connect->_port = (_u32)port;
	p_connect->_callback_fun = callback_fun;
	
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = p_connect->_ip;
	addr._sin_port = sd_htons(port);

	ret_val = socket_proxy_connect_with_timeout(sockfd, &addr, SEARCH_CONNECT_TIMEOUT, http_server_single_connect_callback, (void*)p_connect);

	if(ret_val != SUCCESS)
	{
		socket_proxy_close(sockfd);
		g_search_info._used_fd--;
		sd_free(p_connect);
		p_connect = NULL;
		return ret_val;
	}
	return ret_val;
	
}

_int32 http_server_single_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data)
{
	SINGLE_CONNECTION *p_connect = (SINGLE_CONNECTION*)user_data;
	_int32 ret_val = SUCCESS;
	char* buffer = NULL;
	_u32 buffer_len = 0;
	char ip_addr[24] = {0};
	_u32 local_ip = 0;
	_u32 netmask = 0;
	_u32 n_start_ip = 0;
	_u32 n_end_ip = 0;
	
	sd_inet_ntoa(p_connect->_ip, ip_addr, 24);
	//printf("%s:%u connect result %d\n", ip_addr, p_connect->_port, errcode);

	g_search_info._connected_num--;
	
	if(SUCCESS == errcode)
	{
		ret_val = http_server_build_query_cmd(&buffer, &buffer_len, p_connect);
		if(ret_val == SUCCESS)
		{
			socket_proxy_send(p_connect->_sockfd, buffer, buffer_len, http_server_single_send_callback, user_data);
		}
		else
		{
			socket_proxy_close(p_connect->_sockfd);
			g_search_info._used_fd--;
			sd_free(p_connect);		
			p_connect = NULL;
		}
	}
	else
	{
		socket_proxy_close(p_connect->_sockfd);
		g_search_info._used_fd--;
		sd_free(p_connect);
		p_connect = NULL;
	}
	if (g_search_info._connected_num == 0)
	{

		if(g_search_info._search_state == SEARCH_STOPPING)
		{
			g_search_info._search_state = SEARCH_IDLE;
			if(g_search_need_restart == TRUE)
			{
				/* 需要restart，重新获取ip地址进行搜索 */
				sd_set_local_ip(0);
				local_ip = sd_get_local_ip();
				netmask = sd_get_local_netmask();

				if((local_ip == 0) || (netmask == 0))
				{
					return -1;
				}
				
				n_start_ip = local_ip & netmask;
				n_end_ip = local_ip | (~netmask);
				
				g_search_info._start_ip = sd_ntohl(n_start_ip) + 1;
				g_search_info._end_ip = sd_ntohl(n_end_ip) - 1;	
				g_search_info._search_state = SEARCH_RUNNING;
				sd_assert(g_search_info._used_fd == 0);
				g_search_info._current_ip = g_search_info._start_ip - 1;
				// 先搜索缓存地址
				if (http_server_start_cache_poll() != SUCCESS) 
				{
					http_server_start_local_poll(g_search_info._start_ip, g_search_info._end_ip, g_search_info._start_port);
				} 
				g_search_need_restart = FALSE;
			}
		}
		else if(g_search_info._search_state == SEARCH_RUNNING)
		{
			if (g_search_info._current_ip == g_search_info._end_ip) 
			{
				if (g_search_info._used_fd == 0) 
				{
					http_server_search_finished_callback();	
				}		
			}
			else {
				http_server_start_local_poll(g_search_info._current_ip+ 1, g_search_info._end_ip, g_search_info._start_port);
			}

		}
	}

	return ret_val;
	
}

_int32 http_server_build_query_cmd(char** buffer, _u32* len, SINGLE_CONNECTION* p_connect)
{
	_int32 ret_val;
	char ip_addr[24] = {0};
	char _Tbuffer[MAX_URL_LEN];
	char cookie[256] = {0};
	char os[MAX_OS_LEN]={0},device_name[64]={0},hardware_ver[64]={0};
	char ui_version[64];
	_int32 x,y;
	char	peerid[PEER_ID_SIZE + 1] = {0};

	sd_memset((void*)_Tbuffer, 0, sizeof(_Tbuffer));
	sd_inet_ntoa(p_connect->_ip, ip_addr, 24);
	
	ret_val = sd_get_os(os, MAX_OS_LEN);
	CHECK_VALUE(ret_val);
		
	ret_val = sd_get_screen_size(&x, &y);
	CHECK_VALUE(ret_val);
	
	sd_memset(ui_version, 0x00, 64);
	settings_get_str_item("system.ui_version", ui_version);
	
	get_peerid(peerid, PEER_ID_SIZE + 1);
	
	sd_get_device_name(device_name,64);
	sd_get_hardware_version(hardware_ver, 64);
	
	sd_snprintf(cookie, 256, "TE=%s,%dx%d&%s&%s&%s&peerid=%s", os, x, y, ui_version, device_name, hardware_ver, peerid);
	sd_snprintf(_Tbuffer, MAX_URL_LEN, "GET /xl_server_info HTTP/1.1\r\nHost: %s:%u\r\nUser-Agent: Mozilla/4.0\r\nConnection: Keep-Alive\r\nAccept: */*\r\nCookie: %s\r\n\r\n\r\n",
		ip_addr, p_connect->_port, cookie);

	*len = sd_strlen(_Tbuffer);
	ret_val = sd_malloc((*len+1) , (void**)buffer);
	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("http_server_build_query_cmd, malloc failed.");
		CHECK_VALUE(ret_val);
	}
	sd_memset(*buffer, 0 , *len+1);
	sd_memcpy(*buffer, _Tbuffer, *len);

	return ret_val;
	
}

_int32 http_server_single_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	SINGLE_CONNECTION *p_connect = (SINGLE_CONNECTION*)user_data;
	_int32 ret = SUCCESS;
	char ip_addr[24] = {0};
	_u32 port = 0;

	LOG_DEBUG("http_server_single_send_callback errcode=%d...", errcode);

	sd_free((char*)buffer);
	buffer = NULL;
	if(errcode != SUCCESS)
	{
		//port++ and connect 
		if(p_connect->_port < g_search_info._end_port)
		{
			port = p_connect->_port + 1;
			sd_inet_ntoa(sd_htonl(p_connect->_ip), ip_addr, 24);
			ret = http_server_single_connect(ip_addr, port, g_search_info._find_server_callback);
			if(ret != SUCCESS)
			{
				socket_proxy_close(p_connect->_sockfd);
				g_search_info._used_fd--;
				sd_free(p_connect);	
				p_connect = NULL;
			}
			g_search_info._connected_num++;
		}
		
		socket_proxy_close(p_connect->_sockfd);
		//assert(g_search_info._used_fd != 0);
		g_search_info._used_fd--;
		sd_free(p_connect);		
		p_connect = NULL;
		return ret;
	}
	
	/*send success*/
	
	p_connect->_had_recv_len = 0;
	if(p_connect->_recv_buf == NULL)
	{
		ret = sd_malloc(1024, (void**)&p_connect->_recv_buf);
		p_connect->_had_recv_len = 1024;
		if(ret != SUCCESS)
		{	
			LOG_DEBUG("http_server_single_send_callback, malloc failed.");
			CHECK_VALUE(ret);
		}
		sd_memset(p_connect->_recv_buf, 0, 1024);
	}
	
	ret = socket_proxy_uncomplete_recv(p_connect->_sockfd, p_connect->_recv_buf, p_connect->_had_recv_len, http_server_single_recv_callback, (void*)p_connect);
	if(ret != SUCCESS)
	{
		socket_proxy_close(p_connect->_sockfd);
		sd_free(p_connect->_recv_buf);
		p_connect->_recv_buf = NULL;
		sd_free(p_connect);
		p_connect = NULL;
		g_search_info._used_fd--;		
	}
		
	return ret;
}

_int32 http_server_single_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	_int32 ret = SUCCESS;
	SERVER_INFO server;
	char* p_cookie = NULL;
	char* p_cookie_en = NULL;
	SINGLE_CONNECTION *p_connect = (SINGLE_CONNECTION*)user_data;
	_int32 i;
	BOOL is_notified = FALSE; /* 是否已经通知过界面 */

	if((errcode == SUCCESS) && (g_search_info._search_state == SEARCH_RUNNING))
	{
		sd_memset(&server, 0, sizeof(SERVER_INFO));
		server._type = ST_HTTP;
		server._ip = p_connect->_ip;
		server._port = p_connect->_port;
		
		p_cookie = sd_strstr(p_connect->_recv_buf, "Cookie:", 0);
		if(p_cookie != NULL)
		{
			p_cookie_en = sd_strstr(p_cookie, "\r\n", 0);
			if (p_cookie_en) {
				sd_memcpy(server._description, p_cookie, MIN(p_cookie_en - p_cookie, 255));
				server._description[255] = 0;				
			}
			/* 把该地址加入cache中 */
			for(i = 0; i < SEARCH_CACHE_ADDRESS_MAX_NUM; i++)
			{
				if(g_search_cache_address[i]._ip == server._ip)
				{
					is_notified = g_search_cache_address[i]._notified;
					if(!is_notified)
					{
						g_search_cache_address[i]._notified = TRUE;
					}
					break;
				}
				if(g_search_cache_address[i]._ip == 0)
				{
					g_search_cache_address[i]._ip = server._ip;
					g_search_cache_address[i]._port = server._port;
					g_search_cache_address[i]._notified = TRUE;
					break;
				}
			}
			if(!is_notified)
			{
				g_search_info._find_server_callback(&server);
			}
		}
#if defined(_DEBUG) && defined(MACOS)&&defined(MOBILE_PHONE)
		printf("============================%s\n", p_connect->_recv_buf);
#endif
	}

	socket_proxy_close(p_connect->_sockfd);
	sd_free(p_connect->_recv_buf);
	p_connect->_recv_buf = NULL;
	sd_free(p_connect);
	p_connect = NULL;
	//assert(g_search_info._used_fd != 0);
	g_search_info._used_fd--;
	
	if ((g_search_info._current_ip == g_search_info._end_ip) && (g_search_info._used_fd == 0) ) 
	{
		http_server_search_finished_callback();	
	}

	return ret;
}

_int32 http_server_search_finished_callback()
{
	_u32 time;
	sd_time_ms(&time);
	g_search_info._search_state = SEARCH_FINISHED;
	g_search_info._search_finished_callback(SUCCESS);
#if defined(_DEBUG) && defined(MACOS)&&defined(MOBILE_PHONE)
	printf("finish time:%u=================search finish!\n", time);
#endif
	return SUCCESS;
}



//////////////////////////////////////////////////////////////
/////17 手机助手相关的http server

#ifdef ENABLE_ASSISTANT	

/* 网下载库设置回调函数 */
_int32 assistant_set_callback_func_impl(ASSISTANT_INCOM_REQ_CALLBACK req_callback,ASSISTANT_SEND_RESP_CALLBACK resp_callback)
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("assistant_set_callback_func_impl");
	
	g_assistant_incom_req_callback = req_callback;
	g_assistant_send_resp_callback = resp_callback;
	
	return ret_val;
}


/* 发送响应 */
_int32 assistant_send_resp_file_impl(_u32 ma_id,const char * resp_file,void * user_data)
{
	_int32 ret_val = SUCCESS;
	_u64 file_size = 0;
	HTTP_SERVER_ACCEPT_SOCKET_DATA* data = (HTTP_SERVER_ACCEPT_SOCKET_DATA*)ma_id;

	LOG_DEBUG("assistant_send_resp_file_impl:ma_id=0x%X,gp_http_data=0x%X,resp_file=%s,g_http_server_timer_id=%u",ma_id,gp_http_data,resp_file,g_http_server_timer_id);

	if(gp_http_data!=data||g_http_server_timer_id==0)
	{
		return -1;
	}
	
	sd_assert(data->_post_type!=-1);

	cancel_timer(g_http_server_timer_id);
	g_http_server_timer_id = 0;

	sd_delete_file(g_local_file_path);

	sd_strncpy(g_local_file_path, resp_file, MAX_LONG_FULL_PATH_BUFFER_LEN-1);
	

	ret_val = http_server_get_file_size(data, &file_size);
	if(ret_val!=SUCCESS) goto ErrHandler;
	
	ret_val =  http_server_send_response_header( data,data->_task_id,0, file_size, 0);
	if(ret_val!=SUCCESS) goto ErrHandler;
	
	data->_post_type = 4;
	data->_user_data = user_data;
	return SUCCESS;
	
ErrHandler:
	LOG_ERROR("assistant_send_resp_file_impl .ret_val = %d", ret_val); 
	ret_val = http_server_response_header(503, data->_buffer, 0,0);
	sd_assert(ret_val==SUCCESS);
	g_current_task_id = 0;
	gp_http_data = NULL;

	g_local_file_path[0]='\0';
	
	data->_state = HTTP_SERVER_STATE_SENDING_ERR;
       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
	CHECK_VALUE(ret_val);
	return SUCCESS;
}


/* 取消异步操作 */
_int32 assistant_cancel_impl(_u32 ma_id)
{
	_int32 ret_val = SUCCESS;
	HTTP_SERVER_ACCEPT_SOCKET_DATA* data = (HTTP_SERVER_ACCEPT_SOCKET_DATA*)ma_id;

	LOG_DEBUG("assistant_cancel_impl:ma_id=0x%X,gp_http_data=0x%X,g_http_server_timer_id=%u,data->_post_type=%d",ma_id,gp_http_data,g_http_server_timer_id,data->_post_type);

	if(gp_http_data!=data)
	{
		return -1;
	}
	
	sd_assert(data->_post_type!=-1);
	
	if(data!=NULL&&data->_task_id==HTTP_LOCAL_FILE_TASK_ID&&data->_post_type>=3)
	{
		if(g_http_server_timer_id!=0)
		{
			cancel_timer(g_http_server_timer_id);
			g_http_server_timer_id = 0;
		}
		
		ret_val = http_server_response_header(503, data->_buffer, 0,0);
		sd_assert(ret_val==SUCCESS);
		g_current_task_id = 0;
		gp_http_data = NULL;

		g_local_file_path[0]='\0';
		
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
	       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
		CHECK_VALUE(ret_val);
	}
	
	return SUCCESS;
 }


/* 网下载库设置回调函数 */
_int32 assistant_set_callback_func(void * _param )
{
	TM_POST_PARA_2* p_param = (TM_POST_PARA_2*)_param;
	ASSISTANT_INCOM_REQ_CALLBACK req_callback = (ASSISTANT_INCOM_REQ_CALLBACK)p_param->_para1;
	ASSISTANT_SEND_RESP_CALLBACK resp_callback = (ASSISTANT_SEND_RESP_CALLBACK)p_param->_para2;

	p_param->_result = assistant_set_callback_func_impl( req_callback, resp_callback);
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}

/* 发送响应 */
_int32 assistant_send_resp_file(void * _param )
{
	TM_POST_PARA_3* p_param = (TM_POST_PARA_3*)_param;
	_u32 ma_id = (_u32)p_param->_para1;
	const char * resp_file = (const char *)p_param->_para2;
	void * user_data = (void *)p_param->_para3;

	p_param->_result = assistant_send_resp_file_impl( ma_id, resp_file, user_data);
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}

/* 取消异步操作 */
_int32 assistant_cancel(void * _param )
{
	TM_POST_PARA_1* p_param = (TM_POST_PARA_1*)_param;
	_u32 ma_id = (_u32)p_param->_para1;

	p_param->_result = assistant_cancel_impl( ma_id);
	p_param->_result = NOT_IMPLEMENT;
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}
#endif	/* ENABLE_ASSISTANT */

///////////////////////////////////////////////////////
_int32 http_server_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{	
	_int32 ret_val = SUCCESS;
	HTTP_SERVER_ACCEPT_SOCKET_DATA* data = gp_http_data;

	LOG_DEBUG("http_server_timeout:g_http_server_timer_id=%u,msgid=%u,errcode=%d,g_local_file_path=%s,g_current_task_id=0x%X,gp_http_data=0x%X",g_http_server_timer_id,msgid,errcode,g_local_file_path,g_current_task_id,gp_http_data);
	if(errcode == MSG_CANCELLED)
	{
		if(msgid==g_http_server_timer_id)
		{
			g_http_server_timer_id = 0;
		}
		return SUCCESS;
	}

	sd_assert(msgid==g_http_server_timer_id);
	if(msgid==g_http_server_timer_id)
	{
		g_http_server_timer_id = 0;
	}
	sd_delete_file(g_local_file_path);
	g_local_file_path[0] = '\0';
	g_current_task_id = 0;
	gp_http_data = NULL;
	
	ret_val =  http_server_response_header(503,  data->_buffer, 0,0);
	sd_assert(ret_val==SUCCESS);
	data->_state = HTTP_SERVER_STATE_SENDING_ERR;
       ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), http_server_handle_send,(void*)data);
	sd_assert(ret_val==SUCCESS);

	return SUCCESS;
}



///////////////////////////////////////////////////////
#ifdef ENABLE_HTTP_SERVER
static BOOL g_hs_inited = FALSE;
static HTTP_SERVER_MANAGER g_hs_mgr;

_int32 hs_handle_send( _int32 _errcode, _u32 notice_count_left,const char* buffer, _u32 had_send,void *user_data);
_int32 hs_handle_recv( _int32 _errcode, _u32 notice_count_left,const char* buffer, _u32 had_recv,void *user_data);
_int32 hs_create_dir_list_info(HS_AC* p_action);
_int32 init_http_server_module(_u16 * port)
{
    _int32 ret_val = SUCCESS;

	if(g_hs_inited) 
	{
		*port = (_u16)g_hs_mgr._port;
		return SUCCESS;
	}
	
	sd_memset(&g_hs_mgr,0x00,sizeof(HTTP_SERVER_MANAGER));
	list_init(&g_hs_mgr._path_list);
	map_init(&g_hs_mgr._actions,hs_id_comp);
	g_hs_mgr._accept_sock = INVALID_SOCKET;
	
	ret_val = http_server_start(port);
	if(ret_val==SUCCESS)
	{
		g_hs_mgr._state == HS_RUNNING;
		g_hs_mgr._port = *port;
	}
	return ret_val;
}
_int32 uninit_http_server_module(void)
{
    _int32 ret_val = SUCCESS;

	if(!g_hs_inited) 
	{
		return SUCCESS;
	}

	ret_val = http_server_stop();
	sd_assert(list_size(&g_hs_mgr._path_list)==0);
	sd_assert( map_size(&g_hs_mgr._actions)==0);
	sd_memset(&g_hs_mgr,0x00,sizeof(HTTP_SERVER_MANAGER));
	return ret_val;
}

_int32 hs_id_comp(void *E1, void *E2)
{
	_u32 id1 = (_u32)E1,id2 = (_u32)E2;
	
	if(id1==id2)
		return 0;
	else
	if(id1>id2)
		return 1;
	else
		return -1;
}


_int32 hs_ac_malloc(HS_AC ** pp_action)
{
	_int32 ret_val = SUCCESS;

	ret_val = sd_malloc(sizeof(HS_AC),(void**)pp_action);
	CHECK_VALUE(ret_val);
	sd_memset(*pp_action,0x00,sizeof(HS_AC));
	
	g_hs_mgr._action_id_num++;

	(*pp_action)->_action._action_id = g_hs_mgr._action_id_num;
	(*pp_action)->_data._action_ptr = (void*)(*pp_action);
	return SUCCESS;
}
_int32 hs_ac_free(HS_AC * p_action)
{
	SAFE_DELETE(p_action);
	return SUCCESS;
}
_int32 hs_ac_close(HS_AC * p_action)
{
	_int32 ret_val = SUCCESS,socket_err = SUCCESS;
	_u32 op_count = 0;
	LOG_DEBUG("hs_ac_close:sock=%u,fetch_file_pos=%llu,fetch_file_length=%llu,buffer_len=%u,task_id=%u,state=%d,close_socket=%d" , p_action->_data._sock,p_action->_data._fetch_file_pos, p_action->_data._fetch_file_length, p_action->_data._buffer_len ,p_action->_data._task_id, p_action->_data._state);
	ret_val = hs_remove_action_from_map(p_action->_action._action_id);
	sd_assert(ret_val==SUCCESS);

	socket_err = get_socket_error(p_action->_data._sock);
	ret_val = socket_proxy_peek_op_count(p_action->_data._sock, DEVICE_SOCKET_TCP,&op_count);
	sd_assert(ret_val==SUCCESS);
				
	LOG_DEBUG("hs_ac_close:sock=%u,socket_err=%d,op_ret=%d,op_count=%u" , p_action->_data._sock,socket_err,ret_val,op_count);
	if(op_count!=0)
	{
		ret_val = socket_proxy_cancel(p_action->_data._sock,DEVICE_SOCKET_TCP);
		return SUCCESS;
	}
	
	ret_val = socket_proxy_close(p_action->_data._sock);
	CHECK_VALUE(ret_val);

	if( 0 != p_action->_data._file_index )
	{
		sd_close_ex(p_action->_data._file_index);
		p_action->_data._file_index = 0;
	}

	if(g_hs_mgr._http_server_callback_function!=NULL&&p_action->_data._need_cb)
	{
		p_action->_action._ac_size = sizeof(HS_AC_LITE);
		p_action->_action._type = HST_RESP_SENT;
		g_hs_mgr._http_server_callback_function(&p_action->_action);
	}

	return hs_ac_free(p_action);
}

_int32 hs_add_action_to_map(HS_AC * p_action)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
	
	info_map_node._key = (void*)(p_action->_action._action_id);
	info_map_node._value = (void*)p_action;

	ret_val = map_insert_node( &g_hs_mgr._actions, &info_map_node);
		
	return ret_val;
}

HS_AC * hs_get_action_from_map(_u32 action_id)
{
	_int32 ret_val = SUCCESS;
	HS_AC * hs_ac = NULL;

	ret_val = map_find_node(&g_hs_mgr._actions, (void *)(action_id), (void **)&hs_ac);
	sd_assert(ret_val==SUCCESS);

	return hs_ac;
}

_int32 hs_remove_action_from_map(_u32 action_id)//HS_AC * p_action)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR map_node = NULL;
	HS_AC *hs_ac_in_map = NULL;
	
	ret_val = map_find_iterator(&g_hs_mgr._actions, action_id, &map_node);
	if( map_node != NULL && map_node != MAP_END(g_hs_mgr._actions))
	{
        	hs_ac_in_map = (HS_AC *)MAP_VALUE(map_node);
		map_erase_iterator(&g_hs_mgr._actions, map_node);
	}

	return ret_val;
}

_int32 hs_clear_action_map(void)
{
	MAP_ITERATOR  cur_node = NULL; 
	HS_AC * hs_ac_in_map = NULL;

	LOG_DEBUG("hs_clear_action_map:%u",map_size(&g_hs_mgr._actions)); 
	  
    cur_node = MAP_BEGIN(g_hs_mgr._actions);
    while(cur_node != MAP_END(g_hs_mgr._actions))
    {
    	hs_ac_in_map = (HS_AC *)MAP_VALUE(cur_node);
		map_erase_iterator(&g_hs_mgr._actions, cur_node);
	    cur_node = MAP_BEGIN(g_hs_mgr._actions);
	}

	return SUCCESS;

}
////////////////////////////////////////////////////
_int32 hs_path_malloc(HS_PATH ** pp_path)
{
	_int32 ret_val = SUCCESS;

	ret_val = sd_malloc(sizeof(HS_PATH),(void**)pp_path);
	CHECK_VALUE(ret_val);
	sd_memset(*pp_path,0x00,sizeof(HS_PATH));
	
	return SUCCESS;
}
_int32 hs_path_free(HS_PATH * p_path)
{
	SAFE_DELETE(p_path);
	return SUCCESS;
}

_int32 hs_add_path_to_list(HS_PATH * p_path)
{
	_int32 ret_val = SUCCESS;
	ret_val = list_push(&g_hs_mgr._path_list,( void * )p_path);
	CHECK_VALUE(ret_val);

	return ret_val;
}

/*解析传入的全路径，获取出虚拟路径(目录),并从列表中找出该虚拟路径对应的HS_PATH。
virtual_path: 传入的是全路径,eg:/httpserver/hx/1.txt
设置路径时只支持一级目录，所以只解析第一层目录进行匹配
*/
HS_PATH * hs_get_path_from_list(char * virtual_path)
{
	pLIST_NODE cur_node = NULL;
	HS_PATH * hs_path = NULL;
	char virtual_dir[MAX_LONG_FULL_PATH_BUFFER_LEN]={0};
	char * pos = sd_strchr(virtual_path,  0x2F, 1);
	
    LOG_DEBUG("hs_get_path_from_list"); 
	if( list_size(&g_hs_mgr._path_list) == 0 )	
		return NULL;

	//sd_assert(pos!=NULL);
	if(pos==NULL)
	{
		virtual_dir[0] = 0x2F;
	}
	else
	{
		sd_strncpy(virtual_dir, virtual_path, pos-virtual_path+1);
	}
	cur_node = LIST_BEGIN(g_hs_mgr._path_list);
	while(cur_node != LIST_END(g_hs_mgr._path_list))
	{
		hs_path = (HS_PATH * )LIST_VALUE(cur_node);
		if( 0 == sd_strcmp(virtual_dir, hs_path->_virtual_path))
		{
			return hs_path;
		}
		cur_node = LIST_NEXT(cur_node);
	}
	return NULL;
}

_int32 hs_remove_path_from_list(HS_PATH * p_path)
{
	pLIST_NODE cur_node = NULL;
	HS_PATH * hs_path_in_list = NULL;
	
    LOG_DEBUG("hs_remove_path_from_list"); 
	if( list_size(&g_hs_mgr._path_list) == 0 )	
	{
		CHECK_VALUE( HS_ACTION_NOT_FOUND);
	}
	cur_node = LIST_BEGIN(g_hs_mgr._path_list);
	while(cur_node != LIST_END(g_hs_mgr._path_list))
	{
		hs_path_in_list = (HS_PATH * )LIST_VALUE(cur_node);
		if(p_path == hs_path_in_list)
		{
			list_erase(&g_hs_mgr._path_list, cur_node);
			return SUCCESS;
		}
		cur_node = LIST_NEXT(cur_node);
	}
	return HS_ACTION_NOT_FOUND;
}

_int32 hs_clear_path_list(void)
{
	_int32 ret_val = SUCCESS;
	pLIST_NODE cur_node = NULL;
	pLIST_NODE next_node = NULL ;
	HS_PATH * hs_path = NULL;
	
    LOG_DEBUG("hs_clear_path_list"); 
	if(list_size(&g_hs_mgr._path_list) == 0)	
		return SUCCESS;

	cur_node = LIST_BEGIN(g_hs_mgr._path_list);
	while(cur_node != LIST_END(g_hs_mgr._path_list))
	{
		next_node = LIST_NEXT(cur_node);
		hs_path = (HS_PATH * )LIST_VALUE(cur_node);

		hs_remove_path_from_list(hs_path);
		cur_node = next_node;
	}

	list_clear(&g_hs_mgr._path_list);
	return SUCCESS;
}

_int32 hs_restart_server(void)
{
	_int32 ret_val = SUCCESS;
	
	return SUCCESS;
}
const char * hs_get_resp_status_header(_int32 err_code)
{
	const char * p_header = NULL;
	switch(err_code)
	{
		case 0:
		case HS_ERR_200_OK:
			p_header = "HTTP/1.1 200 OK\r\n";
			break;
		case HS_ERR_206_PARTIAL_CONTENT:
			p_header = "HTTP/1.1 206 Partial Content\r\n";
			break;
		case HS_ERR_304_NOT_MODIFIED:
			p_header = "HTTP/1.1 304 Not Modified\r\n";
			break;
		case HS_ERR_400_BAD_REQUEST:
			p_header = "HTTP/1.1 400 Bad Request\r\n";
			break;
		case HS_ERR_401_UNAUTHORIZED:
			p_header = "HTTP/1.1 401 Unauthorized\r\n";
			break;
		case HS_ERR_403_FORBIDDEN:
			p_header = "HTTP/1.1 403 Forbidden\r\n";
			break;
		case HS_ERR_404_NOT_FOUND:
			p_header = "HTTP/1.1 404 Not Found\r\n";
			break;
		case HS_ERR_405_METHOD_NOT_ALLOWED:
			p_header = "HTTP/1.1 405 Method Not Allowed\r\n";
			break;
		case HS_ERR_406_NOT_ACCEPTABLE:
			p_header = "HTTP/1.1 406 Not Acceptable\r\n";
			break;
		case HS_ERR_408_REQUEST_TIME_OUT:
			p_header = "HTTP/1.1 408 Request Time-Out\r\n";
			break;
		case HS_ERR_409_CONFLICT:
			p_header = "HTTP/1.1 409 Conflict\r\n";
			break;
		case HS_ERR_410_GONE:
			p_header = "HTTP/1.1 410 Gone\r\n";
			break;
		case HS_ERR_411_LENGTH_REQUIRED:
			p_header = "HTTP/1.1 411 Length Required\r\n";
			break;
		case HS_ERR_413_REQUEST_ENTITY_TOO_LARGE:
			p_header = "HTTP/1.1 413 Request Entity Too Large\r\n";
			break;
		case HS_ERR_414_REQUEST_URI_TOO_LARGE:
			p_header = "HTTP/1.1 414 Request-URL Too Large\r\n";
			break;
		case HS_ERR_415_UNSUPPORTED_MEDIA_TYPE:
			p_header = "HTTP/1.1 415 Unsupported Media Type\r\n";
			break;
		case HS_ERR_416_REQUESTED_RANGE_NOT_SATISFIABLE:
			p_header = "HTTP/1.1 416 Requested range not satisfiable\r\n";
			break;
		case HS_ERR_417_EXPECTATION_FAILED:
			p_header = "HTTP/1.1 417 Expectation Failed\r\n";
			break;
		case HS_ERR_500_INTERNAL_SERVER_ERROR:
			p_header = "HTTP/1.1 500 Server Error\r\n";
			break;
		case HS_ERR_501_NOT_IMPLEMENTED:
			p_header = "HTTP/1.1 501 Not Implemented\r\n";
			break;
		case HS_ERR_502_BAD_GETEWAY:
			p_header = "HTTP/1.1 502 Bad Gateway\r\n";
			break;
		case HS_ERR_503_SERVICE_UNAVAILABLE:
			p_header = "HTTP/1.1 503 Out of Resources\r\n";
			break;
		case HS_ERR_504_GATEWAY_TIME_OUT:
			p_header = "HTTP/1.1 504 Gateway Time-Out\r\n";
			break;
		case HS_ERR_505_HTTP_VERSION_NOT_SUPPORTED:
			p_header = "HTTP/1.1 505 HTTP Version not supported\r\n";
			break;
		default:
			p_header = "HTTP/1.1 500 Server Error\r\n";
			break;
	}
	return p_header;
}
_int32 hs_handle_http_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	_int32 ret_val = SUCCESS;
	char*  ptr_crlf = NULL;

	HS_AC* p_action = (HS_AC*)user_data;
	HS_AC_DATA * data = &p_action->_data;
	
	LOG_DEBUG("handle_http_recv_callback..., sock=%d,errcode=%d,had_recv=%u", p_action->_data._sock,errcode,had_recv);
	if(errcode != SUCCESS||p_action->_data._is_canceled)
	{
		ret_val = hs_ac_close(p_action);
		sd_assert(ret_val == SUCCESS);
		return SUCCESS;
	}
	//接收成功了
	sd_assert(had_recv > 0);
	data->_buffer_offset += had_recv;
	/*wait for \r\n\r\n here*/
	ptr_crlf = sd_strstr(data->_buffer, "\r\n\r\n", 0);

	if(NULL == ptr_crlf)
	{
		ret_val =  socket_proxy_uncomplete_recv(data->_sock, data->_buffer + data->_buffer_offset, data->_buffer_len - data->_buffer_offset, hs_handle_http_recv_callback, p_action);
		sd_assert(ret_val == SUCCESS);
		if(ret_val != SUCCESS)
		{
			ret_val = hs_ac_close(p_action);
			sd_assert(ret_val == SUCCESS);
		}
		return SUCCESS;
	}

	ret_val = hs_handle(p_action, ptr_crlf+4-data->_buffer);
	sd_assert(ret_val == SUCCESS);
	return SUCCESS;
}

_int32 hs_handle_http_accept_callback(_int32 errcode, _u32 pending_op_count, SOCKET conn_sock, void* user_data)
{
	_int32 ret_val = SUCCESS;
	HS_AC *	p_action = NULL;
       LOG_DEBUG("hs_handle_http_accept_callback errcode = %d, conn_sock=%d", errcode, conn_sock); 
	if(errcode == MSG_CANCELLED)
	{
		if(pending_op_count == 0)
		{
			ret_val =  socket_proxy_close(g_hs_mgr._accept_sock);
			g_hs_mgr._accept_sock = INVALID_SOCKET;
		}
		return SUCCESS;
	}
	
	if(errcode == SUCCESS)
	{	//accept success

		ret_val = hs_ac_malloc(&p_action) ;
		if(ret_val!=SUCCESS)
		{
			sd_assert(FALSE);
			socket_proxy_close(conn_sock);
			goto ErrHandler;
		}
		
       	LOG_DEBUG("hs_handle_http_accept_callback malloc HS_AC=0x%X", p_action); 
		p_action->_data._sock = conn_sock;
		p_action->_data._buffer_len = SEND_MOVIE_BUFFER_LENGTH_LOOG;
		p_action->_data._buffer_offset = 0;
		sd_time_ms(&p_action->_data._fetch_time_ms);
		
		ret_val = socket_proxy_uncomplete_recv(p_action->_data._sock, p_action->_data._buffer, RECV_HTTP_HEAD_BUFFER_LEN, hs_handle_http_recv_callback, p_action);
		if(ret_val!=SUCCESS)
		{
			sd_assert(FALSE);
			socket_proxy_close(conn_sock);
			hs_ac_free(p_action);
			goto ErrHandler;
		}
		
		ret_val = hs_add_action_to_map(p_action);
		if(ret_val!=SUCCESS)
		{
			sd_assert(FALSE);
			socket_proxy_close(conn_sock);
			hs_ac_free(p_action);
			goto ErrHandler;
		}
	}
	else
	{
		sd_assert(FALSE);
	}
	
ErrHandler:
	if(g_hs_mgr._accept_sock != INVALID_SOCKET)
    {
		ret_val =  socket_proxy_accept(g_hs_mgr._accept_sock, hs_handle_http_accept_callback, NULL);
		//sd_assert(ret_val);
		if(ret_val != SUCCESS)
		{
			ret_val = hs_restart_server();
			sd_assert(ret_val);
		}
	}
    return SUCCESS;
}
_int32 http_server_start(_u16 * port)
{
	/*create tcp accept*/
	SD_SOCKADDR	addr;
	_int32 ret = socket_proxy_create(&g_hs_mgr._accept_sock, SD_SOCK_STREAM);

	LOG_DEBUG("http_server_start...");
	
	CHECK_VALUE(ret);
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ANY_ADDRESS;		/*any address*/
	addr._sin_port = sd_htons(*port);
	ret = socket_proxy_bind(g_hs_mgr._accept_sock, &addr);
	if(ret != SUCCESS)
	{
	        LOG_DEBUG("http_server_start...socket_proxy_bind ret=%d", ret);
		socket_proxy_close(g_hs_mgr._accept_sock);
		g_hs_mgr._accept_sock = INVALID_SOCKET;
		return ret;
	}
	*port = sd_ntohs(addr._sin_port);
	LOG_DEBUG("http_server_start bind port %d...", *port);
	ret = socket_proxy_listen(g_hs_mgr._accept_sock, *port);
	if(ret != SUCCESS)
	{
	        LOG_DEBUG("http_server_start...socket_proxy_listen ret=%d", ret);
		socket_proxy_close(g_hs_mgr._accept_sock);
		g_hs_mgr._accept_sock = INVALID_SOCKET;
		return ret;
	}
	ret = socket_proxy_accept(g_hs_mgr._accept_sock, hs_handle_http_accept_callback, NULL);
	if(ret != SUCCESS)
	{
	        LOG_DEBUG("http_server_start...socket_proxy_accept ret=%d", ret);
		socket_proxy_close(g_hs_mgr._accept_sock);
		g_hs_mgr._accept_sock = INVALID_SOCKET;
	}

	return ret;
}

_int32 http_server_stop()
{
	_u32 count = 0;
	_int32 ret = SUCCESS;
	LOG_DEBUG("http_server_stop...");
	if(g_hs_mgr._accept_sock == INVALID_SOCKET)
		return SUCCESS;
	ret = socket_proxy_peek_op_count(g_hs_mgr._accept_sock, DEVICE_SOCKET_TCP, &count);
	if(SUCCESS != ret )
	{
	  LOG_DEBUG("http_server_stop. http_server is not running, ret = %d", ret);
	  g_hs_mgr._accept_sock = INVALID_SOCKET;
	   return ret;
	}
	//sd_assert(count == 1);
	if(count > 0)
	{
		ret =  socket_proxy_cancel(g_hs_mgr._accept_sock, DEVICE_SOCKET_TCP);
	}
	else
	{
		ret  =  socket_proxy_close(g_hs_mgr._accept_sock);
	       g_hs_mgr._accept_sock = INVALID_SOCKET;
	}
	return ret;

}


_int32 http_server_add_account( const  char * name,const char * password )
{
	_int32 ret = SUCCESS;
	if( (sd_strlen(name) < MAX_USER_NAME_LEN) && (sd_strlen(password) < MAX_PASSWORD_LEN) )
	{
		sd_strncpy(g_hs_mgr._account._user_name, name, sd_strlen(name));
		sd_strncpy(g_hs_mgr._account._password, password, sd_strlen(password));
	}
	else
		ret = HS_BEYOND_MAX_LEN;
	return ret;
}

_int32 http_server_add_path(const  char * real_path,const char * virtual_path,BOOL need_auth)
{
	_int32 ret_val = SUCCESS;
	HS_PATH * p_hs_path = NULL;
	char * pos = NULL;
	
	if((sd_strlen(real_path) !=0)&& (sd_strlen(real_path) < MAX_LONG_FULL_PATH_BUFFER_LEN) &&(sd_strlen(virtual_path) !=0)&& (sd_strlen(virtual_path) < MAX_LONG_FULL_PATH_BUFFER_LEN) )
	{
		ret_val = hs_path_malloc(&p_hs_path);
		CHECK_VALUE(ret_val);
		sd_strncpy(p_hs_path->_real_path, real_path, sd_strlen(real_path));
		if(p_hs_path->_real_path[sd_strlen(p_hs_path->_real_path) - 1] != DIR_SPLIT_CHAR)
		{
			/* 一定要以'/' 结尾 */
			p_hs_path->_real_path[sd_strlen(p_hs_path->_real_path)] =DIR_SPLIT_CHAR;
		}

		if(virtual_path[0] != 0x2F)
		{
			/* 一定要以'/' 开头 */
			p_hs_path->_virtual_path[0] =  0x2F;
		}
		sd_strcat(p_hs_path->_virtual_path, virtual_path, sd_strlen(virtual_path));
		pos = sd_strchr(p_hs_path->_virtual_path,  0x2F,1);
		if((pos!=NULL)&&(sd_strlen(pos)!=1))
		{
			hs_path_free(p_hs_path);
			/* 只接受一级目录 */
			return HS_INVALID_PATH;
		}
		if(p_hs_path->_virtual_path[sd_strlen(p_hs_path->_virtual_path)-1]!= 0x2F)
		{
			/* 一定要以'/' 结尾 */
			p_hs_path->_virtual_path[sd_strlen(p_hs_path->_virtual_path)] = 0x2F;
		}

		p_hs_path->_need_auth = need_auth;
		ret_val = hs_add_path_to_list(p_hs_path);
		sd_assert(ret_val==SUCCESS);
		if(ret_val!=SUCCESS)
		{
			hs_path_free(p_hs_path);
			CHECK_VALUE(ret_val);
		}
	}
	else
		ret_val = HS_BEYOND_MAX_LEN;
	return ret_val;
}

_int32 http_server_get_path_list(const char * virtual_path,char ** path_array_buffer,_u32 * path_num)
{
	_int32 ret_val = SUCCESS;
	FILE_ATTRIB *dir_list = NULL;
	_u32 dir_num = 0;
	char real_path[HS_MAX_FILE_PATH_LEN] = {0};
	HS_PATH *path = NULL;
	path = hs_get_path_from_list(virtual_path);
	
	if( NULL == path )
		return HS_FILE_PATH_ERROR;
	else
		sd_strncpy(real_path, path->_real_path, sd_strlen(path->_real_path));
	if( NULL == path_array_buffer )
	{
		if( sd_dir_exist(real_path) )
		{	
			ret_val = sd_get_sub_files(real_path, NULL, 0, &path_num);
			CHECK_VALUE(ret_val);
		}
		else
			return HS_DIR_NOT_EXIST;
	}
	else
	{
		if( sd_dir_exist(real_path) )
		{	
			if( 0 != *path_num)
			{
				ret_val = sd_get_sub_files(real_path, NULL, 0, &dir_num);
				CHECK_VALUE(ret_val);
				// 请求的数目大于实际目录的文件数
				if( *path_num > dir_num )
					*path_num = dir_num;
				ret_val = sd_malloc( (*path_num) * sizeof(FILE_ATTRIB), &dir_list);
				CHECK_VALUE(ret_val);
				sd_memset(dir_list, 0, (*path_num) * sizeof(FILE_ATTRIB));
				ret_val = sd_get_sub_files(real_path, dir_list, *path_num, &dir_num);
				CHECK_VALUE(ret_val);
				
				for(int i = 0; i < dir_num; i++)
				{
					sd_strncpy(path_array_buffer[i], dir_list[i]._name, sd_strlen(dir_list[i]._name));
				}
			}
		}
		else
		{
			ret_val = HS_DIR_NOT_EXIST;
		}
	}
	return ret_val;
}

_int32 http_server_set_callback_func(HTTP_SERVER_CALLBACK callback)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("http_server_set_callback_func");
	
	g_hs_mgr._http_server_callback_function = callback;

	return ret;
}

_int32 http_server_send_resp(HS_AC_LITE* p_param)
{
	_int32 ret_val = SUCCESS;
	_u64 recvlen = 0;
	char *tmp_pos = NULL;

	//找到之前的data
	HS_AC *p_action = hs_get_action_from_map(p_param->_action_id);
	if( NULL == p_action)
		return HS_ACTION_NOT_FOUND;
	HS_AC_DATA* data = &(p_action->_data);
	
	switch(p_param->_type)
	{
		case HST_RESP_SERVER_INFO:
		{
			sd_memset(data->_buffer, 0, sizeof(data->_buffer));
			HS_RESP_SERVER_INFO *resp = (HS_RESP_SERVER_INFO *)p_param;
			hs_response_header((HS_AC_LITE *)resp, data->_buffer, data->_range_from, data->_fetch_file_length,data->_file_size,TRUE,FALSE,0,resp->_server_info,NULL);
			data->_state = HTTP_SERVER_STATE_SENDING_HEADER;
			p_action->_action._user_data = resp->_ac._user_data;
			ret_val = socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), hs_handle_send, (void*)p_action);
			break;
		}
		case HST_RESP_BIND:
		{
			sd_memset(data->_buffer, 0, sizeof(data->_buffer));
			HS_RESP_BIND *resp = (HS_RESP_BIND *)p_param;
			hs_response_header((HS_AC_LITE *)resp, data->_buffer, data->_range_from, data->_fetch_file_length,data->_file_size,TRUE,FALSE,0,NULL,NULL);
			data->_state = HTTP_SERVER_STATE_SENDING_HEADER;
			p_action->_action._user_data = resp->_ac._user_data;
			ret_val = socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), hs_handle_send, (void*)p_action);
			break;
		}
		case HST_RESP_GET_FILE:
		{
			BOOL is_dir = FALSE;
			sd_memset(data->_buffer, 0, sizeof(data->_buffer));
			// 处理获取文件列表请求
			if( 0x2F == data->_ui_resp_file_path[sd_strlen(data->_ui_resp_file_path) - 1])
			{
				is_dir = TRUE;
				ret_val = hs_create_dir_list_info(p_action);
				if(ret_val != SUCCESS)
				{
					p_action->_action._result = ret_val; 
					goto ErrHandler;
				}
			}
			HS_RESP_GET_FILE *resp = (HS_RESP_GET_FILE *)p_param;
			hs_response_header((HS_AC_LITE *)resp, data->_buffer, data->_range_from, data->_fetch_file_length,data->_file_size,is_dir,FALSE,0,NULL,NULL);
			data->_state = HTTP_SERVER_STATE_SENDING_HEADER;
			p_action->_action._user_data = resp->_ac._user_data;
	       	ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), hs_handle_send,(void*)data->_action_ptr);
			break;
		}
		case HST_RESP_POST_FILE:
		{
			BOOL is_dir = FALSE;
			HS_RESP_POST_FILE *resp = (HS_RESP_POST_FILE *)p_param;
			if( resp->_ac._result == 0 || resp->_ac._result == 200)
			{
				// 处理创建文件夹请求
				tmp_pos = data->_ui_resp_file_path + sd_strlen(data->_req_path._real_path);
				if( (0 != sd_strlen(tmp_pos)) && (0x2F  == data->_ui_resp_file_path[sd_strlen(data->_ui_resp_file_path) - 1]))
				{
					if( !sd_dir_exist(data->_ui_resp_file_path) )
					{	
						is_dir = TRUE;
						ret_val = sd_mkdir(data->_ui_resp_file_path);
						if(ret_val != SUCCESS)
						{
							p_action->_action._result = ret_val; 
							resp->_ac._result = HS_ERR_417_EXPECTATION_FAILED;
						}
					}
					else
						resp->_ac._result = HS_ERR_409_CONFLICT;
				}
				// 处理POST请求头部后携带数据的情况
				else if( data->_buffer_offset > 0 )
				{
					recvlen = (_u64)data->_buffer_offset;
					data->_fetch_file_length -= recvlen;
					ret_val = hs_write_file(p_action, data->_range_from + data->_fetch_file_pos, recvlen, data->_buffer);
					sd_assert(ret_val==SUCCESS);
					if(ret_val != SUCCESS)
					{
						p_action->_action._result = ret_val; 
						resp->_ac._result = HS_ERR_500_INTERNAL_SERVER_ERROR;
					}
					else
						return SUCCESS;
				}
				// 继续接收数据
				else
				{
					sd_memset(data->_buffer, 0, sizeof(data->_buffer));
					data->_state = HTTP_SERVER_STATE_RECVING_BODY;
					data->_buffer_len = RECV_HTTP_HEAD_BUFFER_LEN;
					ret_val =  socket_proxy_recv(data->_sock, data->_buffer, data->_buffer_len, hs_handle_recv,(void*)data->_action_ptr);
					sd_assert(ret_val==SUCCESS);
					if(ret_val != SUCCESS) 	goto ErrHandler;
					return SUCCESS;
				}
			}
			// 发送给客户端的信息
			sd_memset(data->_buffer, 0, sizeof(data->_buffer));
			hs_response_header((HS_AC_LITE *)resp, data->_buffer, data->_range_from, data->_fetch_file_length,data->_file_size,is_dir,FALSE,0,NULL,NULL);
			data->_state = HTTP_SERVER_STATE_SENDING_HEADER;
			p_action->_action._user_data = resp->_ac._user_data;
			data->_fetch_file_length = 0;
		    ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), hs_handle_send,(void*)data->_action_ptr);

			break;
		}
		default:
			break;
	}
	sd_assert(ret_val==SUCCESS);
	if(ret_val != SUCCESS) 	goto ErrHandler;
	return SUCCESS;
	
ErrHandler:
	 ret_val = hs_end_of_send(p_action);
	 sd_assert(ret_val == SUCCESS);
	return ret_val;

}

_int32 http_server_cancel(_u32 action_id)
{
	_int32 ret = SUCCESS;
	return ret;
}

_int32 http_server_add_account_handle( void * _param )
{
	TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	char *username = (char *)_p_param->_para1;
	char *password = (char *)_p_param->_para2;
	
  	_p_param->_result = http_server_add_account(username, password);

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 http_server_add_path_handle( void *_param )
{
	TM_POST_PARA_3* _p_param = (TM_POST_PARA_3*)_param;
	char *real_path = (char *)_p_param->_para1;
	char *virtual_path = (char *)_p_param->_para2;
	BOOL need_auth = (BOOL)_p_param->_para3;
	
  	_p_param->_result = http_server_add_path(real_path, virtual_path, need_auth);

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 http_server_get_path_list_handle( void *_param )
{
	TM_POST_PARA_3* _p_param = (TM_POST_PARA_3*)_param;
	char *virtual_path = (char *)_p_param->_para1;
	char **path_array_buffer = (char **)_p_param->_para2;
	_u32 path_num = (_u32)_p_param->_para3;

  	_p_param->_result = http_server_get_path_list(virtual_path, path_array_buffer, path_num);
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 http_server_set_callback_func_handle( void *_param )
{
	TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)_param;
	HTTP_SERVER_CALLBACK callback = (HTTP_SERVER_CALLBACK)_p_param->_para1;

  	_p_param->_result = http_server_set_callback_func(callback);

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 http_server_send_resp_handle( void *_param )
{
	TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)_param;
	HS_AC_LITE* fb_hs_ac = (HS_AC_LITE *)_p_param->_para1;

  	_p_param->_result = http_server_send_resp(fb_hs_ac);

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 http_server_cancel_handle( void *_param )
{
	TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)_param;
	_u32 action_id = (_u32)_p_param->_para1;

  	_p_param->_result = http_server_cancel(action_id);

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 hs_create_dir_list_info(HS_AC* p_action)
{
	_int32 ret_val = SUCCESS;
	FILE_ATTRIB *dir_list = NULL;
	_u32 dir_num = 0;
	_u32 tmp_num = 0;
	_u32 file_id = 0;
	char tmp_buf[HS_MAX_FILE_NAME_LEN] = {0};
	char xml_buf[SEND_MOVIE_BUFFER_LENGTH_LOOG] = {0};
	char folder_end[] = "<folder>";
	char xml_path[HS_MAX_FILE_PATH_LEN] = {0};
	char xml_name[] = "dir_list.xml";
	_u32 writesize = 0;
	_u32 xml_data_size = 0;

	sd_snprintf(xml_buf, 128, "<folder name=\"%s\" >\r\n", p_action->_data._req_path._virtual_path);
	if( sd_dir_exist(p_action->_data._ui_resp_file_path) )
	{	
		ret_val = sd_get_sub_files(p_action->_data._ui_resp_file_path, NULL, 0, &dir_num);
		CHECK_VALUE(ret_val);
		if( 0 != dir_num)
		{
			ret_val = sd_malloc(dir_num * sizeof(FILE_ATTRIB), &dir_list);
			CHECK_VALUE(ret_val);
			sd_memset(dir_list, 0, dir_num * sizeof(FILE_ATTRIB));
			ret_val = sd_get_sub_files(p_action->_data._ui_resp_file_path, dir_list, dir_num, &tmp_num);
			CHECK_VALUE(ret_val);
			// 组装xml文件
			sd_snprintf(xml_path, sizeof(xml_path), "%s%s", p_action->_data._ui_resp_file_path, xml_name);
			if( sd_file_exist(xml_path) )
			{
				ret_val = sd_open_ex(xml_path, O_FS_WRONLY, &file_id);
				if(ret_val != SUCCESS) 
					goto ErrHandler;
			}
			else
			{
				ret_val = sd_open_ex(xml_path, O_FS_CREATE, &file_id);
				if(ret_val != SUCCESS) 
					goto ErrHandler;
			}
			// 拼凑文件列表的xml格式并写入文件
			for(int i = 0; i < dir_num; i++)
			{
				sd_memset(tmp_buf, 0, sizeof(tmp_buf));
				if( dir_list[i]._is_dir )
					sd_snprintf(tmp_buf, sizeof(tmp_buf), "  <folder name=\"%s\" ></folder>\r\n", dir_list[i]._name);		
				else
					sd_snprintf(tmp_buf, sizeof(tmp_buf), "  <file name=\"%s\"></file>\r\n", dir_list[i]._name);
				// 超过缓冲区限制，先部分写入文件
				if( sd_strlen(xml_buf) + sd_strlen(tmp_buf) > sizeof(xml_buf))
				{
					ret_val = sd_pwrite(file_id, xml_buf, sd_strlen(xml_buf), 0, &writesize);
					if(ret_val != SUCCESS) 
						goto ErrHandler;
					sd_memset(xml_buf, 0, sizeof(xml_buf));
					xml_data_size += writesize;
				}
				else
					sd_strcat(xml_buf, tmp_buf, sd_strlen(tmp_buf));
			}
			sd_strcat(xml_buf, folder_end, sd_strlen(folder_end));
			// 写入文件
			ret_val = sd_pwrite(file_id, xml_buf, sd_strlen(xml_buf), 0, &writesize);
			if(ret_val != SUCCESS) 
				goto ErrHandler;
			if( sd_strlen(xml_buf) != writesize)
				ret_val = HS_WRITE_FILE_WRONG;
			xml_data_size += writesize;
			// 更新将要发出的文件信息
			sd_memset(p_action->_data._ui_resp_file_path, 0, sizeof(p_action->_data._ui_resp_file_path));
			sd_strncpy(p_action->_data._ui_resp_file_path, xml_path, sd_strlen(xml_path));
			p_action->_data._fetch_file_pos = 0;
			p_action->_data._range_from = 0;
			p_action->_data._range_to = 0;
			p_action->_data._fetch_file_length = xml_data_size;
			p_action->_data._file_size = xml_data_size;
		}
	}
	else
	{
		ret_val = HS_DIR_NOT_EXIST;
		return ret_val;
	}
	
ErrHandler:

	SAFE_DELETE(dir_list);
	if( file_id != 0)
	{
		sd_close_ex(file_id);
		file_id = 0;
	}
	return ret_val;
}

_int32 hs_end_of_send(HS_AC * p_action)
{
	p_action->_data._need_cb = TRUE;
	return hs_ac_close(p_action);
}

_int32 hs_end_of_recv(HS_AC * p_action)
{
	_int32 ret_val = SUCCESS;

	HS_AC_DATA* data = (HS_AC_DATA*)&p_action->_data;
	//p_action->_action._type = HST_RESP_POST_FILE;
	hs_response_header(&p_action->_action, data->_buffer, data->_range_from, data->_fetch_file_length,data->_file_size,TRUE,FALSE,0,NULL,NULL);
	data->_state = HTTP_SERVER_STATE_SENDING_HEADER;
	data->_fetch_file_length = 0;
	ret_val =  socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), hs_handle_send,(void*)data->_action_ptr);
	sd_assert(ret_val == SUCCESS);
	if(ret_val != SUCCESS)
	{
		p_action->_action._result = ret_val; 
	}
	return ret_val;
}
/*
功能: 同步读取文件数据
参数:
in: data:操作数据结构体
	start_pos: 起始读取文件的位置
	request_len: 请求读取文件数据的大小
out:  read_buf: 存放读取文件的数据
*/
_int32 hs_read_file(HS_AC_DATA* data, _u64 start_pos, _u64 request_len, char *read_buf)
{
	_int32 ret_val = SUCCESS;
	_u32 readsize = 0;  //实际读写的字节数

	LOG_DEBUG("hs_sync_read_file:task_id=%u,file_index=%u,start_pos=%llu,len=%llu",data->_task_id,data->_file_index,start_pos,request_len);

	if( 0 == data->_file_index )
	{
		if( sd_file_exist(data->_ui_resp_file_path) )
		{
			ret_val = sd_open_ex(data->_ui_resp_file_path, O_FS_RDONLY, (_u32 *)&data->_file_index);
			CHECK_VALUE(ret_val);
		}
		else
		{
			CHECK_VALUE(HS_FILE_NO_EXIST);
		}
	}

	ret_val = sd_pread((_u32 )data->_file_index, read_buf, request_len, start_pos, &readsize);
	LOG_DEBUG("hs_sync_read_file:ret_val=%d,readsize=%u",ret_val,readsize);
	if(ret_val != SUCCESS) 	goto ErrHandler;

	//判断实际读写的字节数和请求的字节数是否一致
	sd_assert(readsize == request_len);
	if(readsize != request_len) 
	{
		ret_val = HS_READ_FILE_WRONG;
		goto ErrHandler;
	}
	ret_val = socket_proxy_send(data->_sock, read_buf, readsize, hs_handle_send,(void*)data->_action_ptr);
	sd_assert(ret_val==SUCCESS);
	if(ret_val != SUCCESS) 	goto ErrHandler;
	
	return SUCCESS;
	
ErrHandler:
	if( 0 != data->_file_index )
	{
		sd_close_ex(data->_file_index);
		data->_file_index = 0;
	}
	return ret_val;
}

_int32 hs_handle_send( _int32 _errcode, _u32 notice_count_left,const char* buffer, _u32 had_send,void *user_data)
{
	_int32 ret = SUCCESS;
	_u64  request_length = 0; //请求发送的长度
	HS_AC * p_action = (HS_AC*)user_data;
	HS_AC_DATA* data = (HS_AC_DATA*)&p_action->_data;
	
	LOG_DEBUG("http_server_handle_send...%d bytes, _errcode=%d, sock=%d,state=%d", had_send, _errcode, data->_sock,data->_state);

	if(data->_state == HTTP_SERVER_STATE_SENDING_ERR)
	{
		/* 出错信息已经发送,接下来应该断开链接 */
		if(p_action->_data._need_cb)
		{
			/* 需要通知界面 */
			ret = hs_end_of_send(p_action);
			sd_assert(ret==SUCCESS);
		}
		else
		{
			/* 不需要通知界面 */
			ret = hs_ac_close(p_action);
			sd_assert(ret==SUCCESS);
		}
		return SUCCESS;
	}

	if(_errcode != SUCCESS || had_send == 0 || (data->_is_canceled))
	{
		if((_errcode!=SUCCESS)&&(p_action->_action._result != _errcode))
		{
			p_action->_action._result = _errcode;
		}
		
		if(p_action->_action._result==SUCCESS)
		{
			p_action->_action._result = -1;
		}
		goto ErrHandler;
	}

	/* 发送成功 */
	sd_memset(data->_buffer, 0, sizeof(data->_buffer));
	if(HTTP_SERVER_STATE_SENDING_HEADER == data->_state)
	{
		if( 0 == data->_fetch_file_length )
		{
			goto ErrHandler;
		}
		data->_buffer_offset = 0;
		data->_buffer_len = SEND_MOVIE_BUFFER_LENGTH_LOOG;//hs_get_buffer_len(data->_fetch_file_pos, data->_fetch_file_length);
		data->_state = HTTP_SERVER_STATE_SENDING_BODY;
	    request_length = MIN(data->_fetch_file_length, (_u64)data->_buffer_len);
		LOG_DEBUG("http_server_handle_send HTTP_SERVER_STATE_SENDING_HEADER, vdm_vod_async_read_file, pos = %llu, length= %llu", data->_fetch_file_pos,data->_fetch_file_length);

		if(request_length==0)
		{
			goto ErrHandler;
		}
		
		if( data->_req_path._need_auth )
		{
			//验证
			
		}
		ret = hs_read_file(data, data->_range_from + data->_fetch_file_pos, request_length, data->_buffer);
		sd_assert(ret==SUCCESS);
		if(ret!=SUCCESS)
		{
			 p_action->_action._result = ret; 
			goto ErrHandler;
		}
	}
	else if(HTTP_SERVER_STATE_SENDING_BODY == data->_state)
	{
		sd_assert(had_send <= data->_fetch_file_length);
    	had_send = MIN(had_send, data->_fetch_file_length);
		if((had_send>0)&&(g_hs_mgr._http_server_callback_function!=NULL))
		{
			HS_NOTIFY_TRANSFER_FILE_PROGRESS transfer;
			sd_memset(&transfer, 0, sizeof(HS_NOTIFY_TRANSFER_FILE_PROGRESS));
			transfer._ac._ac_size= sizeof(HS_NOTIFY_TRANSFER_FILE_PROGRESS);
			transfer._ac._type = HST_NOTIFY_TRANSFER_FILE_PROGRESS;
			transfer._ac._action_id = p_action->_action._action_id;
			transfer._ac._user_data = p_action->_action._user_data;
			transfer._ac._result = 0;
			transfer._file_size = data->_file_size;
			transfer._range_size = data->_range_to-data->_range_from;
			transfer._transfer_size = data->_fetch_file_pos+had_send;
			/* 通知界面发送进度 */
			g_hs_mgr._http_server_callback_function((HS_AC_LITE*)(&transfer));
		}
		
		data->_buffer_offset = 0;
		data->_fetch_file_pos += had_send; 
		data->_fetch_file_length  -= had_send;
		data->_buffer_len = SEND_MOVIE_BUFFER_LENGTH_LOOG;
		data->_state = HTTP_SERVER_STATE_SENDING_BODY;
		sd_time_ms(&data->_fetch_time_ms);
	    request_length = MIN(data->_fetch_file_length, (_u64)data->_buffer_len);
		if(request_length > 0)
		{
			LOG_DEBUG("hs_handle_send HTTP_SERVER_STATE_SENDING_BODY, vdm_vod_async_read_file, pos = %llu, length= %llu", data->_fetch_file_pos,data->_fetch_file_length);
			ret = hs_read_file(data,data->_range_from + data->_fetch_file_pos, request_length, data->_buffer);
			sd_assert(ret==SUCCESS);
			if(ret!=SUCCESS)
			{
				 p_action->_action._result = ret; 
				goto ErrHandler;
			}
		}
		else
		{
			LOG_DEBUG("hs_handle_send request to file end, then stop");
			goto ErrHandler;
		}
		
	}
	return SUCCESS;
	
ErrHandler:
	//sd_assert(p_action->_action._result!=SUCCESS);
	 ret = hs_end_of_send(p_action);
	 sd_assert(ret==SUCCESS);
	 return SUCCESS;
}


/*
功能: 同步读取文件数据
参数:
in: data:操作数据结构体
	start_pos: 起始读取文件的位置
	recv_len: 请求写入文件数据的大小
  	write_buf: 需要写入文件的数据缓冲区
*/
_int32 hs_write_file(HS_AC* p_action, _u64 start_pos, _u64 recv_len, char *write_buf)
{
	_int32 ret_val = SUCCESS;
	_u32 writesize = 0;  //实际读写的字节数
	HS_AC_DATA* data = &(p_action->_data);
	LOG_DEBUG("hs_sync_read_file:task_id=%u,file_index=%u,start_pos=%llu,len=%llu",data->_task_id,data->_file_index,start_pos,recv_len);

	//第一次写入文件时的处理
	if( 0 == data->_file_index )
	{
		if( sd_file_exist(data->_ui_resp_file_path) )
		{
			ret_val = sd_open_ex(data->_ui_resp_file_path, O_FS_WRONLY, (_u32 *)&data->_file_index);
			if(ret_val != SUCCESS) 
				goto ErrHandler;
		}
		else
		{
			ret_val = sd_open_ex(data->_ui_resp_file_path, O_FS_CREATE, (_u32 *)&data->_file_index);
			if(ret_val != SUCCESS) 
				goto ErrHandler;
		}
	}
	ret_val = sd_pwrite((_u32 )data->_file_index, write_buf, recv_len, start_pos, &writesize);
	LOG_DEBUG("hs_sync_read_file:ret_val=%d,readsize=%u", ret_val, writesize);
	if(ret_val != SUCCESS) 
		goto ErrHandler;

	sd_assert(writesize == recv_len);
	if(writesize != recv_len) 
	{
		ret_val = HS_WRITE_FILE_WRONG;
		goto ErrHandler;
	}
	
	data->_fetch_file_pos += recv_len; 
	data->_fetch_file_length  -= recv_len;
	sd_time_ms(&data->_fetch_time_ms);
	//  如果请求发送的数据已经收完，则结束接收
	if( 0 == data->_fetch_file_length )
	{	
		ret_val = hs_end_of_recv(p_action);
	 	sd_assert(ret_val==SUCCESS);
		return ret_val;
	}
	// 如果请求发送的数据没有接收完，则继续接收
	sd_memset(data->_buffer, 0, sizeof(data->_buffer));
	data->_buffer_len = RECV_HTTP_HEAD_BUFFER_LEN;
	data->_state = HTTP_SERVER_STATE_RECVING_BODY;
	ret_val = socket_proxy_recv(data->_sock, data->_buffer, data->_buffer_len, hs_handle_recv,(void*)data->_action_ptr);
	sd_assert(ret_val==SUCCESS);
	if(ret_val != SUCCESS) 	goto ErrHandler;
	
	return SUCCESS;
	
ErrHandler:
	CHECK_VALUE(ret_val);
	if( 0 != data->_file_index )
	{
		sd_close_ex(data->_file_index);
		data->_file_index = 0;
	}
	return ret_val;
}

_int32 hs_handle_recv( _int32 _errcode, _u32 notice_count_left,const char* buffer, _u32 had_recv,void *user_data)
{
	_int32 ret_val = SUCCESS;
	_u64  recv_length = 0; //请求接收的长度
	HS_AC * p_action = (HS_AC*)user_data;
	HS_AC_DATA* data = (HS_AC_DATA*)&p_action->_data;
	
	LOG_DEBUG("hs_handle_recv...%d bytes, _errcode=%d, sock=%d,state=%d", had_recv, _errcode, data->_sock,data->_state);

	if(_errcode != SUCCESS || had_recv == 0 || (data->_is_canceled))
	{
		if((_errcode != SUCCESS) && (p_action->_action._result != _errcode))
			p_action->_action._result = _errcode;
		if(p_action->_action._result == SUCCESS)
			p_action->_action._result = -1;
		goto ErrHandler;
	}

	if(HTTP_SERVER_STATE_RECVING_BODY == data->_state)
	{
		//sd_assert(had_recv <= data->_fetch_file_length);
    	had_recv = MIN(had_recv, data->_fetch_file_length);
		if((had_recv > 0)&&(g_hs_mgr._http_server_callback_function!=NULL))
		{
			/* 通知界面发送进度 */
			HS_NOTIFY_TRANSFER_FILE_PROGRESS transfer;
			sd_memset(&transfer, 0, sizeof(HS_NOTIFY_TRANSFER_FILE_PROGRESS));
			transfer._ac._ac_size= sizeof(HS_NOTIFY_TRANSFER_FILE_PROGRESS);
			transfer._ac._type = HST_NOTIFY_TRANSFER_FILE_PROGRESS;
			transfer._ac._action_id = p_action->_action._action_id;
			transfer._ac._user_data = p_action->_action._user_data;
			transfer._ac._result = 0;
			transfer._file_size = data->_file_size;
			transfer._range_size = data->_range_to - data->_range_from;
			transfer._transfer_size = data->_fetch_file_pos + had_recv;
			g_hs_mgr._http_server_callback_function((HS_AC_LITE*)(&transfer));
		}

		ret_val = hs_write_file(p_action, data->_range_from + data->_fetch_file_pos, had_recv, data->_buffer);
		sd_assert(ret_val == SUCCESS);
		if(ret_val != SUCCESS)
		{
			p_action->_action._result = HS_ERR_500_INTERNAL_SERVER_ERROR; 
			goto ErrHandler;
		}
	}
	return SUCCESS;
ErrHandler:
	ret_val = hs_end_of_recv(p_action);
	sd_assert(ret_val==SUCCESS);
	return SUCCESS;
}

_int32 hs_time_u32_to_str(_u32 unix_time,char * str)
{
	sd_strncpy(str, "Fri, 29 Aug 2008 09:41:59 GMT",sd_strlen("Fri, 29 Aug 2008 09:41:59 GMT"));
	return SUCCESS;
}
/*
功能: 响应不同客户端请求的头部信息封装
参数:
in: param: 基本动作结构体
	start_pos: 文件读取位置(响应获取文件时传入，其余响应填0)
	content_length: 内容体将要发送的内容长度
out: 	header_buffer: 存放封装后的头部信息
*/
_int32 hs_response_header(HS_AC_LITE* param, char* header_buffer, _u64 start_pos, _u64 content_length, _u64 file_size,BOOL is_txt,BOOL is_gzip,_u32 last_modified,char * cookie,char * file_name)
{
	_int32 ret = SUCCESS;
	_u32 len = 0;
	//char _Tbuffer[MAX_URL_LEN];
/*
Date: Fri, 29 Aug 2008 09:41:59 GMT
Content-Length: 393216
Content-Type: audio/mpeg
Content-Range: bytes 1572864-1966079/6799326
Last-Modified: Tue, 29 Jul 2008 07:29:28 GMT
Content-Encoding: gzip
Content-Disposition: attachment; filename="56354028.mp3"
Set-Cookie: cdb_sid=8pupcs; expires=Fri, 05 Sep 2008 09:17:54 GMT; path=/

*/
	char Date[64] = "Date: ";
	char Content_Length[64] = "Content-Length: ";
	char Content_Type[32] = "Content-Type: ";
	
	char Content_Range[256] = "Content-Range: bytes ";
	char Last_Modified[64] = "Last-Modified: ";
	char Content_Encoding[] = "Content-Encoding: gzip";
	char Content_Disposition[512] = "Content-Disposition: attachment; filename=";
	char Set_Cookie[1024] = "Set-Cookie: ";
	
	char http_header_tail[] = "Server: fb_server/1.0\r\nAccept-Ranges: bytes\r\nConnection: close\r\n\r\n";
	const char * p_status_header = NULL;

	_u32 cur_time;
	sd_time(&cur_time);

	if(param->_result!=SUCCESS)
	{
		/* 出错信息 */
		p_status_header = hs_get_resp_status_header(param->_result);
	}
	else
	if(!is_txt)
	{
		sd_assert(content_length!=0);
		sd_assert(file_size!=0);
		sd_assert(start_pos+content_length<=file_size);
		if((start_pos!=0 )|| (start_pos+content_length!=file_size))
		{
			/* 分段传输 */
			p_status_header = hs_get_resp_status_header(HS_ERR_206_PARTIAL_CONTENT);
		}
		else
		{
			p_status_header = hs_get_resp_status_header(HS_ERR_200_OK);
		}
	}
	else
	{
		p_status_header = hs_get_resp_status_header(HS_ERR_200_OK);
	}
	
	len = sd_strlen(Date);
	hs_time_u32_to_str(cur_time,Date+len);
	
	len = sd_strlen(Content_Length);
	sd_snprintf(Content_Length+len,64-len-1,"%llu\r\n", content_length);

	if(is_txt||(param->_result!=SUCCESS))
	{
		sd_strcat(Content_Type, "text/html\r\n", sd_strlen("text/html\r\n"));
	}
	else
	{
		sd_strcat(Content_Type, "application/octet-stream\r\n", sd_strlen("application/octet-stream\r\n"));
	}

	sd_snprintf(header_buffer,MAX_URL_LEN,"%s%s%s%s", p_status_header,Date,Content_Length,Content_Type);

	
	if((param->_result==SUCCESS)&&(!is_txt)&&((start_pos!=0 )|| (start_pos+content_length!=file_size)))
	{
		len = sd_strlen(Content_Range);
		sd_snprintf(Content_Range+len,256-len-1,"%llu-%llu/%llu\r\n", start_pos,start_pos+content_length-1,file_size);
		sd_strcat(header_buffer, Content_Range, sd_strlen(Content_Range));
	}

	if(last_modified!=0)
	{
		len = sd_strlen(Last_Modified);
		hs_time_u32_to_str(last_modified,Last_Modified+len);
		sd_strcat(header_buffer, Last_Modified, sd_strlen(Last_Modified));
	}

	if(is_gzip)
	{
		sd_strcat(header_buffer, Content_Encoding, sd_strlen(Content_Encoding));
	}

	if((param->_result==SUCCESS)&&(!is_txt)&&(file_name!=NULL)&& (sd_strlen(file_name)!=0))
	{
		len = sd_strlen(Content_Disposition);
		sd_assert(sd_strlen(file_name)<400);
		sd_snprintf(Content_Disposition+len,512-len-1,"\"%s\"\r\n",file_name);
		sd_strcat(header_buffer, Content_Disposition, sd_strlen(Content_Disposition));
	}

	if((cookie!=NULL)&& (sd_strlen(cookie)!=0))
	{
		len = sd_strlen(Set_Cookie);
		sd_assert(sd_strlen(cookie)<1000);
		if(sd_strncmp(cookie, "Set-Cookie: ", sd_strlen("Set-Cookie: "))==0)
		{
			sd_strncpy(Set_Cookie, cookie, 1023);
		}
		else
		if(sd_strncmp(cookie, "Cookie: ", sd_strlen("Cookie: "))==0)
		{
			sd_strncpy(Set_Cookie+len, cookie+sd_strlen("Cookie: "), 1000);
		}
		else
		{
			sd_strcat(Set_Cookie, cookie, sd_strlen( cookie));
		}
		len = sd_strlen(Set_Cookie);
		if(Set_Cookie[len-1]!='\n')
		{
			sd_strcat(Set_Cookie, "\r\n", sd_strlen( "\r\n"));
		}
		sd_strcat(header_buffer, Set_Cookie, sd_strlen(Set_Cookie));
	}

	sd_strcat(header_buffer, http_header_tail, sd_strlen(http_header_tail));
	
	header_buffer[sd_strlen(header_buffer)] = '\0';
	
	return SUCCESS;
}


_int32 hs_handle_req_server_info(HS_AC* action, _u32 header_len)
{
	_int32 ret = SUCCESS;
	char * p_cookie = NULL;
	char * p_cookie_end = NULL;
	_int32 cookie_len = 0;
	char * fb_auth = NULL;

	HS_AC_DATA *data = &(action->_data);
	LOG_DEBUG("hs_handle_req_server_info,sock=%u. got a HTTP request[%u]:%s",data->_sock,header_len,data->_buffer);
	
	HS_INCOM_REQ_SERVER_INFO *req_server_info = NULL;
	ret = sd_malloc((_u32)sizeof(HS_INCOM_REQ_SERVER_INFO), (void **)&req_server_info );
	//CHECK_VALUE(ret);
	if(SUCCESS !=  ret)
	{
		LOG_DEBUG("sd_malloc. error ret=%d", ret);
		return HS_ERR_500_INTERNAL_SERVER_ERROR;
	}
	sd_memset(req_server_info, 0x00, sizeof(HS_INCOM_REQ_SERVER_INFO));
    LOG_DEBUG("hs_handle_req_server_info malloc HTTP_SERVER_ACCEPT_SOCKET_DATA=0x%X", req_server_info); 

	req_server_info->_ac._ac_size= sizeof(HS_INCOM_REQ_SERVER_INFO);
	req_server_info->_ac._type = HST_REQ_SERVER_INFO;
	req_server_info->_ac._action_id = action->_action._action_id;
	req_server_info->_ac._result = data->_errcode;
	
	*(data->_buffer+header_len) ='\0'; 
	p_cookie = sd_strstr(data->_buffer, "Cookie: ", 0);
	if( NULL != p_cookie )
	{
		p_cookie_end = sd_strstr(p_cookie, "\r\n", 0);
		if( NULL != p_cookie_end)
		{
			p_cookie += sd_strlen("Cookie: ");
			cookie_len = p_cookie_end - p_cookie;
			if( cookie_len > HS_MAX_HS_INFO_LEN)
				cookie_len = HS_MAX_HS_INFO_LEN;
			
			sd_strncpy(req_server_info->_client_info, p_cookie, cookie_len);
		}
		
		fb_auth = sd_strstr(p_cookie, "fb_auth=", 0);
		if( NULL != fb_auth )
		{
			if(fb_auth[sd_strlen("fb_auth=")]=='0')
			{
				g_hs_mgr._account._auth = FALSE;
			}
			else
			{
				g_hs_mgr._account._auth = TRUE;
			}
		}
	}
	
	//回调UI传入的回调函数
	if(g_hs_mgr._http_server_callback_function!=NULL)
	{
		g_hs_mgr._http_server_callback_function((HS_AC_LITE*)req_server_info);
	}

	sd_free(req_server_info);
	return SUCCESS;

}

_int32 hs_handle_req_bind(HS_AC* action, _u32 header_len)
{
	_int32 ret = SUCCESS;
	char * p_auth = NULL;
	char * p_auth_end = NULL;
	_int32 auth_len = 0;
	char base64[MAX_URL_LEN] = {0};
	char username[MAX_USER_NAME_LEN] = {0};
	char password[MAX_PASSWORD_LEN] = {0};
	
	HS_AC_DATA *data = &(action->_data);
	LOG_DEBUG("fb_handle_req_bind,sock=%u. got a HTTP request[%u]:%s",data->_sock,header_len,data->_buffer);
	
	HS_INCOM_REQ_BIND *req_bind = NULL;
	ret = sd_malloc((_u32)sizeof(HS_INCOM_REQ_BIND), (void **)&req_bind );
	if(SUCCESS !=  ret)
	{
		LOG_DEBUG("sd_malloc. error ret=%d", ret);
		return HS_ERR_500_INTERNAL_SERVER_ERROR;
	}
	sd_memset(req_bind, 0x00, sizeof(HS_INCOM_REQ_BIND));
    LOG_DEBUG("fb_handle_req_bind malloc HS_INCOM_REQ_BIND=0x%X", req_bind); 

	req_bind->_ac._ac_size= sizeof(HS_INCOM_REQ_BIND);
	req_bind->_ac._type = HST_REQ_BIND;
	req_bind->_ac._action_id = action->_action._action_id;
	req_bind->_ac._result = data->_errcode;
	
	*(data->_buffer+header_len) ='\0'; 
	p_auth = sd_strstr(data->_buffer, "Authorization: ", 0);
	if( NULL != p_auth )
	{
		p_auth_end = sd_strstr(p_auth, "\r\n", 0);
		if( NULL != p_auth_end)
		{
			p_auth += sd_strlen("Authorization: ");
			auth_len = p_auth_end - p_auth;
			if( auth_len > MAX_URL_LEN)
			{
				sd_free(req_bind);
				return HS_ERR_401_UNAUTHORIZED;
			}
			if( g_hs_mgr._account._auth )
			{
				//调用base64解密函数，解析出username:password
				
			}
			sd_strncpy(req_bind->_user_name, username, sd_strlen(username));
			sd_strncpy(req_bind->_password, password, sd_strlen(password));
			if( (0 != sd_strcmp(username, g_hs_mgr._account._user_name))  || (0 != sd_strcmp(password, g_hs_mgr._account._password))) 
				req_bind->_ac._result = HS_AUTHRIZATION_FAILURED;
		}
	}
	//回调UI传入的回调函数
	if(g_hs_mgr._http_server_callback_function!=NULL)
	{
		g_hs_mgr._http_server_callback_function((HS_AC_LITE*)req_bind);
	}

	sd_free(req_bind);
	return SUCCESS;
}

BOOL hs_check_account(HS_ACCOUNT * p_account)
{
	if(sd_strcmp(p_account->_user_name, g_hs_mgr._account._user_name)==0 && sd_strcmp(p_account->_password, g_hs_mgr._account._password)==0)
	{
		return TRUE;
	}
	return FALSE;
}
_int32 hs_get_account_from_request(char * request,HS_ACCOUNT * p_account)
{
	_int32 ret_val = SUCCESS;
	_u32 auth_len = 0;
	char base64_str[256] = {0};
	char decoded_str[256] = {0};
	char *p_auth_end = NULL;
	char * p_auth = sd_strstr(request, "Authorization: Basic ", 0);
	if( NULL != p_auth )
	{
		p_auth_end = sd_strstr(p_auth, "\r\n", 0);
		if( NULL != p_auth_end)
		{
			p_auth += sd_strlen("Authorization: Basic ");
			auth_len = p_auth_end - p_auth;
			sd_strncpy(base64_str,p_auth,auth_len);
			if( auth_len >= 256)
			{
				return HS_ERR_401_UNAUTHORIZED;
			}

			auth_len = 255;
			ret_val = sd_base64_decode(base64_str,(_u8*)decoded_str,&auth_len);
			if( ret_val !=SUCCESS)
			{
				return HS_ERR_401_UNAUTHORIZED;
			}
			p_auth_end = sd_strchr(decoded_str, ':', 0);
			if( p_auth_end==NULL )
			{
				return HS_ERR_401_UNAUTHORIZED;
			}

			auth_len = p_auth_end-decoded_str;
			if(auth_len>=MAX_USER_NAME_LEN)
			{
				return HS_ERR_401_UNAUTHORIZED;
			}
			sd_strncpy(p_account->_user_name, decoded_str, auth_len);
			p_auth_end++;
			if(sd_strlen(p_auth_end)>=MAX_PASSWORD_LEN)
			{
				return HS_ERR_401_UNAUTHORIZED;
			}
			sd_strncpy(p_account->_password, p_auth_end, sd_strlen(p_auth_end));
		}
		else
		{
			return HS_ERR_401_UNAUTHORIZED;
		}
	}
	return SUCCESS;
}
_int32 hs_parse_get_file_info(HS_AC* p_action,char* buffer, _u32 length, char *file_path, char* file_name, _u64* start_pos, _u64* end_pos)
{
	_int32 ret = SUCCESS;
	char * ptr_header;
	char * ptr_header_end;

	char * file_path_pos = NULL;
	char * file_path_pos_end = NULL;
	_int32 file_path_len = 0;
	char * file_name_pos = NULL;
	char file_virtual_full_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};
	char * file_real_full_path = p_action->_data._ui_resp_file_path;
	
	char * range_pos = NULL;
	char * range_pos_end = NULL;
	char *tmp_pos = NULL;
	char str_start_pos[HS_MAX_FILE_LEN_STRING] = {0};
	char str_end_pos[HS_MAX_FILE_LEN_STRING] = {0};
	_u32 tmp;
	HS_PATH * path = NULL;
	HS_ACCOUNT account = {0};
	
	*start_pos = 0;
	*end_pos = 0;
	
	ptr_header = buffer;
	ptr_header_end = sd_strstr( (char*)ptr_header,(const char*) "\r\n\r\n" ,0);
	LOG_DEBUG("hs_parse_get_file_info..");
	if(NULL == ptr_header_end || (ptr_header_end - ptr_header) > length )
	{
	    LOG_DEBUG("hs_parse_get_file_info..NULL == ptr_header_end || (ptr_header_end - ptr_header) > length ");
		return HS_ERR_400_BAD_REQUEST;
	}
 	// 获取文件路径
	file_path_pos = ptr_header + sd_strlen("GET ");
	if(NULL == file_path_pos)
	{
	    LOG_DEBUG("hs_parse_get_file_info..not found space after GET/ ");
		return HS_ERR_400_BAD_REQUEST;
	}
	file_path_pos_end = sd_strstr(file_path_pos, " ", 0);
	if(NULL == file_path_pos || NULL == file_path_pos_end)
	{
	    LOG_DEBUG("hs_parse_get_file_info..not found space after GET/ ");
		return HS_ERR_400_BAD_REQUEST;
	}
	file_path_len = file_path_pos_end-file_path_pos;
	if( file_path_len > MAX_FILE_PATH_LEN)
	{
		LOG_DEBUG("hs_parse_get_file_info..file_path is too long");
		return HS_ERR_414_REQUEST_URI_TOO_LARGE;
	}
	
	sd_strncpy(file_virtual_full_path, file_path_pos, file_path_len);
	sd_strncpy(file_path, file_path_pos, file_path_len);
	file_name_pos = sd_strrchr(file_path, 0x2F);
	if(file_name_pos==NULL)
	{
		LOG_DEBUG("hs_parse_get_file_info..file_name not found");
		return HS_ERR_403_FORBIDDEN;
	}

	file_name_pos++;
	if(sd_strlen(file_name_pos) != 0)
	{
		sd_strncpy(file_name, file_name_pos, sd_strlen(file_name_pos));
	}

	file_path_len = file_name_pos - file_path;
	file_path[file_path_len] = '\0'; 

	ret = hs_get_account_from_request(ptr_header,&account);
	CHECK_VALUE(ret);

	path = hs_get_path_from_list(file_path);
	if(path == NULL)
	{
		if(file_path_len==1)
		{
			/* 获取无真实路径的根目录列表 */
			sd_assert(sd_strlen(file_name)==0);
			if(sd_strlen(file_name)!=0)
			{
				return HS_ERR_404_NOT_FOUND;
			}

			if(!hs_check_account(&account))
			{
				return HS_ERR_403_FORBIDDEN;
			}
		}
		else
		{
			return HS_ERR_404_NOT_FOUND;
		}
	}
	else
	{
		if(path->_need_auth)
		{
			if(!hs_check_account(&account))
			{
				return HS_ERR_401_UNAUTHORIZED;
			}
		}
		
		sd_memcpy(&p_action->_data._req_path, path, sizeof(HS_PATH));
		tmp_pos = file_virtual_full_path+sd_strlen(path->_virtual_path);
		if(sd_strlen(tmp_pos)==0)
		{
			// 获取_virtual_path目录列表 
			sd_snprintf(file_real_full_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s",path->_real_path);
			sd_assert(sd_strlen(file_name)==0);
		}
		else
		{
			sd_snprintf(file_real_full_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s%s",path->_real_path,tmp_pos);
		}

		if(file_real_full_path[sd_strlen(file_real_full_path)-1]==0x2F)
		{
			// 获取目录列表 
			sd_snprintf(file_path, HS_MAX_FILE_PATH_LEN-1, "%s%s",path->_real_path,tmp_pos);
			sd_assert(sd_strlen(file_name)==0);
		}
		else
		{
			tmp_pos = sd_strrchr(file_real_full_path, 0x2F);
			sd_assert(tmp_pos!=NULL);
			sd_assert(tmp_pos-file_real_full_path+1<HS_MAX_FILE_PATH_LEN);
			sd_memset(file_path,0x00,HS_MAX_FILE_PATH_LEN);
			sd_strncpy(file_path, file_real_full_path, tmp_pos-file_real_full_path+1);
		}
	}

	sd_assert(sd_dir_exist(file_path)==TRUE);
	if(sd_dir_exist(file_path)!=TRUE)
	{
		return HS_ERR_404_NOT_FOUND;
	}
	
	//判断文件名是否为空
	if( sd_strlen(file_name)==0 )
	{
		*start_pos = 0;
		*end_pos = 0;
		return SUCCESS;
	}

	sd_assert(path!=NULL);
	if(path==NULL)
	{
		return HS_ERR_404_NOT_FOUND;
	}

	ret = sd_get_file_size_and_modified_time(file_real_full_path, &p_action->_data._file_size, &tmp);
	if(ret!=SUCCESS)
	{
		return HS_ERR_404_NOT_FOUND;
	}
	
	// 获取文件起始位置
	range_pos = sd_strstr(ptr_header, "Range: bytes=", 0);
	if( NULL != range_pos )
	{
		range_pos_end = sd_strstr( range_pos, "\r\n", 0);
		if( NULL != range_pos_end)
		{
			range_pos += sd_strlen("Range: bytes=");
			tmp_pos = sd_strchr(range_pos, '-', 0);
			sd_strncpy(str_start_pos, range_pos, tmp_pos - range_pos);
			sd_str_to_u64(str_start_pos, sd_strlen(str_start_pos), start_pos);

			tmp_pos += sd_strlen("-");
			sd_strncpy(str_end_pos, tmp_pos, range_pos_end - tmp_pos);
			sd_str_to_u64(str_end_pos, sd_strlen(str_end_pos), end_pos);
		}
	}
	//无Range字段则表示下载整个文件
	else 
	{
		*start_pos = 0;
		*end_pos = p_action->_data._file_size;
	}
	return SUCCESS;
}

/*
  处理客户端get文件的请求
*/
_int32 hs_handle_file_get(HS_AC* p_action, _u32 header_len)
{
	_int32 ret = SUCCESS;
	char * p_auth = NULL;
	char * p_auth_end = NULL;
	_int32 auth_len = 0;
	char * p_cookie = NULL;
	char * p_cookie_end = NULL;
	_int32 cookie_len = 0;
	char * p_gzip = NULL;
	char base64[MAX_URL_LEN] = {0};
	char username[MAX_USER_NAME_LEN] = {0};
	char password[MAX_PASSWORD_LEN] = {0};

	_u64 start_pos = 0;
	_u64 end_pos = 0;

	HS_AC_DATA * data = &(p_action->_data);	
	LOG_DEBUG("hs_handle_file_get,sock=%u. got a HTTP request[%u]:%s",data->_sock,header_len,data->_buffer);
	
	HS_INCOM_REQ_GET_FILE *req_get_file = NULL;
	ret = sd_malloc((_u32)sizeof(HS_INCOM_REQ_GET_FILE), (void **)&req_get_file );
	if(SUCCESS !=  ret)
	{
		LOG_DEBUG("sd_malloc. error ret=%d", ret);
		ret = HS_ERR_500_INTERNAL_SERVER_ERROR;
		return ret;
	}
	sd_memset(req_get_file, 0x00, sizeof(HS_INCOM_REQ_GET_FILE));
    LOG_DEBUG("hs_handle_file_get malloc HS_INCOM_REQ_GET_FILE=0x%X", req_get_file); 

	req_get_file->_ac._ac_size= sizeof(HS_INCOM_REQ_GET_FILE);
	req_get_file->_ac._type = HST_REQ_GET_FILE;
	req_get_file->_ac._action_id = p_action->_action._action_id;
	req_get_file->_ac._result = data->_errcode;

	*(data->_buffer+header_len) ='\0'; 
	//从包头中解析出文件路径和文件名称
	ret = hs_parse_get_file_info(p_action,data->_buffer, sd_strlen(data->_buffer), req_get_file->_file_path, req_get_file->_file_name, &start_pos, &end_pos);
	if( SUCCESS != ret)
	{
		sd_free(req_get_file);
		return ret;
	}
	// 填充发送给客户端的回调函数
	req_get_file->_range_from = start_pos;
	req_get_file->_range_to = end_pos;

	// 填充操作的结构体
	data->_fetch_file_pos = 0;
	data->_fetch_file_length = end_pos - start_pos;
	data->_range_from = start_pos;
	data->_range_to = end_pos;
	
	p_cookie = sd_strstr(data->_buffer, "Cookie: ", 0);
	if( NULL != p_cookie )
	{
		p_cookie_end = sd_strstr(p_cookie, "\r\n", 0);
		if( NULL != p_cookie_end)
		{
			p_cookie += sd_strlen("Cookie: ");
			cookie_len = p_cookie_end - p_cookie;
			if( cookie_len >= MAX_URL_LEN)
			{
				sd_free(req_get_file);
				return HS_ERR_400_BAD_REQUEST;
			}
			sd_strncpy(req_get_file->_cookie, p_cookie, cookie_len);
		}
	}
	p_gzip = sd_strstr(data->_buffer, "Content-Encoding: ", 0);
	if( NULL != p_gzip )
		req_get_file->_accept_gzip = TRUE;
	else
		req_get_file->_accept_gzip = FALSE;
	data->_is_gzip = req_get_file->_accept_gzip;

	//回调UI传入的回调函数
	if(g_hs_mgr._http_server_callback_function!=NULL)
	{
		g_hs_mgr._http_server_callback_function((HS_AC_LITE*)req_get_file);
	}

	sd_free(req_get_file);
	return SUCCESS;

}

_int32 hs_parse_post_file_info(HS_AC* p_action, _u32 length, char *file_path, char* file_name, _u64* start_pos, _u64* end_pos)
{
	_int32 ret = SUCCESS;
	char * ptr_header;
	char * ptr_header_end;
	char * file_path_pos = NULL;
	char * file_path_pos_end = NULL;
	_int32 file_path_len = 0;
	char * file_name_pos = NULL;
	char file_virtual_full_path[MAX_LONG_FULL_PATH_BUFFER_LEN] = {0};
	char * file_real_full_path = p_action->_data._ui_resp_file_path;
	char * range_pos = NULL;
	char * range_pos_end = NULL;
	char * tmp_pos = NULL;
	char str_start_pos[HS_MAX_FILE_LEN_STRING] = {0};
	char str_end_pos[HS_MAX_FILE_LEN_STRING] = {0};
	HS_PATH *path;
	
	*start_pos = 0;
	*end_pos = 0;

	ptr_header = p_action->_data._buffer;
	ptr_header_end = sd_strstr( (char*)ptr_header,(const char*) "\r\n\r\n" ,0);
	LOG_DEBUG("hs_parse_get_file_info..");
	if(NULL == ptr_header_end || ptr_header_end - ptr_header > length )
	{
	    LOG_DEBUG("hs_parse_get_file_info..NULL == ptr_header_end || (ptr_header_end - ptr_header) > length ");
		return HS_ERR_400_BAD_REQUEST;
	}

 	// 获取文件路径
	file_path_pos = ptr_header + sd_strlen("POST ");
	if(NULL == file_path_pos)
	{
	    LOG_DEBUG("hs_parse_get_file_info..not found space after GET/ ");
		return HS_ERR_400_BAD_REQUEST;
	}
	file_path_pos_end = sd_strstr(file_path_pos, " ", 0);
	if(NULL == file_path_pos_end)
	{
	    LOG_DEBUG("hs_parse_get_file_info..not found space after GET/ ");
		return HS_ERR_400_BAD_REQUEST;
	}
	file_path_len = file_path_pos_end - file_path_pos;
	if( file_path_len > MAX_FILE_PATH_LEN)
	{
		LOG_DEBUG("hs_parse_get_file_info..file_path is too long");
		return HS_ERR_414_REQUEST_URI_TOO_LARGE;
	}
	// 获取文件名称

	sd_strncpy(file_virtual_full_path, file_path_pos, file_path_len);
	sd_strncpy(file_path, file_path_pos, file_path_len);
	// 获取路径中最后一个斜杠位置
	file_name_pos = sd_strrchr(file_path, 0x2F);
	if(file_name_pos == NULL)
	{
		LOG_DEBUG("hs_parse_post_file_info..file_name not found");
		return HS_ERR_400_BAD_REQUEST;
	}
	//移动到最后一个斜杠位置后面
	file_name_pos++; 
	if(sd_strlen(file_name_pos) != 0)
	{
		sd_strncpy(file_name, file_name_pos, sd_strlen(file_name_pos));
	}
	else
	{
		
	}
	path = hs_get_path_from_list(file_path);
	if(path == NULL)
	{
		if(sd_strlen(file_name_pos) == 0)
		{
			// 获取无真实路径的根目录列表 
			sd_assert(sd_strlen(file_name)==0);
		}
		else
		{
			return HS_ERR_404_NOT_FOUND;
		}
	}
	else
	{
		sd_memcpy(&p_action->_data._req_path, path, sizeof(HS_PATH));
		tmp_pos = file_virtual_full_path + sd_strlen(path->_virtual_path);
		// 路径是以斜杠结束(tmp_pos指向路径最后一个斜杠位置后面)，表示创建文件夹请求
		if(sd_strlen(tmp_pos) == 0)
		{
			// 获取_virtual_path目录列表 
			sd_snprintf(file_real_full_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s",path->_real_path);
			sd_assert(sd_strlen(file_name)==0);
		}
		else
		{
			sd_snprintf(file_real_full_path, MAX_LONG_FULL_PATH_BUFFER_LEN-1, "%s%s",path->_real_path,tmp_pos);
		}
		// 获取file_path
		if(file_real_full_path[sd_strlen(file_real_full_path)-1]==0x2F)
		{
			sd_snprintf(file_path, HS_MAX_FILE_PATH_LEN-1, "%s%s",path->_real_path,tmp_pos);
			sd_assert(sd_strlen(file_name)==0);
		}
		else
		{
			tmp_pos = sd_strrchr(file_real_full_path, 0x2F);
			sd_assert(tmp_pos!=NULL);
			sd_assert(tmp_pos-file_real_full_path+1<HS_MAX_FILE_PATH_LEN);
			sd_memset(file_path,0x00,HS_MAX_FILE_PATH_LEN);
			sd_strncpy(file_path, file_real_full_path, tmp_pos-file_real_full_path+1);
		}
		
	}

	//判断文件名是否为空
	if( sd_strlen(file_name)==0 )
	{
		*start_pos = 0;
		*end_pos = 0;
		return SUCCESS;
	}

	sd_assert(path!=NULL);
	if(path==NULL)
	{
		return HS_ERR_404_NOT_FOUND;
	}
	// 获取文件起始位置
	range_pos = sd_strstr(ptr_header, "Range: bytes=", 0);
	if( NULL != range_pos )
	{
		range_pos_end = sd_strstr( range_pos, "\r\n", 0);
		if( NULL != range_pos_end)
		{
			range_pos += sd_strlen("Range: bytes=");
			tmp_pos = sd_strchr(range_pos, '-', 0);
			sd_strncpy(str_start_pos, range_pos, tmp_pos - range_pos);
			sd_str_to_u64(str_start_pos, sd_strlen(str_start_pos), start_pos);

			tmp_pos += sd_strlen("-");
			sd_strncpy(str_end_pos, tmp_pos, range_pos_end - tmp_pos);
			sd_str_to_u64(str_end_pos, sd_strlen(str_end_pos), end_pos);
		}
	}
	//无Range字段则表示下载整个文件
	else 
	{
		*start_pos = 0;
		*end_pos = 0;
	}
	return SUCCESS;
}


/*
  处理客户端post文件的请求
*/
_int32 hs_handle_file_post(HS_AC* p_action, _u32 header_len)
{
	_int32 ret = SUCCESS;
	char * p_size = NULL;
	char * p_size_end = NULL;
	_int32 size_len = 0;
	char file_size_str[HS_MAX_FILE_LEN_STRING] = {0};
	_u64 file_size = 0;
	char * p_auth = NULL;
	char * p_auth_end = NULL;
	_int32 auth_len = 0;
	char * p_cookie = NULL;
	char * p_cookie_end = NULL;
	_int32 cookie_len = 0;
	char * p_gzip = NULL;
	char base64[MAX_URL_LEN] = {0};
	char username[MAX_USER_NAME_LEN] = {0};
	char password[MAX_PASSWORD_LEN] = {0};

	_u64 start_pos = 0;
	_u64 end_pos = 0;
	char tmp_buf[SEND_MOVIE_BUFFER_LENGTH_LOOG] = {0};
	
	HS_AC_DATA * data = &p_action->_data;	
	LOG_DEBUG("hs_handle_file_post,sock=%u. got a HTTP request[%u]:%s",data->_sock,header_len,data->_buffer);
	
	HS_INCOM_REQ_POST_FILE *req_post_file = NULL;
	ret = sd_malloc((_u32)sizeof(HS_INCOM_REQ_POST_FILE), (void **)&req_post_file );
	if(SUCCESS != ret)
	{
		LOG_DEBUG("sd_malloc. error ret=%d", ret);
		return HS_ERR_500_INTERNAL_SERVER_ERROR;
	}
	sd_memset(req_post_file, 0x00, sizeof(HS_INCOM_REQ_POST_FILE));
    LOG_DEBUG("hs_handle_file_post malloc HS_INCOM_REQ_GET_FILE=0x%X", req_post_file); 

	req_post_file->_ac._ac_size = sizeof(HS_INCOM_REQ_POST_FILE);
	req_post_file->_ac._type = HST_REQ_POST_FILE;
	req_post_file->_ac._action_id = p_action->_action._action_id;
	req_post_file->_ac._result = data->_errcode;
	req_post_file->_ac._user_data = p_action->_action._user_data;

	//从包头中解析出文件路径和文件名称
	ret = hs_parse_post_file_info(p_action, header_len, req_post_file->_file_path, req_post_file->_file_name, &start_pos, &end_pos);
	if( SUCCESS != ret)
	{
		sd_free(req_post_file);
		return ret;
	}
	req_post_file->_range_from = start_pos;
	req_post_file->_range_to = end_pos;

	data->_fetch_file_pos = 0;
	data->_fetch_file_length = end_pos - start_pos;
	data->_file_size = end_pos - start_pos;
	data->_range_from = start_pos;
	data->_range_to = end_pos;
	
	// 解析文件上传的大小
	p_size = sd_strstr(data->_buffer, "Content-Length: ", 0);
	if( NULL != p_size )
	{
		p_size_end = sd_strstr(p_size, "\r\n", 0);
		if( NULL != p_size_end)
		{
			p_size += sd_strlen("Content-Length: ");
			size_len = p_size_end - p_size;
			if( size_len > MAX_URL_LEN)
			{
				sd_free(req_post_file);
				return HS_ERR_400_BAD_REQUEST;
			}
			
			sd_strncpy(file_size_str, p_size, size_len);
			sd_str_to_u64(file_size_str, sd_strlen(file_size_str), &file_size);
			req_post_file->_file_size = file_size;	
			data->_fetch_file_length = file_size;
			data->_file_size = file_size;
		}
		else
		{
			sd_free(req_post_file);
			return HS_ERR_400_BAD_REQUEST;
		}
	}
	else
	{
		sd_free(req_post_file);
		return HS_ERR_411_LENGTH_REQUIRED;
	}

	// 解析出cookie
	p_cookie = sd_strstr(data->_buffer, "Cookie: ", 0);
	if( NULL != p_cookie )
	{
		p_cookie_end = sd_strstr(p_cookie, "\r\n", 0);
		if( NULL != p_cookie_end)
		{
			p_cookie += sd_strlen("Cookie: ");
			cookie_len = p_cookie_end - p_cookie;
			if( cookie_len >= MAX_URL_LEN)
			{
				sd_free(req_post_file);
				return HS_ERR_400_BAD_REQUEST;
			}
			sd_strncpy(req_post_file->_cookie, p_cookie, cookie_len);
		}
	}
	// 解析文件是否已压缩
	p_gzip = sd_strstr(data->_buffer, "Content-Encoding: ", 0);
	if( NULL != p_gzip )
		req_post_file->_is_gzip= TRUE;
	else
		req_post_file->_is_gzip = FALSE;
	data->_is_gzip = req_post_file->_is_gzip;
	// 解析文件是否已加密和加密的key

	// post头部之后携带的数据处理
	if( data->_buffer_offset > header_len)
	{
		sd_memcpy(tmp_buf, data->_buffer + header_len, data->_buffer_offset - header_len);
		sd_memset(data->_buffer, 0, sizeof(data->_buffer));
		data->_buffer_offset -= header_len;
		if( data->_buffer_offset > data->_fetch_file_length )
			data->_buffer_offset = data->_fetch_file_length;
		sd_memcpy(data->_buffer, tmp_buf, data->_buffer_offset);
	}
	else if(data->_buffer_offset == header_len)
		data->_buffer_offset = 0;
	
ErrHandler:		
	//回调UI传入的回调函数
	if(g_hs_mgr._http_server_callback_function!=NULL)
	{
		g_hs_mgr._http_server_callback_function((HS_AC_LITE*)req_post_file);
	}

	sd_free(req_post_file);
	return SUCCESS;

}

_int32 hs_handle(HS_AC* action, _u32 header_len)
{
	_int32 ret = SUCCESS;
	HS_AC_DATA *data = &(action->_data);
	// 处理服务器配置请求
	if(sd_strncmp(data->_buffer, "GET /xl_server_info HTTP/1.1", sd_strlen("GET /xl_server_info HTTP/1.1"))==0)
	{
		ret = hs_handle_req_server_info(action, header_len);
	}
	// 处理绑定请求
	else if(sd_strncmp(data->_buffer, "GET /xl_server_info?cmd=bind", sd_strlen("GET /xl_server_info?cmd=bind"))==0)
	{
		ret = hs_handle_req_bind(action, header_len);
	}
	// 处理其他GET请求
	else if(sd_strncmp(data->_buffer, "GET /xl_server_info", sd_strlen("GET /xl_server_info"))==0)
	{
		//ret = fb_http_server_handle_unknown_get();
	}
	// 处理下载文件请求
	else if(sd_strncmp(data->_buffer, "GET /", sd_strlen("GET /")) == 0 )
	{
		ret = hs_handle_file_get(action, header_len);
	}
	// 处理其他POST请求
	else if(sd_strncmp(data->_buffer, "POST /xl_server_info", sd_strlen("POST /xl_server_info"))==0)
	{
		//ret = fb_http_server_handle_unknown_post();
	}
	// 处理上传文件请求
	else if(sd_strncmp(data->_buffer, "POST ", sd_strlen("POST ")) == 0 )
	{
		ret = hs_handle_file_post(action, header_len);
	}
	else
	{
		ret = HS_ERR_400_BAD_REQUEST;
	}

	if(ret!=SUCCESS)
	{
		action->_action._result = ret;
		sd_memset(data->_buffer, 0, sizeof(data->_buffer));
		data->_range_from = 0;
		data->_fetch_file_length = 0;
		data->_file_size = 0;
		data->_need_cb = FALSE; /* 处理请求的时候出错，不需要通知界面，直接拒绝客户端 */
		hs_response_header((HS_AC_LITE *)action, data->_buffer, data->_range_from, data->_fetch_file_length,data->_file_size,TRUE,FALSE,0,NULL,NULL);
		data->_state = HTTP_SERVER_STATE_SENDING_ERR;
		ret = socket_proxy_send(data->_sock, data->_buffer, sd_strlen(data->_buffer), hs_handle_send, (void*)action);
		sd_assert(ret==SUCCESS);
	}
	return SUCCESS;
}

#endif


