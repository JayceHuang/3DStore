#ifndef _REPORTER_MOBILE_H_20100420_1455_
#define _REPORTER_MOBILE_H_20100420_1455_
#if defined(MOBILE_PHONE)

#ifdef __cplusplus
extern "C" 
{
#endif

/****************************************************/

#include "utility/define.h"

/****************************************************/

// 用户登录行为。对应值为:
// 1-安装应用程序
// 2-打开应用程序
// 3-退出应用程序
#define REPORTER_USER_ACTION_TYPE_LOGIN (0)

// 播放器行为。对应值为:
// 1-打开
// 2-退出
#define REPORTER_USER_ACTION_TYPE_PLAYER (1)

// TEXT行为。对应值为:
// 1-mov
// 2-text
// 3-software
// 9-other
#define REPORTER_USER_ACTION_TYPE_DOWNLOAD_FILE (2)

//看看播放器打开行为
// 1-表示进入看看播放器
#define REPORTER_USER_ACTION_ENTER_KANKAN_PLAYER   (3)

//看看播放器播放信息
//11001-表示remark(data)字段存放的是KANKAN_PLAY_INFO_STAT数据协议包
//12001-表示remark(data)字段存放的是KANKAN_USER_ACTION_STAT数据协议包
#define REPORTER_USER_ACTION_PLAY_INFO    (999)

#define KANKAN_PLAY_INFO_STAT_CMD_TYPE    (11001)
#define KANKAN_USER_ACTION_STAT_CMD_TYPE    (11003)

/****************************************************/

// 初始化
_int32 reporter_init(const char *record_dir, _u32 record_dir_len);

// 反初始化
_int32 reporter_uninit();

// 设置peerid.
_int32 reporter_set_peerid(char *peer_id, _u32 size);

// 设置ver.
_int32 reporter_set_version(const char *ver, _int32 size,_int32 product,_int32 partner_id);

//将安装或者开机的行为记录在上报文件中
_int32 reporter_new_install(BOOL is_install);

// 将用户的行为记录在文件中。
_int32 reporter_mobile_user_action_to_file(_u32 action_type, _u32 action_value, void *data, _u32 data_len);

_int32 reporter_set_version_handle(void * _param);

_int32 get_ui_version(char *buffer, _int32 bufsize);

_int32 reporter_new_install_handle(void * _param);

_int32 reporter_mobile_user_action_to_file_handle( void * _param );

//timer 上报用户行为记录文件的开关，默认开启
_int32 reporter_mobile_enable_user_action_report_handle(void * _param);

// 当连接上网络时，上报网络信息。
_int32 reporter_mobile_network(void);

// 从文件中读取统计信息，并上报。
_int32 reporter_from_file();

// 上报成功回调函数。
_int32 reporter_from_file_callback();

/****************************************************/

#ifdef __cplusplus
}
#endif

#endif
#endif
