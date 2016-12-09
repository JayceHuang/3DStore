#include "utility/define.h"
#ifdef _DK_QUERY
#ifdef ENABLE_EMULE
#include "utility/local_ip.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

#include "find_source_handler.h"
#include "dk_protocal_builder.h"
#include "dk_define.h"
#include "emule/emule_interface.h"
#include "emule/emule_utility/emule_tag_list.h"
#include "emule/emule_utility/emule_memory_slab.h"

#include "dk_socket_handler.h"
#include "dk_manager.h"
#include "dk_setting.h"

_int32 find_source_init( FIND_SOURCE_HANDLER *p_find_source_handler, _u8 *p_target, _u32 target_len, void *p_user )
{
	_int32 ret_val = SUCCESS;
    
	LOG_DEBUG( "find_source_init.p_find_source_handler:0x%x", p_find_source_handler );

    //p_find_source_handler->_protocal_handler._response_handler = find_source_res_notify_handler;   
    p_find_source_handler->_res_notify._protocal_handler._response_handler = find_source_res_notify_handler;   

	ret_val = fnh_init( KAD_TYPE, &p_find_source_handler->_find_node, p_target, target_len, KAD_FIND_STOREFILE );
	CHECK_VALUE( ret_val );

	find_node_set_response_handler( &p_find_source_handler->_find_node, find_node_wrap_handler );

	//p_find_source_handler->_user_ptr = p_user;   
	p_find_source_handler->_res_notify._user_ptr = p_user;  
	p_find_source_handler->_res_notify._find_source_ptr = p_find_source_handler;  
    
	list_init( &p_find_source_handler->_unqueryed_node_info_list );
    
	//p_find_source_handler->_file_size = 0;   
	return SUCCESS;
}

_int32 find_source_update( FIND_SOURCE_HANDLER *p_find_source_handler )
{

	LOG_DEBUG( "find_source_update.p_find_source_handler:0x%x", p_find_source_handler );
	
	fnh_update( &p_find_source_handler->_find_node );
	if( find_source_get_status( p_find_source_handler) == FIND_NODE_FINISHED )
	{
        return SUCCESS;
	}

    find_source_find( p_find_source_handler );

	return SUCCESS;
}

_int32 find_source_uninit( FIND_SOURCE_HANDLER *p_find_source_handler )
{
	LIST_ITERATOR cur_node = NULL;
	NODE_INFO *p_node_info = NULL;
	LOG_DEBUG( "find_source_uninit.p_find_source_handler:0x%x, unquery_node_info:%d", 
     p_find_source_handler, list_size(&p_find_source_handler->_unqueryed_node_info_list)  );

	fnh_uninit( &p_find_source_handler->_find_node );
	
	//p_find_source_handler->_user_ptr = NULL;
    sd_memset( &p_find_source_handler->_res_notify, 0, sizeof(FIND_SOURCE_NOTIFY) );
	//p_find_source_handler->_file_size 的值保留
	
	cur_node = LIST_BEGIN( p_find_source_handler->_unqueryed_node_info_list );
	while( cur_node != LIST_END( p_find_source_handler->_unqueryed_node_info_list ) )
	{
		p_node_info = (NODE_INFO*)LIST_VALUE( cur_node );
		kad_node_info_destoryer( p_node_info );
        
		cur_node = LIST_NEXT( cur_node );
	}
    list_clear( &p_find_source_handler->_unqueryed_node_info_list );
	sh_clear_resp_handler( sh_ptr(KAD_TYPE), &p_find_source_handler->_res_notify._protocal_handler );

	return SUCCESS;
}

