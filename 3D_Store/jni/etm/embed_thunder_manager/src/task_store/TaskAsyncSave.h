#if !defined(__TASK_ASYNC_SAVE_H_20130301)
#define __TASK_ASYNC_SAVE_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include <utility/define.h>

_int32 init_task_async_save();
_int32 uninit_task_async_save();

typedef enum{ASYNC_OP_READ=1, ASYNC_OP_WRITE, ASYNC_OP_DELETE} ASYNC_OP;
//data 参数，由函数内部负责释放
_int32 output_task_to_file(_u32 task_id, ASYNC_OP op, const char *cszTaskFileName, unsigned char *data, _u32 data_len);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_ASYNC_SAVE_H_20130301 */
