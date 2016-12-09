#include "asyn_frame/msg.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "platform/sd_socket.h"

#include "utility/logid.h"
#define LOGID LOGID_CMD_INTERFACE
#include "utility/logger.h"

#include "cmd_proxy.h"

#include "socket_proxy_interface.h"

#include "res_query/res_query_interface.h"
#include "res_query/res_query_cmd_handler.h"
#include "res_query/res_query_cmd_builder.h"

static _u32 g_cmd_proxq_id = 0;

#define PROXY_CMD_HEADER_LEN		12


#define CMD_PROXY_HTTP_HEAD 0x01
#define CMD_PROXY_AES 0x02
#define CMD_PROXY_LONGCONNECT 0x04
#define CMD_PROXY_ACTIVECONNECT 0x08


#define MAX_RECV_BUFFER_LEN 1024
#define PROXY_CMD_HEADER_LEN 12

_int32 cmd_proxy_create(_u32 type, _u32 attribute, CMD_PROXY **pp_cmd_proxy )
{
    _int32 ret_val = SUCCESS;
    CMD_PROXY *p_cmd_proxy = NULL;

    *pp_cmd_proxy = NULL;

    ret_val = sd_malloc( sizeof(CMD_PROXY), (void**)&p_cmd_proxy );
    if( ret_val != SUCCESS )
    {
        return ret_val;
    }
    
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_create, type:%d, attribute:%u", 
        p_cmd_proxy, type, attribute );
    p_cmd_proxy->_proxy_id = ++g_cmd_proxq_id;
    if( g_cmd_proxq_id == 0 ) g_cmd_proxq_id++;

	p_cmd_proxy->_socket_type = type;
	p_cmd_proxy->_attribute = attribute;
	p_cmd_proxy->_socket_state = CMD_PROXY_SOCKET_IDLE;
	p_cmd_proxy->_socket = INVALID_SOCKET;
    
	p_cmd_proxy->_ip = 0;
	p_cmd_proxy->_port = 0;
	p_cmd_proxy->_host = NULL;

    list_init( &p_cmd_proxy->_cmd_list );
	p_cmd_proxy->_last_cmd = NULL; 

    
	p_cmd_proxy->_recv_buffer = NULL;
	p_cmd_proxy->_recv_buffer_len = MAX_RECV_BUFFER_LEN;
	p_cmd_proxy->_had_recv_len = 0;
    p_cmd_proxy->_recv_cmd_type = MAX_U32;
    
    p_cmd_proxy->_recv_cmd_id = 0;
    list_init( &p_cmd_proxy->_recv_cmd_list );
    
    
	p_cmd_proxy->_timeout_id = INVALID_MSG_ID;
    p_cmd_proxy->_close_flag = FALSE;
    p_cmd_proxy->_err_code = SUCCESS;
    
    *pp_cmd_proxy = p_cmd_proxy;

    return SUCCESS;
}

_int32 cmd_proxy_destroy( CMD_PROXY *p_cmd_proxy )
{
    _int32 ret_val = SUCCESS;
    LIST_ITERATOR cur_node = NULL;
    CMD_INFO *p_cmd_info = NULL;
    CMD_PROXY_RECV_INFO *p_recv_info = NULL; 
    
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_destroy begin", p_cmd_proxy );
    ret_val = cmd_proxy_try_close_socket( p_cmd_proxy );
    if( ret_val != SUCCESS ) return SUCCESS;

    cur_node = LIST_BEGIN( p_cmd_proxy->_cmd_list );
    while( cur_node != LIST_END( p_cmd_proxy->_cmd_list ) )
    {
        p_cmd_info = (CMD_INFO *)LIST_VALUE( cur_node );
        cmd_proxy_free_cmd_info( p_cmd_info );
        cur_node = LIST_NEXT( cur_node );
    }
    list_clear( &p_cmd_proxy->_cmd_list );
    
    cur_node = LIST_BEGIN( p_cmd_proxy->_recv_cmd_list );
    while( cur_node != LIST_END( p_cmd_proxy->_recv_cmd_list ) )
    {
        p_recv_info = (CMD_PROXY_RECV_INFO *)LIST_VALUE( cur_node );
        cmd_proxy_uninit_recv_info( p_recv_info );
        cur_node = LIST_NEXT( cur_node );
    }    
    list_clear( &p_cmd_proxy->_recv_cmd_list );
    SAFE_DELETE( p_cmd_proxy->_host );
    cmd_proxy_final_close( p_cmd_proxy );
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_destroy end", p_cmd_proxy );
    sd_free( p_cmd_proxy );
	p_cmd_proxy = NULL;

    return SUCCESS;
}

