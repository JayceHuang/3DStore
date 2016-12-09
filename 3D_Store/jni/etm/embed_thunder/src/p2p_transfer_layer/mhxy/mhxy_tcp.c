#include "mhxy_tcp.h"

#ifdef _SUPPORT_MHXY

#include "encryption_algorithm.h"
#include "encryption_algorithm_1.h"
#include "encryption_algorithm_2.h"
#include "encryption_algorithm_3.h"
#include <memory.h>
#include <assert.h>
#include <stdio.h>

#include "utility/sd_assert.h"

#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

_int32 mhxy_tcp_new(MHXY_TCP **pp_mhxy_tcp)
{
    *pp_mhxy_tcp = (MHXY_TCP *)malloc(sizeof(MHXY_TCP));
    sd_assert(*pp_mhxy_tcp);
    //LOG_DEBUG("mhxy_tcp_new:0x%x.", *pp_mhxy_tcp);
    
    return mhxy_tcp_init(*pp_mhxy_tcp);
}

_int32 mhxy_tcp_init(MHXY_TCP *p_mhxy_tcp)
{
    if (!p_mhxy_tcp) return -1;
    p_mhxy_tcp->_mhxy_type = 1; // 加密方法

    p_mhxy_tcp->_is_first_decrypt = TRUE;
    p_mhxy_tcp->_is_first_encrypt = TRUE;
    p_mhxy_tcp->_ea_decrypt = NULL;
    p_mhxy_tcp->_ea_encrypt = NULL;

    //LOG_DEBUG("mhxy_tcp_init:0x%x.", p_mhxy_tcp);

    return mhxy_tcp_set_type(p_mhxy_tcp, 1);
}

_int32 mhxy_tcp_set_type(MHXY_TCP *p_mhxy_tcp, _int32 type)
{
    if (!p_mhxy_tcp || type<0 || type>3) return -1;

    //LOG_DEBUG("mhxy_tcp_set_type:0x%x, old type:%d, new type:%d, _ea_decrypt:0x%x, _ea_encrypt:0x%x.", p_mhxy_tcp, p_mhxy_tcp->_mhxy_type, type, p_mhxy_tcp->_ea_decrypt, p_mhxy_tcp->_ea_encrypt);

    if (p_mhxy_tcp->_mhxy_type==type && p_mhxy_tcp->_ea_decrypt && p_mhxy_tcp->_ea_encrypt ) return 0;
    
    if (p_mhxy_tcp->_mhxy_type!=type && p_mhxy_tcp->_ea_decrypt != NULL)
    {
        
    	encryption_algorithm_destroy( &p_mhxy_tcp->_ea_decrypt );
    	p_mhxy_tcp->_ea_decrypt = NULL;
    }
    if (p_mhxy_tcp->_mhxy_type!=type && p_mhxy_tcp->_ea_encrypt != NULL)
    {
    	encryption_algorithm_destroy( &p_mhxy_tcp->_ea_encrypt );
    	p_mhxy_tcp->_ea_encrypt = NULL;
    }

    p_mhxy_tcp->_mhxy_type = type;
    BOOL bRet = FALSE;

    switch (type)
    {
    case 0:
        break;
    case 1:
        if (p_mhxy_tcp->_ea_decrypt == NULL)   bRet = encryption_algorithm_1_new( &p_mhxy_tcp->_ea_decrypt );
        sd_assert(bRet);
        if (p_mhxy_tcp->_ea_encrypt == NULL)  bRet = encryption_algorithm_1_new( &p_mhxy_tcp->_ea_encrypt );
        sd_assert(bRet);
        break;
    case 2:
        if (p_mhxy_tcp->_ea_decrypt == NULL)   bRet = encryption_algorithm_2_new( &p_mhxy_tcp->_ea_decrypt );
        sd_assert(bRet);
        if (p_mhxy_tcp->_ea_encrypt == NULL)  bRet = encryption_algorithm_2_new( &p_mhxy_tcp->_ea_encrypt );
        sd_assert(bRet);
        break;
    case 3:
        if (p_mhxy_tcp->_ea_decrypt == NULL)   bRet = encryption_algorithm_3_new( &p_mhxy_tcp->_ea_decrypt );
        sd_assert(bRet);
        if (p_mhxy_tcp->_ea_encrypt == NULL)  bRet = encryption_algorithm_3_new( &p_mhxy_tcp->_ea_encrypt );
        sd_assert(bRet);
        break;
    default:
        sd_assert(0);
        return -1;
    	break;
    }
    //LOG_DEBUG("mhxy_tcp_set_type:0x%x, old type:%d, new type:%d, _ea_decrypt:0x%x, _ea_encrypt:0x%x.", p_mhxy_tcp, p_mhxy_tcp->_mhxy_type, type, p_mhxy_tcp->_ea_decrypt, p_mhxy_tcp->_ea_encrypt);
    return 0;

}

