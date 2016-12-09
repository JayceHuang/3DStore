#include "udt_slow_start_cca.h"
#include "utility/utility.h"

#define	MIN_THRESH_WINDOW	(16)
#define	MIN_CC_WINDOW		(1)
#define	MAX_THRESH_WINDOW	(320)

void udt_init_slow_start_cca(SLOW_START_CCA* cca, _u32 mtu)
{
	cca->_mtu = mtu;
	cca->_min_thresh_window = MIN_THRESH_WINDOW * cca->_mtu;
	cca->_min_cc_window = MIN_CC_WINDOW * cca->_mtu;
	cca->_max_thresh_window = MAX_THRESH_WINDOW * cca->_mtu;
	cca->_current_congestion_window = cca->_min_cc_window;
	cca->_current_thresh_window = cca->_max_thresh_window;
}

void udt_handle_package_lost(SLOW_START_CCA* cca, BOOL lost, BOOL timeout)
{
	if(lost)
	{
		cca->_current_thresh_window = MAX( 7 * cca->_current_congestion_window / 8, cca->_min_thresh_window);
		if(timeout)
		{
			cca->_current_congestion_window = MAX( 3 * cca->_current_congestion_window / 4, cca->_min_cc_window);
		}
		else
		{
			cca->_current_congestion_window = cca->_current_thresh_window;
		}
	}
	else
	{
		if(cca->_current_congestion_window < cca->_current_thresh_window)
		{
			cca->_current_congestion_window += cca->_mtu;
		}
		else
		{
			cca->_current_congestion_window += cca->_mtu * cca->_mtu / cca->_current_congestion_window;
		}
		if(cca->_current_congestion_window >= cca->_max_thresh_window)
		{
			cca->_current_congestion_window = cca->_max_thresh_window;
		}
	}
}

_u32 udt_get_cur_congestion_window(SLOW_START_CCA* cca)
{
	return cca->_current_congestion_window / cca->_mtu * cca->_mtu;
}

_u32 udt_get_min_congestion_window(SLOW_START_CCA* cca)
{
	return cca->_min_cc_window;
}
