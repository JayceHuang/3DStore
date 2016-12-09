#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_tag_list.h"
#include "emule_memory_slab.h"
#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/string.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_tag_comparator(void *E1, void *E2)
{
	EMULE_TAG* left = (EMULE_TAG*)E1;
	EMULE_TAG* right = (EMULE_TAG*)E2;
	return sd_strcmp(left->_name, right->_name);
}

void emule_tag_list_init(EMULE_TAG_LIST* tag_list)
{
    tag_list->_count_type = TAG_COUNT_U32; 
	set_init(&tag_list->_emule_tag_set, emule_tag_comparator);
}

void emule_tag_list_set_count_type( EMULE_TAG_LIST* tag_list, EMULE_COUNT_TYPE count_type )
{
    tag_list->_count_type = count_type; 
}

void emule_tag_list_uninit(EMULE_TAG_LIST* tag_list, BOOL free_tag)
{
	EMULE_TAG* tag = NULL;
	SET_ITERATOR cur_iter, next_iter; 
	LOG_DEBUG("emule_tag_list_clear, tag_list = 0x%x", tag_list);
	cur_iter = SET_BEGIN(tag_list->_emule_tag_set);
	while(cur_iter != SET_END(tag_list->_emule_tag_set))
	{
		next_iter = SET_NEXT(tag_list->_emule_tag_set, cur_iter);
		if(free_tag)
		{
			tag = (EMULE_TAG*)SET_DATA(cur_iter);
			emule_destroy_tag(tag);
		}
		set_erase_iterator(&tag_list->_emule_tag_set, cur_iter);
		cur_iter = next_iter;
		tag = NULL;
	}
}

_int32 emule_tag_list_add(EMULE_TAG_LIST* tag_list, EMULE_TAG* tag)
{
	return set_insert_node(&tag_list->_emule_tag_set, tag);
}

const EMULE_TAG* emule_tag_list_get(EMULE_TAG_LIST* tag_list, char* tag_name)
{
	EMULE_TAG* tag = NULL;
	set_find_node(&tag_list->_emule_tag_set, (void*)tag_name, (void**)&tag);
	return tag;
}


//该函数获得把tag_list存到buffer中需要的buffer_size大小
_u32 emule_tag_list_size(EMULE_TAG_LIST* tag_list)
{
	_u32 len = 0;
	SET_ITERATOR iter;
	EMULE_TAG* tag = NULL;
	if(set_size(&tag_list->_emule_tag_set) == 0)
		return 0;
	len += sizeof(_u32);		//前面是四个字节的tag num
	for(iter = SET_BEGIN(tag_list->_emule_tag_set); iter != SET_END(tag_list->_emule_tag_set); iter = SET_NEXT(tag_list->_emule_tag_set, iter))
	{
		tag = SET_DATA(iter);
		len += emule_tag_size(tag);
	}
	return len;
}

_int32 emule_tag_list_to_buffer(EMULE_TAG_LIST* tag_list, char** buffer, _int32* len)
{
	_int32 ret = SUCCESS;
	EMULE_TAG* tag = NULL;
	SET_ITERATOR iter;
	sd_assert(*len >= emule_tag_list_size(tag_list));
    if( tag_list->_count_type == TAG_COUNT_U32 )
    {
        ret = sd_set_int32_to_lt(buffer, len, set_size(&tag_list->_emule_tag_set));
    }
    else if( tag_list->_count_type == TAG_COUNT_U8 )
    {
        ret = sd_set_int8(buffer, len, (_u8)set_size(&tag_list->_emule_tag_set));
    }
    else
    {
        sd_assert( FALSE );
    }
    
	for(iter = SET_BEGIN(tag_list->_emule_tag_set); iter != SET_END(tag_list->_emule_tag_set); iter = SET_NEXT(tag_list->_emule_tag_set, iter))
	{
		tag = SET_DATA(iter);
        ret = emule_tag_to_buffer(tag, buffer, len );
        CHECK_VALUE( ret );
	}
	return ret;
}

_int32 emule_tag_list_from_buffer(EMULE_TAG_LIST* tag_list, char** buffer, _int32* len)
{
	EMULE_TAG* tag = NULL;
	_int32 ret = SUCCESS, tag_num = 0;
	sd_get_int32_from_lt(buffer, len, &tag_num);
	while(tag_num > 0)
	{
		--tag_num;
        ret = emule_tag_from_buffer( &tag, buffer, len);
        if(ret != SUCCESS)
			return ret;
		ret = emule_tag_list_add(tag_list, tag);
        if( ret != SUCCESS )
        {
            emule_destroy_tag(tag);
        }        
	}
	return ret;
}

#endif /* ENABLE_EMULE */

