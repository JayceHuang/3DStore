/*----------------------------------------------------------------------------------------------------------
author:		ZengYuqing
created:	2008/09/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _REPORTER_CMD_BUILDER_H_
#define _REPORTER_CMD_BUILDER_H_
#ifdef __cplusplus
extern "C" 
{
#endif


#include "reporter/reporter_cmd_define.h"
#include "reporter/reporter_device.h"

_u32 reporter_get_seq();

_int32 reporter_build_report_license_cmd(char** buffer, _u32* len, REPORT_LICENSE_CMD* cmd);

_int32 reporter_build_report_dw_stat_cmd(char** buffer, _u32* len, REPORT_DW_STAT_CMD* cmd);

_int32 reporter_build_report_dw_fail_cmd(char** buffer, _u32* len, REPORT_DW_FAIL_CMD* cmd);

_int32 reporter_build_report_insertsres_cmd(char** buffer, _u32* len, REPORT_INSERTSRES_CMD* cmd);

_int32 reporter_build_report_bt_normal_cdn_stat_cmd(char** buffer, 
													_u32* len, 
													BT_REPORT_NORMAL_CDN_STAT *cmd);

_int32 reporter_build_report_bt_sub_task_normal_cdn_stat_cmd(char** buffer, 
															 _u32* len, 
															 BT_REPORT_SUB_TASK_NORMAL_CDN_STAT *cmd);

_int32 reporter_build_report_p2sp_normal_cdn_stat_cmd(char** buffer, 
													  _u32* len, 
													  P2SP_REPORT_NORMAL_CDN_STAT *cmd);

#ifdef ENABLE_BT
_int32 reporter_build_report_bt_insert_res_cmd(char** buffer, _u32* len, REPORT_INSERTBTRES_CMD* cmd);
_int32 reporter_build_report_bt_download_stat_cmd(char** buffer, _u32* len, REPORT_BT_DL_STAT_CMD* cmd);
_int32 reporter_build_report_bt_task_download_stat_cmd(char** buffer, _u32* len, REPORT_BT_TASK_DL_STAT_CMD* cmd);
#endif /* ENABLE_BT */

#ifdef ENABLE_EMULE
_int32 reporter_build_emule_report_insertsres_cmd(char** buffer, _u32* len, REPORT_INSERT_EMULE_RES_CMD* cmd);
_int32 reporter_build_report_emule_dl_cmd(char** buffer, _u32* len, REPORT_EMULE_DL_STAT_CMD* cmd);
_int32 reporter_build_report_emule_normal_cdn_stat_cmd(char** buffer, 
													   _u32* len, 
													   EMULE_REPORT_NORMAL_CDN_STAT *cmd);

#endif

#ifdef UPLOAD_ENABLE
_int32 reporter_build_isrc_online_cmd(char** buffer, _u32* len, REPORT_RC_ONLINE_CMD* cmd);

_int32 reporter_build_rc_list_cmd(char** buffer, _u32* len, REPORT_RC_LIST_CMD* cmd,LIST_NODE ** pp_start_node,_int32 * start_node_index);

_int32 reporter_build_insert_rc_cmd(char** buffer, _u32* len, REPORT_INSERT_RC_CMD* cmd);

_int32 reporter_build_delete_rc_cmd(char** buffer, _u32* len, REPORT_DELETE_RC_CMD* cmd);
#endif

#ifdef EMBED_REPORT

_int32 emb_reporter_build_common_task_download_stat_cmd(char** buffer, _u32* len, EMB_REPORT_COMMON_TASK_DL_STAT_CMD* cmd);

#ifdef ENABLE_BT
_int32 emb_reporter_build_bt_task_download_stat_cmd(char** buffer, _u32* len, EMB_REPORT_BT_TASK_DL_STAT_CMD* cmd);
_int32 emb_reporter_build_bt_file_download_stat_cmd(char** buffer, _u32* len, EMB_REPORT_BT_FILE_DL_STAT_CMD* cmd);
#endif

_int32 emb_reporter_build_common_task_download_fail_cmd(char** buffer, _u32* len, EMB_REPORT_COMMON_TASK_DL_FAIL_CMD* cmd);

#ifdef ENABLE_BT
_int32 emb_reporter_build_bt_file_download_fail_cmd(char** buffer, _u32* len, EMB_REPORT_BT_FILE_DL_FAIL_CMD* cmd);
#endif

_int32 emb_reporter_build_dns_abnormal_cmd(char** buffer, _u32* len, EMB_DNS_ABNORMAL_CMD* cmd);
_int32 emb_reporter_build_online_peer_state_cmd(char** buffer, _u32* len, EMB_REPORT_ONLINE_PEER_STATE_CMD* cmd);
_int32 emb_reporter_build_common_stop_task_cmd(char** buffer, _u32* len, EMB_REPORT_COMMON_STOP_TASK_CMD* cmd);

#ifdef ENABLE_BT
_int32 emb_reporter_build_bt_stop_task_cmd(char** buffer, _u32* len, EMB_REPORT_BT_STOP_TASK_CMD* cmd);
_int32 emb_reporter_build_bt_stop_file_cmd(char** buffer, _u32* len, EMB_REPORT_BT_STOP_FILE_CMD* cmd);
#endif

#ifdef ENABLE_EMULE
_int32 emb_reporter_build_emule_stop_task_cmd(char** buffer, _u32* len, EMB_REPORT_EMULE_STOP_TASK_CMD* cmd);
#endif

#endif

_int32 build_report_http_header(char* buffer, _u32* len, _u32 data_len, enum REPORTER_DEVICE_TYPE report_type);

_int32 reporter_build_report_task_create_stat_cmd(char** buffer, _u32* len,  EMB_REPORT_TASK_CREATE_STAT *cmd,_u32 * p_seq,_u32 flag);
_int32 reporter_build_report_task_p2sp_stat_cmd(char** buffer, _u32* len,  EMB_REPORT_TASK_P2SP_STAT *cmd,_u32 * p_seq);
_int32 reporter_build_report_task_emule_stat_cmd(char** buffer, _u32* len, EMB_REPORT_TASK_EMULE_STAT *cmd,_u32 * p_seq);
_int32 reporter_build_report_task_bt_stat_cmd(char** buffer, _u32* len,  EMB_REPORT_TASK_BT_STAT *cmd,_u32 * p_seq);
_int32 reporter_build_report_task_bt_file_stat_cmd(char** buffer, _u32* len,  EMB_REPORT_TASK_BT_FILE_STAT *cmd,_u32 * p_seq);

_int32 reporter_build_report_upload_stat_cmd(char** buffer, _u32* len,EMB_REPORT_UPLOAD_STAT *cmd,_u32 * p_seq);


#ifdef __cplusplus
}
#endif


#endif    /* _REPORTER_CMD_BUILDER_H_ */


