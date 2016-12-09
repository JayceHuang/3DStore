#include "utility/errcode.h"
#include "platform/sd_customed_interface.h"

static void* g_custom_fun[CI_IDX_COUNT] = {NULL, NULL, NULL, NULL, 
NULL, NULL, NULL, NULL, 
NULL, NULL, NULL, NULL, 
NULL, NULL, NULL, NULL, 
NULL, NULL, NULL, NULL};

_int32 set_customed_interface(_int32 fun_idx, void *fun_ptr)
{
    if(fun_idx < 0 || fun_idx >= CI_IDX_COUNT)
        return INVALID_CUSTOMED_INTERFACE_IDX;
    if(fun_ptr == NULL)
        return INVALID_CUSTOMED_INTERFACE_PTR;
    
    g_custom_fun[fun_idx] = fun_ptr;
    return SUCCESS;
}

BOOL is_available_ci(_int32 fun_idx)
{
    return !!(g_custom_fun[fun_idx]);
}

void* ci_ptr(_int32 fun_idx)
{
    return g_custom_fun[fun_idx];
}