_int32 cmd_proxy_create_socket( CMD_PROXY *p_cmd_proxy )
{
    _int32 ret_val = SUCCESS;
	SD_SOCKADDR addr;
	addr._sin_family = AF_INET;
	addr._sin_addr = p_cmd_proxy->_ip;
	addr._sin_port = sd_htons( p_cmd_proxy->_port );

    if( p_cmd_proxy->_socket_type == DEVICE_SOCKET_TCP )
    {
        ret_val = socket_proxy_create(&p_cmd_proxy->_socket, SD_SOCK_STREAM);
        if( ret_val != SUCCESS ) return ret_val;
        
        LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_create_TCP_socket, _socket:%u, host:%s, ip:%d, port:%d", 
            p_cmd_proxy, p_cmd_proxy->_socket, p_cmd_proxy->_host, p_cmd_proxy->_ip, p_cmd_proxy->_port );
        
        if( p_cmd_proxy->_host == NULL )
        {
            sd_assert( p_cmd_proxy->_ip != 0 );
            ret_val = socket_proxy_connect(p_cmd_proxy->_socket, &addr, cmd_proxy_connect_callback, p_cmd_proxy );
        }
        else
        {
            ret_val = socket_proxy_connect_by_domain( p_cmd_proxy->_socket, p_cmd_proxy->_host, p_cmd_proxy->_port, cmd_proxy_connect_callback, p_cmd_proxy );
        }
        
        if( ret_val != SUCCESS ) return ret_val;
    }
    else
    {
        return UNSUPPORT_TYPE;
    }
    
    cmd_proxy_enter_socket_state( p_cmd_proxy, CMD_PROXY_SOCKET_CREATING );
    return SUCCESS;
}

_int32 cmd_proxy_connect_callback( _int32 errcode, _u32 pending_op_count, void* user_data )
{
    CMD_PROXY *p_cmd_proxy = (CMD_PROXY *)user_data;
    
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_connect_callback, errcode:%d", 
        p_cmd_proxy, errcode );

    if( errcode == MSG_CANCELLED )
    {
        cmd_proxy_destroy( p_cmd_proxy );
        return SUCCESS;
    }
    if( errcode != SUCCESS )
    {
        return cmd_proxy_handle_err( p_cmd_proxy, errcode );
    }
    cmd_proxy_enter_socket_state( p_cmd_proxy, CMD_PROXY_SOCKET_WORKING );

    cmd_proxy_send_update( p_cmd_proxy );
    cmd_proxy_recv_update( p_cmd_proxy );
    
    return SUCCESS;
}

_int32 cmd_proxy_send_update( CMD_PROXY *p_cmd_proxy )
{
    _int32 ret_val = SUCCESS;
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_send_update, _socket_state:%d, cmd_list_size:%u", 
        p_cmd_proxy, p_cmd_proxy->_socket_state, list_size(&p_cmd_proxy->_cmd_list) );

    if( p_cmd_proxy->_last_cmd == NULL && list_size(&p_cmd_proxy->_cmd_list) != 0 )
    {
        list_pop( &p_cmd_proxy->_cmd_list, (void **)&p_cmd_proxy->_last_cmd );
    }
    if( p_cmd_proxy->_last_cmd == NULL ) return SUCCESS;
    
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_send_update, _last_cmd:0x%x", 
        p_cmd_proxy, p_cmd_proxy->_last_cmd );

    switch( p_cmd_proxy->_socket_state )
    {
        case CMD_PROXY_SOCKET_IDLE:
            ret_val = cmd_proxy_create_socket( p_cmd_proxy );
            break;
        case CMD_PROXY_SOCKET_CREATING:
            break;
        case CMD_PROXY_SOCKET_WORKING:
            ret_val = cmd_proxy_send_last_cmd( p_cmd_proxy );
            break;
        case CMD_PROXY_SOCKET_CLOSEING:
        case CMD_PROXY_SOCKET_CLOSED:
        case CMD_PROXY_SOCKET_ERR:
            break;
    }
    if( ret_val != SUCCESS )
    {
        cmd_proxy_handle_err( p_cmd_proxy, ret_val );
    }
    return ret_val;
}

