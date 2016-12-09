#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_setting.h"
#include "utility/base64.h"
#include "utility/settings.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_get_user_id(_u8 user_id[USER_ID_SIZE])
{
	_u8 i = 0;
	char str[33] = {0};
	settings_get_str_item("emule.userid", str);
	if(sd_strlen(str) == 0)
	{
		sd_memset(user_id, 0, USER_ID_SIZE);
		for(i = 0; i < USER_ID_SIZE; ++i)
		{
			user_id[i] = sd_rand() % 256;
		}
		user_id[5] = 14;   // emule 客户端的特殊标识
		user_id[14]= 111;  // emule 客户端的特殊标识
		return emule_set_user_id(user_id);
	}
	else
    {
        _u8 user_id_for_decode[2*USER_ID_SIZE + 1] = {0};
        _int32 output_size_for_decode = 0;
        sd_base64_decode(str, user_id_for_decode, &output_size_for_decode);
        //LOG_DEBUG("sd_base64_decode, output_size_for_decode = %d.", output_size_for_decode);
        printf("sd_base64_decode, output_size_for_decode = %d.", output_size_for_decode);
        sd_memcpy(user_id, user_id_for_decode, USER_ID_SIZE);

        return SUCCESS;
    }
}

_int32 emule_set_user_id(_u8 user_id[USER_ID_SIZE])
{
	_int32 ret = SUCCESS;
	char str[33] = {0};
	ret = sd_base64_encode((const _u8*)user_id,USER_ID_SIZE, str);
	CHECK_VALUE(ret);
	return settings_set_str_item("emule.userid", str);
}

void emule_log_print_user_id(_u8 user_id[USER_ID_SIZE])
{
#ifdef _LOGGER
    _u8 log_user_id[2*USER_ID_SIZE + 1] = {0};
    str2hex((const char*)user_id, USER_ID_SIZE, (char*)log_user_id, 2*USER_ID_SIZE);
    LOG_DEBUG("user_id = %s.", log_user_id);
#endif
}

BOOL emule_enable_p2sp(void)
{
	BOOL result = TRUE;
	settings_get_bool_item("emule.enable_p2sp", &result);
	return result;
}

BOOL emule_enable_server(void)
{
	BOOL result = TRUE;
	settings_get_bool_item("emule.enable_server", &result);
	return result;
}

BOOL emule_enable_obfuscation(void)
{
	BOOL result = FALSE;
	settings_get_bool_item("emule.enable_obfuscation", &result);
	return result;
}

BOOL emule_enable_compress(void)
{
	BOOL result = TRUE;
	settings_get_bool_item("emule.enable_compress", &result);
	return result;
}

BOOL emule_enable_source_exchange(void)
{
	BOOL result = TRUE;
	settings_get_bool_item("emule.enable_source_exchange", &result);
	return result;
}

BOOL emule_designate_server(_u32* ip, _u16* port)
{
	char ip_str[24] = {0};
	_int32 port32 = 0;
	settings_get_str_item("emule.server_ip", ip_str);
	settings_get_int_item("emule.server_port", &port32);
	if(sd_strlen(ip_str) == 0)
		return FALSE;
	*ip = sd_inet_addr(ip_str);
	if(port32 == 0)
		return FALSE;
	*port = port32;
	return TRUE;
}

BOOL emule_enable_kad(void)
{
	BOOL result = TRUE;
	settings_get_bool_item("emule.enable_kad", &result);
	return result;
}

BOOL emule_enable_thunder_tag(void)
{
	BOOL result = FALSE;
	settings_get_bool_item("emule.enable_thunder_tag", &result);
	return result;
}

#endif /* ENABLE_EMULE */

