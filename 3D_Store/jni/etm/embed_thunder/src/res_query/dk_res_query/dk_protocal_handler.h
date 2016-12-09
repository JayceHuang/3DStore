/*****************************************************************************
 *
 * Filename: 			dk_protocal_handler.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				abstruct structs of dk_protocal.
 *
 *****************************************************************************/
	 
	 
#if !defined(__dk_protocal_handler_20091015)
#define __dk_protocal_handler_20091015

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _DK_QUERY

#include "res_query/dk_res_query/k_node.h"

struct tagDK_PROTOCAL_HANDLER;

typedef _int32 (*dk_protocal_response_handler)( struct tagDK_PROTOCAL_HANDLER *p_handler, _u32 para_1, _u32 para_2, _u32 para_3, void *p_data );

typedef struct tagDK_PROTOCAL_HANDLER
{
	dk_protocal_response_handler _response_handler;
}DK_PROTOCAL_HANDLER;

#endif


#ifdef __cplusplus
}
#endif

#endif // !defined(__dk_protocal_handler_20091015)



