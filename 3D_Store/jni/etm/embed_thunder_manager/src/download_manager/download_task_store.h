#if !defined(__DOWNLOAD_TASK_STORE_H_20090915)
#define __DOWNLOAD_TASK_STORE_H_20090915

#include "em_common/fm_common.h"
#include "download_manager/download_task_interface.h"

/* 以下为虚拟结构体，只在存储到文件中才体现 ,因此切不可定义宏STRUCT_FOR_STORING */
#ifdef STRUCT_FOR_STORING
/* Structures of ETM for  storing */
typedef struct t_task_store_file{
	EM_STORE_HEAD _head;
	_u32 _total_task_num;		//这个主要用来累加生成新的task_id
	u32 _running_task_id[MAX_ALOW_TASKS]; //对应ETM_RUNNING_TASK _running_task[MAX_ALOW_TASKS]; 中的task_id
	u32 _dling_task_num;
	u32 _dling_task_order[MAX_TASK_NUM];	//对应LIST  _dling_task_order 中的task_id
	DT_TASK_STORE  task;  //....
}TASK_STORE_FILE;


#endif /* 0 */

/* Structures for task storing 
typedef struct t_dt_task_store
{
	_u16 _valid; 	//该任务信息是否有效，如果该任务被删除，或数据长度被修改(修改的执行是把旧的信息置为_valid=0，然后在文件末尾重写该任务更新后的信息)
	_u16 _crc;	//从u32 _len之后(不包括u32 _len)开始到这个任务(有可能是ETM_P2SP_TASK 也有可能是ETM_BT_TASK)的信息的最后一byte的检验值
	_u32 _len;	//从u32 _len之后(不包括u32 _len)开始到这个任务(有可能是ETM_P2SP_TASK 也有可能是ETM_BT_TASK)的信息的最后一byte的长度
	BOOL _have_name;
	_u8 _eigenvalue[CID_SIZE];
	TASK_INFO task_info;//P2SP_TASK/BT_TASK _task_info;   // 任务详细信息

}DT_TASK_STORE;
*/



/* Structures for task storing */
typedef struct t_dt_task_store_head
{
	_u16 _valid; 	
	_u16 _crc;	
	_u32 _len;	
}DT_TASK_STORE_HEAD;
typedef struct t_dt_task_store
{
	_u16 _valid; 	//该任务信息是否有效，如果该任务被删除，或数据长度被修改(修改的执行是把旧的信息置为_valid=0，然后在文件末尾重写该任务更新后的信息)
	_u16 _crc;	//从u32 _len之后(不包括u32 _len)开始到这个任务(有可能是ETM_P2SP_TASK 也有可能是ETM_BT_TASK)的信息的最后一byte的检验值
	_u32 _len;	//从u32 _len之后(不包括u32 _len)开始到这个任务(有可能是ETM_P2SP_TASK 也有可能是ETM_BT_TASK)的信息的最后一byte的长度
	TASK_INFO _task_info;
#if 0
	char *_file_path; 
	char * _file_name;

///////////	
	char * _url;
	char * _ref_url;
	void * _user_data;
	_u8  _tcid[CID_SIZE];			

////////
	char * _seed_file_path;
	void * _user_data;
	_u16* _dl_file_index_array;//array[_need_dl_num]  指向用于存储所有需要下载的子文件的文件序号
	BT_FILE * _file_array; //array[_need_dl_num]  指向用于存储所有需要下载的子文件的信息数组

//////
#endif
}DT_TASK_STORE;

/* 由于需求的增加，任务的user_data 被迫用来存放更多的东西 */
// 存放规则: _PACKING_1,_PACKING_2,_item_num,item1_type,item1_size,item1,item2_type,item2_size,item2,item3_type,item3_size,item3....
#define PACKING_1_VALUE (0x0000)
#define PACKING_2_VALUE (0xFFFF)
#define USER_DATA_VERSION (0x0000)
typedef struct t_user_data_store_head
{
	_u16 _PACKING_1; 	// 约定等于0x0000,用于与旧版本区分
	_u16 _PACKING_2;	// 约定等于0xFFFF,用于与旧版本区分
	_u16 _ver;			//user_data存储方式版本号
	_u16 _item_num;		//后面存放的item个数
}USER_DATA_STORE_HEAD;

/* Item 的类型 */
#define SIZE_OF_SERVER_RES 					(sizeof(EM_RES))
#define SIZE_OF_PEER_RES 					(sizeof(EM_RES))
#define SIZE_OF_LIXIAN_MODE 				(sizeof(BOOL))
#define SIZE_OF_VOD_DOWNLOAD_MODE 		(sizeof(VOD_DOWNLOAD_MODE))
#define SIZE_OF_HSC_MODE 					(sizeof(BOOL))
#define SIZE_OF_LIXIAN_ID 					(sizeof(DT_LIXIAN_ID))
#define SIZE_OF_AD_MODE						(sizeof(BOOL))