_int32 cmd_proxy_send_last_cmd( CMD_PROXY *p_cmd_proxy )
{
    _int32 ret_val = SUCCESS;
    CMD_INFO *p_cmd_info = p_cmd_proxy->_last_cmd;

    sd_assert( p_cmd_info != NULL );
    if( p_cmd_info == NULL ) return SUCCESS;
    
    ret_val = cmd_proxy_format_cmd( p_cmd_proxy, p_cmd_info );
    if( ret_val != SUCCESS ) return SUCCESS;


    ret_val = socket_proxy_send( p_cmd_proxy->_socket, p_cmd_proxy->_last_cmd->_cmd_buffer_ptr, 
        p_cmd_proxy->_last_cmd->_cmd_len, cmd_proxy_send_callback, p_cmd_proxy );
    CHECK_VALUE( ret_val );
    
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_send_last_cmd SUCCESS, _last_cmd:0x%x, cmd_len:%u", 
        p_cmd_proxy, p_cmd_info, p_cmd_proxy->_last_cmd->_cmd_len );    
	return SUCCESS;
}

_int32 cmd_proxy_send_callback( _int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data )
{
    CMD_PROXY *p_cmd_proxy = (CMD_PROXY *)user_data;
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_send_callback, errcode:%d, had_send:%u", 
         p_cmd_proxy, errcode, had_send );

    if( errcode == MSG_CANCELLED )
    {
        cmd_proxy_destroy( p_cmd_proxy );
        return SUCCESS;
    }    
    if( errcode != SUCCESS )
    {
        return cmd_proxy_handle_err( p_cmd_proxy, errcode );
    }
    cmd_proxy_free_cmd_info( p_cmd_proxy->_last_cmd );
    p_cmd_proxy->_last_cmd = NULL;
    
    cmd_proxy_send_update( p_cmd_proxy );
    
    return SUCCESS;
}

_int32 cmd_proxy_recv_update( CMD_PROXY *p_cmd_proxy )
{
    _int32 ret_val = SUCCESS;
    p_cmd_proxy->_had_recv_len = 0;
    
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_recv_update, _socket_type:%d, _socket_state:%d, _recv_buffer_len:%u", 
         p_cmd_proxy, p_cmd_proxy->_socket_type, p_cmd_proxy->_socket_state, p_cmd_proxy->_recv_buffer_len );

    if( p_cmd_proxy->_socket_state != CMD_PROXY_SOCKET_WORKING ) return SUCCESS;
    if( p_cmd_proxy->_socket_type == DEVICE_SOCKET_TCP )
    {
        if( p_cmd_proxy->_recv_buffer == NULL )
        {
            ret_val = sd_malloc( p_cmd_proxy->_recv_buffer_len, (void **)&p_cmd_proxy->_recv_buffer );
            if( ret_val != SUCCESS )
            {
                sd_assert( FALSE );
                return cmd_proxy_handle_err( p_cmd_proxy, ret_val );
            }
        }
        
        sd_memset( p_cmd_proxy->_recv_buffer, 0, p_cmd_proxy->_recv_buffer_len );
        ret_val =  socket_proxy_uncomplete_recv( p_cmd_proxy->_socket, p_cmd_proxy->_recv_buffer, 
            p_cmd_proxy->_recv_buffer_len - 1, cmd_proxy_recv_callback, p_cmd_proxy );
        if( ret_val != SUCCESS )
        {
            return cmd_proxy_handle_err( p_cmd_proxy, ret_val );
        }
   
}
   else
   {
        sd_assert( FALSE );
   }

   return SUCCESS;
}

