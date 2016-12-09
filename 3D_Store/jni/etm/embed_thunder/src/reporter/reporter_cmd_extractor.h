/*----------------------------------------------------------------------------------------------------------
author:		ZengYuqing
created:	2008/09/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _REPORTER_CMD_EXTRACTOR_H_
#define _REPORTER_CMD_EXTRACTOR_H_

#ifdef __cplusplus
extern "C" 
{
#endif


#include "reporter/reporter_cmd_define.h"

_int32 reporter_extract_report_license_resp_cmd(char* buffer, _u32 len, REPORT_LICENSE_RESP_CMD* cmd);
_int32 reporter_extract_report_dw_resp_cmd(char* buffer, _u32 len, REPORT_DW_RESP_CMD* cmd);
#ifdef UPLOAD_ENABLE
_int32 reporter_extract_rc_resp_cmd(char* buffer, _u32 len,_u32* cmd_seq);
#endif

#ifdef EMBED_REPORT

_int32 reporter_extract_emb_common_resp_cmd(char* buffer, _u32 len, EMB_COMMON_RESP_CMD* cmd);
_int32 reporter_extract_emb_online_peer_stat_resp_cmd(char* buffer, _u32 len, EMB_ONLINE_PEER_STAT_RESP_CMD* cmd);

#endif

#ifdef __cplusplus
}
#endif


#endif /* _REPORTER_CMD_EXTRACTOR_H_ */
