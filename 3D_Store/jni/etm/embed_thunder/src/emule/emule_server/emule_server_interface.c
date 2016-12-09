#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_server_interface.h"
#include "emule_server_impl.h"
#include "../emule_utility/emule_setting.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_server_start(void)
{
	LOG_DEBUG("emule_server_start.");
	emule_server_init();
	emule_server_load_impl();
	return emule_try_connect_server();
}

_int32 emule_server_stop(void)
{
	LOG_DEBUG("emule_server_stop.");
	return emule_server_uninit();
}

_int32 emule_server_close(void)
{
	return SUCCESS;
}

_int32 emule_server_query_source(_u8 file_id[FILE_ID_SIZE], _u64 file_size)
{
	if(emule_enable_server() == FALSE)
		return SUCCESS;
	LOG_DEBUG("emule_server_query_source, file_size = %llu.", file_size);
	return emule_server_query_source_impl(file_id, file_size);
}

BOOL emule_is_local_server(_u32 ip, _u16 port)
{
	return emule_is_local_server_impl(ip, port);
}


#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

