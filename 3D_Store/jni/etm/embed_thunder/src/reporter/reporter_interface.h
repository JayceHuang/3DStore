/*----------------------------------------------------------------------------------------------------------
author:		ZengYuqing
created:	2008/09/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _REPORTER_INTERFACE_H_
#define _REPORTER_INTERFACE_H_

#ifdef __cplusplus
extern "C" 
{
#endif


#include "utility/define.h"
#include "reporter/reporter_device.h"

#include "download_task/download_task.h"

typedef struct tagDNS_SERVER_IP_STATE
{
	_u32 _ip;
	_u32 _result;
} DNS_SERVER_IP_STATE;

typedef struct tagVOD_REPORT_PARA
{
	_u32 _vod_play_time;
	_u32 _vod_first_buffer_time;
	_u32 _vod_interrupt_times; 
	_u32 _min_interrupt_time; 
	_u32 _max_interrupt_time; 
	_u32 _avg_interrupt_time;
	
	_u32 _bit_rate;			        //比特率
	_u32 _vod_total_buffer_time;    //总的播放缓冲时间（秒）
	_u32 _vod_total_drag_wait_time;	//播放拖动后的等待播放时间（秒）
	_u32 _vod_drag_num;		        //拖动次数
	_u32 _play_interrupt_1;         // 1分钟内中断
	_u32 _play_interrupt_2;	        // 2分钟内中断
	_u32 _play_interrupt_3;		    //6分钟内中断
	_u32 _play_interrupt_4;		    //10分钟内中断
	_u32 _play_interrupt_5;		    //15分钟内中断
	_u32 _play_interrupt_6;		    //15分钟以上中断
	_u32 _cdn_stop_times;		    //关闭cdn次数
#ifdef IPAD_KANKAN
	BOOL _is_ad_type;			//广告类型
#endif
} VOD_REPORT_PARA;


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for task_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_reporter_module(void);
_int32 uninit_reporter_module(void);
_int32 force_close_report_module_res(void);


/* license report */
_int32 reporter_license(void);


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for download_task				        */
/*--------------------------------------------------------------------------*/
/* REPORTDWSTAT */
_int32 reporter_dw_stat(TASK* _p_task);
/* REPORTDWFAIL */
_int32 reporter_dw_fail(TASK* _p_task);

/* INSERTSRES */
_int32 reporter_insertsres(TASK* _p_task);

#ifdef ENABLE_BT
//INSERTBTRES
_int32 reporter_bt_insert_res(TASK* _p_task,_u32 file_index);
//REPORTBTDOWNLOADSTAT	
_int32 reporter_bt_download_stat(TASK* _p_task,_u32 file_index);
//REPORTBTTASKDOWNLOADSTAT	 
_int32 reporter_bt_task_download_stat(TASK* _p_task);
//BT_REPORT_SUB_TASK_NORMAL_CDN_STAT
_int32 reporter_bt_sub_normal_cdn_stat(TASK *task, _u32 file_index, int stat_type);
//BT_REPORT_NORMAL_CDN_STAT
_int32 reporter_bt_task_normal_cdn_stat(TASK *task, int stat_type);
#endif  /* ENABLE_BT */

#ifdef ENABLE_EMULE
_int32 reporter_emule_insert_res(TASK* _p_task);
_int32 reporter_emule_download_stat(TASK* _p_task);
//EMULE_REPORT_NORMAL_CDN_STAT
_int32 reporter_emule_normal_cdn_stat(TASK *task, int stat_type);
#endif
//P2SP_REPORT_NORMAL_CDN_STAT
_int32 reporter_p2sp_normal_cdn_stat(TASK *task, int stat_type);

/*
function : cancel the report
*/
//_int32 reporter_cancel(DOWNLOAD_TASK* _p_task);

#ifdef UPLOAD_ENABLE
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for upload_manager				        */
/*--------------------------------------------------------------------------*/
/* isrc_online */
_int32 reporter_isrc_online(void);

/* reportRClist */
_int32 reporter_rc_list(const LIST * rc_list,const _u32 p2p_capability);

/* insertRC */
_int32 reporter_insert_rc(const _u64 file_size,const _u8* tcid,const _u8* gcid,const _u32 p2p_capability);

/* deleteRC */
_int32 reporter_delete_rc(const _u64 file_size,const _u8* tcid,const _u8* gcid);
#endif


#ifdef EMBED_REPORT
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for embed report				        */
/*--------------------------------------------------------------------------*/

//COMMONTASKDLSTAT	 
_int32 emb_reporter_common_task_download_stat(TASK* _p_task);

#ifdef ENABLE_BT

//BTTASKDLSTAT	 
_int32 emb_reporter_bt_task_download_stat(TASK* _p_task);

//BTFILEDLSTAT	 
_int32 emb_reporter_bt_file_download_stat(TASK* _p_task, _u32 file_index);
#endif


//COMMONTASKDLFAIL	 
_int32 emb_reporter_common_task_download_fail(TASK* _p_task);

//BTFILEDLFAIL	 
_int32 emb_reporter_bt_file_download_fail(TASK* _p_task, _u32 file_index);

//REPORTDNSABNORMAL	 
_int32 emb_reporter_dns_abnormal( _u32 err_code, LIST *p_dns_ip_list, 
	char *p_hub_domain, LIST *p_parse_ip_list );


//ONLINEPEERSTAT	 
_int32 emb_reporter_online_peer_state( void );

//VODSTAT	
_int32 emb_reporter_common_stop_task( TASK* _p_task, VOD_REPORT_PARA *p_report_para );


#ifdef ENABLE_BT

_int32 emb_reporter_bt_stop_task( TASK* _p_task );

_int32 emb_reporter_bt_stop_file( TASK* _p_task, _u32 file_index, VOD_REPORT_PARA *p_report_para, BOOL is_vod_file );
#endif

#ifdef ENABLE_EMULE
_int32 emb_reporter_emule_stop_task( TASK* _p_task, VOD_REPORT_PARA *p_report_para, enum TASK_STATUS status );
#endif

#endif


/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 init_reporter_device(REPORTER_DEVICE* device,enum REPORTER_DEVICE_TYPE		_type);
_int32 uninit_reporter_device(REPORTER_DEVICE* device);
_int32 reporter_get_license(char *buffer, _int32 bufsize);
_int32 force_close_reporter_device_res(REPORTER_DEVICE* device);


/* 生命周期上报 */
_int32 reporter_task_p2sp_stat(TASK *task);
_int32 reporter_task_emule_stat(TASK *task);
_int32 reporter_task_bt_stat(TASK *task);
_int32 reporter_task_bt_file_stat(TASK* task, _u32 file_index);
_int32 report_task_create(TASK* task, _u32 file_index);

#ifdef UPLOAD_ENABLE
/*上传数据上报*/
_int32 report_upload_stat(_u32	_begin_time, _u32	_end_time , _u32 up_duration, _u32 up_use_duration, _u64 up_data_bytes,
							 _u32 up_pipe_num ,_u32 up_passive_pipe_num, _u32  * p_seqno);

#endif
#ifdef __cplusplus
}
#endif


#endif /* _REPORTER_INTERFACE_H_ */



