#if !defined(__SETTINGS_H_20080726)
#define __SETTINGS_H_20080726

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/errcode.h"
#include "utility/list.h"



typedef struct 
{
	char  _name[MAX_CFG_NAME_LEN];
	char _value[MAX_CFG_VALUE_LEN];

}CONFIG_ITEM;

typedef struct  
{
	LIST  _item_list;
	//char _peerid[MAX_PEER_ID_LEN];
	//char _product_peerid[MAX_PEER_ID_LEN];
	//char _product_version[MAX_VERSION_LEN];
	//char  _local_ip[MAX_LOCAL_IP_LEN];
	//_u64 max_download_filesize; //最大可下载的文件大小, 单位M
	//_u32 _product_flag;
	//_u32 _product_release_id;

	//_u32 min_download_filesize; //最小可下载的文件大小, 单位B
	//_u32 max_running_tasks;
	//_u32 max_tasks;
	//_u32 max_speed;

	BOOL _enable_cfg_file;

}SETTINGS;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other modules				        */
/*--------------------------------------------------------------------------*/
/* *_item_value 为一个in/out put 参数，若找不到与_item_name 对应的项则以*_item_value 为默认值生成新项 */
_int32 settings_get_str_item(char * _item_name,char *_item_value);
_int32 settings_get_int_item(char * _item_name,_int32 *_item_value);
_int32 settings_get_bool_item(char * _item_name,BOOL *_item_value);
_int32 settings_set_str_item(char * _item_name,char *_item_value);
_int32 settings_set_int_item(char * _item_name,_int32 _item_value);
_int32 settings_set_bool_item(char * _item_name,BOOL _item_value);
//_int32 settings_del_item(char * _item_name);


/*--------------------------------------------------------------------------*/
/*                Interfaces provid just for task_manager				        */
/*--------------------------------------------------------------------------*/

_int32 settings_initialize( void);
_int32 settings_uninitialize( void);
_int32 settings_config_load(char* _cfg_file_name,LIST * _p_settings_item_list);
_int32 settings_config_save(void);

_int32 setting_cfg_dir(char* system_dir, _int32 system_dir_len);

char * setting_get_cfg_dir();

#ifdef __cplusplus
}
#endif


#endif // !defined(__SETTINGS_H_20080726)