_int32 find_node_wrap_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 version, void *p_data )
{
	_int32 ret_val = SUCCESS;
	FIND_SOURCE_HANDLER *p_find_source_handler = (FIND_SOURCE_HANDLER *)p_handler;

#ifdef _DEBUG
    char addr[24] = {0};
    sd_inet_ntoa( ip, addr, 24);
    LOG_DEBUG( "find_node_wrap_handler.p_handler:0x%x, ip:%s, port:%d", 
        p_handler, addr, port );
#endif

    //handle find node resp
    kad_find_node_response_handler( p_handler, ip, port, version, p_data );

    sd_assert( p_find_source_handler->_file_size != 0 );
	ret_val = find_source_add_node( p_find_source_handler, ip, port, version, NULL, 0 );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32 find_source_res_notify_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 id_para, void *p_data )
{
    _u8 *p_id = (_u8 *)id_para;
	FIND_SOURCE_NOTIFY *p_res_notify = (FIND_SOURCE_NOTIFY *)p_handler;
	FIND_SOURCE_HANDLER *p_find_source_handler = p_res_notify->_find_source_ptr;
    struct tagEMULE_TAG_LIST *p_tag_list = (struct tagEMULE_TAG_LIST *)p_data;
    
	LOG_DEBUG( "find_source_res_notify_handler.p_res_notify:0x%x", p_res_notify );
    find_source_remove_node_info( p_find_source_handler, ip, port );
    
    kad_res_nofity( p_res_notify->_user_ptr, p_id, p_tag_list );
	return SUCCESS;
}

FIND_NODE_STATUS find_source_get_status( FIND_SOURCE_HANDLER *p_find_source_handler )
{
	LOG_DEBUG( "find_source_get_status.p_find_source_handler:0x%x", p_find_source_handler );
	return fnh_get_status( &p_find_source_handler->_find_node );
}

void find_source_set_file_size( FIND_SOURCE_HANDLER *p_find_source_handler, _u64 file_size )
{
	LOG_DEBUG( "find_source_set_file_size.p_find_source_handler:0x%x, file_size:%llu", 
     p_find_source_handler, file_size );
    p_find_source_handler->_file_size = file_size;
}

_int32 find_source_find( FIND_SOURCE_HANDLER *p_find_source_handler )
{
    _int32 ret_val = SUCCESS;
    KAD_NODE_INFO *p_kad_node_info = NULL;
    NODE_INFO *p_node_info = NULL;
    DK_SOCKET_HANDLER *p_dk_socket_handler = NULL;
    _u32 find_num = 0;
    _u32 unquery_node_num = list_size( &p_find_source_handler->_unqueryed_node_info_list);
    K_NODE_ID *p_k_target_id = &p_find_source_handler->_find_node._target;
    char *p_cmd_buffer = NULL;
	_u32 buffer_len = 0;
	_u32 key = 0;
    LIST_ITERATOR cur_node = NULL;
    EMULE_TAG_LIST tag_list;

#ifdef _LOGGER
    char addr[24] = {0};
#endif	

    LOG_DEBUG( "find_source_find.p_find_source_handler:0x%x, unquery num:%d", p_find_source_handler, unquery_node_num );

    if( unquery_node_num == 0 )
    {
        return SUCCESS;
    }
    else
    {
        find_node_clear_idle_ticks( &p_find_source_handler->_find_node );
    }
    
    p_dk_socket_handler = sh_ptr( KAD_TYPE );
    while( find_num < MIN( dk_once_find_node_num(), unquery_node_num ) )
    {
        ret_val = list_pop( &p_find_source_handler->_unqueryed_node_info_list, (void **)&p_kad_node_info );
        if( ret_val != SUCCESS || p_kad_node_info == NULL )
        {
            return SUCCESS;
        }
        p_node_info = (NODE_INFO *)p_kad_node_info;
#ifdef _LOGGER
        sd_inet_ntoa(p_node_info->_peer_addr._ip, addr, 24);
        LOG_DEBUG( "find_source_find, p_find_source_handler:0x%x, p_node_info:0x%x, ip:%s, port:%u, version:%d", 
            p_find_source_handler, p_kad_node_info, addr, p_node_info->_peer_addr._port, p_kad_node_info->_version ); 
#endif	    
        
        ret_val = kad_build_find_source_cmd( p_node_info->_peer_addr._ip, p_node_info->_peer_addr._port, p_kad_node_info->_version, p_k_target_id,
            p_find_source_handler->_file_size, &p_cmd_buffer, &buffer_len, &key );
        CHECK_VALUE( ret_val );

        ret_val = sh_send_packet( sh_ptr(KAD_TYPE), p_node_info->_peer_addr._ip, p_node_info->_peer_addr._port, p_cmd_buffer, buffer_len, &p_find_source_handler->_res_notify._protocal_handler, key );
        if( ret_val != SUCCESS ) 
        {
            SAFE_DELETE( p_cmd_buffer );
            cur_node = LIST_BEGIN( p_find_source_handler->_unqueryed_node_info_list );
            list_insert( &p_find_source_handler->_unqueryed_node_info_list, (void *)p_node_info, cur_node );
            LOG_DEBUG( "find_source_find.p_find_source_handler:0x%x, sh_send_packet failed!! ", p_find_source_handler );
            return ret_val;
        }
        
         //send announce cmd
        emule_tag_list_init( &tag_list );
        ret_val = find_source_fill_announce_tag_list( p_find_source_handler, &tag_list );
        CHECK_VALUE( ret_val );       
         
        ret_val = kad_build_announce_source_cmd( p_kad_node_info->_version, p_k_target_id,
            &tag_list, &p_cmd_buffer, &buffer_len );
        CHECK_VALUE( ret_val );

        emule_tag_list_uninit( &tag_list, TRUE );
        
        ret_val = sh_send_packet( sh_ptr(KAD_TYPE), p_node_info->_peer_addr._ip, p_node_info->_peer_addr._port, p_cmd_buffer, buffer_len, NULL, 0 );
        if( ret_val != SUCCESS ) 
        {
            SAFE_DELETE( p_cmd_buffer );
        }   

        p_node_info->_try_times++;
        
        if( p_node_info->_try_times < dk_find_node_retry_times() )
        {
            ret_val = list_push( &p_find_source_handler->_unqueryed_node_info_list, (void *)p_node_info );
            if( ret_val != SUCCESS )
            {
                kad_node_info_destoryer( p_node_info );
                CHECK_VALUE( ret_val );
            }
        }   
        else
        {
#ifdef _LOGGER
            LOG_DEBUG( "find_source_remove_node_info, p_find_source_handler:0x%x, try exceed max time p_node_info:0x%x, ip:%s, port:%u, version:%d, retry_time:%u", 
                p_find_source_handler, p_node_info, addr, p_node_info->_peer_addr._port, p_kad_node_info->_version, p_node_info->_try_times ); 
#endif	    
            kad_node_info_destoryer( p_node_info );
        }

        find_num++;
    }

    return SUCCESS;
}

