#include "utility/errcode.h"
#include "utility/define_const_num.h"
#include "utility/rw_sharebrd.h"
#include "utility/mempool.h"

#include "utility/logid.h"
#define LOGID LOGID_CMD_INTERFACE
#include "utility/logger.h"
#include "cmd_proxy.h"
#include "asyn_frame/device.h"
#include "utility/peerid.h"

#include "cmd_proxy_manager.h"

static CMD_PROXY_MANAGER g_cmd_proxy_manager;
#define CMD_PROXY_MAX_WAIRING_NUM 50

_int32 init_cmd_proxy_module( void )
{
    LOG_DEBUG("init_cmd_proxy_module" );
    list_init( &g_cmd_proxy_manager._cmd_proxy_list );
    return SUCCESS;
}

_int32 uninit_cmd_proxy_module( void )
{
    CMD_PROXY *p_cmd_proxy = NULL;
    LIST_ITERATOR cur_node ;
    
    LOG_DEBUG("uninit_cmd_proxy_module" );

    cur_node = LIST_BEGIN(g_cmd_proxy_manager._cmd_proxy_list);
    while( cur_node != LIST_END(g_cmd_proxy_manager._cmd_proxy_list) )
    {
        p_cmd_proxy = (CMD_PROXY *)LIST_VALUE(cur_node);
        cmd_proxy_destroy(p_cmd_proxy);
        cur_node = LIST_NEXT( cur_node );
    }

    list_clear( &g_cmd_proxy_manager._cmd_proxy_list );
    return SUCCESS;
}


_int32 pm_create_cmd_proxy( void *p_param )
{
    _int32 ret_val = SUCCESS;
    PM_CREATE_CMD_PROXY *cpm_param = (PM_CREATE_CMD_PROXY *)p_param;
    CMD_PROXY *p_cmd_proxy = NULL;
    
    ret_val = cmd_proxy_create( DEVICE_SOCKET_TCP, cpm_param->_attribute, &p_cmd_proxy );
    if( ret_val != SUCCESS )
    {
        cpm_param->_result = ret_val;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }
    p_cmd_proxy->_ip = cpm_param->_ip;
    p_cmd_proxy->_port = cpm_param->_port;
    p_cmd_proxy->_host = NULL;

    ret_val = list_push( &g_cmd_proxy_manager._cmd_proxy_list, p_cmd_proxy );
    if( ret_val != SUCCESS )
    {
        cmd_proxy_destroy( p_cmd_proxy );
        cpm_param->_result = ret_val;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }
    
    LOG_DEBUG("pm_create_cmd_proxy, id:%u", p_cmd_proxy->_proxy_id );

    cpm_param->_result = SUCCESS;
    *cpm_param->_cmd_proxy_id_ptr = p_cmd_proxy->_proxy_id;
    return signal_sevent_handle(&(cpm_param->_handle));
}


_int32 pm_create_cmd_proxy_by_domain( void *p_param )
{
    _int32 ret_val = SUCCESS;
    PM_CREATE_CMD_PROXY_BY_DOMAIN *cpm_param = (PM_CREATE_CMD_PROXY_BY_DOMAIN *)p_param;
    CMD_PROXY *p_cmd_proxy = NULL;

    ret_val = cmd_proxy_create( DEVICE_SOCKET_TCP, cpm_param->_attribute, &p_cmd_proxy);
    if( ret_val != SUCCESS )
    {
        cpm_param->_result = ret_val;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }

    p_cmd_proxy->_port = cpm_param->_port;

    if( sd_strlen(cpm_param->_host) > MAX_HOST_NAME_LEN )
    {
        cmd_proxy_destroy( p_cmd_proxy );
        cpm_param->_result = HOST_TOO_LENG;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }

    ret_val = sd_malloc( sd_strlen(cpm_param->_host)+1, (void**)&p_cmd_proxy->_host );
    if( ret_val != SUCCESS )
    {
        cmd_proxy_destroy( p_cmd_proxy );
        cpm_param->_result = ret_val;
        return signal_sevent_handle(&(cpm_param->_handle));
    }
    
    sd_memset( p_cmd_proxy->_host, 0, sd_strlen(cpm_param->_host)+1 );
    sd_memcpy(p_cmd_proxy->_host, cpm_param->_host, sd_strlen(cpm_param->_host));

    ret_val = list_push( &g_cmd_proxy_manager._cmd_proxy_list, p_cmd_proxy );
    if( ret_val != SUCCESS )
    {
        cmd_proxy_destroy( p_cmd_proxy );
        cpm_param->_result = ret_val;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }
    LOG_DEBUG("pm_create_cmd_proxy_by_domain, id:%u", p_cmd_proxy->_proxy_id );

    cpm_param->_result = SUCCESS;
    *cpm_param->_cmd_proxy_id_ptr = p_cmd_proxy->_proxy_id;
    return signal_sevent_handle(&(cpm_param->_handle));
    
}

