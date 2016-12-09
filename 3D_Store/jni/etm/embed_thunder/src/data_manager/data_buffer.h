#if !defined(__DATA_BUFFER_20080704)
#define __DATA_BUFFER_20080704

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/range.h"


typedef struct _tag_buffer_node
{
     struct _tag_buffer_node* _next;
}BUFFER_NODE;


typedef  struct data_buffer_pool
{
      BUFFER_NODE* buffer_array;
      
      _u32  m_alloc_size;	  
      _u32  m_max_alloc_size;
      _u32  m_max_cache_size;	  
    
       _u32  m_block_size;
      _u32  m_min_block;
      _u32  m_max_block;  
			
}DATA_BUFFER_POOL;

_int32 get_data_buffer_cfg_parameter(void);

_int32 initialize_data_buffer_pool(DATA_BUFFER_POOL* p_data_buffer_pool,_u32 block_size,
	          _u32 min_block, _u32 max_block, _u32 max_alloc_size, _u32 max_cache_size);

_int32 unintialize_data_buffer_pool(DATA_BUFFER_POOL* p_data_buffer_pool);

_int32 dm_data_buffer_init(void);
_int32 dm_data_buffer_unint(void);

_u32 dm_get_buffer_from_data_buffer( _u32*  min_alloc_size, _int8** ptr_tobuffer);

_u32 dm_free_buffer_to_data_buffer(_u32 data_size, _int8**  ptr_to_buffer );

#ifdef __cplusplus
}
#endif

#endif /* !defined(__DATA_BUFFER_20080704)*/


