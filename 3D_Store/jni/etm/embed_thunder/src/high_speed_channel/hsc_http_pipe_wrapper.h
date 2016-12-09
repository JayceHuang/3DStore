#ifndef     __hsc_http_pipe_wrapper__
#define     __hsc_http_pipe_wrapper__

#include    "utility/define.h"
#ifdef      ENABLE_HSC
#include    "hsc_data_pipe_wrapper.h"

_int32  hsc_init_hsc_http_pipe_interface();

_int32  hsc_init_http_hdpi(HSC_DATA_PIPE_INTERFACE* p_HDPI, const char* host, const _u32 host_len, const _u32 port);

_int32  hsc_uninit_http_hdpi(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32 hsc_send_http_data(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_cancel_http_sending(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32 hsc_find_pipe_from_manager_by_pipe(DATA_PIPE* p_pipe, HSC_DATA_PIPE_INTERFACE** pp_HDPI);


_int32 hsc_http_pipe_change_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *range, BOOL cancel_flag);

_int32 hsc_http_pipe_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_dispatcher_range, void *p_pipe_range);

_int32 hsc_http_pipe_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, struct _tag_range *p_dispatcher_range);

_u64 hsc_http_pipe_get_file_size(struct tagDATA_PIPE *p_data_pipe);

_int32 hsc_http_pipe_set_file_size(struct tagDATA_PIPE *p_data_pipe, _u64 filesize);

_int32 hsc_http_pipe_get_data_buffer(struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 *p_data_len);

_int32 hsc_http_pipe_free_data_buffer(struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 data_len);

_int32 hsc_http_pipe_put_data_buffer(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_range, char **pp_data_buffer, _u32 data_len, _u32 buffer_len, struct tagRESOURCE *p_resource);

_int32 hsc_http_pipe_read_data(struct tagDATA_PIPE *p_data_pipe, void *p_user, struct _tag_range_data_buffer *p_data_buffer, notify_read_result p_call_back);

_int32 hsc_http_pipe_notify_dispatcher_data_finish(struct tagDATA_PIPE *p_data_pipe);

_int32 hsc_http_pipe_get_file_type(struct tagDATA_PIPE *p_data_pipe);

#endif      //ENABLE_HSC
#endif      //__hsc_http_pipe_wrapper__
