#include "utility/define.h"
//#ifdef XUNLEI_MODE

#include "file_manager.h"
#include "file_manager_interface_xl.h"
#include "utility/settings.h"

#include "utility/logid.h"
#define LOGID LOGID_FILE_MANAGER
#include "utility/logger.h"
#include "utility/define_const_num.h"

static SLAB* g_rw_data_buffer_slab = NULL;
static _u32 g_fm_open_file_timeout = WAIT_INFINITE;
static _u32 g_fm_rw_file_timeout = WAIT_INFINITE;//15000;
static _u32 g_fm_close_file_timeout = WAIT_INFINITE;

static _u32 g_fm_max_merge_range_num  = 128;

//rw_data_buffer malloc/free wrap
_int32 init_rw_data_buffer_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_rw_data_buffer_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( RW_DATA_BUFFER ), MIN_RW_DATA_BUFFER, 0, &g_rw_data_buffer_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_rw_data_buffer_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_rw_data_buffer_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_rw_data_buffer_slab );
		CHECK_VALUE( ret_val );
		g_rw_data_buffer_slab = NULL;
	}
	return ret_val;
}

_int32 rw_data_buffer_malloc_wrap(RW_DATA_BUFFER **pp_node)
{
 	return mpool_get_slip( g_rw_data_buffer_slab, (void**)pp_node );
}

_int32 rw_data_buffer_free_wrap(RW_DATA_BUFFER *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_rw_data_buffer_slab, (void*)p_node );
}

#if 0
//range_data_buffer_list malloc/free wrap
_int32 init_range_data_buffer_list_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_range_data_buffer_list_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( RANGE_DATA_BUFFER_LIST ), MIN_RANGE_DATA_BUFFER_LIST, 0, &g_range_data_buffer_list_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_range_data_buffer_list_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_range_data_buffer_list_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_range_data_buffer_list_slab );
		CHECK_VALUE( ret_val );
		g_range_data_buffer_list_slab = NULL;
	}
	return ret_val;
}

_int32 range_data_buffer_list_malloc_wrap(RANGE_DATA_BUFFER_LIST **pp_node)
{
	_int32 ret_val = mpool_get_slip( g_range_data_buffer_list_slab, (void**)pp_node );
	CHECK_VALUE( ret_val );
	LOG_DEBUG( "range_data_buffer_list_malloc_wrap:0x%x ", *pp_node );			
	return SUCCESS;
}

_int32 range_data_buffer_list_free_wrap(RANGE_DATA_BUFFER_LIST *p_node)
{
	_int32 ret_val = SUCCESS;
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	LOG_DEBUG( "range_data_buffer_list_free_wrap:0x%x ", p_node );		
	ret_val = mpool_free_slip( g_range_data_buffer_list_slab, (void*)p_node );
	CHECK_VALUE( ret_val );	
	
	return SUCCESS;
}
#endif
void fm_init_setting(void)
{	
	settings_get_int_item( "file_manager.open_file_timeout", (_int32*)&g_fm_open_file_timeout );
	// NOTE:
	// 读写在嵌入式设备上可能确实很慢，遇到问题虽然写成功，但是消息回调处理时超时
	// 仍然会导致任务失败，因此这里默认将其设置为不超时，不过这也可能导致隐患，后续观察
	//settings_get_int_item( "file_manager.rw_file_timeout", (_int32*)&g_fm_rw_file_timeout );
	settings_get_int_item( "file_manager.close_file_timeout", (_int32*)&g_fm_close_file_timeout );
    settings_get_int_item("file_manager.max_merge_range_num", (_int32*)&g_fm_max_merge_range_num);
	LOG_DEBUG("file_manager.max_merge_range_num:%u, file_manager.open_file_timeout = %u, \
              file_manager.rw_file_timeout = %u, file_manager.close_file_timeout = %u"
        , g_fm_max_merge_range_num, g_fm_open_file_timeout, g_fm_rw_file_timeout, g_fm_close_file_timeout);	

}

_u32 fm_open_file_timeout(void)  
{
	return g_fm_open_file_timeout;
}

_u32 fm_rw_file_timeout(void)  
{
	return g_fm_rw_file_timeout;
}

_u32 fm_close_file_timeout(void)  
{
	return g_fm_close_file_timeout;
}

_u32 fm_max_merge_range_num(void)  
{
	return g_fm_max_merge_range_num;
}


//#endif
