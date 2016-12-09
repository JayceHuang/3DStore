/*----------------------------------------------------------------------------------------------------------
author:		zhangxiaobing
created:	2013/12/10
-----------------------------------------------------------------------------------------------------------*/
#ifndef _MHXY_UDP_H_
#define	_MHXY_UDP_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef _SUPPORT_MHXY

#include "utility/define.h"

struct tagENCRYPTION_ALGORITHM;


// ��һ�׷���

/**
@purpose			: �������� 
@param	type		: ���ܷ���
@param	in_buffer	: ����ܵ�����
@param	in_len		: ����ܵ����ݳ���
@param	out_buffer	: ���ܺ������
@param	out_len 	: ���ܺ�����ݳ���
@return 			: ���ؽ��ֵ
*/
void mhxy_udp_encrypt1(_int32 type, _u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len);

/**
@purpose			: �������� 
@param	in_buffer	: ����ܵ�����
@param	in_len		: ����ܵ����ݳ���
@param	out_buffer	: ���ܺ������
@param	out_len 	: ���ܺ�����ݳ���
@return 			: ���ؼ��ܷ���
*/
_int32 mhxy_udp_decrypt1(_u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len);


// �ڶ��׷���

/**
@purpose			: �������� 
@param	type		: ���ܷ���
@param	io_buffer	: ����ǰ�������
@param	io_len		: ����ǰ������ݳ���
@return 			: ���ؽ��ֵ
*/
void mhxy_udp_encrypt2(_int32 type, _u8 *io_buffer, _u32 *p_io_len);

/**
@purpose			: �������� 
@param	io_buffer	: ����ǰ�������
@param	io_len		: ����ǰ������ݳ���
@return 			: ���ؼ��ܷ���
*/
_int32 mhxy_udp_decrypt2(_u8 *io_buffer, _u32 *p_io_len);

#endif //#ifdef _SUPPORT_MHXY


#ifdef __cplusplus
}
#endif
#endif
