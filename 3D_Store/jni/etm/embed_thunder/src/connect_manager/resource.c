#include "utility/utility.h"
#include "utility/string.h"
#include "utility/url.h"
#include "utility/logid.h"
#define LOGID LOGID_RES_QUERY
#include "utility/logger.h"	

#include "resource.h"
#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "p2p_data_pipe/p2p_data_pipe.h"


#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#endif

_int32 inc_resource_error_times(RESOURCE* res)
{
	res->_error_blocks++;
	return res->_error_blocks;
}

void set_resource_type(RESOURCE* res, RESOURCE_TYPE state)
{
	res->_resource_type = state;
}

RESOURCE_TYPE get_resource_type(RESOURCE* res) 
{
    return  res->_resource_type;
}
void set_resource_level(RESOURCE* res, _u32 level)
{
	res->_level= level;
}

_u32 get_resource_level(RESOURCE* res) 
{
    return  res->_level;
}

_int32 set_resource_state(RESOURCE* res, DISPATCH_STATE state)
{
	res->_dispatch_state = state;
	return SUCCESS;
}

void set_resource_err_code(RESOURCE* res, _u32 err_code)
{
    res->_err_code = err_code;
}

_u32 get_resource_err_code(RESOURCE* res)
{
    return res->_err_code;
}

DISPATCH_STATE get_resource_state(RESOURCE* res)
{
	return res->_dispatch_state;
}

void init_resource_info(RESOURCE *res, RESOURCE_TYPE res_type)
{
	res->_resource_type = res_type;
	res->_dispatch_state = NEW;
	res->_retry_times = 0;
	res->_score = 0;
    res->_max_retry_time = 3;
  	res->_retry_time_interval = 0;  
  	res->_last_failture_time = 0;  
	res->_pipe_num = 0;
	res->_connecting_pipe_num = 0;
	res->_error_blocks = 0;
	res->_max_speed = 0;
    
	res->_err_code = SUCCESS;
    
	res->_is_global_selected = FALSE;
	res->_global_wrap_ptr = NULL;

}

void uninit_resource_info(RESOURCE *res)
{
}

void set_resource_max_retry_time(RESOURCE *res, _u32 max_retry_time)
{
    res->_max_retry_time = max_retry_time;
}

BOOL is_peer_res_equal(RESOURCE* res1, RESOURCE* res2)
{
    P2P_RESOURCE *peer_res1 = NULL;
    P2P_RESOURCE *peer_res2 = NULL;
    RESOURCE_TYPE res_type1 = UNDEFINE_RESOURCE;
    RESOURCE_TYPE res_type2 = UNDEFINE_RESOURCE;

    sd_assert(res1 != NULL);
    sd_assert(res2 != NULL);

    res_type1 = res1->_resource_type;
    res_type2 = res2->_resource_type;

    LOG_DEBUG("is_peer_res_equal, res1 = 0x%x, type = %d, res2 = 0x%x, type = %d.", res1, res_type1, res2, res_type2);

    if (res_type1 != res_type2)
    {
        return FALSE;
    }

    switch(res_type1)
    {
    case PEER_RESOURCE:
        return compare_peer_resource((P2P_RESOURCE *)res1, (P2P_RESOURCE *)res2);
#ifdef ENABLE_EMULE
    case EMULE_RESOURCE_TYPE:
        return compare_emule_resource((EMULE_RESOURCE *)res1, (EMULE_RESOURCE *)res2);
#endif
    default:
        LOG_ERROR("can't compare such res type = %d", res_type1);
        sd_assert(FALSE);
        break;
    }

    return FALSE;
}


#ifdef _DEBUG
void output_resource_list(RESOURCE_LIST* res_list)
{
    LIST_ITERATOR cur_node, end_node;
    RESOURCE *p_res = NULL;

    if (NULL == res_list)
    {
        return;
    }

    cur_node = LIST_BEGIN( res_list->_res_list );
    end_node = LIST_END( res_list->_res_list );	

    LOG_DEBUG("resource count=%u", res_list->_res_list._list_size);
    while(cur_node != end_node)
    {
        p_res = LIST_VALUE( cur_node );
        if (p_res)
        {
            LOG_DEBUG("[resource dump]res:0x%x, type:%d, state:%d, retry_times:%d, max_retry_time:%d,"
                      "retry_time_interval:%d, last_failture_time:%d, pipe_num:%d, connecting_pipe_num:%d, max_speed:%d,"
                      "score:%d, level:%d, error_blocks:%d, err_code:%d",
                          p_res,
                          p_res->_resource_type,
                          p_res->_dispatch_state,
                          p_res->_retry_times,
                          p_res->_max_retry_time,
                          p_res->_retry_time_interval,
                          p_res->_last_failture_time,
                          p_res->_pipe_num,
                          p_res->_connecting_pipe_num,
                          p_res->_max_speed,
                          p_res->_score,
                          p_res->_level,
                          p_res->_error_blocks,
                          p_res->_err_code
                          );
        }
        cur_node = LIST_NEXT( cur_node );
    }	
}

#endif
