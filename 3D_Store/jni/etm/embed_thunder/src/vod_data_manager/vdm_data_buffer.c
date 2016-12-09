#include "utility/string.h"
#include "utility/utility.h"
#include "utility/url.h"
#include "utility/map.h"
#include "utility/define.h"
#include "utility/define_const_num.h"
#include "platform/sd_socket.h"
#include "platform/sd_mem.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/settings.h"
#include "asyn_frame/msg.h"
#include "data_manager/data_manager_interface.h"
#include "data_manager/data_receive.h"
#include "utility/range.h"
#include "task_manager/task_manager.h"
#include "download_task/download_task.h"

#include "vod_data_manager_interface.h"
#include "vdm_data_buffer.h"

#include "utility/logid.h"
#define LOGID LOGID_DATA_BUFFER
#include "utility/logger.h"

static _u32 MIN_BLOCK;
static _u32 MAX_BLOCK;
static _u32 MAX_CACHE_BUFFER_SIZE;
static _u32 MAX_ALLOC_BUFFER_SIZE;

static VDM_DATA_BUFFER_POOL g_vdm_data_buffer_pool;

void*  g_vdm_mem;
void* g_vdm_current_pos_mem;
_int32 g_vdm_buffer_size;

_int32 vdm_get_buffer_from_os(void)
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("vdm_get_buffer_from_os.");
#ifdef _EXTENT_MEM_FROM_OS
	ret_val = sd_get_extent_mem_from_os(get_data_unit_size()*(g_vdm_buffer_size), &g_vdm_mem);
#else
	ret_val = sd_get_mem_from_os(get_data_unit_size()*(g_vdm_buffer_size) , &g_vdm_mem);
#endif
	LOG_DEBUG("vdm_get_buffer_from_os. ret = %u size=%u", ret_val, get_data_unit_size()*(g_vdm_buffer_size));
	g_vdm_current_pos_mem = g_vdm_mem;
	CHECK_VALUE(ret_val);
	return SUCCESS;

}
_int32 vdm_free_buffer_to_os(void)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("vdm_free_buffer_to_os.");
	if (NULL != g_vdm_mem)
	{
#ifdef _EXTENT_MEM_FROM_OS
		ret_val = sd_free_extent_mem_to_os(g_vdm_mem, get_data_unit_size()*(g_vdm_buffer_size));
#else
		ret_val = sd_free_mem_to_os(g_vdm_mem, get_data_unit_size()*(g_vdm_buffer_size));
#endif
	}
	g_vdm_current_pos_mem = NULL;
	g_vdm_mem = NULL;
	return ret_val;
}

BOOL vdm_is_buffer_alloced(void)
{
	return (NULL != g_vdm_mem);
}

_int32 vdm_set_vod_buffer_size(_int32 buffer_size)
{
	if (TRUE == vdm_is_buffer_alloced())
	{
		LOG_DEBUG("vdm_set_vod_buffer_size, vod buffer allocated, cant change size.");
		return ERR_VOD_DATA_BUFFER_ALLOCATED;
	}

	if (buffer_size < 1024 * 1024 * 2)  //2M
	{
		return ERR_VOD_DATA_BUFFER_SIZE_TOO_SMALL;
	}

	g_vdm_buffer_size = buffer_size / get_data_unit_size();
	MAX_ALLOC_BUFFER_SIZE = g_vdm_buffer_size*get_data_unit_size();
	MAX_CACHE_BUFFER_SIZE = g_vdm_buffer_size*get_data_unit_size();
	return SUCCESS;
}

_int32 vdm_get_vod_buffer_size(_int32* buffer_size)
{
	LOG_DEBUG("vdm_get_vod_buffer_size, buffersize=%u.", g_vdm_buffer_size*get_data_unit_size());
	*buffer_size = g_vdm_buffer_size*get_data_unit_size();
	return SUCCESS;
}


_int32 vdm_malloc(_u32 memsize, void **mem)
{
	if (NULL == g_vdm_mem || NULL == g_vdm_current_pos_mem)
	{
		return OUT_OF_MEMORY;
	}

	if (((_u32)g_vdm_current_pos_mem + memsize - (_u32)g_vdm_mem) >= get_data_unit_size()*(g_vdm_buffer_size))
	{
		return OUT_OF_MEMORY;
	}

	LOG_DEBUG("vdm_malloc, buffersize=%u.", memsize);
	*mem = (void*)g_vdm_current_pos_mem;
	g_vdm_current_pos_mem = (void*)((_u32)g_vdm_current_pos_mem + memsize);
	return SUCCESS;
}

