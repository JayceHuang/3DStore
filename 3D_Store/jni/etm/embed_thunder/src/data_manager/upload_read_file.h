

#if !defined(__UPLOAD_READ_FILE_20081126)
#define __UPLOAD_READ_FILE_20081126

#ifdef __cplusplus
extern "C" 
{
#endif

#include "data_manager/file_manager_interface.h"


_int32 up_create_file_struct( const char *p_file_name, const char *p_file_path, _u64 file_size, void *p_user, notify_file_create p_call_back, struct tagTMP_FILE **pp_file_struct );


_int32  up_file_asyn_read_buffer( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user );


_int32 up_file_close( struct tagTMP_FILE *p_file_struct, notify_file_close p_call_back, void *p_user );//?????


#ifdef __cplusplus
}
#endif

#endif // !defined(__UPLOAD_READ_FILE_20081126)

