/*----------------------------------------------------------------------------------------------------------
author:		ZengYuqing
created:	2008/09/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _REPORTER_CMD_HANDLER_H_
#define _REPORTER_CMD_HANDLER_H_

#ifdef __cplusplus
extern "C" 
{
#endif


#include "reporter/reporter_device.h"

_int32 reporter_handle_recv_resp_cmd(char* buffer, _u32 len, REPORTER_DEVICE* device);

_int32 reporter_handle_report_dw_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd);

_int32 reporter_handle_report_license_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd);

#ifdef UPLOAD_ENABLE
_int32 reporter_handle_rc_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd);
#endif


#ifdef EMBED_REPORT

_int32 reporter_handle_emb_common_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd);
_int32 reporter_handle_emb_online_peer_stat_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd);

#endif

#ifdef __cplusplus
}
#endif


#endif /* _REPORTER_CMD_HANDLER_H_ */