_int32 vdm_free(void *mem)
{
	LOG_DEBUG("vdm_free");
	sd_assert(FALSE);
	return SUCCESS;
}


_int32 initialize_vdm_data_buffer_pool(VDM_DATA_BUFFER_POOL* p_data_buffer_pool, _u32 block_size,
	_u32 min_block, _u32 max_block, _u32 max_alloc_size, _u32 max_cache_size)
{
	_int32 block_array_num = max_block - min_block + 1;
	_int32 ret_val = SUCCESS;
	_int32 i = 0;

	LOG_DEBUG("initialize_data_buffer_pool, VDM_DATA_BUFFER_POOL:0x%x,block_size:%u, min_block:%u,"
		"max_block:%u,max_alloc_size:%u, max_cache_size:%u .",
		p_data_buffer_pool, block_size, min_block, max_block, max_alloc_size, max_cache_size);

	p_data_buffer_pool->m_block_size = block_size;
	p_data_buffer_pool->m_min_block = min_block;
	p_data_buffer_pool->m_max_block = max_block;
	p_data_buffer_pool->m_max_alloc_size = max_alloc_size;
	p_data_buffer_pool->m_max_cache_size = max_cache_size;

	p_data_buffer_pool->m_alloc_size = 0;

	p_data_buffer_pool->buffer_array = 0;

	ret_val = sd_malloc(block_array_num*sizeof(VDM_BUFFER_NODE), (void **)&p_data_buffer_pool->buffer_array);
	CHECK_VALUE(ret_val);

	for (i = 0; i < block_array_num; i++)
	{
		p_data_buffer_pool->buffer_array[i]._next = NULL;
	}
	//vdm_get_buffer_from_os();

	return SUCCESS;

}

_int32 reset_vdm_data_buffer_pool(VDM_DATA_BUFFER_POOL* p_data_buffer_pool, _u32 block_size,
	_u32 min_block, _u32 max_block, _u32 max_alloc_size, _u32 max_cache_size)
{
	_int32 block_array_num = max_block - min_block + 1;
	_int32 ret_val = SUCCESS;
	_int32 i = 0;

	LOG_DEBUG("reset_vdm_data_buffer_pool, VDM_DATA_BUFFER_POOL:0x%x,block_size:%u, min_block:%u,"
		"max_block:%u,max_alloc_size:%u, max_cache_size:%u .",
		p_data_buffer_pool, block_size, min_block, max_block, max_alloc_size, max_cache_size);

	p_data_buffer_pool->m_block_size = block_size;
	p_data_buffer_pool->m_min_block = min_block;
	p_data_buffer_pool->m_max_block = max_block;
	p_data_buffer_pool->m_max_alloc_size = max_alloc_size;
	p_data_buffer_pool->m_max_cache_size = max_cache_size;

	p_data_buffer_pool->m_alloc_size = 0;

	for (i = 0; i < block_array_num; i++)
	{
		p_data_buffer_pool->buffer_array[i]._next = NULL;
	}
	//vdm_get_buffer_from_os();

	return ret_val;

}

_int32 unintialize_vdm_data_buffer_pool(VDM_DATA_BUFFER_POOL* p_data_buffer_pool)
{
	_int32 i = 0;
	VDM_BUFFER_NODE* p_cur_node = NULL;
	VDM_BUFFER_NODE* p_tmp_node = NULL;
	_int32 block_array_num = p_data_buffer_pool->m_max_block - p_data_buffer_pool->m_min_block + 1;

	LOG_DEBUG("initialize_data_buffer_pool, VDM_DATA_BUFFER_POOL:0x%x .", p_data_buffer_pool);


	for (i = 0; i < block_array_num; i++)
	{
		p_cur_node = p_data_buffer_pool->buffer_array[i]._next;
		while (p_cur_node != NULL)
		{
			p_tmp_node = p_cur_node;
			p_cur_node = p_cur_node->_next;
			//sd_free(p_tmp_node);			 
			//vdm_free(p_tmp_node);			 
		}

		p_data_buffer_pool->buffer_array[i]._next = NULL;
	}

	sd_free(p_data_buffer_pool->buffer_array);
	p_data_buffer_pool->buffer_array = NULL;


	vdm_free_buffer_to_os();

	return SUCCESS;
}

_int32 get_vdm_data_buffer_cfg_parameter(void)
{
	return SUCCESS;
}

_int32 vdm_get_data_block_num(_u32 data_size, _u32 block_size)
{
	return (data_size + block_size - 1) / block_size;
}