_int32 cmd_proxy_add_recv_cmd( CMD_PROXY *p_cmd_proxy, char *p_recv_cmd, _u32 cmd_len )
{
    _int32 ret_val = SUCCESS;
	char* tmp = NULL;
    CMD_PROXY_RECV_INFO *p_recv_info = NULL;
    
#ifdef REPORTER_DEBUG
        {
            char test_buffer[1024];
            sd_memset(test_buffer,0,1024);
            str2hex( (char*)p_recv_cmd, cmd_len, test_buffer, 1024);
            LOG_DEBUG( "cmd_proxy_add_recv_cmd before aes:*buffer[%u]=%s", 
                        cmd_len, test_buffer);
            
        }
#endif /* _DEBUG */

	ret_val = sd_malloc( cmd_len, (void**)&tmp );
    CHECK_VALUE(ret_val);
    
	sd_memcpy( tmp, p_recv_cmd, cmd_len );

    if( (p_cmd_proxy->_attribute & CMD_PROXY_AES) != 0 )
    {
        ret_val = xl_aes_decrypt(tmp, &cmd_len);
        CHECK_VALUE(ret_val);
    }

	ret_val = sd_malloc( sizeof(CMD_PROXY_RECV_INFO), (void**)&p_recv_info );
    p_recv_info->_recv_cmd_id = p_cmd_proxy->_recv_cmd_id++;
    p_recv_info->_buffer = tmp;
    p_recv_info->_cmd_len = cmd_len;

    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_add_recv_cmd, cmd_len:%u, recv_id:%u", 
         p_cmd_proxy, cmd_len, p_recv_info->_recv_cmd_id );

    
#ifdef REPORTER_DEBUG
    {
        char test_buffer[1024];
        sd_memset(test_buffer,0,1024);
        str2hex( (char*)(p_recv_info->_buffer), p_recv_info->_cmd_len, test_buffer, 1024);
        LOG_DEBUG( "cmd_proxy_add_recv_cmd after aes:*buffer[%u]=%s", 
                    p_recv_info->_cmd_len, test_buffer);
        
    }
#endif /* _DEBUG */

    list_push( &p_cmd_proxy->_recv_cmd_list, p_recv_info );
    //p_cmd_proxy->_cmd_handler( SUCCESS, p_recv_info->_recv_cmd_id, cmd_len, p_cmd_proxy->_user_data );
	return SUCCESS;
}

