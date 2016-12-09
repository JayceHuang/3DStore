#if !defined(__HTTP_PROTOCOL_C_20090305)
#define __HTTP_PROTOCOL_C_20090305

#include "platform/sd_socket.h"
#include "utility/settings.h"
#include "utility/base64.h"
#include "utility/sd_iconv.h"
#include "platform/sd_network.h"
#include "utility/version.h"
#include "utility/peerid.h"

#if defined(MACOS)&&defined(MOBILE_PHONE)
#include "platform/ios_interface/sd_ios_device_info.h"
#endif /* MACOS && MOBILE_PHONE */
#include "utility/logid.h"
#define LOGID LOGID_HTTP_PIPE
#include "utility/logger.h"

#include "http_data_pipe.h"
#include "download_task/download_task.h"
#include "socket_proxy_interface.h"

_int32 http_create_request(char** request_buffer,_u32* request_buffer_len,char* full_path,char * ref_url,char * host,_u32 port,char* user_name,char* pass_words,char * cookies,_u64 rang_from,_u64 rang_to,BOOL is_browser,
	BOOL is_post,_u8 * post_data,_u32 post_data_len,_u32 * total_send_bytes,BOOL gzip,BOOL post_gzip,_u64 post_content_len)
{

	char request_header[MAX_FULL_PATH_BUFFER_LEN] = "GET ";
	char http_info[] = " HTTP/1.1\r\n";
	char host_line[MAX_HOST_NAME_LEN*2+256] = "Host: ";
	char authorization[256] ,usr_pas_base64[256];
	char range[256] = "Range: bytes=";
	char referer [MAX_URL_LEN+12]= "Referer: ";
	char request_header_tail[512] = "Connection: close\r\n";
	char _Tbuffer[64];
	char* request = *request_buffer,*cookie_header = NULL;
    char* p_tmp = NULL;
    BOOL is_form = FALSE;
	_int32 _full_path_len = 0,length = 0,buffer_len = *request_buffer_len,cookie_len = 0;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( " enter http_create_request:is_browser%d,is_post=%d,post_data_len=%u,gzip=%d,post_gzip=%d,post_content_len=%llu",is_browser,is_post,post_data_len,gzip,post_gzip,post_content_len);

	/*--------------- Get or POST----------------------*/
	if(is_post)
	{
		sd_snprintf(request_header,MAX_FULL_PATH_BUFFER_LEN,"%s","POST ");
	}

	/*---------------  full path----------------------*/
#if defined(MOBILE_PHONE)
	if((NT_GPRS_WAP & sd_get_net_type())/*||is_post*/)
	{
		sd_strcat(request_header, "http://",sd_strlen("http://")); 
		sd_strcat(request_header,host,sd_strlen(host)); 
		if(port  != HTTP_DEFAULT_PORT )  
		{
			sd_snprintf(_Tbuffer,50,":%u",port);
			sd_strcat(request_header,_Tbuffer,sd_strlen(_Tbuffer)); 
		}	
	}
#endif /* MOBILE_PHONE */
	
	_full_path_len = sd_strlen(full_path);
	if(_full_path_len>MAX_FULL_PATH_BUFFER_LEN)
		return HTTP_ERR_FULL_PATH_TOO_LONG;
	
	sd_strcat(request_header,full_path,_full_path_len);

#if defined(MOBILE_PHONE)
	/*--------------- X-Online-Host ----------------------*/
	if(NT_GPRS_WAP & sd_get_net_type())
	{
		sd_snprintf(host_line,MAX_HOST_NAME_LEN*2+256,"%s","X-Online-Host: ");
	}
#endif /* MOBILE_PHONE */

	/*--------------- host ----------------------*/
	sd_strcat(host_line,host,sd_strlen(host)); 
	if(port  != HTTP_DEFAULT_PORT )  
	{
		sd_snprintf(_Tbuffer,50,":%u",port);
		sd_strcat(host_line,_Tbuffer,sd_strlen(_Tbuffer)); 
	}	
	sd_strcat(host_line, "\r\n",sd_strlen("\r\n")); 
	
	/*--------------- Authorization ----------------------*/
	if((user_name!=NULL)&&(user_name[0]!='\0')&&(pass_words!=NULL)&&(pass_words[0]!='\0'))
	{
		sd_memset(authorization,0,256);
		sd_memset(usr_pas_base64,0,256);
		
		sd_snprintf(authorization,256,"%s:%s",user_name,pass_words);
		ret_val = sd_base64_encode((const _u8 *)authorization,sd_strlen(authorization),usr_pas_base64);
		CHECK_VALUE(ret_val);
		sd_memset(authorization,0,256);
		sd_snprintf(authorization,256,"Authorization: Basic %s",usr_pas_base64);
		sd_strcat(authorization, "\r\n",sd_strlen("\r\n")); 
		sd_strcat(host_line, authorization,sd_strlen(authorization)); 
	}
		
	/*--------------- Range ----------------------*/
	if(is_post)
	{
        //判断是否为 xx=xx&xx=xx& 格式的
        p_tmp = (char*)post_data;
        while (++p_tmp - (char*)post_data < post_data_len) 
        {
            if (*p_tmp == '=') {
                while (++p_tmp - (char*)post_data < post_data_len) 
                {
                    if (*p_tmp == '&') {
                        is_form = TRUE;
                        break;
                    }
                }
                break;
            }
        }
        if (is_form) 
        {
            sd_snprintf(range,256,"Content-Length: %llu\r\nContent-Type: application/x-www-form-urlencoded\r\n",post_content_len);            
        }
        else
        {
            sd_snprintf(range,256,"Content-Length: %llu\r\nContent-Type: application/octet-stream\r\n",post_content_len);            
        }
	}
	else
	{
		if(rang_from!=-1)
		{
			sd_snprintf(_Tbuffer,50,"%llu-",rang_from);
			sd_strcat(range, _Tbuffer,sd_strlen(_Tbuffer)); 

			if((rang_to!=0)&&(rang_to!=-1))
			{
				sd_snprintf(_Tbuffer,50,"%llu",rang_to-1);
				sd_strcat(range, _Tbuffer,sd_strlen(_Tbuffer)); 
			}
			sd_strcat(range, "\r\n",sd_strlen("\r\n")); 		
		}
		else
		{
			range[0]='\0';
		}
	}	
	/*--------------- referer url ----------------------*/
	if((ref_url!=NULL)&&(ref_url[0] != '\0'))
	{
		sd_strcat(referer, ref_url,sd_strlen(ref_url)); 
		sd_strcat(referer, "\r\n",sd_strlen("\r\n")); 
	}
	else
	{
		referer[0]='\0';
	}
		
#if defined(MOBILE_PHONE)
		/*--------------- User-Agent ----------------------*/
		if(!(NT_GPRS_WAP & sd_get_net_type()))
		{
			sd_snprintf(request_header_tail,512,"%s","User-Agent: Mozilla/5.0\r\nConnection: Keep-Alive\r\nAccept: */*\r\n");
		}
#else
			sd_snprintf(request_header_tail,512,"%s","User-Agent: Mozilla/5.0\r\nConnection: Keep-Alive\r\nAccept: */*\r\n");
#endif /* MOBILE_PHONE */
	
	/*--------------- is_browser ----------------------*/
	if(is_browser)
	{
		// Add Accept-Encoding: gzip,deflate to the request for browser 
		char os[MAX_OS_LEN];
		char ui_version[64]={0},peer_id[PEER_ID_SIZE+4]={0},device_name[64]={0},hardware_ver[64]={0};
		_int32 x,y,ui_version_len = 0;
		
		ret_val = sd_get_os(os, MAX_OS_LEN);
		CHECK_VALUE(ret_val);
		
		ret_val = sd_get_screen_size(&x, &y);
		CHECK_VALUE(ret_val);
		
		ui_version[0] = '0';
		settings_get_str_item("system.ui_version",ui_version);
		ui_version_len = sd_strlen(ui_version);
		
		get_peerid(peer_id,PEER_ID_SIZE);

		sd_get_device_name(device_name,64);
		sd_get_hardware_version(hardware_ver, 64);

		if(gzip)
		{
			sd_strcat(request_header_tail, "Accept-Encoding: gzip,deflate",sd_strlen("Accept-Encoding: gzip,deflate")); 
			sd_strcat(request_header_tail, "\r\n",sd_strlen("\r\n")); 
		}
#if defined(MOBILE_PHONE)
		if(NT_CMWAP == sd_get_net_type() && gzip)
		{
			LOG_DEBUG( "Do Not send Content-Encoding: gzip when Accept-Encoding: gzip,deflate is added in CMWAP!");
		}
		else
#endif /* MOBILE_PHONE */
		if(is_post&&post_gzip)
		{
			sd_strcat(request_header_tail, "Content-Encoding: gzip",sd_strlen("Content-Encoding: gzip")); 
			sd_strcat(request_header_tail, "\r\n",sd_strlen("\r\n")); 
		}
		
		// Add system info and screen size to the request for browser 
		/* Cookie: TE=iPad_4.3.1,748x1024&2.3.1.44&zyq_ipad&ipad1&peerid=442A606B8509003V    ipad看看的版本号和ipad名字,ipad硬件版本 */
		sd_snprintf(request_header_tail+sd_strlen(request_header_tail),512-sd_strlen(request_header_tail),"Cookie: TE=%s,%dx%d&%s&%s&%s&peerid=%s",os,x,y,ui_version,device_name,hardware_ver,peer_id);
		if(cookies!=NULL && sd_strlen(cookies)!=0)
		{
			/* 将外面传进来的cookie接到TE后面 */
			cookie_header = sd_strstr(cookies, "Cookie:", 0);
			if(cookie_header) 
			{
				sd_assert(cookie_header==cookies);
				/* 去掉 "Cookie:" 头*/
				cookie_header+=7;
				
				/* 去掉空格 */
				if(*cookie_header == ' ') cookie_header++;
				
				cookie_len = sd_strlen(cookie_header);
				if(cookie_len>0)
				{
					sd_strcat(request_header_tail, "; ",sd_strlen("; ")); 
					sd_strcat(request_header_tail, cookie_header,cookie_len); 
					if(cookie_header[cookie_len-1]!='\n')
					{
						sd_strcat(request_header_tail, "\r\n",sd_strlen("\r\n")); 
					}
				}
				/* 不再需要，将cookie置空 */
				cookies = NULL;
			}
			else
			{
				sd_strcat(request_header_tail, "\r\n",sd_strlen("\r\n")); 
			}
		}
		else
		{
			sd_strcat(request_header_tail, "\r\n",sd_strlen("\r\n")); 
		}
	}

	/*--------------- ok ----------------------*/
	length = sd_strlen(request_header)+sd_strlen(http_info)+sd_strlen(host_line)+sd_strlen(range)+sd_strlen(referer)+sd_strlen(cookies)+sd_strlen(request_header_tail)+sd_strlen("\r\n")+post_data_len;

	if(length+1>buffer_len)
	{
		if(buffer_len==0)
		{
			if(length+1>MAX_HTTP_REQ_HEADER_LEN)
				buffer_len = length+1;
			else
				buffer_len = MAX_HTTP_REQ_HEADER_LEN;
		}else
		{
			SAFE_DELETE(request);
			buffer_len = length+1;
		}

		ret_val = sd_malloc(buffer_len, (void**)&(request));
		if(ret_val!=SUCCESS) return ret_val;

		*request_buffer = request;
		*request_buffer_len = buffer_len;
		
	}

		
	sd_memset(request,0,buffer_len);
	sd_snprintf(request, buffer_len,"%s%s%s%s%s%s%s",request_header,http_info,host_line,range,referer,cookies,request_header_tail);
	sd_strcat(request,"\r\n",sd_strlen("\r\n"));
	*total_send_bytes =sd_strlen(request);
	if(is_post)
	{
		sd_memcpy(request+sd_strlen(request),post_data, post_data_len);
		*total_send_bytes +=post_data_len;
	}
	return SUCCESS;
			
}


 _int32 http_parse_header( HTTP_RESPN_HEADER * p_http_respn )
{
	char line_temp[1024], status_line[MAX_STATUS_LINE_LEN],*respn_str=NULL,*header_end=NULL,*cur_line_end = NULL,*line= NULL,* cur_line_begin = NULL;
	_int32 _lep = 0,str_len = 0,ret_val = SUCCESS;

	LOG_DEBUG( " enter http_parse_header()..." );

	/* Find out the start point of  the response header  */
	respn_str = sd_strstr(p_http_respn->_header_buffer,"HTTP/", 0);

	if(respn_str == NULL ) return HTTP_ERR_HEADER_NOT_FOUND;

	/* Find out the end point of the response header */
	header_end = sd_strstr(respn_str, "\r\n\r\n", 0);
	if( header_end!= NULL)
	{
        LOG_DEBUG("find \r\n\r\n, so head has recved");
		p_http_respn->_body_start_pos= header_end+4;
		p_http_respn->_body_len = p_http_respn->_header_buffer_end_pos -(p_http_respn->_body_start_pos-p_http_respn->_header_buffer);
	}
    else
	{
		header_end = sd_strstr(respn_str,  "\n\n", 0);
        LOG_DEBUG("find \n\n, so head has recved");
		if( header_end != NULL)
		{
			p_http_respn->_body_start_pos= header_end+2;
			p_http_respn->_body_len = p_http_respn->_header_buffer_end_pos -(p_http_respn->_body_start_pos-p_http_respn->_header_buffer);
		}
		else
		{			
			LOG_DEBUG("received %u bytes,but don't find response header end tag: CRLFCRLF,  maybe other side close connection prematurity",p_http_respn->_header_buffer_end_pos);
			return  HTTP_ERR_HEADER_NOT_FOUND; /* ??????????????? zyq mark ????????????????? */
		}
	}
	
#ifdef _DEBUG
{

	sd_memset(line_temp,0,1024);
	sd_memcpy(line_temp,respn_str,header_end-respn_str);

	LOG_DEBUG("+++ Recv http response header[%d]:%s\n",header_end-respn_str,line_temp);
	if(header_end-respn_str>400)
	{
		LOG_DEBUG("+++ [%d]:\n%s\n",header_end-respn_str-400,line_temp+400);
	}
}
#endif

	cur_line_begin = respn_str;

	/* Parse first line to get the status code */
	if(http_parse_header_search_line_end(cur_line_begin ,&_lep) == 0)
		cur_line_end = cur_line_begin + _lep;

	sd_memset(status_line,0,MAX_STATUS_LINE_LEN);
	if(_lep>=MAX_STATUS_LINE_LEN)
		sd_strncpy(status_line, cur_line_begin, MAX_STATUS_LINE_LEN-1);
	else
		sd_memcpy(status_line, cur_line_begin, _lep);
	
	ret_val = http_parse_header_get_status_code( status_line,&(p_http_respn->_status_code) );
	CHECK_VALUE(ret_val);
	
	/* parse every line */
	while( cur_line_end != NULL )
	{
		line= NULL;
		
		cur_line_begin = cur_line_end+1;

		if(cur_line_begin>=header_end)
			break;

		if(http_parse_header_search_line_end(cur_line_begin ,&_lep) == 0)
			cur_line_end = cur_line_begin + _lep;
		else
			cur_line_end = NULL;

		if( cur_line_end == NULL )
		{
			line = cur_line_begin;
		}
		else
		{
			sd_memset(line_temp,0, 1024);
			if(cur_line_end-cur_line_begin>1023)
			sd_strncpy(line_temp,cur_line_begin,1023 );
			else
			sd_memcpy(line_temp,cur_line_begin,cur_line_end-cur_line_begin );
			line = line_temp;
		}
		str_len = sd_strlen(line);
		if( (str_len > 0) && (line[str_len-1] == '\r') )
		{
			line[str_len-1] = '\0';
		}
		if(line[0]!='\0' )
		{
			http_parse_header_one_line( p_http_respn,line );
		}
	}

	return SUCCESS;
}
_int32 http_parse_header_search_line_end(  char* _line,_int32 * _pos )
{
	 char* temp = _line,*n_pos = NULL;
	_int32 _pos_temp = 0;
	char c ;
//		HTTP_TRACE( " enter http_pipe_search_line_end()..." );
	while( TRUE )
	{
		n_pos = sd_strchr(temp, '\n', 0);
		if( n_pos == NULL)
			return -1;
		else
			_pos_temp = n_pos - temp;

		temp = n_pos;
		c = *(temp+1);
		if( (c == ' ') || (c == '\t') )
		{
			++temp;
			continue;
		}
		else
		{
			* _pos = _pos_temp;
			return SUCCESS;
		}
	}
			return -1;

}

 _int32 http_parse_header_get_status_code( char* status_line,_u32 * status_code )
{
	char  code[10],*HTTP_pos=NULL,*space_pos1 = NULL,*space_pos2 = NULL;
	_int32 str_len = 0,status_code_t=0;
	
	LOG_DEBUG("http_parse_header_get_status_code:status_line=%s",status_line);
	
	str_len = sd_strlen(status_line);
	if(str_len == 0 )
	{
		return HTTP_ERR_STATUS_LINE_NOT_FOUND;
	}
	
	HTTP_pos = sd_strstr(status_line, "HTTP",0);
	if(HTTP_pos==NULL)
	{
		return HTTP_ERR_STATUS_LINE_NOT_FOUND;
	}

	space_pos1 = sd_strchr(HTTP_pos, ' ', 0);	
	if(space_pos1 == NULL )
	{
		return HTTP_ERR_STATUS_LINE_NOT_FOUND;
	}
	
	space_pos2 = sd_strchr(space_pos1, ' ', 1);	

	if(space_pos2 == NULL )
	{
		space_pos2 = sd_strchr(space_pos1, '\r', 1);	
	}
	
	if((space_pos2 == NULL )||(space_pos2-space_pos1==1))
	{
		return HTTP_ERR_STATUS_LINE_NOT_FOUND;
	}
	else 	
	{
		/* Get the status code */
		sd_memset(code,0,10);
		sd_strncpy(code,space_pos1+1,9);

		status_code_t = (_u32)sd_atoi(code);
		if(status_code_t == 0)
		{
			return HTTP_ERR_STATUS_CODE_NOT_FOUND;
		}
		
  		*status_code = status_code_t;
	}

	return SUCCESS;
}

