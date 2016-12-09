#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_utility.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/settings.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_get_nick_name(char* nick_name, _u32 len)
{
	sd_memcpy(nick_name, "[CHN]yourname", sizeof("[CHN]yourname"));
	settings_get_str_item("emule.nickname", nick_name);
	return SUCCESS;
}

_int32 emule_get_user_id_type(_u8 user_id[USER_ID_SIZE])
{
	if(user_id[5] == 13 && user_id[14] == 110)
		return SO_OLDEMULE;
	else if (user_id[5] == 14 && user_id[14] == 111)
		return SO_EMULE;
	else if (user_id[5] == 'M' && user_id[14] == 'L')
		return SO_MLDONKEY;
	else
		return SO_UNKNOWN;
}

_int32 emule_log_print_file_id(_u8 file_id[FILE_ID_SIZE], const char *desc)
{
    _int32 ret = SUCCESS;

#ifdef _LOGGER    
    if (file_id != NULL)
    {
        char *log_file_id = NULL;
        ret = sd_malloc(2*FILE_ID_SIZE+1, (void **)&log_file_id);
        CHECK_VALUE(ret);
        sd_memset((void *)log_file_id, 0, 2*FILE_ID_SIZE+1);
        str2hex((const char*)file_id, FILE_ID_SIZE, log_file_id, 2*FILE_ID_SIZE);
        LOG_DEBUG("emule_log_print_file_id, %s, file_id = %s.", desc, log_file_id);
        sd_free(log_file_id);
        log_file_id = NULL;
    }    
#endif

    return ret;
}

_int32 emule_log_print_gcid(_u8 gcid[CID_SIZE], const char *desc)
{
    _int32 ret = SUCCESS;

#ifdef _LOGGER
    if (gcid != NULL)
    {
        char *log_gcid = NULL;
        ret = sd_malloc(2*CID_SIZE+1, (void **)&log_gcid);
        CHECK_VALUE(ret);
        sd_memset((void *)log_gcid, 0, 2*CID_SIZE+1);
        str2hex((const char*)gcid, CID_SIZE, log_gcid, 2*CID_SIZE);
        LOG_DEBUG("emule_log_print_gcid, %s, log_gcid = %s.", desc, log_gcid);
        sd_free(log_gcid);
        log_gcid = NULL;
    }
#endif

    return ret;
}

#endif /* ENABLE_EMULE */