_int32 mhxy_tcp_destroy(MHXY_TCP **pp_mhxy_tcp)
{
    if ((*pp_mhxy_tcp)->_ea_decrypt) encryption_algorithm_destroy( &(*pp_mhxy_tcp)->_ea_decrypt);
    if ((*pp_mhxy_tcp)->_ea_encrypt) encryption_algorithm_destroy( &(*pp_mhxy_tcp)->_ea_encrypt);
    //LOG_DEBUG("mhxy_tcp_destroy:0x%x.", *pp_mhxy_tcp);
    free(*pp_mhxy_tcp);
    *pp_mhxy_tcp = NULL;
}

BOOL mhxy_tcp_is_encrypt(MHXY_TCP *p_mhxy_tcp)
{
    return (p_mhxy_tcp->_mhxy_type != 0);
}

// 有两套方法，建议第一次加密/解密用第一套方法，之后使用第二套方法。

// 第一套方法

/**
@purpose			: 加密数据 
@param	in_buffer	: 需加密的数据
@param	in_len		: 需加密的数据长度
@param	out_buffer	: 加密后的数据
@param	out_len		: 加密后的数据长度
@return				: 返回结果值
*/
void mhxy_tcp_encrypt1(MHXY_TCP *p_mhxy_tcp, _u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len)
{
    assert((in_buffer!=NULL)&&(out_buffer!=NULL));
    _u32 key_pos = 0;
    _u32 key_len = 0;
    if (p_mhxy_tcp->_ea_encrypt) 
    {
        key_pos = p_mhxy_tcp->_ea_encrypt->_key_pos;
        key_len = p_mhxy_tcp->_ea_encrypt->_key_len;
    }
   //LOG_DEBUG("mhxy_tcp_encrypt1 begin:0x%x, in_len:%u. is_first_encrypt:%d, key_len:%u, key_pos:%u", p_mhxy_tcp, in_len, p_mhxy_tcp->_is_first_encrypt, key_len, key_pos);

    if (p_mhxy_tcp->_mhxy_type != 0)
    {
        assert(p_mhxy_tcp->_ea_encrypt!=NULL);
        if (p_mhxy_tcp->_is_first_encrypt)
        {
        	_u32 key_len;
        	p_mhxy_tcp->_ea_encrypt->create_key(p_mhxy_tcp->_ea_encrypt, 0, 0, out_buffer, &key_len);
        	memcpy(out_buffer+key_len, in_buffer, in_len);
        	(*p_out_len) = key_len + in_len;
        	p_mhxy_tcp->_ea_encrypt->encrypt(p_mhxy_tcp->_ea_encrypt, out_buffer+key_len, in_len);
        	p_mhxy_tcp->_is_first_encrypt = FALSE;
        }
        else
        {
        	(*p_out_len) = in_len;
        	memcpy(out_buffer, in_buffer, (*p_out_len));
        	p_mhxy_tcp->_ea_encrypt->encrypt(p_mhxy_tcp->_ea_encrypt, out_buffer, (*p_out_len));
        }
    } 
    else
    {
    	(*p_out_len) = in_len;
    	memcpy(out_buffer, in_buffer, (*p_out_len));
    }
    if (p_mhxy_tcp->_ea_encrypt) 
    {
        key_pos = p_mhxy_tcp->_ea_encrypt->_key_pos;
        key_len = p_mhxy_tcp->_ea_encrypt->_key_len;
    }
   //LOG_DEBUG("mhxy_tcp_encrypt1 end:0x%x, in_len:%u, out len:%u. key_len:%u, key_pos:%u", p_mhxy_tcp, in_len, *p_out_len, key_len, key_pos);
}