_int32 http_parse_header_one_line( HTTP_RESPN_HEADER * p_http_respn,  char* line )
{
	_int32 i=0,name_len = 0;
	char field_name[32],field_value[1024],* colon_pos = NULL;

	LOG_DEBUG( " enter http_pipe_parse_one_line(%s)...",line );
	colon_pos = sd_strchr(line,  ':', 0);
	if( colon_pos == NULL)
			return HTTP_ERR_INVALID_FIELD;
	name_len = colon_pos - line;
	if((name_len == 0)||( name_len+1 == sd_strlen(line) ))
			return HTTP_ERR_INVALID_FIELD;

	sd_memset(field_name,0,32);

	// 某些情况下，服务器返回的name_len要超过32
	name_len = (name_len >= 32) ? 31 : name_len;
	sd_memcpy(field_name, line,name_len);
	
	for(i=0;i<MAX_NUM_OF_FIELD_NAME;i++)
	{
		if(0==sd_stricmp(field_name, g_http_head_field_name[i]))
			break;
	}

	if(i !=MAX_NUM_OF_FIELD_NAME)
	{
		
		sd_memset(field_value,0,1024);
		sd_memcpy(field_value, colon_pos+1,sd_strlen(colon_pos+1));
		return http_parse_header_field_value( p_http_respn, i,field_value );
	}
	
	 return SUCCESS;

}