_int32 cmd_proxy_parse_recv_cmd( CMD_PROXY *p_cmd_proxy, char *buffer, _u32 had_recv )
{
    _int32 ret_val = SUCCESS;
	char* http_header = NULL;
	_u32 http_header_len = 0;
	char* recv_buffer = NULL;
	_int32 buffer_len = 0;
	_u32 version = 0, sequence= 0 , body_len = 0, total_len = 0, left_len = 0;

     p_cmd_proxy->_had_recv_len += had_recv;
     
     LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_parse_recv_cmd, _had_recv_len:%u", 
          p_cmd_proxy, p_cmd_proxy->_had_recv_len );

     if( (p_cmd_proxy->_attribute & CMD_PROXY_HTTP_HEAD) != 0 )
     {
         http_header = sd_strstr( p_cmd_proxy->_recv_buffer, "\r\n\r\n", 0);
         if(http_header == NULL)     //没找到，继续收
         {
             sd_assert( p_cmd_proxy->_recv_buffer_len > p_cmd_proxy->_had_recv_len );
             ret_val =  socket_proxy_uncomplete_recv( p_cmd_proxy->_socket, p_cmd_proxy->_recv_buffer + p_cmd_proxy->_had_recv_len, 
                 p_cmd_proxy->_recv_buffer_len - p_cmd_proxy->_had_recv_len - 1, cmd_proxy_recv_callback, p_cmd_proxy );
             if( ret_val != SUCCESS )
             {
                 sd_assert( FALSE );
                 return cmd_proxy_handle_err( p_cmd_proxy, ret_val );
             }
             return SUCCESS;
         }
         http_header_len = http_header - p_cmd_proxy->_recv_buffer + 4;
     }
     
     LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_parse_recv_cmd, http_header_len:%u", 
          p_cmd_proxy, http_header_len );
     
     if( p_cmd_proxy->_had_recv_len - http_header_len < PROXY_CMD_HEADER_LEN )
     {
         ret_val =  socket_proxy_uncomplete_recv( p_cmd_proxy->_socket, p_cmd_proxy->_recv_buffer + p_cmd_proxy->_had_recv_len,  
             p_cmd_proxy->_recv_buffer_len - p_cmd_proxy->_had_recv_len - 1, cmd_proxy_recv_callback, p_cmd_proxy );
         if( ret_val != SUCCESS )
         {
             sd_assert( FALSE );
             return cmd_proxy_handle_err( p_cmd_proxy, ret_val );
         }
         return SUCCESS;
     }
     else
     {
         recv_buffer = p_cmd_proxy->_recv_buffer + http_header_len;
         buffer_len = p_cmd_proxy->_had_recv_len - http_header_len;
         sd_get_int32_from_lt(&recv_buffer, &buffer_len, (_int32*)&version);
         sd_get_int32_from_lt(&recv_buffer, &buffer_len, (_int32*)&sequence);
         sd_get_int32_from_lt(&recv_buffer, &buffer_len, (_int32*)&body_len);
         
         LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_parse_recv_cmd, body_len:%u", 
              p_cmd_proxy, body_len );
         
         total_len = body_len + PROXY_CMD_HEADER_LEN + http_header_len;
		if( total_len > 1024 * 1024 )
		{	/*length invalid, should be wrong protocol*/
			return cmd_proxy_handle_err(p_cmd_proxy, CMD_PROXY_EXTRACT_CMD_FAIL);
		}
         /*judge receive data's integrity*/
         if(total_len <= p_cmd_proxy->_had_recv_len)
         {   /*receive a complete protocol package*/
             ret_val = cmd_proxy_add_recv_cmd( p_cmd_proxy, p_cmd_proxy->_recv_buffer + http_header_len, total_len - http_header_len); 
             if( ret_val != SUCCESS )
             {
                sd_assert( FALSE );
                return SUCCESS;
             }
             
             left_len = p_cmd_proxy->_had_recv_len - total_len;
             if( left_len == 0 )
             {
                cmd_proxy_recv_update( p_cmd_proxy );     
             }
             else
             {
                 p_cmd_proxy->_had_recv_len = 0;
                 sd_memmove( p_cmd_proxy->_recv_buffer, p_cmd_proxy->_recv_buffer + total_len, left_len );
                 LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_parse_recv_cmd, continue parser!!!left_len:%u", 
                      p_cmd_proxy, left_len );
                 cmd_proxy_parse_recv_cmd( p_cmd_proxy, p_cmd_proxy->_recv_buffer, left_len );
             }
             return SUCCESS;
         }
         else if( total_len > p_cmd_proxy->_had_recv_len )
         {   /*receive a uncomplete package*/
             if( total_len > p_cmd_proxy->_recv_buffer_len )
             {   /*extend receive buffer*/
                 ret_val = cmd_proxy_extend_recv_buffer(p_cmd_proxy, total_len );
             }
             
             if(ret_val ==SUCCESS)
             {
                 ret_val =  socket_proxy_recv( p_cmd_proxy->_socket, p_cmd_proxy->_recv_buffer + p_cmd_proxy->_had_recv_len,
                    total_len - p_cmd_proxy->_had_recv_len, cmd_proxy_recv_callback, p_cmd_proxy );
             }
             if( ret_val != SUCCESS )
             {
                 sd_assert( FALSE );
                 return cmd_proxy_handle_err( p_cmd_proxy, ret_val );
             }
         }
     }
     return SUCCESS;

}

_int32 cmd_proxy_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
    CMD_PROXY *p_cmd_proxy = (CMD_PROXY *)user_data;
    
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_recv_callback, errcode:%d, had_recv:%u", 
        p_cmd_proxy, errcode, had_recv ); 
#ifdef REPORTER_DEBUG
    {
        char test_buffer[1024];
        sd_memset(test_buffer,0,1024);
        str2hex( (char*)buffer, had_recv, test_buffer, 1024);
        LOG_DEBUG( "cmd_proxy_add_recv_cmd before aes:*buffer[%u]=%s", 
                    had_recv, test_buffer);
        
    }
#endif /* _DEBUG */



    if( errcode == MSG_TIMEOUT )
    {
        cmd_proxy_recv_update( p_cmd_proxy );
        return SUCCESS;
    }   
    
    if( errcode == MSG_CANCELLED )
    {
        cmd_proxy_destroy( p_cmd_proxy );
        return SUCCESS;
    }    
    
    if( errcode != SUCCESS )
    {
        return cmd_proxy_handle_err( p_cmd_proxy, errcode );
    }    
    
    cmd_proxy_parse_recv_cmd( p_cmd_proxy, buffer, had_recv );
    return SUCCESS;
 }