typedef enum t_user_data_item_type
{
	UDIT_USER_DATA = 0, 	// UI 在创建任务时传进来的user_data
	UDIT_SERVER_RES, 		// 离线下载加进来的server 资源
	UDIT_PEER_RES, 			// 离线下载加进来的peer 资源
	UDIT_LIXIAN_MODE, 			// 离线下载模式
	UDIT_VOD_DOWNLOAD_MODE,		//VOD任务的下载模式
	UDIT_HSC_MODE, 			// 高速通道模式
	UDIT_LIXIAN_ID, 			// 离线服务器返回的 64位的离线任务id
	UDIT_AD_MODE				//广告任务模式
} USER_DATA_ITEM_TYPE;

typedef struct t_user_data_item_head
{
	_u16 _type; 	// Item 的类型，USER_DATA_ITEM_TYPE
	_u16 _size;	//Item 的长度,byte为单位
}USER_DATA_ITEM_HEAD;

/// file format:crc ver len count ETM_TASK_STORE ETM_TASK_STORE ETM_TASK_STORE ETM_TASK_STORE....

#define TASK_STORE_FILE_VERSION 	ETM_STORE_FILE_VERSION
#define MAX_INVALID_TASK_NUM 			(50)

#define TASK_STORE_FILE 			"runmit_task.dat"


#define POS_TOTAL_TASK_NUM 			(sizeof(EM_STORE_HEAD))
#define POS_RUNNING_TASK_ARRAY 		(POS_TOTAL_TASK_NUM+sizeof(_u32))
#define POS_ORDER_LIST_SIZE 			(POS_RUNNING_TASK_ARRAY+MAX_ALOW_TASKS*sizeof(_u32))
#define POS_ORDER_LIST_ARRAY 			(POS_ORDER_LIST_SIZE+sizeof(_u32))

#define POS_TASK_START 				(POS_ORDER_LIST_ARRAY+MAX_TASK_NUM*sizeof(_u32))
////////////////////////////
#define POS_VALID 					(p_task->_offset)
#define POS_CRC 						(POS_VALID+sizeof(_u16))
#define POS_LEN 						(POS_CRC+sizeof(_u16))
#define POS_TASK_INFO 				(POS_LEN+sizeof(_u32))
///////////////////////////
//#define POS_TASK_ID					POS_TASK_INFO 
#define POS_STATE 					(POS_TASK_INFO+sizeof(_u32)) 
#define POS_IS_DELETED				(POS_STATE+1)
#define POS_HAVE_NAME				(POS_IS_DELETED)
#define POS_PATH_LEN 				(POS_IS_DELETED+1)
#define POS_NAME_LEN				(POS_PATH_LEN+sizeof(_u8))
#define POS_URL_LEN					(POS_NAME_LEN+sizeof(_u8))
#define POS_DL_NUM					(POS_URL_LEN)
#define POS_REF_URL_LEN			(POS_URL_LEN+sizeof(_u16))
#define POS_SEED_PATH_LEN			(POS_REF_URL_LEN)
#define POS_USER_DATA_LEN			(POS_REF_URL_LEN+sizeof(_u16))

#define POS_EIGENVALUE				(POS_USER_DATA_LEN+sizeof(_u32))

#define POS_FILE_SIZE				(POS_EIGENVALUE+CID_SIZE)
#define POS_DLED_SIZE 				(POS_FILE_SIZE+sizeof(_u64))
#define POS_START_TIME				(POS_DLED_SIZE+sizeof(_u64))
#define POS_FINISH_TIME				(POS_START_TIME+sizeof(_u32))
#define POS_FAILED_CODE			(POS_FINISH_TIME+sizeof(_u32))
/////////////////////////////
#define POS_PATH 					(POS_TASK_INFO+sizeof(TASK_INFO))
#define POS_NAME					(POS_PATH+(p_task->_task_info->_file_path_len))


