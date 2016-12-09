#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_tag.h"
#include "emule_memory_slab.h"
#include "utility/mempool.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "utility/bytebuffer.h"

#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"


_int32 emule_create_str_tag(EMULE_TAG** tag, char* name, char* value)
{
	_int32 ret = SUCCESS;
	ret = emule_get_emule_tag_slip(tag);
	CHECK_VALUE(ret);
	(*tag)->_tag_type = TAGTYPE_STRING;
	sd_assert(sd_strlen(name) <= MAX_TAG_NAME_SIZE);
	sd_strncpy((*tag)->_name, name, sd_strlen(name));
	ret = sd_malloc(sd_strlen(value) + 1, (void**)&((*tag)->_value._val_str));
	CHECK_VALUE(ret);
	sd_memcpy((*tag)->_value._val_str, value, sd_strlen(value) + 1);
	return ret;
}

_int32 emule_create_u64_tag(EMULE_TAG** tag, char* name, _u64 value)
{
	_int32 ret = SUCCESS;
	ret = emule_get_emule_tag_slip(tag);
	CHECK_VALUE(ret);
	(*tag)->_tag_type = TAGTYPE_UINT64;
	sd_assert(sd_strlen(name) <= MAX_TAG_NAME_SIZE);
	sd_strncpy((*tag)->_name, name, sd_strlen(name));
	(*tag)->_value._val_u64 = value;
	return ret;
}

_int32 emule_create_u32_tag(EMULE_TAG** tag, char* name, _u32 value)
{
	_int32 ret = SUCCESS;
	ret = emule_get_emule_tag_slip(tag);
	CHECK_VALUE(ret);
	(*tag)->_tag_type = TAGTYPE_UINT32;
	sd_assert(sd_strlen(name) <= MAX_TAG_NAME_SIZE);
	sd_strncpy((*tag)->_name, name, sd_strlen(name));
	(*tag)->_value._val_u32 = value;
	return ret;
}

_int32 emule_create_u16_tag(EMULE_TAG** tag, char* name, _u16 value)
{
	_int32 ret = SUCCESS;
	ret = emule_get_emule_tag_slip(tag);
	CHECK_VALUE(ret);
	(*tag)->_tag_type = TAGTYPE_UINT16;
	sd_assert(sd_strlen(name) <= MAX_TAG_NAME_SIZE);
	sd_strncpy((*tag)->_name, name, sd_strlen(name));
	(*tag)->_value._val_u16 = value;
	return ret;
}

_int32 emule_create_u8_tag(EMULE_TAG** tag, char* name, _u8 value)
{
	_int32 ret = SUCCESS;
	ret = emule_get_emule_tag_slip(tag);
	CHECK_VALUE(ret);
	(*tag)->_tag_type = TAGTYPE_UINT8;
	sd_assert(sd_strlen(name) <= MAX_TAG_NAME_SIZE);
	sd_strncpy((*tag)->_name, name, sd_strlen(name));
	(*tag)->_value._val_int8 = value;
	return ret;
}

_int32 emule_destroy_tag(EMULE_TAG* tag)
{
	switch(tag->_tag_type)
	{
		case TAGTYPE_STRING:
   	    case TAGTYPE_STR1:
    	case TAGTYPE_STR2:
    	case TAGTYPE_STR3:
    	case TAGTYPE_STR4:
    	case TAGTYPE_STR5:
    	case TAGTYPE_STR6:
    	case TAGTYPE_STR7:
    	case TAGTYPE_STR8:
    	case TAGTYPE_STR9:
    	case TAGTYPE_STR10:
    	case TAGTYPE_STR11:
    	case TAGTYPE_STR12:
    	case TAGTYPE_STR13:
    	case TAGTYPE_STR14:
    	case TAGTYPE_STR15:
    	case TAGTYPE_STR16:
    	case TAGTYPE_STR17:
    	case TAGTYPE_STR18:
    	case TAGTYPE_STR19:
    	case TAGTYPE_STR20:
    	case TAGTYPE_STR21:
    	case TAGTYPE_STR22:	  
        case TAGTYPE_HASH:          
		{
			sd_free(tag->_value._val_str);
			tag->_value._val_str = NULL;
			break;
		}
	}
	return emule_free_emule_tag_slip(tag);
}

_u32 emule_tag_size(EMULE_TAG* tag)
{
	_u32 len = 0;
	len += sizeof(_u8);		//类型长度
	len += sizeof(_u16);		//name的长度
	len += sd_strlen(tag->_name);
	switch(tag->_tag_type)
	{
		case TAGTYPE_STRING:
		{
			len += sizeof(_u16);		//string的长度
			len += sd_strlen(tag->_value._val_str);
			break;
		}
		case TAGTYPE_UINT64:
		{
			len += sizeof(_u64);
			break;
		}
		case TAGTYPE_UINT32:
		{
			len += sizeof(_u32);
			break;
		}        
 		case TAGTYPE_UINT16:
		{
			len += sizeof(_u16);
			break;
		}  
  		case TAGTYPE_UINT8:
		{
			len += sizeof(_u8);
			break;
		}           
		default:
			sd_assert(FALSE);
	}
	return len;
}

BOOL emule_tag_is_u16(const EMULE_TAG* tag)
{
	if(tag->_tag_type == TAGTYPE_UINT16)
		return TRUE;
	else
		return FALSE;
}

BOOL emule_tag_is_u32(const EMULE_TAG* tag)
{
	if(tag->_tag_type == TAGTYPE_UINT32)
		return TRUE;
	else
		return FALSE;
}

BOOL emule_tag_is_str(const EMULE_TAG* tag)
{
	if(tag->_tag_type == TAGTYPE_STRING)
		return TRUE;
	else
		return FALSE;
}