_int32 cmd_proxy_get_old_recv_info( CMD_PROXY *p_cmd_proxy, _u32 *p_recv_cmd_id, _u32 *p_data_buffer_len )
{
    LIST_ITERATOR cur_node = NULL;
    CMD_PROXY_RECV_INFO *p_recv_info = NULL; 

    if( list_size( &p_cmd_proxy->_recv_cmd_list ) == 0 ) 
    {
        *p_recv_cmd_id = 0;
        *p_data_buffer_len = 0;
        return SUCCESS;
    }
    cur_node = LIST_BEGIN( p_cmd_proxy->_recv_cmd_list );
    p_recv_info = (CMD_PROXY_RECV_INFO *)LIST_VALUE( cur_node );

    *p_recv_cmd_id = p_recv_info->_recv_cmd_id;
    *p_data_buffer_len = p_recv_info->_cmd_len;
    
	LOG_DEBUG("cmd_proxy_get_old_recv_info, p_recv_cmd_id;%u, data_len:%u",
     *p_recv_cmd_id, *p_data_buffer_len );
    
    return SUCCESS;
}

_int32 cmd_proxy_get_recv_info( CMD_PROXY *p_cmd_proxy, _u32 recv_cmd_id, _u32 data_buffer_len, CMD_PROXY_RECV_INFO **pp_recv_info )
{
    LIST_ITERATOR cur_node = LIST_BEGIN( p_cmd_proxy->_recv_cmd_list );
    CMD_PROXY_RECV_INFO *p_recv_info = NULL; 
    
	LOG_DEBUG("cmd_proxy_get_recv_info:recv_cmd_id=%u",recv_cmd_id);
    
    while( cur_node != LIST_END( p_cmd_proxy->_recv_cmd_list ) )
    {
        p_recv_info = (CMD_PROXY_RECV_INFO *)LIST_VALUE( cur_node );
        if( p_recv_info->_recv_cmd_id == recv_cmd_id )
        {
            if( p_recv_info->_cmd_len > data_buffer_len )
            {
                return INVALID_RECV_CMD_BUFFER;
            }
            else
            {
                *pp_recv_info = p_recv_info;
                list_erase( &p_cmd_proxy->_recv_cmd_list, cur_node );
                return SUCCESS;
            }
        }
        cur_node = LIST_NEXT( cur_node );
    }
    return INVALID_RECV_CMD_ID;
}

void cmd_proxy_uninit_recv_info( CMD_PROXY_RECV_INFO *p_recv_info )
{
    SAFE_DELETE( p_recv_info->_buffer );
    sd_free( p_recv_info );
	p_recv_info = NULL;
}

_int32 cmd_proxy_try_close_socket( CMD_PROXY *p_cmd_proxy )
{
	_u32 op_count = 0;
    _int32 ret_val = SUCCESS;
    
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_try_close_socket, _socket:%d, state:%d", 
        p_cmd_proxy, p_cmd_proxy->_socket, p_cmd_proxy->_socket_state );

    if( p_cmd_proxy->_socket_state == CMD_PROXY_SOCKET_CLOSED )
    {
        sd_assert( FALSE );
        return SUCCESS;
    }

    if(p_cmd_proxy->_socket != INVALID_SOCKET)
    {
        socket_proxy_peek_op_count( p_cmd_proxy->_socket, p_cmd_proxy->_socket_type, &op_count);
        if( op_count > 0 )
        {
            if(p_cmd_proxy->_socket_state == CMD_PROXY_SOCKET_WORKING
|| p_cmd_proxy->_socket_state == CMD_PROXY_SOCKET_ERR            
                || p_cmd_proxy->_socket_state == CMD_PROXY_SOCKET_CREATING )
            {
                LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_try_close_socket, op_count:%u", 
                    p_cmd_proxy, op_count );
                ret_val = socket_proxy_cancel( p_cmd_proxy->_socket, p_cmd_proxy->_socket_type );
                sd_assert( ret_val == SUCCESS );
                cmd_proxy_enter_socket_state( p_cmd_proxy, CMD_PROXY_SOCKET_CLOSEING );
                return -1;
            }    
            else if( p_cmd_proxy->_socket_state == CMD_PROXY_SOCKET_CLOSEING )
            {
                return -1;
            }
            else
            {
                sd_assert( FALSE );
            }
        }
    }
    
   	ret_val = socket_proxy_close(p_cmd_proxy ->_socket );
    sd_assert( ret_val == SUCCESS );
	p_cmd_proxy->_socket = INVALID_SOCKET; 

    cmd_proxy_enter_socket_state( p_cmd_proxy, CMD_PROXY_SOCKET_CLOSED );
     
 	return SUCCESS;
}

