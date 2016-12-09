#if !defined(__P2SP_QUERY_H_20090319)
#define __P2SP_QUERY_H_20090319

#ifdef __cplusplus
extern "C" 
{
#endif

#include "asyn_frame/msg.h"
#include "download_task/download_task.h"


enum TASK_RES_QUERY_SHUB_STEP 
{
	RQSS_ORIGIN= 0,			/* 直接用url */ 
	RQSS_NO_AUTH_DECODE,  	/* url no_auth 并去除'%' --- decode */ 
	RQSS_GBK2UTF8,    			/* url no_auth 并去除'%'，再由GBK转UTF8  --- decode->gbk_2_utf8  */ 
	RQSS_BIG52UTF8,    			/* url no_auth 并去除'%'，再由BIG5转UTF8  --- decode->big5_2_utf8  */ 
	RQSS_UTF82GBK,     			/* url no_auth 并去除'%'，再由UTF8转GBK  --- decode->utf8_2_gbk  */ 
	//RQSS_REDIRECT,			/* 用重定向后的url */ 
	RQSS_RELATION,				/* 查询关联文件cid等索引*/
	RQSS_RELATION_RESOURCE,   /* 通过关联文件cid查资源*/
	RQSS_END    		
		
};

	

/*--------------------------------------------------------------------------*/
/*                Structures for posting functions				        */
/*--------------------------------------------------------------------------*/


_int32 pt_start_query(TASK * p_task);
_int32 pt_stop_query(TASK * p_task);
_int32 pt_start_query_phub_tracker_cdn(TASK * p_task,_u8 * cid,_u8 * gcid,_u64 file_size,_int32 bonus_res_num);
#ifdef ENABLE_HSC
_int32 pt_start_query_vip_hub(TASK * p_task,_u8 * cid,_u8 * gcid,_u64 file_size,_int32 bonus_res_num);
#endif /* ENABLE_HSC */

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for res_query				        */
/*--------------------------------------------------------------------------*/

_int32 pt_notify_only_res_query_shub(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag);
_int32 pt_notify_res_query_shub(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag);
_int32 pt_notify_relation_file_query_shub(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag);
_int32 pt_notify_res_query_phub(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
_int32 pt_notify_res_query_tracker(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval,_u32 query_stamp);
#ifdef ENABLE_CDN
_int32 pt_notify_res_query_cdn(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* host, _u16 port);
_int32 pt_notify_res_query_normal_cdn(void* user_data, _int32 errcode, _u8 result, _u32 retry_interval);
#ifdef ENABLE_KANKAN_CDN
_int32 pt_notify_res_query_kankan_cdn(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* url,  _u32 url_len);
#endif
#ifdef ENABLE_HSC
_int32 pt_notify_res_query_vip_hub(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
#endif /* ENABLE_HSC */
#endif /* ENABLE_CDN */
_int32 pt_notify_res_query_partner_cdn( void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
_int32 pt_notify_query_dphub_result(void* user_data,_int32 errcode, _u16 result, _u32 retry_interval, BOOL is_continued);

 /*--------------------------------------------------------------------------*/
/*                Callback function for timer				        */
/*--------------------------------------------------------------------------*/
_int32 pt_handle_query_shub_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 pt_handle_query_peer_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 pt_handle_wait_dphub_root_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 pt_handle_query_normal_cdn_manager_timeout(const MSG_INFO *msg_info, 
												  _int32 errcode, 
												  _u32 notice_count_left, 
												  _u32 expired,
												  _u32 msgid);

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 pt_convert_url_format(enum TASK_RES_QUERY_SHUB_STEP * rqss ,const char* origin_url, char* str_buffer ,_int32 buffer_len );

#ifdef __cplusplus
}
#endif


#endif // !defined(__P2SP_QUERY_H_20090319)