_int32 pm_cmd_proxy_get_info( void *p_param )
{
    _int32 ret_val = SUCCESS;
    PM_CMD_PROXY_GET_INFO *cpm_param = (PM_CMD_PROXY_GET_INFO *)p_param;
    CMD_PROXY *p_cmd_proxy = NULL;
        
    pm_get_cmd_proxy( cpm_param->_cmd_proxy_id, &p_cmd_proxy, FALSE );
    
    LOG_DEBUG("pm_cmd_proxy_get_info, id:%u, p_cmd_proxy:0x%x", 
        cpm_param->_cmd_proxy_id, p_cmd_proxy );

    if( p_cmd_proxy == NULL ) 
    {
        cpm_param->_result = INVALID_CMD_PROXY_ID;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }

    if( p_cmd_proxy->_err_code != SUCCESS )
    {
        cpm_param->_result = SUCCESS;
        *cpm_param->_errcode = p_cmd_proxy->_err_code;
        *cpm_param->_recv_cmd_id = 0;
        *cpm_param->_data_len = 0;
        
        return signal_sevent_handle( &(cpm_param->_handle) );
    }
    
    ret_val = cmd_proxy_get_old_recv_info( p_cmd_proxy, cpm_param->_recv_cmd_id, cpm_param->_data_len );
    if( ret_val != SUCCESS ) 
    {
        cpm_param->_result = ret_val;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }

    return signal_sevent_handle( &(cpm_param->_handle) );
}


_int32 pm_cmd_proxy_send( void *p_param )
{
    _int32 ret_val = SUCCESS;
    PM_CMD_PROXY_SEND *cpm_param = (PM_CMD_PROXY_SEND *)p_param;
    CMD_PROXY *p_cmd_proxy = NULL;
    CMD_INFO *p_cmd_info = NULL;
    _u32 size = 0;
    

    pm_get_cmd_proxy( cpm_param->_cmd_proxy_id, &p_cmd_proxy, FALSE );

    if( p_cmd_proxy == NULL ) 
    {
        cpm_param->_result = INVALID_CMD_PROXY_ID;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }

	size = list_size( &p_cmd_proxy->_cmd_list );

    LOG_DEBUG("pm_cmd_proxy_send, id:%u, p_cmd_proxy:0x%x, cmd_len:%u, list_size:%d", 
        cpm_param->_cmd_proxy_id, p_cmd_proxy, cpm_param->_len, size );
    
    if( size  > CMD_PROXY_MAX_WAIRING_NUM )
    {
    	//sd_assert( FALSE );
        ret_val = SEND_CMD_FULL;
        goto ErrHandler1;
    }

    ret_val = sd_malloc( sizeof(CMD_INFO), (void**)&p_cmd_info );
    if( ret_val != SUCCESS )
    {
        goto ErrHandler1;

    }
    sd_memset( p_cmd_info, 0, sizeof(CMD_INFO) );
    
    ret_val = sd_malloc( cpm_param->_len, (void**)&p_cmd_info->_cmd_buffer_ptr );
    if( ret_val != SUCCESS )
    {
        goto ErrHandler0;
    }
    
    sd_memcpy( p_cmd_info->_cmd_buffer_ptr, cpm_param->_buffer, cpm_param->_len );
    p_cmd_info->_cmd_len = cpm_param->_len;

    ret_val = list_push( &p_cmd_proxy->_cmd_list, p_cmd_info );
    if( ret_val != SUCCESS )
    {
        cmd_proxy_free_cmd_info( p_cmd_info );
        cpm_param->_result = ret_val;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }


    if( p_cmd_proxy->_last_cmd == NULL )
    {
        cmd_proxy_send_update( p_cmd_proxy );
    }
    cpm_param->_result = SUCCESS;
    
    return signal_sevent_handle(&(cpm_param->_handle));

ErrHandler0:
    sd_free( p_cmd_info );
	p_cmd_info = NULL;

ErrHandler1:
    cpm_param->_result = ret_val;
    return signal_sevent_handle( &(cpm_param->_handle) );    
}