///////////////////////////
#define POS_URL						(POS_NAME+(p_task->_task_info->_file_name_len))
#define POS_REF_URL 				(POS_URL+(p_task->_task_info->_url_len_or_need_dl_num))
#define POS_P2SP_USER_DATA 		(POS_REF_URL+(p_task->_task_info->_ref_url_len_or_seed_path_len))
#define POS_TCID 					(POS_P2SP_USER_DATA+(p_task->_task_info->_user_data_len))
/////////////////////
#define POS_SEED_PATH				(POS_NAME+(p_task->_task_info->_file_name_len)) 
#define POS_BT_USER_DATA 			(POS_SEED_PATH+(p_task->_task_info->_ref_url_len_or_seed_path_len))
#define POS_INDEX_ARRAY			(POS_BT_USER_DATA+(p_task->_task_info->_user_data_len)) 
#define POS_FILE_ARRAY				(POS_INDEX_ARRAY+(p_task->_task_info->_url_len_or_need_dl_num)*sizeof(_u16)) 
#define POS_FILE(i) 					(POS_FILE_ARRAY+i*sizeof(BT_FILE)) 

///////////////////////////////////////

char * dt_get_task_store_file_path(void);
_int32 dt_close_task_file(BOOL force_close);
BOOL dt_is_task_file_need_clear_up(void);
_int32 dt_create_task_file(void);
_int32 dt_clear_task_file_impl(void* param);

_int32 dt_stop_clear_task_file(void);
_int32 dt_get_total_task_num_from_file(_u32 * total_task_num);
_int32 dt_save_total_task_num_to_file(_u32  total_task_num);
_u32 * dt_get_running_tasks_from_file(void);
_int32 dt_save_running_tasks_to_file(_u32 *running_tasks_array);
_u32 dt_get_order_list_size_from_file(void);
_int32 dt_get_order_list_from_file(_u32 * task_id_array_buffer,_u32 buffer_len);
_int32 dt_save_order_list_to_file(_u32 order_list_size,_u32 *task_id_array);

_int32 dt_backup_file(void);
BOOL dt_recover_file(void);

_int32 dt_get_task_crc_value(EM_TASK * p_task,_u16 * p_crc_value,_u32 * p_data_len);
_int32 dt_add_task_to_file(EM_TASK * p_task);
_int32 dt_add_bt_task_part_to_file(EM_TASK * p_task);
_int32 dt_add_p2sp_task_part_to_file(EM_TASK * p_task);
_int32 dt_disable_task_in_file(EM_TASK * p_task);
EM_TASK *  dt_get_task_from_file(void);
_int32 dt_save_task_to_file(EM_TASK * p_task);
_int32 dt_save_p2sp_task_url_to_file(EM_TASK * p_task,char* new_url,_u32  url_len);
_int32 dt_save_p2sp_task_tcid_to_file(EM_TASK * p_task,_u8  * tcid);
_int32 dt_save_p2sp_task_path_to_file(EM_TASK * p_task,char* new_path,_u32  path_len);
_int32 dt_save_p2sp_task_name_to_file(EM_TASK * p_task,char* new_name,_u32  name_len);
_int32 dt_save_bt_task_name_to_file(EM_TASK * p_task,char* new_name,_u32  name_len);
 char *  dt_get_task_file_path_from_file(EM_TASK * p_task);
 char *  dt_get_task_file_name_from_file(EM_TASK * p_task);
 char * dt_get_task_url_from_file(EM_TASK * p_task);
 char *  dt_get_task_ref_url_from_file(EM_TASK * p_task);
_u8 *  dt_get_task_tcid_from_file(EM_TASK * p_task);
_int32  dt_get_p2sp_task_user_data_from_file(EM_TASK * p_task,void* data_buffer,_u32 * buffer_len);
_int32  dt_save_p2sp_task_user_data_to_file(EM_TASK * p_task,_u8 * new_user_data,_u32  new_user_data_len);
_int32  dt_get_bt_task_user_data_from_file(EM_TASK * p_task,void* data_buffer,_u32 * buffer_len);
_int32 dt_save_bt_task_user_data_to_file(EM_TASK * p_task,_u8 * new_user_data,_u32  new_user_data_len);
 char *  dt_get_task_seed_file_from_file(EM_TASK * p_task);
_u16 *  dt_get_task_bt_need_dl_file_index_array(EM_TASK * p_task);
_int32  dt_release_task_bt_need_dl_file_index_array(_u16 * file_index_array);
BT_FILE *  dt_get_task_bt_sub_file_from_file(EM_TASK * p_task,_u16 array_index);
_int32  dt_set_task_bt_sub_file_to_file(EM_TASK * p_task,_u16 array_index,BT_FILE * p_bt_file);
_int32 dt_save_bt_task_need_dl_file_change_to_file(EM_TASK * p_task,_u16 * need_dl_file_index_array,_u16 need_dl_file_num);
_int32  dt_check_task_file(_u32 task_num);

#endif /* __DOWNLOAD_TASK_STORE_H_20090915 */
