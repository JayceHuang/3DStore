/*****************************************************************************
 *
 * Filename: 			pipe_interface.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/03/04
 *	
 * Purpose:				pipe_interface call by dispatcher module and pipe module.
 *
 *****************************************************************************/

#if !defined(__PIPE_INTERFACE_20080920)
#define __PIPE_INTERFACE_20080920

#ifdef __cplusplus
extern "C" 
{
#endif


#include "connect_manager/pipe_function_table.h"
#include "data_manager/data_receive.h"


struct tagDATA_PIPE;
struct tagRESOURCE;


typedef enum tagPIPE_INTERFACE_TYPE
{ 
	COMMON_PIPE_INTERFACE_TYPE,
	SPEEDUP_PIPE_INTERFACE_TYPE,
	BT_PIPE_INTERFACE_TYPE,
	EMULE_PIPE_INTERFACE_TYPE,
	EMULE_SPEEDUP_PIPE_INTERFACE_TYPE,
	BT_MAGNET_INTERFACE_TYPE,
	UNKNOWN_PIPE_INTERFACE_TYPE
} PIPE_INTERFACE_TYPE;

typedef  enum tagPIPE_FILE_TYPE
{ 
	PIPE_FILE_UNKNOWN_TYPE,
	PIPE_FILE_BINARY_TYPE,
	PIPE_FILE_HTML_TYPE 
} PIPE_FILE_TYPE;

typedef struct tagPIPE_INTERFACE
{
	enum tagPIPE_INTERFACE_TYPE _pipe_interface_type;
	_u32 _file_index;//it must be INVALID_FILE_INDEX if it not have sub connect_manager.
	void *_range_switch_handler;
	void **_func_table_ptr;
} PIPE_INTERFACE;



/*****************************************************************************
 * init pipe_interface function. 
 *****************************************************************************/

void pi_init_pipe_interface(PIPE_INTERFACE *p_pipe_interface, 
    enum tagPIPE_INTERFACE_TYPE pipe_interface_type, 
    _u32 file_index, 
    void *range_switch_handler, 
    void **pp_fuct_table);


/*****************************************************************************
 * get pipe_interface info function. 
 *****************************************************************************/
_u32 pi_get_file_index(PIPE_INTERFACE *p_pipe_interface);


/*****************************************************************************
 * change range platform:
 * dispatcher need to call the pi_pipe_change_range to change range.
 *****************************************************************************/
_int32 pi_pipe_change_range(struct tagDATA_PIPE *p_data_pipe, RANGE *p_range, BOOL cancel_flag);


/*****************************************************************************
 * Handle download_range of DISPATCH_INFO used by http/ftp/p2p/bt pipe module.
 *****************************************************************************/
_int32 pi_pipe_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe,
    const RANGE *p_dispatcher_range, 
    void *p_pipe_range);

_int32 pi_pipe_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe,
    const void *p_pipe_range,
    RANGE *p_dispatcher_range);


/*****************************************************************************
 * Handle platform of data_manager used by http/ftp/p2p/bt pipe module.
 *****************************************************************************/
_u64 pi_get_file_size(struct tagDATA_PIPE *p_data_pipe);

_int32 pi_set_file_size(struct tagDATA_PIPE *p_data_pipe,_u64 filesize);

_int32 pi_get_data_buffer(struct tagDATA_PIPE *p_data_pipe,
	char **pp_data_buffer, _u32 *p_data_len);

_int32 pi_put_data(struct tagDATA_PIPE *p_data_pipe, 
	const RANGE *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len,
	struct tagRESOURCE *p_resource);

_int32  pi_pipe_read_data_buffer(struct tagDATA_PIPE *p_data_pipe,  
	RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user);

_int32 pi_free_data_buffer(struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 data_len);

_int32 pi_notify_dispatch_data_finish(struct tagDATA_PIPE *p_data_pipe);

PIPE_FILE_TYPE pi_get_file_type(struct tagDATA_PIPE *p_data_pipe);
RANGE_LIST* pi_get_checked_range_list(struct tagDATA_PIPE *p_data_pipe);

PIPE_INTERFACE_TYPE pi_get_pipe_interface_type(struct tagDATA_PIPE *p_data_pipe);


#ifdef __cplusplus
}
#endif

#endif // !defined(__PIPE_INTERFACE_20080920)

