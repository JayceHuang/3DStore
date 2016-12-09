#ifndef EM_DEFINE_H_00138F8F2E70_200806111929
#define EM_DEFINE_H_00138F8F2E70_200806111929

#include "utility/define.h"

#if  defined(_EM_MEMPOOL_10M)
#define  EM_MEMPOOL_10M
#elif defined(_EM_MEMPOOL_8M)
#define  EM_MEMPOOL_8M
#elif defined(_EM_MEMPOOL_5M)
#define  EM_MEMPOOL_5M
#elif defined(_EM_MEMPOOL_3M)
#define  EM_MEMPOOL_3M
#else
#define  EM_MEMPOOL_64K 
#endif

#define VALID_ET_PRI_VER 	1
#define VALID_ET_SUB_VER 	3
#define VALID_ET_BUILD_VER 3

#define STRONG_FILE_CHECK  
#define MAX_FILE_BACKUP_COUNT  10
#define MAX_FILE_CHECK_COUNT  0

#include "em_define_const_num.h"

#endif
