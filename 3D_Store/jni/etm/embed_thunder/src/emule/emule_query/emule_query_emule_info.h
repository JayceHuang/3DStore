
/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/09/17
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_QUERY_EMULE_INFO_H_
#define _EMULE_QUERY_EMULE_INFO_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE

#include "emule/emule_utility/emule_define.h"
#include "utility/define_const_num.h"

#define		QUERY_EMULE_INFO			5001
#define		QUERY_EMULE_INFO_RESP	5002

typedef struct tagQUERY_EMULE_INFO_CMD
{
	_u16	_cmd_type;			//5001
	_u32	_local_peerid_len;
	char		_local_peerid[PEER_ID_SIZE + 1];
	_u32	_file_hash_len;
	_u8		_file_id[FILE_ID_SIZE];
	_u64	_file_size;
	_u32	_query_times;
	_u32	_partner_id_len;
	char		_partner_id[PARTNER_ID_LEN];
	_u32	_product_flag;
}QUERY_EMULE_INFO_CMD;

typedef struct tagQUERY_EMULE_INFO_RESP_CMD
{
	_u16	_cmd_type;		//5002
	_u8		_result;			//0 查询失败    ;      1 查询成功
	_u32	_has_record;
	_u32	_aich_hash_len;
	_u8		_aich_hash[AICH_HASH_LEN];
	_u32	_part_hash_len;
	_u8*	_part_hash;
	_u32	_cid_len;
	_u8		_cid[CID_SIZE];
	_u32	_gcid_len;
	_u8		_gcid[CID_SIZE];
	_u32	_gcid_part_size;		//GCID分块大小
	_u32	_gcid_level;
	_u32	_control_flag;
}QUERY_EMULE_INFO_RESP_CMD;

struct tagEMULE_TASK;

_int32 emule_query_emule_info(struct tagEMULE_TASK* emule_task, _u8 file_hash[FILE_ID_SIZE], _u64 file_size);

_int32 emule_build_query_emule_info_cmd(QUERY_EMULE_INFO_CMD* cmd, char** buffer, _u32* len);

_int32 emule_extract_query_emule_info_resp_cmd(char* buffer, _u32 len, QUERY_EMULE_INFO_RESP_CMD* cmd);

_int32 emule_notify_query_emule_info_result(_int32 errcode, char* buffer, _u32 len, void* user_data);

_int32 emule_build_query_emule_info_cmd_rsa(QUERY_EMULE_INFO_CMD* cmd, char** buffer, _u32* len, 
												_u8 *aeskey, _int32 pubkey_verison);


typedef _int32 ( *EMULE_NOTIFY_EVENT)(_u32 task_id, _u32 event);
_int32 emule_query_set_notify_event_callback(void * callback_ptr);


_int32 emule_just_query_emule_info(void* userdata, _u8 file_hash[FILE_ID_SIZE], _u64 file_size);
_int32 emule_notify_just_query_emule_info_result(_int32 errcode, char* buffer, _u32 len, void* user_data);


#endif /* ENABLE_EMULE */

#endif