static void parse_http_resp_range_content(const char* field_value, _u64* start_pos, _u64* end_pos)
{
    //bytes 671744-34226175/58863937
    char* digit_start_pos = NULL;
    char* pos = field_value;
    char ch = *pos;
    _int32 state = 0;  // 是否找数字的标识
    _int32 digit_len = 0;
    _u64 start_range_pos = 0;
    _u64 end_range_pos = 0;
    while(ch != '\0')
    {
        if(state == 0) // 找第一个数字
        {
            if( IS_DIGIT(ch) )
            {
                digit_start_pos = pos;
                state = 1;
                digit_len += 1;
            }
        }
        else 
        {
            if( IS_DIGIT(ch) )
            {
                digit_len += 1;
            }
            else if(ch == '-')
            {
                LOG_DEBUG("parse_http_resp_range_content, find - digit_len = %d", digit_len);
                sd_str_to_u64( digit_start_pos, digit_len, &start_range_pos );
                state = 0;
                digit_len = 0;

            }
            else if(ch == '/')
            {
                LOG_DEBUG("parse_http_resp_range_content, find / digit_len = %d", digit_len);
                sd_str_to_u64( digit_start_pos, digit_len, &end_range_pos);
                break;
            }
            else
            {
                LOG_URGENT("parse_http_resp_range_content, error ch = %c",ch);
            }
        }
        pos++;
        ch = *pos;
    }
    *start_pos = start_range_pos;
    *end_pos = end_range_pos;

}


