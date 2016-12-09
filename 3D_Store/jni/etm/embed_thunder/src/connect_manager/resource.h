//////////////////////////////////////////////////////////////////////

#if !defined(__RESOURCE_INTERFACE_20080617)
#define __RESOURCE_INTERFACE_20080617

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/list.h"

typedef enum tagRESOURCE_TYPE 
{
    UNDEFINE_RESOURCE = 0,
    PEER_RESOURCE = 101, 
    HTTP_RESOURCE, 
    FTP_RESOURCE, 
    BT_RESOURCE_TYPE, 
    EMULE_RESOURCE_TYPE 
} RESOURCE_TYPE;

typedef enum tagDISPATCH_STATE 
{
    NEW = 0, 
    VALID, 
    ABANDON 
} DISPATCH_STATE;

typedef struct tagRESOURCE
{
	RESOURCE_TYPE  _resource_type;  
	DISPATCH_STATE _dispatch_state;
	
	_u32 _retry_times;
    _u32 _max_retry_time;
    _u32 _retry_time_interval;
    _u32 _last_failture_time;
	_u32 _pipe_num;	//changed by cm
	_u32 _connecting_pipe_num;//changed by cm
	_u32 _max_speed;
    _u32 _err_code;
	_u32 _score;

	//错块数目
	_u32  _error_blocks; 

	BOOL _is_global_selected;
	void *_global_wrap_ptr;
	_u32	_level;  // Add by zyq @20101215 

	_u32 _last_pipe_connect_time;	// 如果这个资源上会开启多次pipe，这个值就是上一次pipe connect到成功的时长
	_u32 _open_normal_cdn_pipe_time;	//开始建立pipe的时刻，当前pipe的周期
	_u32 _close_normal_cdn_pipe_time; //关闭pipe的时刻，当前pipe的周期
}RESOURCE;

typedef struct tagRESOURCE_LIST
{       
	LIST  _res_list;
} RESOURCE_LIST;


_int32 inc_resource_error_times(RESOURCE* res);

void set_resource_type(RESOURCE* res, RESOURCE_TYPE state);
RESOURCE_TYPE get_resource_type(RESOURCE* res);

void set_resource_level(RESOURCE* res, _u32 level);
_u32 get_resource_level(RESOURCE* res) ;

void set_resource_err_code(RESOURCE* res, _u32 err_code);
_u32 get_resource_err_code(RESOURCE* res);

_int32 set_resource_state(RESOURCE* res, DISPATCH_STATE state);
DISPATCH_STATE get_resource_state(RESOURCE* res);

void init_resource_info(RESOURCE *res , RESOURCE_TYPE res_type);
void uninit_resource_info(RESOURCE *res);

void set_resource_max_retry_time(RESOURCE *res, _u32 max_retry_time);

BOOL is_peer_res_equal(struct tagRESOURCE* res1, struct tagRESOURCE* res2);

#ifdef _DEBUG
void output_resource_list(RESOURCE_LIST* res_list);
#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__RESOURCE_INTERFACE_20080617)


