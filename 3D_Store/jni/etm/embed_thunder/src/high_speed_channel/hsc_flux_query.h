#ifndef     __HSC_FLUX_INFO_QUERY__
#define     __HSC_FLUX_INFO_QUERY__

#include "utility/define.h"
#ifdef      ENABLE_HSC
#include "high_speed_channel/hsc_flux_info.h"
#include "high_speed_channel/hsc_perm_query.h"

_int32      hsc_query_hsc_flux_info(HSC_QUERY_HSC_FLUX_INFO_CALLBACK);

_int32      hsc_query_flux_info_impl();

_int32      hsc_cancel_query_flux_info();

_int32      hsc_handle_recv_data(const char* p_data, const _int32 data_len);

_int32      hsc_fire_query_flux_event();

_int32 hsc_build_flux_query_cmd_struct(HSC_BATCH_COMMIT_REQUEST* request);

_int32 hsc_build_batch_commit_cmd(HSC_BATCH_COMMIT_REQUEST* p_request, void** pp_buffer, _int32* p_buffer_len);

#endif      //ENABLE_HSC
#endif      //__HSC_FLUX_INFO_QUERY__