_int32 http_parse_header_field_value( HTTP_RESPN_HEADER * p_http_respn, enum HTTP_HEAD_FIELD_NAME field_name, char* field_value )
{
	_int32 ret_val=SUCCESS;
	char *semicolon_pos = NULL,*slash_pos=NULL,*file_name_pos=NULL,*tmp=NULL;

	LOG_DEBUG( " enter http_parse_header_field_value,field_name=%d",field_name );

	switch(field_name)
	{
		case HHFT_CONTENT_TYPE:  		/* "Content-Type" */ 
			LOG_DEBUG( "Content-Type=%s",field_value);
			sd_trim_prefix_lws(field_value);

			if(field_value[0]!='\0')
			{
				/* is_binary_file */
				if(0!=sd_strncmp( field_value, "text/html",sd_strlen("text/html") ) )
				{
					p_http_respn->_is_binary_file = TRUE;
					LOG_DEBUG( "p_http_respn->_is_binary_file = TRUE");
				}
				else
				{
					p_http_respn->_is_binary_file = FALSE;
					LOG_DEBUG( "p_http_respn->_is_binary_file = FALSE");
				}

				if(NULL!=sd_strstr( field_value, "vnd.wap.",0 ) )
				{
					p_http_respn->_is_vnd_wap = TRUE;
					LOG_DEBUG( "p_http_respn->_is_vnd_wap = TRUE");
				}
			}
			break;
		case HHFT_TRANSFER_ENCODING:  		/* "Transfer-Encoding" */ 
			LOG_DEBUG( "Transfer-Encoding=%s",field_value);
			sd_trim_prefix_lws(field_value);

			if(field_value[0]!='\0')
			{
				if(0==sd_strncmp( field_value, "chunked" ,sd_strlen("chunked")) )
				{
					p_http_respn->_is_chunked = TRUE;
					LOG_DEBUG( "p_http_respn->_is_chunked = TRUE");
				}
			}
			break;
		case HHFT_LOCATION:    				/* "Location" */ 
		case HHFT_LOCATION2:    				/* "location" */ 
			LOG_DEBUG( "Location=%s",field_value);
			sd_trim_prefix_lws(field_value);
			sd_trim_postfix_lws( field_value ); 

			if(field_value[0]!='\0')
			{
				//if(HHFT_LOCATION==field_name)
					tmp = sd_stristr(p_http_respn->_header_buffer, "\r\nLocation:", 0);
				//else
					//tmp = sd_strstr(p_http_respn->_header_buffer, "\r\nlocation:", 0);
				
				if(tmp!=NULL)
				{
					p_http_respn->_location = sd_strstr(tmp, field_value, 0);
					if(p_http_respn->_location!=NULL)
					{
						p_http_respn->_location_len = sd_strlen(field_value);
						LOG_DEBUG( "p_http_respn->_location_len = %u",p_http_respn->_location_len);
					}
				}
			}
			break;
		case HHFT_CONTENT_RANGE:  			/* "Content-Range" */ 
			LOG_DEBUG( "Content-Range=%s",field_value);
            //bytes 671744-34226175/58863937
            sd_trim_prefix_lws(field_value);
			sd_trim_postfix_lws( field_value ); 
			if(field_value[0]!='\0')
			{
                _u64 start_range_pos = 0;
                _u64 end_range_pos = 0;
				slash_pos =  sd_strrchr(field_value,'/');
				if( slash_pos!=NULL)
				{
					ret_val = sd_str_to_u64( slash_pos+1, sd_strlen(slash_pos+1), &(p_http_respn->_entity_length) );
					if(ret_val==SUCCESS)
					{
						p_http_respn->_has_entity_length = TRUE;
						LOG_DEBUG( "p_http_respn->_entity_length = %llu",p_http_respn->_entity_length);
					}
				}
                
                // 获取_content_range_length
                parse_http_resp_range_content(field_value, &start_range_pos, &end_range_pos);
                if (end_range_pos == 0 || end_range_pos < start_range_pos)
                {
                    p_http_respn->_is_content_range_length_valid = FALSE;
                }
                else
                {
                    p_http_respn->_is_content_range_length_valid = TRUE;
                    p_http_respn->_content_range_length = end_range_pos - start_range_pos + 1;
                }
                LOG_DEBUG("parse_http_resp_range_content, start_range_pos = %llu, end_range_pos = %llu"
                    , start_range_pos, end_range_pos);
			}
			break;
		case HHFT_CONTENT_LENGTH:    		/* "Content-Length"  */ 
			LOG_DEBUG( "Content-Length=%s",field_value);
			sd_trim_prefix_lws(field_value);
			sd_trim_postfix_lws( field_value ); 
			if(field_value[0]!='\0')
			{
				ret_val=sd_str_to_u64( field_value, sd_strlen(field_value), &(p_http_respn->_content_length) );
				if(ret_val==SUCCESS)
				{
					p_http_respn->_has_content_length= TRUE;
					LOG_DEBUG( "p_http_respn->_content_length = %llu",p_http_respn->_content_length);
				}
			}
			break;			
		case HHFT_ACCEPT_RANGES:  			/* "Accept-Ranges" */ 
			LOG_DEBUG( "Accept-Ranges=%s",field_value);
			sd_trim_prefix_lws(field_value);

			if(field_value[0]!='\0')
			{
				if(0!=sd_strncmp( field_value,"bytes",sd_strlen("bytes")) ) 
				{
					p_http_respn->_is_support_range = FALSE;
					LOG_DEBUG( "p_http_respn->_is_support_range = FALSE");
				}
			}
			break;
		case HHFT_CONTENT_DISPOSITION:     	/* "Content-Disposition" */ 
			LOG_DEBUG( " Content-Disposition=%s",field_value);
			sd_trim_prefix_lws(field_value);
			sd_trim_postfix_lws( field_value ); 
			if(field_value[0]!='\0')
			{
				file_name_pos=  sd_strstr(field_value, "filename=", 0);
				if( file_name_pos!=NULL)
				{
					file_name_pos+=sd_strlen("filename=");
					sd_trim_prefix_lws(file_name_pos);

					if(file_name_pos[0]!='\0')
					{
						if( file_name_pos[0] == '\"')
						{
							file_name_pos++;
							slash_pos =  sd_strchr(file_name_pos,'\"', 0);
							if( slash_pos!=NULL)
							{
								tmp=NULL;
								tmp= sd_strstr(p_http_respn->_header_buffer, "filename=", 0);
								if(tmp!=NULL)
								{
									p_http_respn->_server_return_name= sd_strstr(tmp, file_name_pos, 0);
									if(p_http_respn->_server_return_name!=NULL)
									{
										if(slash_pos-file_name_pos>=MAX_FILE_NAME_LEN)
											p_http_respn->_server_return_name_len= MAX_FILE_NAME_LEN;
										else
											p_http_respn->_server_return_name_len= slash_pos-file_name_pos;
									}
								}
							}

						}
						else
						{
							semicolon_pos =  sd_strchr(file_name_pos,';', 0);
							if( semicolon_pos!=NULL)
							{
								semicolon_pos[0] = '\0';
							}
							tmp=NULL;
							tmp= sd_strstr(p_http_respn->_header_buffer, "filename=", 0);
							if(tmp!=NULL)
							{
								p_http_respn->_server_return_name= sd_strstr(tmp, file_name_pos, 0);
								if(p_http_respn->_server_return_name!=NULL)
								{
									p_http_respn->_server_return_name_len= sd_strlen(file_name_pos);
								}
							}
						}

					}

				}
			}
			LOG_DEBUG( "p_http_respn->_server_return_name_len = %u",p_http_respn->_server_return_name_len);
			break;
		case HHFT_CONNECTION :   			/* "Connection" */    		
			LOG_DEBUG( " Connection=%s",field_value);
			sd_trim_prefix_lws(field_value);
			if(field_value[0]!='\0')
			{
				if(0==sd_strncmp( field_value,"close",sd_strlen("close")) ) 
				{
					p_http_respn->_is_support_long_connect = FALSE;
					LOG_DEBUG( "p_http_respn->_is_support_long_connect = FALSE");
				}
			}
			break;
		case HHFT_LAST_MODIFIED:   			/* "Last-Modified"  */ 
			LOG_DEBUG( " Last-Modified=%s",field_value);
			sd_trim_prefix_lws(field_value);
			sd_trim_postfix_lws( field_value ); 
			tmp=NULL;
			tmp=sd_strstr(p_http_respn->_header_buffer, "\r\nLast-Modified:", 0);
			if(tmp!=NULL)
			{
				p_http_respn->_last_modified= sd_strstr(tmp, field_value, 0);
				if(p_http_respn->_last_modified!=NULL)
				{
					p_http_respn->_last_modified_len= sd_strlen(field_value);
					LOG_DEBUG( "p_http_respn->_last_modified_len = %u",p_http_respn->_last_modified_len);
				}
			}
			break;
		case HHFT_SET_COOKIE :   				/* "Set-Cookie"  */ 
			LOG_DEBUG( " Set-Cookie=%s",field_value);
			sd_trim_prefix_lws(field_value);
			sd_trim_postfix_lws( field_value ); 
			tmp=NULL;
			tmp=sd_strstr(p_http_respn->_header_buffer, "\r\nSet-Cookie:", 0);
			if(tmp!=NULL)
			{
				p_http_respn->_cookie= sd_strstr(tmp, field_value, 0);
				if(p_http_respn->_cookie!=NULL)
				{
					_int32 cookie_head_len = p_http_respn->_cookie -tmp-6;
					p_http_respn->_cookie_len= sd_strlen(field_value);
					p_http_respn->_cookie-=cookie_head_len;
					p_http_respn->_cookie_len+=cookie_head_len;
					LOG_DEBUG( "p_http_respn->_cookie_len = %u",p_http_respn->_cookie_len);
				}
			}
			break;
		case HHFT_CONTENT_ENCODING :   				/* "Content-Encoding"  */ 
			LOG_DEBUG( " Content-Encoding=%s",field_value);
			sd_trim_prefix_lws(field_value);
			sd_trim_postfix_lws( field_value ); 
			tmp=NULL;
			tmp=sd_strstr(p_http_respn->_header_buffer, "\r\nContent-Encoding:", 0);
			if(tmp!=NULL)
			{
				p_http_respn->_content_encoding= sd_strstr(tmp, field_value, 0);
				if(p_http_respn->_content_encoding!=NULL)
				{
					p_http_respn->_content_encoding_len= sd_strlen(field_value);
					LOG_DEBUG( "p_http_respn->_content_encoding_len = %u",p_http_respn->_content_encoding_len);
				}
			}
			break;
	}

	 return SUCCESS;

}