_int32 vdm_inc_alloc_size(VDM_DATA_BUFFER_POOL* p_data_buffer_pool, _u32 new_alloc_size)
{
	p_data_buffer_pool->m_alloc_size += new_alloc_size;

	return SUCCESS;
}

_int32 vdm_dec_alloc_size(VDM_DATA_BUFFER_POOL* p_data_buffer_pool, _u32 new_alloc_size)
{
	if (p_data_buffer_pool->m_alloc_size < new_alloc_size)
	{
		p_data_buffer_pool->m_alloc_size = 0;
	}
	else
	{
		p_data_buffer_pool->m_alloc_size -= new_alloc_size;
	}

	return SUCCESS;
}

BOOL vdm_buffer_is_full(VDM_DATA_BUFFER_POOL* p_data_buffer_pool)
{
	if (p_data_buffer_pool->m_max_alloc_size > p_data_buffer_pool->m_alloc_size)
	{
		LOG_DEBUG("buffer_is_full: false.");
		return FALSE;
	}
	else
	{
		LOG_DEBUG("buffer_is_full: true.");
		return TRUE;
	}
}

BOOL vdm_cache_is_full(VDM_DATA_BUFFER_POOL* p_data_buffer_pool)
{
	if (p_data_buffer_pool->m_max_cache_size > p_data_buffer_pool->m_alloc_size)
	{
		LOG_DEBUG("cache_is_full: false.");
		return FALSE;
	}
	else
	{
		LOG_DEBUG("cache_is_full: true.");
		return TRUE;
	}
}



_int32 vdm_data_buffer_init(void)
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("vdm_data_buffer_init.");

	MIN_BLOCK = 1;
	MAX_BLOCK = 10;
	MAX_CACHE_BUFFER_SIZE = VOD_DATA_BUFFER_MAX_ALLOC_BUFFER;
	MAX_ALLOC_BUFFER_SIZE = VOD_DATA_BUFFER_MAX_ALLOC_BUFFER;
	g_vdm_mem = NULL;
	g_vdm_current_pos_mem = NULL;
	g_vdm_buffer_size = (VOD_MEM_BUFFER + VOD_EXTEND_BUFFER)*(4 * 1024) / get_data_unit_size();

	ret_val = initialize_vdm_data_buffer_pool(&g_vdm_data_buffer_pool, get_data_unit_size(), MIN_BLOCK, MAX_BLOCK
		, MAX_ALLOC_BUFFER_SIZE, MAX_CACHE_BUFFER_SIZE);
	return ret_val;

}
_int32 vdm_data_buffer_uninit(void)
{
	LOG_DEBUG("vdm_data_buffer_uninit.");
	return unintialize_vdm_data_buffer_pool(&g_vdm_data_buffer_pool);
}

_u32 vdm_get_buffer_from_data_buffer(_u32*  min_alloc_size, _int8** ptr_tobuffer)
{
	_u32  min_alloc_len = *min_alloc_size;
	_u32 index = 0;

	_int32 ret_val = SUCCESS;
	VDM_BUFFER_NODE* p_cur_node = NULL;

	_u32 block_num = vdm_get_data_block_num(min_alloc_len, g_vdm_data_buffer_pool.m_block_size);

	*ptr_tobuffer = NULL;

	LOG_DEBUG("vdm_get_buffer_from_data_buffer.");

	if (block_num<g_vdm_data_buffer_pool.m_min_block || block_num > g_vdm_data_buffer_pool.m_max_block)
	{
		LOG_ERROR("vdm_get_buffer_from_data_buffer, block_num:%u is invalid.", block_num);
		return ALLOC_INVALID_SIZE;
	}

	index = block_num - g_vdm_data_buffer_pool.m_min_block;

	p_cur_node = g_vdm_data_buffer_pool.buffer_array[index]._next;
	if (p_cur_node != NULL)
	{
		g_vdm_data_buffer_pool.buffer_array[index]._next = p_cur_node->_next;
		*ptr_tobuffer = (_int8*)p_cur_node;

		LOG_DEBUG("vdm_get_buffer_from_data_buffer.ret cached buffer:0x%x .", p_cur_node);

	}
	else
	{
		if (vdm_buffer_is_full(&g_vdm_data_buffer_pool) == TRUE)
		{
			LOG_ERROR("vdm_get_buffer_from_data_buffer, cache is full block_num:%u can not alloc.", block_num);

			return DATA_BUFFER_IS_FULL;
		}

		//ret_val = sd_malloc(block_num*(g_vdm_data_buffer_pool.m_block_size), (void**)ptr_tobuffer);   
		ret_val = vdm_malloc(block_num*(g_vdm_data_buffer_pool.m_block_size), (void**)ptr_tobuffer);
		//CHECK_VALUE(ret_val);
		if (ret_val != SUCCESS)
		{
			LOG_ERROR("vdm_get_buffer_from_data_buffer, block_num:%u blocks alloc failure.", block_num);
			return ret_val;
		}
		LOG_DEBUG("vdm_get_buffer_from_data_buffer.malloc buffer:0x%x, block_num:%u .",
			*ptr_tobuffer, block_num);

		vdm_inc_alloc_size(&g_vdm_data_buffer_pool, (block_num*(g_vdm_data_buffer_pool.m_block_size)));
	}

	*min_alloc_size = block_num*g_vdm_data_buffer_pool.m_block_size;

	LOG_DEBUG("vdm_get_buffer_from_data_buffer.ret buffer:0x%x, length:%u .",
		*ptr_tobuffer, *min_alloc_size);

	return SUCCESS;
}

