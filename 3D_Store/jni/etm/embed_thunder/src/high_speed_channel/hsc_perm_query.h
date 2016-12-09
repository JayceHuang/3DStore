#ifndef _PERM_H_20110330
#define _PERM_H_20110330

#include "utility/define.h"
#ifdef ENABLE_HSC
#include "utility/define_const_num.h"
#include "utility/list.h"
#include "asyn_frame/msg.h"

#include "connect_manager/data_pipe.h"
#include "connect_manager/pipe_interface.h"
#include "http_data_pipe/http_data_pipe.h"
#include "high_speed_channel/hsc_perm_query.h"
#include "data_manager/file_manager_interface.h"
#include "download_task/download_task.h"

#include "res_query/res_query_interface.h"

typedef struct tagHSC_USER_INFO
{
    _u64        _user_id;
    _u32        _jump_key_length;
    char*       _p_jump_key;
}HSC_USER_INFO;

typedef enum _hsc_batch_commit_result
{
    HSC_BCR_OK,
    HSC_BCR_FLUX_NOT_ENOUGH,
    HSC_BCR_TIMEOUT,
    HSC_BCR_SERVER_ERROR
}HSC_BCR;

typedef struct tagHSC_BATCH_COMMIT_PARAM
{
    TASK*       _p_task;
    _u8         _p_gcid[CID_SIZE];
    _u8         _p_cid[CID_SIZE];
    _u64        _file_size;
    char*       _p_task_name;
    _u32        _task_name_length;
    LIST        _lst_file_index;

}HSC_BATCH_COMMIT_PARAM;
//jjxh added...
typedef struct tagHSC_PQ_TASK_INFO
{
	HSC_BATCH_COMMIT_PARAM *_p_task;
    _int32      task_size;
	DATA_PIPE *_p_data_pipe;
	_u32 _timer_id;
	BOOL _is_handle_ok;
}HSC_PQ_TASK_INFO;

typedef struct tagHSC_PQ_TASK_MANAGER
{
	LIST _hsc_pq_task_list;
} HSC_PQ_TASK_MANAGER;


//jjxh added.....
typedef struct tagHSC_SHUB_TASKINFO
{
    char*       _p_gcid;
    _u32        _gcid_length;
    char*       _p_cid;
    _u32        _cid_length;
    _u64        _file_size;
}HSC_SHUB_TASKINFO;

typedef struct tagHSC_SHUB_SUB_TASK_ATTRIBUTE
{
    _u32        _sub_task_attr_length;
	_u32 		_gcid_length;
    _u8         _p_gcid[CID_SIZE*2 + 1];
	_u32 		_cid_length;
    _u8         _p_cid[CID_SIZE*2 + 1];
    _u64        _file_size;
    _u32        _file_index;
    _u64        _server_task_id;
	_u32 		_file_name_length;
    char        _p_file_name[URL_MAX_LEN];
    _u16        _flag;
}HSC_SHUB_SUB_TASK_ATTRIBUTE;


enum ECOMMITRESTYPE
{
	ECRT_OTHERS = 0,
	ECRT_HTTP = 1,
	ECRT_FTP = 2,
	ECRT_EMULE = 4,
	ECRT_BT_RES = 5, //< 整个BT资源，对应于BT种子
	ECRT_BT_FILE = 6 //< BT中单个文件资源
};

typedef struct tagHSC_SHUB_MAIN_TASKINFO
{
    _u32        _main_taskinfo_length;
    _u32        _url_length;
    char        _p_url[URL_MAX_LEN];
    _u32        _task_name_length;
    char        _p_task_name[URL_MAX_LEN];
    _u8         _res_type;     //bt ed2k p2sp...
    _u32        _res_id_length;
    char        _p_res_id[URL_MAX_LEN];
    _u64        _main_server_task_id;
    _u64        _task_size;
    _u32        _sub_task_attribute_length;
    HSC_SHUB_SUB_TASK_ATTRIBUTE* _p_sub_task_attribute;
    _u32        _sub_task_attribute_count;
    _u8         _need_query_manager;
}HSC_SHUB_MAIN_TASKINFO;

typedef struct tagHSC_SHUB_SUB_TASK_FLUX_INFO
{
    _int32      _sub_task_flux_length;
    _u64        _needed;
    _u8         _vip_cdn_ok;
    _u16        _code;   //扣除流量的原因
    char*       _p_desc;
    _u32        _desc_length;
}HSC_SHUB_SUB_TASK_FLUX_INFO;

typedef struct tagHSC_SHUB_MAIN_TASK_FLUX_INFO
{
    _u32        _main_task_flux_length;
    _u64        _needed;    //该任务(包括所有子任务)所需流量
    char*       _p_desc;    //该主任务扣费详细说明
    _u32        _desc_length;
    HSC_SHUB_SUB_TASK_FLUX_INFO* _p_subtasks;
    _u32        _subtasks_length;
}HSC_SHUB_MAIN_TASK_FLUX_INFO;

