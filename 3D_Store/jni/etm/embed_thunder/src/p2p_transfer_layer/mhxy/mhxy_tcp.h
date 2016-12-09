/*----------------------------------------------------------------------------------------------------------
author:		zhangxiaobing
created:	2013/12/10
-----------------------------------------------------------------------------------------------------------*/
#ifndef _MHXY_TCP_H_
#define	_MHXY_TCP_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#ifdef _SUPPORT_MHXY

struct tagENCRYPTION_ALGORITHM;

typedef struct tagMHXY_TCP
{
	_int32 _mhxy_type; // ���ܷ���

	struct tagENCRYPTION_ALGORITHM* _ea_decrypt;
	struct tagENCRYPTION_ALGORITHM* _ea_encrypt;

	BOOL _is_first_decrypt;
	BOOL _is_first_encrypt;

}MHXY_TCP;


_int32 mhxy_tcp_new(MHXY_TCP **pp_mhxy_tcp);
_int32 mhxy_tcp_init(MHXY_TCP *p_mhxy_tcp);
_int32 mhxy_tcp_destroy(MHXY_TCP **pp_mhxy_tcp);

_int32 mhxy_tcp_set_type(MHXY_TCP *p_mhxy_tcp, _int32 type);

BOOL mhxy_tcp_is_encrypt(MHXY_TCP *p_mhxy_tcp);

// �����׷����������һ�μ���/�����õ�һ�׷�����֮��ʹ�õڶ��׷�����

// ��һ�׷���

/**
@purpose			: �������� 
@param	in_buffer	: ����ܵ�����
@param	in_len		: ����ܵ����ݳ���
@param	out_buffer	: ���ܺ������
@param	out_len		: ���ܺ�����ݳ���
@return				: ���ؽ��ֵ
*/
void mhxy_tcp_encrypt1(MHXY_TCP *p_mhxy_tcp, _u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len);

/**
@purpose			: �������� 
@param	in_buffer	: ����ܵ�����
@param	in_len		: ����ܵ����ݳ���
@param	out_buffer	: ���ܺ������
@param	out_len		: ���ܺ�����ݳ���
@return				: ���ؼ��ܷ���
*/
_int32 mhxy_tcp_decrypt1(MHXY_TCP *p_mhxy_tcp, _u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len);


// �ڶ��׷���

/**
@purpose			: �������� 
@param	io_buffer	: ����ǰ�������
@param	io_len		: ����ǰ������ݳ���
@return				: ���ؽ��ֵ
*/
void mhxy_tcp_encrypt2(MHXY_TCP *p_mhxy_tcp, _u8 *io_buffer, _u32 *p_io_len);

/**
@purpose			: �������� 
@param	io_buffer	: ����ǰ�������
@param	io_len		: ����ǰ������ݳ���
@return				: ���ؼ��ܷ���
*/
_int32 mhxy_tcp_decrypt2(MHXY_TCP *p_mhxy_tcp, _u8 *io_buffer, _u32 *p_io_len);

#endif//_SUPPORT_MHXY

#ifdef __cplusplus
}
#endif
#endif
