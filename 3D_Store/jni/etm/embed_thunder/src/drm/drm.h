/*****************************************************************************
 *
 * Filename: 			drm.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2010/08/23
 *	
 * Purpose:				Define the basic structs of drm.
 *
 *****************************************************************************/

#if !defined(__DRM_20100823)
#define __DRM_20100823

#ifdef _cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_DRM 

#include "connect_manager/resource.h"
#include "connect_manager/data_pipe.h"
#include "utility/errcode.h"
#include "utility/map.h"
#include "asyn_frame/msg.h"
#include "data_manager/file_manager_interface.h"


struct _tag_range;


#define DRM_KEY_LEN 128
#define KEY_MD5_LEN 16


typedef struct tagDRM_FILE 
{
    _u32 _drm_id;
    _u32 _file_id;
    char _file_full_path[MAX_FULL_PATH_BUFFER_LEN];
    BOOL _is_certificate_ok;
    BOOL _is_net_err;
    BOOL _is_write_file_ok;
    _u32 _mid;
    _u8  _cid_hex[CID_SIZE*2+1];
    _u8 _key[DRM_KEY_LEN];
    BOOL _is_key_valid;
    _int32 _err_code;
    RESOURCE *_http_res_ptr;
    DATA_PIPE *_http_pipe_ptr;
    _u32 _timeout_id;
    _u8 _key_md5[KEY_MD5_LEN];
    _u32 _append_data_len;
    _u64 _origin_file_size;
    _u64 _curr_file_pos;
}DRM_FILE;

typedef struct tagDRM_FILE_MANAGER 
{
    LIST  drm_file_list;
}DRM_MANAGER;

/* public platform */

_int32 drm_init_module();

_int32 drm_uninit_module();

/* ui public platform */

_int32 drm_set_certificate_path( const char file_full_path[] );

_int32 drm_open_file( const char file_full_path[], _u32 *p_drm_id, _u64 *p_orgin_file_size );

_int32 drm_is_certificate_ok( _u32 drm_id, BOOL *p_is_ok );

_int32 drm_read_file( _u32 drm_id, char *buf, _u32 size, _u64 start_pos, _u32 *p_read_size );

void drm_read_buffer( DRM_FILE *p_drm_file, char *p_buffer, _u64 pos, _u64 len );

_int32 drm_close_file( _u32 drm_id );

/* private platform */

BOOL drm_is_can_read( DRM_FILE *p_drm_file  );

_int32 drm_destroy_drm_file( DRM_FILE *p_drm_file );

_int32 drm_comparator(void *E1, void *E2);

_int32 drm_handle_certificate( DRM_FILE *p_drm_file );

_int32 drm_get_certificate_file_full_path( DRM_FILE *p_drm_file, char file_name[MAX_FULL_PATH_LEN] );

_int32 drm_download_certificate( DRM_FILE *p_drm_file );

void drm_enter_err( DRM_FILE *p_drm_file, _int32 err );

_int32 drm_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 drm_destory_res_and_pips( DRM_FILE *p_drm_file );

_int32 drm_get_fille_ptr( _u32 drm_id, DRM_FILE **pp_drm_file, BOOL is_remove );

_int32 drm_get_fille_ptr_by_pipe( DATA_PIPE *p_data_pipe, DRM_FILE **pp_drm_file );

_int32 drm_rsa_decode_xlv_head( _u8 *p_input, _u32 input_len, _u8 *p_output, _u32 *p_output_len );

_int32 drm_rsa_decode_certificate( _u8 *p_input, _u32 input_len, _u8 *p_output, _u32 *p_output_len );

_int32 drm_aes_decode_xlmv_append_data( DRM_FILE *p_drm_file, char *p_append_data, _u32 data_len );

_int32 drm_aes_decode_certificate( _u8 *pDataBuff, _u32 *nBuffLen );

_int32 drm_aes_decode( _u8 *pDataBuff, _u32 *nBuffLen, _u8 *key, _u32 key_len );

_int32 drm_pipe_change_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *range, BOOL cancel_flag );

_int32 drm_pipe_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_dispatcher_range, void *p_pipe_range );

_int32 drm_pipe_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, struct _tag_range *p_dispatcher_range );

_u64 drm_pipe_get_file_size( struct tagDATA_PIPE *p_data_pipe);

_int32 drm_pipe_set_file_size( struct tagDATA_PIPE *p_data_pipe, _u64 filesize);

_int32 drm_pipe_get_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 *p_data_len);

_int32 drm_pipe_free_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 data_len);

_int32 drm_pipe_put_data_buffer( struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len, struct tagRESOURCE *p_resource );

_int32 drm_write_certificate_data( DRM_FILE *p_drm_file, char *p_buffer, _u32 data_len );

_int32 drm_pipe_read_data( struct tagDATA_PIPE *p_data_pipe, void *p_user, struct _tag_range_data_buffer* p_data_buffer, notify_read_result p_call_back );

_int32 drm_pipe_notify_dispatch_data_finish( struct tagDATA_PIPE *p_data_pipe);

_int32 drm_pipe_get_file_type( struct tagDATA_PIPE* p_data_pipe);

#endif

#ifdef _cplusplus
}
#endif

#endif // !defined(__DRM_20100823)




