#include "utility/define.h"

//#ifndef XUNLEI_MODE
#include "file_manager.h"
#include "utility/define_const_num.h"

static SLAB* g_tmp_file_slab = NULL;
static SLAB* g_msg_file_create_para_slab = NULL;
static SLAB* g_msg_file_rw_para_slab = NULL;
static SLAB* g_msg_file_close_para_slab = NULL;
static SLAB* g_block_data_buffer_slab = NULL;
static SLAB* g_range_data_buffer_list_slab = NULL;


//tmp_file malloc/free wrap
_int32 init_tmp_file_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_tmp_file_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( TMP_FILE ), MIN_TMP_FILE, 0, &g_tmp_file_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_tmp_file_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_tmp_file_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_tmp_file_slab );
		CHECK_VALUE( ret_val );
		g_tmp_file_slab = NULL;
	}
	return ret_val;
}

_int32 tmp_file_malloc_wrap(TMP_FILE **pp_node)
{
	_int32 ret_val = SUCCESS;
	ret_val =  mpool_get_slip( g_tmp_file_slab, (void**)pp_node );
	CHECK_VALUE( ret_val );
	sd_memset(*pp_node,0x00,sizeof(TMP_FILE));
	return SUCCESS;
}

_int32 tmp_file_free_wrap(TMP_FILE *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_tmp_file_slab, (void*)p_node );
}


//msg_file_create_para malloc/free wrap
_int32 init_msg_file_create_para_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_msg_file_create_para_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( MSG_FILE_CREATE_PARA ), MIN_MSG_FILE_CREATE_PARA, 0, &g_msg_file_create_para_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_msg_file_create_para_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_msg_file_create_para_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_msg_file_create_para_slab );
		CHECK_VALUE( ret_val );
		g_msg_file_create_para_slab = NULL;
	}
	return ret_val;
}

_int32 msg_file_create_para_malloc_wrap(MSG_FILE_CREATE_PARA **pp_node)
{
	return mpool_get_slip( g_msg_file_create_para_slab, (void**)pp_node );
}

_int32 msg_file_create_para_free_wrap(MSG_FILE_CREATE_PARA *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_msg_file_create_para_slab, (void*)p_node );
}



//msg_file_rw_para_slab malloc/free wrap
_int32 init_msg_file_rw_para_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_msg_file_rw_para_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( MSG_FILE_RW_PARA ), MIN_MSG_FILE_RW_PARA, 0, &g_msg_file_rw_para_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_msg_file_rw_para_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_msg_file_rw_para_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_msg_file_rw_para_slab );
		CHECK_VALUE( ret_val );
		g_msg_file_rw_para_slab = NULL;
	}
	return ret_val;
}

_int32 msg_file_rw_para_slab_malloc_wrap(MSG_FILE_RW_PARA **pp_node)
{
	_int32 ret_val = SUCCESS;
	ret_val =  mpool_get_slip( g_msg_file_rw_para_slab, (void**)pp_node );
	CHECK_VALUE( ret_val );
	sd_memset(*pp_node,0x00,sizeof(MSG_FILE_RW_PARA));
	return SUCCESS;
}

_int32 msg_file_rw_para_slab_free_wrap(MSG_FILE_RW_PARA *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_msg_file_rw_para_slab, (void*)p_node );
}



//msg_file_close_para malloc/free wrap
_int32 init_msg_file_close_para_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_msg_file_close_para_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( MSG_FILE_CLOSE_PARA ), MIN_MSG_FILE_CLOSE_PARA, 0, &g_msg_file_close_para_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_msg_file_close_para_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_msg_file_close_para_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_msg_file_close_para_slab );
		CHECK_VALUE( ret_val );
		g_msg_file_close_para_slab = NULL;
	}
	return ret_val;
}

_int32 msg_file_close_para_malloc_wrap(MSG_FILE_CLOSE_PARA **pp_node)
{
	return mpool_get_slip( g_msg_file_close_para_slab, (void**)pp_node );
}

_int32 msg_file_close_para_free_wrap(MSG_FILE_CLOSE_PARA *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_msg_file_close_para_slab, (void*)p_node );
}



//block_data_buffer malloc/free wrap
_int32 init_block_data_buffer_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_block_data_buffer_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BLOCK_DATA_BUFFER ), MIN_BLOCK_DATA_BUFFER, 0, &g_block_data_buffer_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_block_data_buffer_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_block_data_buffer_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_block_data_buffer_slab );
		CHECK_VALUE( ret_val );
		g_block_data_buffer_slab = NULL;
	}
	return ret_val;
}

_int32 block_data_buffer_malloc_wrap(BLOCK_DATA_BUFFER **pp_node)
{
 	_int32 ret_val = SUCCESS;
	ret_val =  mpool_get_slip( g_block_data_buffer_slab, (void**)pp_node );
	CHECK_VALUE( ret_val );
	sd_memset(*pp_node,0x00,sizeof(BLOCK_DATA_BUFFER));
	return SUCCESS;
}

_int32 block_data_buffer_free_wrap(BLOCK_DATA_BUFFER *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_block_data_buffer_slab, (void*)p_node );
}


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
	return mpool_get_slip( g_range_data_buffer_list_slab, (void**)pp_node );
}

_int32 range_data_buffer_list_free_wrap(RANGE_DATA_BUFFER_LIST *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_range_data_buffer_list_slab, (void*)p_node );
}

//#endif