typedef struct tagHSC_SHUB_HEADER
{
    _u32        _protocol_version;
    _u32        _seq;
    _u32        _cmd_length;
    _u32        _client_ver;
    _u16        _compress_flag;
    _u16        _cmd_type;
}HSC_SHUB_HEADER;

typedef struct tagHSC_BATCH_COMMIT_REQUEST
{
    HSC_SHUB_HEADER _header;
    _u64        _user_id;
    _u32        _login_key_length;
    char        _p_login_key[URL_MAX_LEN];
    _u32        _peer_id_length;
    char        _p_peer_id[PEER_ID_SIZE + 1];
    _u32        _taskinfos_length;
    HSC_SHUB_MAIN_TASKINFO* _p_taskinfos;
    _u32        _main_taskinfo_count;
    _u32        _business_flag;  //0 : hsc  1 lixian
}HSC_BATCH_COMMIT_REQUEST;

typedef struct tagHSC_BATCH_COMMIT_RESPONSE
{
    HSC_SHUB_HEADER _header;
    _u32        _result;
    char*       _p_message;
    _u32        _message_length;
    _u64        _capacity;      //总流量(包括固化 金豆 勋章)
    _u64        _remain;        //本月剩余的总流量(扣费后)
    _u64        _needed;        //服务器返回的所需流量
    HSC_SHUB_MAIN_TASK_FLUX_INFO* _p_flux_infos;
    _u32        _flux_infos_length;
    _u16        _free;
}HSC_BATCH_COMMIT_RESPONSE;

#if !defined(ENABLE_ETM_MEDIA_CENTER)
//jjxh added.....

_int32 hsc_pq_init_module(void);


_int32 hsc_pq_query_permission(TASK *p_task, const _u8 * gcid, const _u8 *cid, _u64 file_size);

BOOL hsc_pq_is_hsc_available(TASK *p_task);

//_int32 hsc_pq_handle_normal_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 hsc_pq_handle_query_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

//jjxh added....
_int32 hsc_batch_commit(HSC_BATCH_COMMIT_PARAM* p_param, _int32 size);

_int32 hsc_handle_batch_commit_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 hsc_build_batch_commit_cmd_struct(HSC_BATCH_COMMIT_PARAM* p_params, _int32 param_size, HSC_BATCH_COMMIT_REQUEST* request);

_int32 hsc_build_batch_commit_cmd(HSC_BATCH_COMMIT_REQUEST* p_request, void** pp_buffer, _int32* p_buffer_len);

_int32 hsc_destroy_batch_commit_request(HSC_BATCH_COMMIT_REQUEST* request);

_int32 hsc_build_shub_header(HSC_SHUB_HEADER* header, _u16 cmd_type);

_int32 hsc_fill_p2sp_task_attribute(HSC_SHUB_SUB_TASK_ATTRIBUTE** pp_param, _int32* len, _int32* count, HSC_BATCH_COMMIT_PARAM* p_info);

_int32 hsc_fill_emule_task_attribute(HSC_SHUB_SUB_TASK_ATTRIBUTE** pp_param, _int32* len, _int32* count, HSC_BATCH_COMMIT_PARAM* p_info);

_int32 hsc_fill_bt_task_attribute(HSC_SHUB_SUB_TASK_ATTRIBUTE** pp_param, _int32* len, _int32* count, HSC_BATCH_COMMIT_PARAM* p_info, _u64* downloading_file_size);

_int32 hsc_parser_batch_commit_response(const char* p_data, const _int32 data_size, HSC_BATCH_COMMIT_RESPONSE* p_response);

_int32 hsc_on_response(const HSC_PQ_TASK_INFO *p_task_info, const char* p_data, const _int32 data_size);

enum ECOMMITRESTYPE hsc_get_batch_commit_task_res_type(TASK* p_task);

_int32 hsc_destroy_batch_commit_response(HSC_BATCH_COMMIT_RESPONSE* p_response);

_int32 hsc_remove_task_from_task_manager(HSC_PQ_TASK_INFO* info);

_int32 hsc_remove_task_according_to_task_info(TASK *task_ptr);

_int32 hsc_find_pq_task_by_data_pipe(HTTP_DATA_PIPE* pipe, HSC_PQ_TASK_INFO** pp_info);

_int32 hsc_destroy_HSC_PQ_TASK_INFO(HSC_PQ_TASK_INFO* info);

_int32 hsc_get_task_name(const HSC_BATCH_COMMIT_PARAM* p_param, char* buffer, _int32* len);

_int32 hsc_set_user_info(const _u64 userid, const char* p_jumpkey, const _u32 jumpkey_len);

_int32 hsc_get_user_info(HSC_USER_INFO** p_user_info);

LIST_ITERATOR hsc_find_value_in_list(LIST* list, const void* value);
//jjxh added....
#endif
#endif /* ENABLE_HSC */
#endif /* _PERM_H_20110330 */