_int32 emule_tag_from_buffer(EMULE_TAG** pp_tag, char** buffer, _int32* len)
{
	_int32 ret = SUCCESS;
	_u16 name_len = 0, str_len = 0;
    EMULE_TAG* tag = NULL;

	ret = emule_get_emule_tag_slip(&tag);
	if(ret != SUCCESS)
		return ret;   
    
	sd_get_int8(buffer, len, (_int8*)&tag->_tag_type);
	sd_get_int16_from_lt(buffer, len, (_int16*)&name_len);
	sd_assert(name_len <= MAX_TAG_NAME_SIZE);
    if(name_len > MAX_TAG_NAME_SIZE)
    {
        emule_free_emule_tag_slip(tag);
        return -1;
    }
	sd_get_bytes(buffer, len, tag->_name, name_len);
    LOG_DEBUG("emule_tag_from_buffer._name:%s", tag->_name);
    
	switch(tag->_tag_type)
	{
		case TAGTYPE_STRING:
		{
			sd_get_int16_from_lt(buffer, len, (_int16*)&str_len);
			ret = sd_malloc(str_len + 1, (void**)&tag->_value._val_str);
			CHECK_VALUE(ret);
			ret = sd_get_bytes(buffer, len, tag->_value._val_str, str_len);
			tag->_value._val_str[str_len] = '\0';
			break;
		}
 		case TAGTYPE_UINT64:
		{
			ret = sd_get_int64_from_lt(buffer, len, (_int64*)&tag->_value._val_u64);
			break;
		}    	
		case TAGTYPE_FLOAT32:
		case TAGTYPE_UINT32:
		{
			ret = sd_get_int32_from_lt(buffer, len, (_int32*)&tag->_value._val_u32);
			break;
		}
 		case TAGTYPE_UINT16:
		{
			ret = sd_get_int16_from_lt(buffer, len, (_int16*)&tag->_value._val_u16);
			break;
		}       
		case TAGTYPE_UINT8:
		{
			ret = sd_get_int8(buffer, len, (_int8*)&tag->_value._val_int8);
			break;
		}        
		case TAGTYPE_BOOL:
		{
			ret = sd_get_int8(buffer, len, (_int8*)&tag->_value._val_int8);
			break;
		}           
    	case TAGTYPE_STR1:
    	case TAGTYPE_STR2:
    	case TAGTYPE_STR3:
    	case TAGTYPE_STR4:
    	case TAGTYPE_STR5:
    	case TAGTYPE_STR6:
    	case TAGTYPE_STR7:
    	case TAGTYPE_STR8:
    	case TAGTYPE_STR9:
    	case TAGTYPE_STR10:
    	case TAGTYPE_STR11:
    	case TAGTYPE_STR12:
    	case TAGTYPE_STR13:
    	case TAGTYPE_STR14:
    	case TAGTYPE_STR15:
    	case TAGTYPE_STR16:
    	case TAGTYPE_STR17:
    	case TAGTYPE_STR18:
    	case TAGTYPE_STR19:
    	case TAGTYPE_STR20:
    	case TAGTYPE_STR21:
    	case TAGTYPE_STR22:	  
        {
            str_len = tag->_tag_type - TAGTYPE_STR1 + 1;
			ret = sd_malloc(str_len + 1, (void**)&tag->_value._val_str);
			CHECK_VALUE(ret);
			ret = sd_get_bytes(buffer, len, tag->_value._val_str, str_len);
			tag->_value._val_str[str_len] = '\0';
    	}
        case TAGTYPE_HASH:
        {
            str_len = KAD_ID_LEN;
			ret = sd_malloc(str_len + 1, (void**)&tag->_value._val_str);
			CHECK_VALUE(ret);
			ret = sd_get_bytes(buffer, len, tag->_value._val_str, str_len);
			tag->_value._val_str[str_len] = '\0';
        }
		default:
            ret = -1;
	}    
    if( ret != SUCCESS ) 
    {
        emule_free_emule_tag_slip(tag);
        return ret;
    }
    *pp_tag = tag;
    return SUCCESS;
}

_int32 emule_tag_to_buffer(EMULE_TAG* tag, char** buffer, _int32* len)
{
	_int32 ret = SUCCESS, str_len = 0;
	sd_set_int8(buffer, len, tag->_tag_type);
	//把name填充进去
	sd_set_int16_to_lt(buffer, len, (_int16)sd_strlen(tag->_name));
	ret = sd_set_bytes(buffer, len, tag->_name, sd_strlen(tag->_name));
    CHECK_VALUE(ret);
    
	switch(tag->_tag_type)
	{
		case TAGTYPE_STRING:
		{
			str_len =  sd_strlen(tag->_value._val_str);
			sd_set_int16_to_lt(buffer, len, str_len);
			ret = sd_set_bytes(buffer, len, tag->_value._val_str, str_len);
			break;
		}
	 	case TAGTYPE_UINT64:
		{
			ret = sd_set_int64_to_lt(buffer, len, (_int64)tag->_value._val_u64);
			break;
		}       	
        case TAGTYPE_UINT32:
		{
			ret = sd_set_int32_to_lt(buffer, len, (_int32)tag->_value._val_u32);
			break;
		}
        case TAGTYPE_UINT16:
		{
			ret = sd_set_int16_to_lt(buffer, len, (_int16)tag->_value._val_u16);
			break;
		}     
        case TAGTYPE_UINT8:
		{
			ret = sd_set_int8(buffer, len, (_int32)tag->_value._val_int8);
			break;
		}

		default:
			sd_assert(FALSE);
            return -1;
	}
    return ret;
}

#endif /* ENABLE_EMULE */

