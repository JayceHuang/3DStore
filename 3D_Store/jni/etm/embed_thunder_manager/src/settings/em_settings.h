#if !defined(__EM_SETTINGS_H_20090915)
#define __EM_SETTINGS_H_20090915

#ifdef __cplusplus
extern "C" 
{

#endif

#include "em_common/em_define.h"
#include "em_common/em_errcode.h"
#include "em_common/em_list.h"


#include "utility/settings.h"

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other modules				        */
/*--------------------------------------------------------------------------*/
/* *_item_value 为一个in/out put 参数，若找不到与_item_name 对应的项则以*_item_value 为默认值生成新项 */
_int32 em_settings_get_str_item(char * _item_name,char *_item_value);
_int32 em_settings_get_int_item(char * _item_name,_int32 *_item_value);
_int32 em_settings_get_bool_item(char * _item_name,BOOL *_item_value);
_int32 em_settings_set_str_item(char * _item_name,char *_item_value);
_int32 em_settings_set_int_item(char * _item_name,_int32 _item_value);
_int32 em_settings_set_bool_item(char * _item_name,BOOL _item_value);
_int32 em_settings_del_item(char * _item_name);


/*--------------------------------------------------------------------------*/
/*                Interfaces provid just for task_manager				        */
/*--------------------------------------------------------------------------*/

_int32 em_settings_initialize( void);
_int32 em_settings_uninitialize( void);
_int32 em_settings_config_load(char* _cfg_file_name,LIST * _p_settings_item_list);
_int32 em_settings_config_save(void);


#ifdef __cplusplus
}

#endif


#endif // !defined(__SETTINGS_H_20090915)