/**
@purpose			: 解密数据 
@param	in_buffer	: 需解密的数据
@param	in_len		: 需解密的数据长度
@param	out_buffer	: 解密后的数据
@param	out_len		: 解密后的数据长度
@return				: 返回加密方法
*/
_int32 mhxy_tcp_decrypt1(MHXY_TCP *p_mhxy_tcp, _u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len)
{
    assert((in_buffer!=0)&&(out_buffer!=0));
    _u32 key_pos = 0;
    _u32 key_len = 0;
    if (p_mhxy_tcp->_ea_decrypt) 
    {
        key_pos = p_mhxy_tcp->_ea_decrypt->_key_pos;
        key_len = p_mhxy_tcp->_ea_decrypt->_key_len;
    }
   //LOG_DEBUG("mhxy_tcp_decrypt1 begin :0x%x, in_len:%u, . is_first_decrypt:%d, key_len:%u, key_pos:%u", p_mhxy_tcp, in_len, p_mhxy_tcp->_is_first_decrypt, key_len, key_pos);

    // 如果是第一次解密
    if (p_mhxy_tcp->_is_first_decrypt)
    {
        // 查找加密方法标志
        _u32 key_len;
        _u32 head = *(_u32 *)in_buffer;
        
        switch (head/0x20000000)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            mhxy_tcp_set_type(p_mhxy_tcp, head/0x20000000);
            break;
        default:
            p_mhxy_tcp->_mhxy_type = 0;
            p_mhxy_tcp->_ea_decrypt = 0;
            break;
        }

        // 如果查到加密标志，还要经过校验才能认定有加密。
        if (p_mhxy_tcp->_ea_decrypt != 0)
        {
            if (p_mhxy_tcp->_ea_decrypt->create_key(p_mhxy_tcp->_ea_decrypt, in_buffer, in_len, 0, &key_len))
            {
                (*p_out_len) = in_len - key_len;
                memcpy(out_buffer, in_buffer+key_len, (*p_out_len));
                p_mhxy_tcp->_ea_decrypt->decrypt(p_mhxy_tcp->_ea_decrypt, out_buffer, (*p_out_len));
            }
            else
            {
                (*p_out_len) = in_len;
                memcpy(out_buffer, in_buffer, in_len);
                p_mhxy_tcp->_mhxy_type = 0;
                free( p_mhxy_tcp->_ea_decrypt );
                p_mhxy_tcp->_ea_decrypt = 0;
                free( p_mhxy_tcp->_ea_encrypt );
                p_mhxy_tcp->_ea_encrypt = 0;
            }
        }
        // 未查找到加密标志，肯定没有加密。
        else
        {
            (*p_out_len) = in_len;
            memcpy(out_buffer, in_buffer, in_len);
        }
        p_mhxy_tcp->_is_first_decrypt = FALSE;
    }
    // 如果不是第一次解密
    else
    {
        // 有加密
        if (p_mhxy_tcp->_ea_decrypt != 0)
        {
            (*p_out_len) = in_len;
            memcpy(out_buffer, in_buffer, (*p_out_len));
            p_mhxy_tcp->_ea_decrypt->decrypt(p_mhxy_tcp->_ea_decrypt, out_buffer, (*p_out_len));
        }
        // 无加密
        else
        {
            (*p_out_len) = in_len;
            memcpy(out_buffer, in_buffer, in_len);
        }
    }
    if (p_mhxy_tcp->_ea_decrypt) 
    {
        key_pos = p_mhxy_tcp->_ea_decrypt->_key_pos;
        key_len = p_mhxy_tcp->_ea_decrypt->_key_len;
    }
   //LOG_DEBUG("mhxy_tcp_decrypt1 end:0x%x, in_len:%u, out len:%u, is_first_decrypt:%d, key_len:%u, key_pos:%u", p_mhxy_tcp, in_len, *p_out_len, p_mhxy_tcp->_is_first_decrypt, key_len, key_pos);

    return p_mhxy_tcp->_mhxy_type;
}


// 第二套方法

