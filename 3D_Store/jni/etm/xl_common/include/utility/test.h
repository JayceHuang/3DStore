#ifndef SD_TEST_H_00138F8F2E70_200806261613
#define SD_TEST_H_00138F8F2E70_200806261613
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/sd_assert.h"
#include "utility/errcode.h"


#define TEST_TRUE(expr)		sd_assert(expr)

#define TEST_RETURN(fun)	sd_assert((fun) == SUCCESS)

#ifdef __cplusplus
}
#endif
#endif
