#include "encryption_algorithm.h"

#ifdef _SUPPORT_MHXY

#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>

void encryption_algorithm_destroy(struct tagENCRYPTION_ALGORITHM ** pp_algorithm)
{
    if (!*pp_algorithm) return ;
    free(*pp_algorithm);
    *pp_algorithm = NULL;
}

#endif //#ifdef _SUPPORT_MHXY