_u32 vdm_free_buffer_to_data_buffer(_u32 data_size, _int8**  ptr_to_buffer)
{
	_u32 index = 0;
	VDM_BUFFER_NODE* p_cur_node = NULL;
	_u32 block_num = vdm_get_data_block_num(data_size, g_vdm_data_buffer_pool.m_block_size);

	LOG_DEBUG("vdm_free_buffer_to_data_buffer.");

	if (block_num<g_vdm_data_buffer_pool.m_min_block || block_num > g_vdm_data_buffer_pool.m_max_block)
	{
		LOG_ERROR("vdm_free_buffer_to_data_buffer, block_num:%u is invalid.", block_num);
		return ALLOC_INVALID_SIZE;
	}
	if (vdm_cache_is_full(&g_vdm_data_buffer_pool) == TRUE)
	{
		LOG_DEBUG("vdm_free_buffer_to_data_buffer.  buffer:0x%x  length:%u block_num:%u is free os..",
			*ptr_to_buffer, data_size, block_num);

		//sd_free(*ptr_to_buffer);
		vdm_free(*ptr_to_buffer);
		vdm_dec_alloc_size(&g_vdm_data_buffer_pool, (block_num*(g_vdm_data_buffer_pool.m_block_size)));
	}
	else
	{
		index = block_num - g_vdm_data_buffer_pool.m_min_block;
		p_cur_node = g_vdm_data_buffer_pool.buffer_array[index]._next;

		((VDM_BUFFER_NODE*)(*ptr_to_buffer))->_next = p_cur_node;
		g_vdm_data_buffer_pool.buffer_array[index]._next = (VDM_BUFFER_NODE*)(*ptr_to_buffer);

		LOG_DEBUG("vdm_free_buffer_to_data_buffer.  buffer:0x%x  length:%u block_num:%u is free to cache..",
			*ptr_to_buffer, data_size, block_num);
	}

	*ptr_to_buffer = NULL;
	return SUCCESS;
}

_int32 vdm_get_data_buffer(VOD_DATA_MANAGER* p_data_manager_interface, char **data_ptr, _u32* data_len)
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("vdm_get_data_buffer .");

	ret_val = vdm_get_buffer_from_data_buffer(data_len, data_ptr);

	if (ret_val == SUCCESS)
	{
		LOG_DEBUG("vdm_get_data_buffer,  data_buffer:0x%x, data_len:%u.", *data_ptr, *data_len);
	}
	else if (ret_val == DATA_BUFFER_IS_FULL || ret_val == OUT_OF_MEMORY)
	{
		LOG_DEBUG("VDM_DATA_MANAGER : 0x%x  , dm_get_data_buffer failue, so flush data,  data_buffer:0x%x, data_len:%u.",
			p_data_manager_interface, *data_ptr, *data_len);

	}

	return ret_val;


}

_int32 vdm_free_data_buffer(VOD_DATA_MANAGER* p_data_manager_interface, char **data_ptr, _u32 data_len)
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("vdm_free_data_buffer,  data_buffer:0x%x, data_len:%u.", *data_ptr, data_len);

	ret_val = vdm_free_buffer_to_data_buffer(data_len, data_ptr);

	return (ret_val);

}


_int32 vdm_reset_data_buffer_pool(void)
{

	_int32 ret_val = SUCCESS;

	ret_val = reset_vdm_data_buffer_pool(&g_vdm_data_buffer_pool, get_data_unit_size(), MIN_BLOCK, MAX_BLOCK
		, MAX_ALLOC_BUFFER_SIZE, MAX_CACHE_BUFFER_SIZE);
	return ret_val;
}