/**
@purpose			: 加密数据 
@param	io_buffer	: 加密前后的数据
@param	io_len		: 加密前后的数据长度
@return				: 返回结果值
*/
void mhxy_tcp_encrypt2(MHXY_TCP *p_mhxy_tcp, _u8 *io_buffer, _u32 *p_io_len)
{
    _u32 key_pos = 0;
    _u32 key_len = 0;
    if (p_mhxy_tcp->_ea_encrypt) 
    {
        key_pos = p_mhxy_tcp->_ea_encrypt->_key_pos;
        key_len = p_mhxy_tcp->_ea_encrypt->_key_len;
    }
   //LOG_DEBUG("mhxy_tcp_encrypt2 begin:0x%x, in_len:%u, is_first_encrypt:%d, key_len:%u, key_pos:%u", p_mhxy_tcp, *p_io_len, p_mhxy_tcp->_is_first_encrypt, key_len, key_pos);
    if (p_mhxy_tcp->_is_first_encrypt)
    {

        _u8* temp_data = (_u8 *)malloc((*p_io_len)+12);
        _u32 temp_len = 0;

        mhxy_tcp_encrypt1(p_mhxy_tcp, io_buffer, (*p_io_len), temp_data, &temp_len);
        memcpy(io_buffer, temp_data, temp_len);
        (*p_io_len) = temp_len;
        free( temp_data );
    }
    else
    {
        if (p_mhxy_tcp->_ea_encrypt != 0)
        {
            p_mhxy_tcp->_ea_encrypt->encrypt(p_mhxy_tcp->_ea_encrypt, io_buffer, (*p_io_len));
        }
    }
    if (p_mhxy_tcp->_ea_encrypt) 
    {
        key_pos = p_mhxy_tcp->_ea_encrypt->_key_pos;
        key_len = p_mhxy_tcp->_ea_encrypt->_key_len;
    }
   //LOG_DEBUG("mhxy_tcp_encrypt2 end:0x%x, in_len:%u, is_first_encrypt:%d, key_len:%u, key_pos:%u", p_mhxy_tcp, *p_io_len, p_mhxy_tcp->_is_first_encrypt, key_len, key_pos);
}

/**
@purpose			: 解密数据 
@param	io_buffer	: 解密前后的数据
@param	io_len		: 解密前后的数据长度
@return				: 返回加密方法
*/
_int32 mhxy_tcp_decrypt2(MHXY_TCP *p_mhxy_tcp, _u8 *io_buffer, _u32 *p_io_len)
{
    _u32 key_pos = 0;
    _u32 key_len = 0;
    if (p_mhxy_tcp->_ea_decrypt) 
    {
        key_pos = p_mhxy_tcp->_ea_decrypt->_key_pos;
        key_len = p_mhxy_tcp->_ea_decrypt->_key_len;
    }

   //LOG_DEBUG("mhxy_tcp_decrypt2 begin :0x%x, in_len:%u, is_first_decrypt:%d, key_len:%u, key_pos:%u", p_mhxy_tcp, *p_io_len, p_mhxy_tcp->_is_first_decrypt, key_len, key_pos);
    if (p_mhxy_tcp->_is_first_decrypt)
    {
        _u8* temp_data = (_u8 *)malloc(*p_io_len);
        _u32 temp_len = 0;
        mhxy_tcp_decrypt1(p_mhxy_tcp, io_buffer, (*p_io_len), temp_data, &temp_len);
        memcpy(io_buffer, temp_data, temp_len);
        (*p_io_len) = temp_len;
        free( temp_data );
    }
    else
    {
        if (p_mhxy_tcp->_ea_decrypt != 0)
        {
            p_mhxy_tcp->_ea_decrypt->decrypt(p_mhxy_tcp->_ea_decrypt, io_buffer, (*p_io_len));
        }
    }
    if (p_mhxy_tcp->_ea_decrypt) 
    {
        key_pos = p_mhxy_tcp->_ea_decrypt->_key_pos;
        key_len = p_mhxy_tcp->_ea_decrypt->_key_len;
    }
   //LOG_DEBUG("mhxy_tcp_decrypt2 end:0x%x, in_len:%u, is_first_decrypt:%d, key_len:%u, key_pos:%u", p_mhxy_tcp, *p_io_len, p_mhxy_tcp->_is_first_decrypt, key_len, key_pos);
    return p_mhxy_tcp->_mhxy_type;
}

#endif //#ifdef _SUPPORT_MHXY