_int32 cmd_proxy_final_close( CMD_PROXY *p_cmd_proxy )
{
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_final_close, _last_cmd:0x%x", 
        p_cmd_proxy, p_cmd_proxy->_last_cmd );
	if( p_cmd_proxy->_last_cmd != NULL )
	{
		cmd_proxy_free_cmd_info( p_cmd_proxy->_last_cmd );
        p_cmd_proxy->_last_cmd = NULL;
	}

	SAFE_DELETE( p_cmd_proxy->_recv_buffer );
	p_cmd_proxy->_recv_buffer = NULL;
	p_cmd_proxy->_recv_buffer_len = 0;
	sd_assert(list_size(&p_cmd_proxy->_cmd_list) == 0);
	return SUCCESS;
}

_int32 cmd_proxy_free_cmd_info( CMD_INFO *p_cmd_info )
{
    if( p_cmd_info == NULL )
    {
        sd_assert( FALSE );
        return SUCCESS;
    }
	sd_free((char*)p_cmd_info->_cmd_buffer_ptr);
	p_cmd_info->_cmd_buffer_ptr = NULL;
	sd_free( p_cmd_info );
	p_cmd_info = NULL;
    LOG_DEBUG("cmd_proxy_free_cmd_info, p_cmd_info:0x%x", p_cmd_info );
    
	return SUCCESS;
}

_int32 cmd_proxy_format_cmd( CMD_PROXY *p_cmd_proxy, CMD_INFO *p_cmd_info )
{
    _int32 ret_val = SUCCESS;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;
    _u32 buffer_len = p_cmd_info->_cmd_len;
    char *p_data_buffer = NULL;
    
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_format_cmd, p_cmd_info:0x%x, _attribute:%d,_is_format:%d, _cmd_len:%d", 
        p_cmd_proxy, p_cmd_info, p_cmd_proxy->_attribute, p_cmd_info->_is_format, p_cmd_info->_cmd_len );
    if( p_cmd_info->_is_format ) return SUCCESS;
    sd_assert( p_cmd_info->_cmd_len != 0 );
    if( (p_cmd_proxy->_attribute & CMD_PROXY_AES) != 0 )
    {
        buffer_len = ( p_cmd_info->_cmd_len + 16 - PROXY_CMD_HEADER_LEN) /16 * 16 + PROXY_CMD_HEADER_LEN;
    }

    if( (p_cmd_proxy->_attribute & CMD_PROXY_HTTP_HEAD) != 0 )
    {
        encode_len = buffer_len;
        ret_val = cmd_proxy_build_http_header(http_header, &http_header_len, 
            encode_len, p_cmd_proxy->_host, p_cmd_proxy->_port );
        CHECK_VALUE( ret_val );
    }
    else
    {
        http_header_len = 0;
    }
    
    ret_val = sd_malloc( buffer_len + http_header_len, (void**)&p_data_buffer );
    if( ret_val != SUCCESS )
    {
        LOG_ERROR("cmd_proxy_format_cmd, malloc failed.");
        CHECK_VALUE( ret_val );
    }
    if( http_header_len != 0 ) 
    {
	    sd_memcpy( p_data_buffer, http_header, http_header_len);
    }

    sd_memcpy( p_data_buffer + http_header_len, p_cmd_info->_cmd_buffer_ptr, p_cmd_info->_cmd_len );

#ifdef REPORTER_DEBUG
        {
            char test_buffer[1024];
            sd_memset(test_buffer,0,1024);
            str2hex( (char*)(p_data_buffer+ http_header_len), p_cmd_info->_cmd_len, test_buffer, 1023);
            LOG_DEBUG( "cmd_proxy_format_cmd: head_len:%u,      *buffer[%u]=%s", 
                        http_header_len, p_cmd_info->_cmd_len, test_buffer);
        }
