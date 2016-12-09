#if !defined(__VDM_DATA_BUFFER_20080704)
#define __VDM_DATA_BUFFER_20080704

#ifdef __cplusplus
extern "C" 
{
#endif
	
#include "utility/define.h"
#include "utility/range.h"
	
	
	typedef struct _tag_vdm_buffer_node
	{
		struct _tag_vdm_buffer_node* _next;
	}VDM_BUFFER_NODE;
	
	
	typedef  struct VDM_DATA_BUFFER_POOL
	{
		VDM_BUFFER_NODE* buffer_array;
		
		_u32  m_alloc_size;	  
		_u32  m_max_alloc_size;
		_u32  m_max_cache_size;	  
		
		_u32  m_block_size;
		_u32  m_min_block;
		_u32  m_max_block;  
		
	}VDM_DATA_BUFFER_POOL;
	
	_int32 get_vdm_data_buffer_cfg_parameter(void);
	
	_int32 initialize_vdm_data_buffer_pool(VDM_DATA_BUFFER_POOL* p_VDM_DATA_BUFFER_POOL,_u32 block_size,
		_u32 min_block, _u32 max_block, _u32 max_alloc_size, _u32 max_cache_size);
	
	_int32 unintialize_vdm_data_buffer_pool(VDM_DATA_BUFFER_POOL* p_VDM_DATA_BUFFER_POOL);
	
	_int32 vdm_data_buffer_init(void);
	_int32 vdm_data_buffer_uninit(void);
	
	_u32 vdm_get_buffer_from_data_buffer( _u32*  min_alloc_size, _int8** ptr_tobuffer);
	
	_u32 vdm_free_buffer_to_data_buffer(_u32 data_size, _int8**  ptr_to_buffer );

	_int32 vdm_get_data_buffer(VOD_DATA_MANAGER* p_data_manager_interface ,  char **data_ptr, _u32* data_len);

	_int32 vdm_free_data_buffer(VOD_DATA_MANAGER* p_data_manager_interface ,  char **data_ptr, _u32 data_len);

       _int32 vdm_reset_data_buffer_pool(void);
	   

	_int32 vdm_get_buffer_from_os(void);

	_int32 vdm_free_buffer_to_os(void);

	BOOL vdm_is_buffer_alloced(void);

	_int32 vdm_set_vod_buffer_size(_int32 buffer_size);

	_int32 vdm_get_vod_buffer_size(_int32* buffer_size);



	
#ifdef __cplusplus
}
#endif

#endif /* !defined(__VDM_DATA_BUFFER_20080704)*/


