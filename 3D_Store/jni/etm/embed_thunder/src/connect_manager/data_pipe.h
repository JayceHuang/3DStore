//
//////////////////////////////////////////////////////////////////////

#if !defined(__DATA_PIPE_INTERFACE_20080617)
#define __DATA_PIPE_INTERFACE_20080617

#ifdef __cplusplus
extern "C"
{
#endif

#include "utility/range.h"
#include "connect_manager/resource.h"
struct RESOURCE;


#ifdef _CONNECT_DETAIL

#define ID_INFO_LEN 64

typedef struct tagPEER_PIPE_INFO
{
    _u32    _connect_type;
    char    _internal_ip[24];
    char    _external_ip[24];
    _u16    _internal_tcp_port;
    _u16    _internal_udp_port;
    _u16    _external_port;
    char    _peerid[20 + 1];
    _u32    _speed;
    _u32    _upload_speed;
    _u32    _score;
    _u32    _pipe_state;
} PEER_PIPE_INFO;

typedef struct tagPEER_PIPE_INFO_ARRAY
{
    PEER_PIPE_INFO _pipe_info_list[ PEER_INFO_NUM ];
    _u32 _pipe_info_num;
} PEER_PIPE_INFO_ARRAY;

typedef struct tagSERVER_PIPE_INFO
{
    char _id_str[ID_INFO_LEN];
    _u32 _speed;
} SERVER_PIPE_INFO;

typedef struct tagSERVER_PIPE_INFO_ARRAY
{
    SERVER_PIPE_INFO _pipe_info_list[ SERVER_INFO_NUM ];
    _u32 _pipe_info_num;
} SERVER_PIPE_INFO_ARRAY;

#endif


typedef enum tagPIPE_TYPE 
{
	P2P_PIPE=201, 
	HTTP_PIPE, 
	FTP_PIPE, 
	BT_PIPE, 
	EMULE_PIPE 
} PIPE_TYPE;

typedef  enum tagPIPE_STATE 
{
	PIPE_IDLE=0, 
	PIPE_CONNECTING, 
	PIPE_CONNECTED, 
	PIPE_CHOKED, 
	PIPE_DOWNLOADING, 
	PIPE_FAILURE 
} PIPE_STATE;

#ifdef UPLOAD_ENABLE
typedef enum tagPEER_UPLOAD_STATE
{
    UPLOAD_CHOKE_STATE = 0, 
	UPLOAD_UNCHOKE_STATE, 
	UPLOAD_FIN_STATE, 
	UPLOAD_FAILED_STATE
} PEER_UPLOAD_STATE;
#endif

typedef struct tagDISPATCH_INFO
{
    _u32 _time_stamp;//set when pipe status changed and need to mark(millisecond)
    _u32 _speed;
    _u32 _score;
    _u32 _try_to_filter_time;

    _u32 _max_speed;
#ifdef _DEBUG
    _u32 _old_speed;
    _u32 _old_score;
    PIPE_STATE _old_state;
#endif
    BOOL _is_support_range;
    BOOL _is_support_long_connect;
    PIPE_STATE _pipe_state;

#ifdef UPLOAD_ENABLE
    PEER_UPLOAD_STATE _pipe_upload_state;
#endif

    RANGE_LIST _can_download_ranges;
    RANGE_LIST _uncomplete_ranges;
    RANGE   _down_range;

    RANGE _correct_error_range;
    BOOL  _cancel_down_range;
    BOOL _is_global_filted;
    void *_global_wrap_ptr;
    BOOL _is_err_get_buffer;
    _u32 _last_recv_data_time;      //最近一次收到数据的时间，MAX_U32表示该值无效
    _u32 _pipe_create_time;
} DISPATCH_INFO;

typedef struct tagDATA_PIPE
{
    PIPE_TYPE  _data_pipe_type;
    DISPATCH_INFO _dispatch_info;
    RESOURCE *_p_resource;
    struct tagPIPE_INTERFACE *_p_pipe_interface;
    void *_p_data_manager;
    _u32 _id;       //just for mini http
    BOOL   _is_user_free;
} DATA_PIPE;

typedef struct tagPIPE_LIST
{
    LIST  _data_pipe_list;
} PIPE_LIST;


void init_pipe_info( DATA_PIPE *p_data_pipe, PIPE_TYPE type_info, RESOURCE *p_res, void *p_data_manager );
void uninit_pipe_info( DATA_PIPE *p_data_pipe );

void pipe_set_err_get_buffer( DATA_PIPE *p_data_pipe, BOOL is_err );
BOOL pipe_is_err_get_buffer( DATA_PIPE *p_data_pipe );

void dp_set_pipe_interface( DATA_PIPE *p_data_pipe, struct tagPIPE_INTERFACE *p_pipe_interface );
_u32 dp_get_pipe_file_index( DATA_PIPE *p_data_pipe );

PIPE_STATE dp_get_state(DATA_PIPE *p_data_pipe );
void dp_set_state(DATA_PIPE *p_data_pipe, PIPE_STATE state);
#ifdef UPLOAD_ENABLE
PEER_UPLOAD_STATE dp_get_upload_state(DATA_PIPE* p_data_pipe);
#endif

/*****************************************************************************
 * Handle _down_range in the DISPATCH_INFO.
 *****************************************************************************/

_int32 dp_get_download_range( DATA_PIPE *p_data_pipe, RANGE *p_download_range );
_int32 dp_set_download_range( DATA_PIPE *p_data_pipe, const RANGE *p_download_range );
_int32 dp_clear_download_range( DATA_PIPE *p_data_pipe );

#ifdef ENABLE_BT
_int32 dp_get_bt_download_range( DATA_PIPE *p_data_pipe, RANGE *p_download_range );
_int32 dp_set_bt_download_range( DATA_PIPE *p_data_pipe, const RANGE *p_download_range );
_int32 dp_clear_bt_download_range( DATA_PIPE *p_data_pipe );
_int32 dp_switch_range_2_bt_range( DATA_PIPE *p_data_pipe, const RANGE *p_range, BT_RANGE *p_bt_range );
#endif


/*****************************************************************************
 * Handle can_download_ranges in the DISPATCH_INFO.
 *****************************************************************************/

_int32 dp_add_can_download_ranges( DATA_PIPE *p_data_pipe, RANGE *p_can_download_range );
_u32 dp_get_can_download_ranges_list_size( DATA_PIPE *p_data_pipe );
_int32 dp_clear_can_download_ranges_list( DATA_PIPE *p_data_pipe );

#ifdef ENABLE_BT
_int32 dp_add_bt_can_download_ranges( DATA_PIPE *p_data_pipe, BT_RANGE *p_can_download_range );
#endif


/*****************************************************************************
 * Handle uncomplete_ranges in the DISPATCH_INFO.
 *****************************************************************************/
_int32 dp_add_uncomplete_ranges( DATA_PIPE *p_data_pipe, const RANGE *p_uncomplete_range );
_u32 dp_get_uncomplete_ranges_list_size( DATA_PIPE *p_data_pipe );
_int32 dp_delete_uncomplete_ranges( DATA_PIPE *p_data_pipe, RANGE *p_range );

#ifdef ENABLE_BT
_int32 dp_add_bt_uncomplete_ranges( DATA_PIPE *p_data_pipe, const BT_RANGE *p_uncomplete_range );
_int32 dp_get_bt_uncomplete_ranges_head_range( DATA_PIPE *p_data_pipe, BT_RANGE *p_head_range );
_int32 dp_delete_bt_uncomplete_ranges( DATA_PIPE *p_data_pipe, BT_RANGE *p_range );
#endif

_int32 dp_clear_uncomplete_ranges_list( DATA_PIPE *p_data_pipe );
_int32 dp_get_uncomplete_ranges_head_range( DATA_PIPE *p_data_pipe, RANGE *p_head_range );

_int32 dp_adjust_uncomplete_2_can_download_ranges( DATA_PIPE *p_data_pipe );
void *dp_get_task_ptr( DATA_PIPE *p_data_pipe );

#ifdef __cplusplus
}
#endif

#endif // !defined(__DATA_PIPE_INTERFACE_20080617)


