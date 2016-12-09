#if !defined(__FM_COMMON_H_20090915)
#define __FM_COMMON_H_20090915

#include "em_common/em_define.h"
#include "em_common/em_errcode.h"
#include "em_common/em_string.h"
#include "em_common/em_mempool.h"
#include "em_common/em_time.h"
#include "em_common/em_crc.h"
#include "em_interface/em_fs.h"
#include "settings/em_settings.h"

#define ETM_STORE_FILE_VERSION    (1)
#define MAX_FILE_IDLE_INTERVAL 	(300*1000)    //5 minutes
#define DEFAULT_FILE_RESERVED_SIZE 			(64)

/* Structures for store file head */
typedef struct t_em_store_head
{
	_u16 _crc;	// 由于task_store.dat和tree_store.dat需要动态修改，无法进行全文的CRC校验，所以这个全文的CRC只在etm_store.dat中有效
	_u16 _ver;
	_u32  _len; //从_u32 _count开始到这个文件的最后一byte的长度
} EM_STORE_HEAD;



#endif /* __FM_COMMON_H_20090915 */
