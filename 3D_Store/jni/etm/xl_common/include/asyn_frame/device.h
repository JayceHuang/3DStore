#ifndef _SD_DEVICE_H_00138F8F2E70_200806051403
#define _SD_DEVICE_H_00138F8F2E70_200806051403

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "platform/sd_socket.h"
#include "asyn_frame/msg.h"


/* Some Macro */
#define VALIDATE_DEVICE(dv_type, op_type)	((dv_type & op_type) || (dv_type) == DEVICE_NONE || (op_type) == OP_NONE)

/***************************
 *Device Type
 ***************************/

#define DEVICE_NONE			(0)
#define DEVICE_COMMON		(DEVICE_NONE)

#define DEVICE_BASE			(256)
#define DEVICE_MASK			(0xFF)

#define DEVICE_FS			(DEVICE_BASE << 1)
#define DEVICE_SOCKET_TCP	(DEVICE_BASE << 2)
#define DEVICE_SOCKET_UDP	(DEVICE_BASE << 3)



/***************************
 * Operating Type
 ***************************/
#define SSL_MAGICNUM (637753480)
#define OP_NONE		(0)
#define OP_COMMON	(OP_NONE)


/* DNS RESOLVE
 * operation_parameter:
 * in : OP_PARA_DNS*
 * out: OP_PARA_DNS*
 */
#define OP_DNS_RESOLVE	(1)

/* TCP */

/* operation_parameter:
 * in :  NULL
 * out:  OP_PARA_ACCEPT*
 */
#define OP_ACCEPT	(DEVICE_SOCKET_TCP | 2)

/* operation_parameter:
 * in : OP_PARA_CONN_SOCKET_RW*
 * out: OP_PARA_CONN_SOCKET_RW*
 */
#define OP_RECV		(DEVICE_SOCKET_TCP | 3)  //1027
#define OP_SEND		(DEVICE_SOCKET_TCP | 4)

/* UDP */

/* operation_parameter:
 * in : OP_PARA_NOCONN_SOCKET_RW*
 * out: OP_PARA_NOCONN_SOCKET_RW*
 */
#define OP_RECVFROM		(DEVICE_SOCKET_UDP | 5)
#define OP_SENDTO		(DEVICE_SOCKET_UDP | 6)

/* TCP && UDP */

/* operation_parameter:
 * in : SD_SOCKADDR*
 * out: NULL
 */
#define OP_CONNECT	(DEVICE_SOCKET_TCP | DEVICE_SOCKET_UDP | 7)

/* operation_parameter:
 * in : OP_PARA_DNS*
 * out: NULL
 */
#define OP_PROXY_CONNECT	(DEVICE_SOCKET_TCP  | 1)


/* FS */

/* operation_parameter:
 * in : OP_PARA_FS_OPEN*
 * out: OP_PARA_FS_OPEN*
 * opened fd returned by _device_id
 */
#define OP_OPEN		(DEVICE_FS | 8)

/* operation_parameter:
 * in : OP_PARA_FS_RW*
 * out: OP_PARA_FS_RW*
 */
#define OP_READ		(DEVICE_FS | 9)
#define OP_WRITE	(DEVICE_FS | 10)

/* operation_parameter:
 * in : NULL
 * out: NULL
 */
#define OP_CLOSE	(DEVICE_FS | 11)

#define OP_CALC_HASH  (DEVICE_FS | 12)

#define OP_MAX		(12)


/***************************
 * Opeartiong parameter:
 ***************************/

typedef struct {
	char *_host_name;
	_u32 _host_name_buffer_len;
	_u32 *_ip_list; /* caller need to pre-malloc this buffer */
	_u32 _ip_count; /* in: expect count    out: the actual count */
#ifdef DNS_ASYNC
    _u32 _server_ip[DNS_SERVER_IP_COUNT_MAX];
    _u32 _server_ip_count;
#endif
} OP_PARA_DNS;

typedef struct {
	_int32 _socket;
	SD_SOCKADDR _addr;
} OP_PARA_ACCEPT;

typedef struct {
	char *_buffer;
	_u32 _expect_size;     /* expect size */
	_u32 _operated_size;   /* actual operate size */
	BOOL _oneshot;
} OP_PARA_CONN_SOCKET_RW;

typedef struct {
	char *_buffer;
	_u32 _expect_size;    /* expect size */
	_u32 _operated_size;  /* actual operate size */
	SD_SOCKADDR _sockaddr;
} OP_PARA_NOCONN_SOCKET_RW;


/* FS Option */

/* whether if create file while it not exist. */
#define O_DEVICE_FS_CREATE		(1)

typedef struct {
	char *_filepath; /* caller need not keep this buffer */
	_u32 _filepath_buffer_len;
	_u32 _open_option;
	_u64 _file_size;
	_u64 _cur_file_size;
} OP_PARA_FS_OPEN;

typedef struct {
	char *_buffer;
	_u32 _expect_size;   /* expect size */
	_u64 _operating_offset;
	_u32 _operated_size; /* actual operate size */
} OP_PARA_FS_RW;

typedef struct 
{
    _u32 _block_no;
    _u32 _data_len;
    _u8* _data_buffer;
    _u32 _hash_result_len;
    _u8* _hash_result;
} OP_PARA_CACL_HASH;

typedef struct tagPROXY_CONNECT_DNS
{
	SOCKET		_sock;
	void*		_callback_handler;
	void*		_user_data;
	_u16		_port;			/*network byte order*/
}PROXY_CONNECT_DNS;

/* Some Macro */

_int32 alloc_and_copy_para(MSG_INFO *dest, const MSG_INFO *src);
_int32 dealloc_parameter(MSG_INFO *info);

BOOL is_a_pending_op(MSG *pmsg);

#ifdef __cplusplus
}
#endif

#endif