_int32 http_reset_respn_header( HTTP_RESPN_HEADER * p_respn_header )
{
		LOG_DEBUG( " enter http_reset_respn_header()..." );

	if(p_respn_header->_header_buffer!=NULL)
		http_release_1024_buffer((void*) (p_respn_header->_header_buffer));

	p_respn_header->_header_buffer=NULL;
	
	sd_memset(p_respn_header,0,sizeof(HTTP_RESPN_HEADER));
	p_respn_header->_is_support_range= TRUE;
	p_respn_header->_is_support_long_connect= TRUE;
	p_respn_header->_is_binary_file= TRUE;

	 return SUCCESS;

}
void http_header_discard_cookies( HTTP_RESPN_HEADER * p_respn_header )
{
	char *cokie_pos=NULL;
	char *crlf=NULL;
	_int32 valid_len = 0,discard_len=0;

	LOG_DEBUG( " enter http_header_discard_cookies()..." );

	/* Find out the first Set-Cookie  */
	cokie_pos = sd_strstr(p_respn_header->_header_buffer,"Set-Cookie:", 0);

	while(cokie_pos != NULL ) 
	{
		crlf = sd_strstr(cokie_pos,"\r\n", 0);
		if(crlf!=NULL)
		{
			discard_len=crlf+2-cokie_pos;
			valid_len = p_respn_header->_header_buffer_end_pos-(crlf+2-p_respn_header->_header_buffer);
			sd_memmove(cokie_pos, crlf+2, valid_len);
			p_respn_header->_header_buffer_end_pos-=discard_len;
			p_respn_header->_header_buffer[p_respn_header->_header_buffer_end_pos]='\0';
			cokie_pos = sd_strstr(p_respn_header->_header_buffer,"Set-Cookie:", 0);
		}
		else
		{
			break;
		}
		
	}
	
}

void http_header_discard_excrescent( HTTP_RESPN_HEADER * p_respn_header )
{
	char *respn_str=NULL;
	_int32 valid_len = 0;

	LOG_DEBUG( " enter http_header_discard_excrescent()..., p_respn_header = %x", p_respn_header );
    
	/* Find out the start point of  the response header  */
	respn_str = sd_strstr(p_respn_header->_header_buffer,"HTTP/", 0);

	if(respn_str != NULL ) 
	{
		if(respn_str!=p_respn_header->_header_buffer)
		{
			valid_len = p_respn_header->_header_buffer_end_pos-(respn_str-p_respn_header->_header_buffer);
			sd_memmove(p_respn_header->_header_buffer, respn_str, valid_len);
			p_respn_header->_header_buffer[valid_len]='\0';
			p_respn_header->_header_buffer_end_pos=valid_len;
		}
		/* Discard  Set-Cookie */
		http_header_discard_cookies( p_respn_header );
	}
	else
	{
		sd_memset(p_respn_header->_header_buffer,0,p_respn_header->_header_buffer_length);
		p_respn_header->_header_buffer_end_pos=0;
	}
	
}


#endif /* __HTTP_PROTOCOL_C_20090305 */
