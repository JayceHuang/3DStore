#include "mhxy_udp.h"

#ifdef _SUPPORT_MHXY

#include "encryption_algorithm.h"
#include "encryption_algorithm_1.h"
#include "encryption_algorithm_2.h"
#include "encryption_algorithm_3.h"
#include <memory.h>
#include <assert.h>
#include <stdio.h>

// 第一套方法

void mhxy_udp_encrypt1(_int32 type, _u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len)
{
	assert((in_buffer!=0)&&(out_buffer!=0));
	_u32 key_len;
	switch (type)
	{
	case 0:
		memcpy(out_buffer, in_buffer, in_len);
		*p_out_len = in_len;
		break;

	case 1:
		{
			ENCRYPTION_ALGORITHM *p_ea = NULL;
			encryption_algorithm_1_new(&p_ea);
			p_ea->create_key(p_ea, 0, 0, out_buffer, &key_len);
			memcpy(out_buffer+key_len, in_buffer, in_len);
			p_ea->encrypt(p_ea, out_buffer+key_len, in_len);
			*p_out_len = key_len + in_len;
			encryption_algorithm_destroy(&p_ea);
		}
		break;

	case 2:
		{
			ENCRYPTION_ALGORITHM *p_ea = NULL;
			encryption_algorithm_2_new(&p_ea);
			p_ea->create_key(p_ea, 0, 0, out_buffer, &key_len);
			memcpy(out_buffer+key_len, in_buffer, in_len);
			p_ea->encrypt(p_ea, out_buffer+key_len, in_len);
			*p_out_len = key_len + in_len;
			encryption_algorithm_destroy(&p_ea);
		}
		break;

	case 3:
		{
			ENCRYPTION_ALGORITHM *p_ea = NULL;
			encryption_algorithm_3_new(&p_ea);
			p_ea->create_key(p_ea, 0, 0, out_buffer, &key_len);
			memcpy(out_buffer+key_len, in_buffer, in_len);
			p_ea->encrypt(p_ea, out_buffer+key_len, in_len);
			*p_out_len = key_len + in_len;
			encryption_algorithm_destroy(&p_ea);
		}
		break;

	default:
		memcpy(out_buffer, in_buffer, in_len);
		*p_out_len = in_len;
		break;
	}

}


_int32 mhxy_udp_decrypt1(_u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len)
{
	assert((in_buffer!=0)&&(out_buffer!=0));
	int type;
	_u32 key_len;
	_u32 head = *(_u32 *)in_buffer;
	switch (head/0x20000000)
	{
	case 1:
		{
			ENCRYPTION_ALGORITHM *p_ea = NULL;
			encryption_algorithm_1_new(&p_ea);
			if (p_ea->create_key(p_ea, in_buffer, in_len, 0, &key_len))
			{
				type = 1;
				*p_out_len = in_len - key_len;
				memcpy(out_buffer, in_buffer+key_len, *p_out_len);
				p_ea->decrypt(p_ea, out_buffer, *p_out_len);
			}
			else
			{
				type = 0;
				*p_out_len = in_len;
				memcpy(out_buffer, in_buffer, in_len);
			}
			encryption_algorithm_destroy(&p_ea);
		}
		break;

	case 2:
		{
			ENCRYPTION_ALGORITHM *p_ea = NULL;
			encryption_algorithm_2_new(&p_ea);
			if (p_ea->create_key(p_ea, in_buffer, in_len, 0, &key_len))
			{
				type = 2;
				*p_out_len = in_len - key_len;
				memcpy(out_buffer, in_buffer+key_len, *p_out_len);
				p_ea->decrypt(p_ea, out_buffer, *p_out_len);
			}
			else
			{
				type = 0;
				*p_out_len = in_len;
				memcpy(out_buffer, in_buffer, in_len);
			}
			encryption_algorithm_destroy(&p_ea);
		}
		break;

	case 3:
		{
			ENCRYPTION_ALGORITHM *p_ea = NULL;
			encryption_algorithm_3_new(&p_ea);
			if (p_ea->create_key(p_ea, in_buffer, in_len, 0, &key_len))
			{
				type = 3;
				*p_out_len = in_len - key_len;
				memcpy(out_buffer, in_buffer+key_len, *p_out_len);
				p_ea->decrypt(p_ea, out_buffer, *p_out_len);
			}
			else
			{
				type = 0;
				*p_out_len = in_len;
				memcpy(out_buffer, in_buffer, in_len);
			}
			encryption_algorithm_destroy(&p_ea);
		}
		break;

	default:
		type = 0;
		*p_out_len = in_len;
		memcpy(out_buffer, in_buffer, in_len);
		break;
	}
	return type;

}


// 第二套方法

void mhxy_udp_encrypt2(_int32 type, _u8 *io_buffer, _u32 *p_io_len)
{
	_u8 temp_data[2048] = {0};
	_u32 temp_len = 0;
	mhxy_udp_encrypt1(type, io_buffer, *p_io_len, temp_data, &temp_len);
	memcpy(io_buffer, temp_data, temp_len);
	*p_io_len = temp_len;
}

_int32 mhxy_udp_decrypt2(_u8 *io_buffer, _u32 *p_io_len)
{
	_u8 temp_data[2048] = {0};
	_u32 temp_len = 0;
	int ret = mhxy_udp_decrypt1(io_buffer, *p_io_len, temp_data, &temp_len);
	memcpy(io_buffer, temp_data, temp_len);
	*p_io_len = temp_len;
	return ret;
}

#endif //#ifdef _SUPPORT_MHXY