_int32 pm_cmd_proxy_get_recv_info( void *p_param )
{
    _int32 ret_val = SUCCESS;
    PM_CMD_PROXY_GET_RECV_INFO *cpm_param = (PM_CMD_PROXY_GET_RECV_INFO *)p_param;
    CMD_PROXY *p_cmd_proxy = NULL;
    CMD_PROXY_RECV_INFO *p_recv_info = NULL;
        
    pm_get_cmd_proxy( cpm_param->_cmd_proxy_id, &p_cmd_proxy, FALSE );
    
    LOG_DEBUG("pm_cmd_proxy_get_recv_info, id:%u, p_cmd_proxy:0x%x", 
        cpm_param->_cmd_proxy_id, p_cmd_proxy );

    if( p_cmd_proxy == NULL ) 
    {
        cpm_param->_result = INVALID_CMD_PROXY_ID;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }
    
    ret_val = cmd_proxy_get_recv_info( p_cmd_proxy, cpm_param->_recv_cmd_id, cpm_param->_data_buffer_len, &p_recv_info );

    if( ret_val != SUCCESS ) 
    {
        cpm_param->_result = ret_val;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }

    sd_assert( p_recv_info != NULL );
    
    cpm_param->_result = SUCCESS;
    sd_memcpy( cpm_param->_data_buffer, p_recv_info->_buffer, p_recv_info->_cmd_len );
    cpm_param->_data_buffer_len = p_recv_info->_cmd_len;
    
    cmd_proxy_uninit_recv_info( p_recv_info );
   
    return signal_sevent_handle( &(cpm_param->_handle) );
}

_int32 pm_cmd_proxy_close( void *p_param )
{
    _int32 ret_val = SUCCESS;
    PM_CMD_PROXY_CLOSE *cpm_param = (PM_CMD_PROXY_CLOSE *)p_param;
    CMD_PROXY *p_cmd_proxy = NULL;
        
    pm_get_cmd_proxy( cpm_param->_cmd_proxy_id, &p_cmd_proxy, TRUE );
    
    LOG_DEBUG("pm_cmd_proxy_close, id:%u, p_cmd_proxy:0x%x", 
        cpm_param->_cmd_proxy_id, p_cmd_proxy );

    if( p_cmd_proxy == NULL ) 
    {
        cpm_param->_result = INVALID_CMD_PROXY_ID;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }
    
    ret_val = cmd_proxy_destroy( p_cmd_proxy );
    if( ret_val != SUCCESS ) 
    {
        cpm_param->_result = ret_val;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }
    
    cpm_param->_result = SUCCESS;
    return signal_sevent_handle( &(cpm_param->_handle) );
    
}

void pm_get_cmd_proxy( _u32 cmd_proxy_id, struct tagCMD_PROXY **pp_cmd_proxy, BOOL is_remove )
{
    LIST_ITERATOR cur_node = LIST_BEGIN(g_cmd_proxy_manager._cmd_proxy_list);
    CMD_PROXY *p_list_cmd_proxy = NULL;
    
    LOG_DEBUG("pm_get_cmd_proxy, id:%u, is_remove:%d", 
        cmd_proxy_id, is_remove );
    
    *pp_cmd_proxy = NULL;
    while( cur_node != LIST_END(g_cmd_proxy_manager._cmd_proxy_list) )
    {
        p_list_cmd_proxy = (CMD_PROXY *)LIST_VALUE(cur_node);
        if( cmd_proxy_id == p_list_cmd_proxy->_proxy_id )
        {
            *pp_cmd_proxy = p_list_cmd_proxy;
            if( is_remove )
            {
                list_erase( &g_cmd_proxy_manager._cmd_proxy_list, cur_node );
            }
            break;
        }
        cur_node = LIST_NEXT( cur_node );
    }
}

_int32 pm_get_peer_id( void *p_param )
{
    _int32 ret_val = SUCCESS;
    PM_GET_PEER_ID *cpm_param = (PM_GET_PEER_ID *)p_param;
    LOG_DEBUG("pm_get_peer_id" );
    
    ret_val = get_peerid( cpm_param->_buffer, cpm_param->_buffer_len );
    if( ret_val != SUCCESS ) 
    {
        cpm_param->_result = ret_val;
        return signal_sevent_handle( &(cpm_param->_handle) );
    }
    
    cpm_param->_result = SUCCESS;
    return signal_sevent_handle( &(cpm_param->_handle) );
}
