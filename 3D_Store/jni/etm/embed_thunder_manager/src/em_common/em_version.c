#include "em_common/em_version.h"
#include "em_common/em_errcode.h"
#include "em_common/em_string.h"
#include "et_interface/et_interface.h"
#include "em_common/em_utility.h"


_int32 em_get_version(char *buffer, _int32 bufsize)
{
	const char * et_version =iet_get_version();	
	em_strncpy(buffer, ETM_INNER_VERSION, bufsize);
	if(em_strlen(buffer)+em_strlen(et_version)+1<bufsize)
	{
		em_strcat(buffer, " -- ", 4);
		em_strcat(buffer, et_version, em_strlen(et_version));
	}
	return SUCCESS;
}

BOOL em_is_et_version_ok(void)
{
	// 1.3.3.2
	char *point=NULL, *p_sub_ver=NULL,*p_build_ver=NULL;
	const char * et_version =iet_get_version();	
	char v_buffer[16];
	int ver=0;

	//Main Version
	point= em_strchr((char *) et_version, '.', 0);
	if(point==NULL) return FALSE;
	em_memset(v_buffer,0,16);
	em_memcpy(v_buffer,et_version,point-et_version);
	ver=em_atoi(v_buffer);
	if(ver<VALID_ET_PRI_VER) return FALSE;
	if(ver>VALID_ET_PRI_VER) return TRUE;

	// Sub Version
	p_sub_ver =point+1;
	point= em_strchr((char *) p_sub_ver, '.', 0);
	if(point==NULL) return FALSE;
	em_memset(v_buffer,0,16);
	em_memcpy(v_buffer,p_sub_ver,point-p_sub_ver);
	ver=em_atoi(v_buffer);
	if(ver<VALID_ET_SUB_VER) return FALSE;
	if(ver>VALID_ET_SUB_VER) return TRUE;

	// Build Version
	p_build_ver =point+1;
	point= em_strchr((char *) p_build_ver, '.', 0);
	if(point==NULL) return FALSE;
	em_memset(v_buffer,0,16);
	em_memcpy(v_buffer,p_build_ver,point-p_build_ver);
	ver=em_atoi(v_buffer);
	if(ver<VALID_ET_BUILD_VER) return FALSE;

	return TRUE;
}