_int32 find_source_add_node( FIND_SOURCE_HANDLER *p_find_source_handler, _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len  )
{
	NODE_INFO *p_kad_node_info = NULL;
    _int32 ret_val = SUCCESS;

#ifdef _DEBUG
    char addr[24] = {0};
    sd_inet_ntoa(ip, addr, 24);
    LOG_DEBUG( "find_source_add_node.p_find_source_handler:0x%x, ip:%s, port:%d", 
        p_find_source_handler, addr, port );
#endif
    
	ret_val = kad_node_info_creater( ip, port, version, p_id, id_len, &p_kad_node_info );
	CHECK_VALUE( ret_val );

	ret_val = list_push( &p_find_source_handler->_unqueryed_node_info_list, (void *)p_kad_node_info );
	if( ret_val != SUCCESS )
	{
        kad_node_info_destoryer( p_kad_node_info );
		return ret_val;   
	}
    return SUCCESS;
}

_int32 find_source_remove_node_info( FIND_SOURCE_HANDLER *p_find_source_handler, _u32 net_ip, _u16 host_port )
{
    LIST_ITERATOR cur_node = LIST_BEGIN( p_find_source_handler->_unqueryed_node_info_list );
    LIST_ITERATOR next_node = NULL;

    NODE_INFO *p_node_info = NULL;
    
#ifdef _LOGGER
    KAD_NODE_INFO *p_kad_node_info = NULL;
    
    char addr[24] = {0};

    sd_inet_ntoa(net_ip, addr, 24);
	LOG_DEBUG( "find_source_remove_node_info, had response,p_find_node_handler:0x%x, ip:%s, port:%u", 
        p_find_source_handler, addr, host_port );
#endif	

    while( cur_node != LIST_END( p_find_source_handler->_unqueryed_node_info_list ) )
    {
        p_node_info = ( NODE_INFO * )LIST_VALUE( cur_node );
        next_node = LIST_NEXT( cur_node );
        
        if( p_node_info->_peer_addr._ip == net_ip
            && p_node_info->_peer_addr._port == host_port )
        {
            
#ifdef _LOGGER
            p_kad_node_info = (KAD_NODE_INFO *)p_node_info;
            LOG_DEBUG( "find_source_remove_node_info success!!!, had response,p_find_node_handler:0x%x, ip:%s, port:%u", 
                p_find_source_handler, addr, host_port, p_kad_node_info->_version );
#endif	
            kad_node_info_destoryer( p_node_info );
            list_erase( &p_find_source_handler->_unqueryed_node_info_list, cur_node );
        }
        cur_node = next_node;
    }
    return SUCCESS;
}

