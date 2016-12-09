#ifndef _RES_QUERY_CMD_EXTRACTOR_H_
#define _RES_QUERY_CMD_EXTRACTOR_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "res_query_cmd_define.h"

/*���øú�������ע��cmd._bcid�ֶ���malloc�����ģ����������ֶβ�ΪNULL�Ļ�����free��*/
_int32 extract_server_res_resp_cmd(char* buffer, _u32 len, SERVER_RES_RESP_CMD* cmd);

_int32 extract_peer_res_resp_cmd(char* buffer, _u32 len, PEER_RES_RESP_CMD* cmd);

_int32 extract_tracker_res_resp_cmd(char* buffer, _u32 len, TRACKER_RES_RESP_CMD* cmd);

/*���øú�������ע��cmd._bcid�ֶ���malloc�����ģ����������ֶβ�ΪNULL�Ļ�����free��*/
_int32 extract_query_bt_info_resp_cmd(char* buffer, _u32 len, QUERY_BT_INFO_RESP_CMD* cmd);

/*���øú����󣬼ǵ��ж�cmd���������list�ṹ�������Ϊ�գ�Ӧ�ð����������free��*/
_int32 extract_enrollsp1_resp_cmd(char* buffer, _u32 len, ENROLLSP1_RESP_CMD* cmd);

/*���øú����󣬼ǵ��ж�cmd���������list�ṹ�������Ϊ�գ�Ӧ�ð����������free��*/
_int32 extract_query_config_resp_cmd(char* buffer, _u32 len, QUERY_CONFIG_RESP_CMD* cmd);

_int32 extract_server_res_info_resp_cmd(char* buffer, _u32 len, QUERY_RES_INFO_RESP* cmd);

_int32 extract_newserver_res_resp_cmd(char* buffer, _u32 len, NEWQUERY_SERVER_RES_RESP* cmd);

_int32 extract_relation_server_res_resp_cmd(char* buffer, _u32 len, QUERY_FILE_RELATION_RES_RESP* cmd);


#ifdef __cplusplus
}
#endif
#endif
