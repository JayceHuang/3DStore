#ifndef _HIGH_SPEED_CHANNEL_INFO_H_20110413
#define _HIGH_SPEED_CHANNEL_INFO_H_20110413

#ifdef ENABLE_HSC

#include "utility/define.h"
typedef enum _hsc_stat
{
    HSC_IDLE, 
    HSC_ENTERING, 
    HSC_SUCCESS, 
    HSC_FAILED, 
    HSC_STOP, 
    HSC_BT_SUBTASK_FAILED
}HSC_STAT;

typedef struct tagHIGH_SPEED_CHANNEL_INFO //ע��: �޸ı��ṹ��һ��ͬ���޸�embed_thunder.h�е�ET_HIGH_SPEED_CHANNEL_INFO�ṹ��
											//�����ڽ����ȡ�ýṹ��Ϣʱ���ƻ��ڴ�
{
	_u64				_product_id; // set by client
	_u64				_sub_id;  // set by client
	_int32				_channel_type;// ͨ������: 0=�̳����3�� 1=�̳���ɫͨ�� 2=vip����ͨ�� -1=������
	HSC_STAT 			_stat;
	_u32 				_res_num;
	_u64 				_dl_bytes;
	_u32 				_speed;
	_int32		 		_errcode;
	BOOL 				_can_use;
	_u64 				_cur_cost_flow;
	BOOL				_use_hsc;
	BOOL				_used_hsc_before;//���������ʹ�ù�����ͨ�������Ժ�����Ȩ�޲�ѯ�������ظ��۳�����
	BOOL              _force_query_permission;
}HIGH_SPEED_CHANNEL_INFO;

typedef struct tagHSC_INFO_BRD
{
	_u32						_task_id;
	HIGH_SPEED_CHANNEL_INFO		_hsc_info;
} HSC_INFO_BRD;

typedef struct t_hsc_info_api
{
	_u64				_product_id;
	_u64				_sub_id;
	_int32				_channel_type;// ͨ������: 0=�̳����3�� 1=�̳���ɫͨ�� 2=vip����ͨ�� -1=������
	HSC_STAT 		_stat;
	_u32 				_res_num;
	_u64 				_dl_bytes;
	_u32 				_speed;
	_int32		 		_errcode;
	BOOL 				_can_use;
	_u64 				_cur_cost_flow;
	_u64 				_remaining_flow;
	BOOL				_used_hsc_before;
}HIGH_SPEED_CHANNEL_INFO_API;
#if !defined(ENABLE_ETM_MEDIA_CENTER)
_int32 hsc_init_info(HIGH_SPEED_CHANNEL_INFO *p_hsc_info);
_int32 hsc_info_set_pdt_sub_id(_u32 task_id, _u32 product_id, _u32 sub_id);
BOOL *hsc_get_g_auto_enable_stat(void);
_u64 *hsc_get_g_remainning_flow(void);
void hsc_set_force_query_permission(HIGH_SPEED_CHANNEL_INFO *p_hsc_info);
#endif
#endif /* ENABLE_HSC */

#endif /* _HIGH_SPEED_CHANNEL_INFO_H_20110413 */