_int32 find_source_fill_announce_tag_list( FIND_SOURCE_HANDLER *p_find_source_handler, struct tagEMULE_TAG_LIST *p_tag_list )
{
    _int32 ret_val = SUCCESS;
    _u32 local_ip = sd_get_local_ip();
    _u8 type = 0;
    EMULE_TAG *p_tag = NULL;
    char file_size[2] =  { FT_FILESIZE, 0 };
    char source_type[2] =  { FT_SOURCETYPE, 0 };
    char source_port[2] =  { FT_SOURCEPORT, 0 };
    _u16 port = 0;//未完成
    
    if( sd_is_in_nat( local_ip ) )
    {
        
    }
    else
    {
		if ( p_find_source_handler->_file_size > OLD_MAX_EMULE_FILE_SIZE )
		{
			type = 4;
            ret_val = emule_create_u64_tag( &p_tag, file_size, p_find_source_handler->_file_size );
            CHECK_VALUE( ret_val );
            
            ret_val = emule_tag_list_add( p_tag_list, p_tag );
            if( ret_val != SUCCESS )
            {
                emule_free_emule_tag_slip( p_tag );
                return ret_val;
            }
            
            ret_val = emule_create_u8_tag( &p_tag, source_type, type );
            CHECK_VALUE( ret_val );       
            
            ret_val = emule_tag_list_add( p_tag_list, p_tag );
            if( ret_val != SUCCESS )
            {
                emule_free_emule_tag_slip( p_tag );
                return ret_val;
            }   
		}
        else
		{
			type = 1;
            
            ret_val = emule_create_u32_tag( &p_tag, file_size, (_u32)p_find_source_handler->_file_size );
            CHECK_VALUE( ret_val );
            
            ret_val = emule_tag_list_add( p_tag_list, p_tag );
            if( ret_val != SUCCESS )
            {
                emule_free_emule_tag_slip( p_tag );
                return ret_val;
            }
            
            ret_val = emule_create_u8_tag( &p_tag, source_type, type );
            CHECK_VALUE( ret_val );       
            
            ret_val = emule_tag_list_add( p_tag_list, p_tag );
            if( ret_val != SUCCESS )
            {
                emule_free_emule_tag_slip( p_tag );
                return ret_val;
            }   
		}

        ret_val = emule_create_u16_tag( &p_tag, source_port, port );
        CHECK_VALUE( ret_val );       
        
        ret_val = emule_tag_list_add( p_tag_list, p_tag );
        if( ret_val != SUCCESS )
        {
            emule_free_emule_tag_slip( p_tag );
            return ret_val;
        }   
    }
    /*
	const uint8 uSupportsCryptLayer	= local.supports_cryptlayer() ? 1 : 0;
	const uint8 uRequestsCryptLayer	= local.requests_cryptlayer() ? 1 : 0;
	const uint8 uRequiresCryptLayer	= local.requires_cryptlayer() ? 1 : 0;
	const uint8 byCryptOptions = (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0);
	tag_list.add_tag(emule_tag(FT_ENCRYPTION,byCryptOptions));
    */
    emule_tag_list_set_count_type( p_tag_list, TAG_COUNT_U8 );
    return SUCCESS;
}

#endif
#endif