#endif /* _DEBUG */

    if( (p_cmd_proxy->_attribute & CMD_PROXY_AES) != 0 )
    {
        ret_val = xl_aes_encrypt( p_data_buffer+http_header_len, &p_cmd_info->_cmd_len );
        if(ret_val != SUCCESS)
        {
            SAFE_DELETE( p_data_buffer );
            CHECK_VALUE( ret_val );
        } 
        
#ifdef REPORTER_DEBUG
        {
            char test_buffer[1024];
            sd_memset(test_buffer,0,1024);
            str2hex( (char*)(p_data_buffer+ http_header_len), p_cmd_info->_cmd_len, test_buffer, 1023);
            LOG_DEBUG( "cmd_proxy_format_cmd:   aes,head_len:%u,*buffer[%u]=%s", 
                        http_header_len, p_cmd_info->_cmd_len, test_buffer);
        }

#endif /* _DEBUG */

/*
#ifdef REPORTER_DEBUG
        {
            char test_cmd_buffer[1024];
            _u32 test_len = p_cmd_info->_cmd_len;
            sd_memset(test_cmd_buffer,0,1024);
            sd_memcpy( test_cmd_buffer, p_data_buffer+http_header_len, MIN(p_cmd_info->_cmd_len,1024) );
            ret_val = aes_decrypt(test_cmd_buffer, &test_len);   
            if(ret_val != SUCCESS)
            {
                sd_assert( FALSE );
            } 

            char test_buffer[1024];
            sd_memset(test_buffer,0,1024);
            str2hex( (char*)(test_cmd_buffer), test_len, test_buffer, 1023);
            LOG_DEBUG( "cmd_proxy_format_cmd:decode:head_len:%u,*buffer[%u]=%s", 
                        http_header_len, test_len, test_buffer);
        }
#endif 
*/

    }

    sd_assert( p_cmd_info->_cmd_len == buffer_len );
    p_cmd_info->_cmd_len = http_header_len + buffer_len;
    sd_free( p_cmd_info->_cmd_buffer_ptr );
    p_cmd_info->_cmd_buffer_ptr = p_data_buffer;

    p_cmd_info->_is_format = TRUE;
	return SUCCESS;
}

_int32 cmd_proxy_extend_recv_buffer( CMD_PROXY *p_cmd_proxy, _u32 total_len )
{
	char* tmp = NULL;
	_int32 ret = sd_malloc( total_len, (void**)&tmp );
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_extend_recv_buffer, total_len:%u", 
         p_cmd_proxy, total_len );
	if(ret != SUCCESS)
	{
        LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_extend_recv_buffer", p_cmd_proxy );
		CHECK_VALUE(ret);
	}
	sd_memcpy( tmp, p_cmd_proxy->_recv_buffer, p_cmd_proxy->_had_recv_len );
	sd_free( p_cmd_proxy->_recv_buffer );
	p_cmd_proxy->_recv_buffer = tmp;
	p_cmd_proxy->_recv_buffer_len = total_len;
	return SUCCESS;
}

_int32 cmd_proxy_handle_err( CMD_PROXY *p_cmd_proxy, _int32 err_code )
{
    LOG_DEBUG("[cmd_proxy=0x%x]cmd_proxy_handle_err, err:%d", p_cmd_proxy, err_code );
    cmd_proxy_enter_socket_state( p_cmd_proxy, CMD_PROXY_SOCKET_ERR );
    p_cmd_proxy->_err_code = err_code;

    //p_cmd_proxy->_cmd_handler( err_code, 0, 0, p_cmd_proxy->_user_data );
    return SUCCESS;
}

_int32 cmd_proxy_enter_socket_state( CMD_PROXY *p_cmd_proxy, CMD_PROXY_SOCKET_STATE state )
{
	LOG_DEBUG("cmd_proxy_enter_socket_state:state=%d",state);
    p_cmd_proxy->_socket_state = state;
	return SUCCESS;
}

_int32 cmd_proxy_build_http_header(char* buffer, _u32* len, _u32 data_len, char *addr, _u16 port )
{
    LOG_DEBUG("build_report_http_header,  addr= %s,port=%u", addr, port);

	*len = sd_snprintf(buffer, *len, "POST http://%s:%u/ HTTP/1.1\r\nContent-Length: %u\r\nContent-Type: application/octet-stream\r\nConnection: Close\r\n\r\n",
				addr, port, data_len );
	sd_assert(*len < 1024);

	return SUCCESS;
}

