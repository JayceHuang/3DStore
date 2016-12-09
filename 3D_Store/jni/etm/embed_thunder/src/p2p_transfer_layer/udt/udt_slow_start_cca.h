/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/12
-----------------------------------------------------------------------------------------------------------*/
#ifndef	_UDT_SLOW_START_CCA_H_
#define	_UDT_SLOW_START_CCA_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

typedef	struct	tagSLOW_START_CCA
{
	_u32	_mtu;
	_u32	_current_congestion_window;
	_u32	_current_thresh_window;
	_u32	_min_thresh_window;
	_u32	_min_cc_window;
	_u32	_max_thresh_window;
}SLOW_START_CCA;

//ÂýÆô¶¯£¬ÓµÈû±ÜÃâ
void udt_init_slow_start_cca(SLOW_START_CCA* cca, _u32 mtu);

void udt_handle_package_lost(SLOW_START_CCA* cca, BOOL lost, BOOL timeout);

_u32 udt_get_cur_congestion_window(SLOW_START_CCA* cca);

_u32 udt_get_min_congestion_window(SLOW_START_CCA* cca);
#ifdef __cplusplus
}
#endif
#endif
