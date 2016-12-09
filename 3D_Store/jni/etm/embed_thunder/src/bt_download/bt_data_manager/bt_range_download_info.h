

/*****************************************************************************
 *
 * Filename: 			bt_range_download_info.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/02/24
 *	
 * Purpose:				Basic bt_range_download_info struct.
 *
 *****************************************************************************/

#if !defined(__BT_RANGE_DOWNLOAD_INFO_20090224)
#define __BT_RANGE_DOWNLOAD_INFO_20090224

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_BT 
#include "utility/range.h"

struct _tag_range_list;


typedef struct tagBT_RANGE_DOWNLOAD_INFO
{
	RANGE_LIST _total_receive_range;//当前打开的file_info中
	RANGE_LIST _cur_need_download_range;
} BT_RANGE_DOWNLOAD_INFO;

_int32 brdi_init_stuct( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info );
_int32 brdi_uninit_stuct( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info );

RANGE_LIST* brdi_get_recved_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info );
RANGE_LIST* brdi_get_need_download_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info );

_int32 brdi_add_recved_range( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_recv_range );

_int32 brdi_del_recved_range( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_recv_range );


_int32 brdi_add_cur_need_download_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_cur_need_download_range );
_int32 brdi_del_cur_need_download_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_cur_need_download_range );

BOOL brdi_is_padding_range_resv( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_recv_range );
void brdi_adjust_resv_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info );
BOOL brdi_is_all_resv( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info );
#endif /* #ifdef ENABLE_BT */


#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_RANGE_DOWNLOAD_INFO_20090224)


