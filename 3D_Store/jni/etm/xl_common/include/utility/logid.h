#ifndef SD_LOGID_H_00138F8F2E70_200806172148
#define SD_LOGID_H_00138F8F2E70_200806172148

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "platform/sd_task.h"

#ifdef _LOGGER


#include <stdio.h>


#define LOGID_COUNT				(130)

/* DEFINE OF LOGID */
#define LOGID_COMMON			         (0)
#define LOGID_INTERFACE			     (1)
#define LOGID_ASYN_FRAME		     (2)
#define LOGID_FTP_PIPE                  (3)
#define LOGID_HTTP_PIPE                (4)
#define LOGID_TASK_MANAGER         (5)
#define LOGID_DISPATCHER              (6)
#define LOGID_ASYN_FRAME_TEST	 (7)
#define LOGID_CONNECT_MANAGER   (8)
#define LOGID_FILE_MANAGER	         (9)
#define LOGID_SOCKET_PROXY		(10)
#define LOGID_RES_QUERY			(11)
#define LOGID_P2P_PIPE			(12)
#define LOGID_DATA_MANAGER	         (13)
#define LOGID_CORRECT_MANAGER	         (14)
#define LOGID_DATA_CHECK	         (15)
#define LOGID_DATA_RECEIVE	         (16)
#define LOGID_FILE_INFO         (17)
#define LOGID_RANGE_MANAGER         (18)
#define LOGID_DATA_BUFFER         (19)
#define LOGID_SETTINGS         (20)
#define LOGID_RANGE_LIST         (21)
#define LOGID_SOCKET_REACTOR         (22)
#define LOGID_FS_REACTOR         (23)
#define LOGID_DNS_REACTOR         (24)
#define LOGID_DEVICE_HANDLER        (25)
#define LOGID_MEMPOOL	        (26)
#define LOGID_MSGLIST			(27)
#define LOGID_TIMER				(28)
#define LOGID_DOWNLOAD_TASK		(29)
#define LOGID_BT_DOWNLOAD       (30)
#define LOGID_REPORTER       (31)
#define	LOGID_P2P_TRANSFER_LAYER (32)
#define	LOGID_UPLOAD_MANAGER (33)
#define LOGID_P2SP_TASK		(34)
#define	LOGID_VOD_DATA_MANAGER (35)
#define	LOGID_INDEX_PARSER (36)
#define	LOGID_HTTP_SERVER (37)
#define LOGID_AUTO_LAN	(38)
#define LOGID_DNS_PARSER         (39)
#define LOGID_EMULE		(40)
#define LOGID_DK_QUERY         (41)
#define LOGID_CMD_INTERFACE    (42)

#define EM_LOGID_COMMON			         (43)
#define EM_LOGID_INTERFACE			     (44)
#define EM_LOGID_ASYN_FRAME		     (45)
#define EM_LOGID_SCHEDULER         (46)
#define EM_LOGID_SETTINGS         (47)
#define EM_LOGID_MEMPOOL	        (48)
#define EM_LOGID_MSGLIST			(49)
#define EM_LOGID_TIMER				(50)
#define EM_LOGID_DOWNLOAD_TASK		(51)
#define EM_LOGID_TREE_MANAGER		(52)
#define EM_LOGID_VOD_MANAGER		(53)
#define EM_LOGID_REMOTE_CONTROL		(54)

#define LOGID_BITMAP			(55)
#define LOGID_CANVAS			(56)
#define LOGID_EUI				(57)
#define LOGID_EVENT				(58)
#define LOGID_EVENT_DRIVER		(59)
#define LOGID_EVENT_PROCESSER	(60)
#define LOGID_EVENT_QUEUE		(61)
#define LOGID_GAL				(62)
#define LOGID_GRAPHIC			(63)
#define LOGID_IAL				(64)
#define LOGID_WIDGET			(65)
#define LOGID_WINDOW			(66)
#define LOGID_CURSOR			(67)
#define LOGID_SCREEN			(68)
#define LOGID_SHORTCUT			(69)
#define LOGID_CACHE				(70)
#define LOGID_SYMBIAN_APPUI		(71)
#define LOGID_SYMBIAN_VIEW		(72)
#define LOGID_UI				(73)
#define LOGID_PLAYER			(74)
#define LOGID_UI_PAINT_FUNC		(75)
#define LOGID_TEXT_RENDER		(76)
#define LOGID_MEMBER	        (77)
#define LOGID_XML				(78)
#define LOGID_MINIBROWSER		(79)
#define LOGID_IMPORT_FILE		(80)
#define LOGID_XMP				(81)
#define LOGID_DRM               (82)
#define LOGID_VULU                        (83)
#define LOGID_KANKAN			(84)
#define LOGID_EKK_PLAYER            (85)
#define LOGID_UI_INTERFACE		(86)
#define LOGID_JSON_INTERFACE			(87)
#define LOGID_HIGH_SPEED_CHANNEL		(88)
#define LOGID_LIXIAN					(89)
#define LOGID_BT_PIPE					(90)
#define LOGID_BT_MAGNET					(91)
#define LOGID_BENCODE					(92)
#define LOGID_BT_TORRENT_PARSER			(93)
#define LOGID_BT_FILE_MANAGER			(94)
#define LOGID_OPENGL			(95)
#define LOGID_WALK_BOX		(96)
#define LOGID_NEIGHBOR		(97)
#define EM_LOGID_REACTOR           (100)
#define EM_LOGID_BOX_LOGIN					(101)
#define EM_LOGID_ENV_DETECTOR			(102)
#define EM_LOGID_LOCAL_CONTROL			(103)
#define EM_LOGID_LICENSE				(104)
#define LOGID_NET_STATE_CHECK           (105)
#define EM_LOGID_REPORTER               (106)

/* @Simple Function@
 * Return : the logdesc about this logid
 */
const char* get_logdesc(_int32 id);


/* @Simple Function@
 * Return : Current logger level of logid
 */
_int32 current_loglv(_int32 logid);


/* @Simple Function@
 * Return : FILE* of current log-file
 */
#if defined(LINUX)
FILE* log_file(void);
void check_logfile_size();
_u32 log_max_filesize();
#else
void check_logfile_size(void);

#endif

/* @Simple Function@
 * Return : lock of current log-file
 */
TASK_LOCK *log_lock(void);

#endif


_int32 sd_log_init(const char *conf_file);

_int32 sd_log_reload(const char *conf_file);

_int32 sd_log_uninit(void);


/* LOG LEVEL */
#define LOG_OFF_LV		(0)

#define LOG_ERROR_LV	(1)
#define LOG_DEBUG_LV	(2)


#ifdef __cplusplus
}
#endif

#endif
