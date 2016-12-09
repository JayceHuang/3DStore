#include "utility/define.h"
#ifdef ENABLE_BT 


#include "bt_range_download_info.h"

#include "utility/logid.h"
#define LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"

_int32 brdi_init_stuct( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info )
{
	LOG_DEBUG( "brdi_init_stuct." );

	range_list_init( &p_bt_range_download_info->_total_receive_range );

	
	range_list_init( &p_bt_range_download_info->_cur_need_download_range );

	return SUCCESS;
}

_int32 brdi_uninit_stuct( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info )
{
	LOG_DEBUG( "brdi_uninit_stuct." );
	
	range_list_clear( &p_bt_range_download_info->_total_receive_range );
	
	range_list_clear( &p_bt_range_download_info->_cur_need_download_range );
	
	return SUCCESS;
}

RANGE_LIST* brdi_get_recved_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info )
{
	LOG_DEBUG( "brdi_get_recved_range_list.p_bt_range_download_info:0x%x",
		&p_bt_range_download_info->_total_receive_range );
	return  &p_bt_range_download_info->_total_receive_range;
}

RANGE_LIST* brdi_get_need_download_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info )
{
	LOG_DEBUG( "brdi_get_need_download_range_list.p_bt_range_download_info:0x%x",
		&p_bt_range_download_info->_cur_need_download_range );

	out_put_range_list( &p_bt_range_download_info->_cur_need_download_range ); 
	return  &p_bt_range_download_info->_cur_need_download_range;
}

_int32 brdi_add_recved_range( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_recv_range )
{
	LOG_DEBUG( "brdi_add_recved_range.range:[%u,%u] ",
		p_recv_range->_index, p_recv_range->_num );
	range_list_add_range( &p_bt_range_download_info->_total_receive_range, p_recv_range, NULL, NULL );
	out_put_range_list( &p_bt_range_download_info->_total_receive_range ); 

	return SUCCESS;
}


_int32 brdi_del_recved_range( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_recv_range )
{
	LOG_DEBUG( "brdi_del_recved_range. p_recv_range:[%u,%u]", p_recv_range->_index, p_recv_range->_num );
	range_list_delete_range( &p_bt_range_download_info->_total_receive_range, p_recv_range, NULL, NULL );
	return SUCCESS;
}

_int32 brdi_add_cur_need_download_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_cur_need_download_range )
{
	
	LOG_DEBUG( "brdi_add_cur_need_download_range_list. p_cur_need_download_range:[%u,%u]", p_cur_need_download_range->_index, p_cur_need_download_range->_num );
	range_list_add_range( &p_bt_range_download_info->_cur_need_download_range, p_cur_need_download_range, NULL, NULL );
	out_put_range_list( &p_bt_range_download_info->_cur_need_download_range ); 

	return SUCCESS;

}

_int32 brdi_del_cur_need_download_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_cur_need_download_range )
{
	
	LOG_DEBUG( "brdi_del_cur_need_download_range_list. p_cur_need_download_range:[%u,%u]", p_cur_need_download_range->_index, p_cur_need_download_range->_num );
	range_list_delete_range( &p_bt_range_download_info->_cur_need_download_range, p_cur_need_download_range, NULL, NULL );
	return SUCCESS;
}

BOOL brdi_is_padding_range_resv( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info, const RANGE *p_recv_range )
{
	BOOL is_resv = FALSE;
	is_resv = range_list_is_include( &p_bt_range_download_info->_total_receive_range, p_recv_range );
	out_put_range_list( &p_bt_range_download_info->_total_receive_range );
	LOG_DEBUG( "brdi_is_padding_range_resv. p_recv_range:[%u,%u], is_resv:%d", p_recv_range->_index, p_recv_range->_num, is_resv );
	return is_resv;
}

void brdi_adjust_resv_range_list( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info )
{
	RANGE_LIST tmp_range_list;
	range_list_init( &tmp_range_list );
	
	LOG_DEBUG( "brdi_adjust_resv_range_list begin. _total_receive_range" );
	out_put_range_list( &p_bt_range_download_info->_total_receive_range );
	
	range_list_cp_range_list( &p_bt_range_download_info->_total_receive_range, &tmp_range_list );
	range_list_delete_range_list( &tmp_range_list, &p_bt_range_download_info->_cur_need_download_range );
	range_list_delete_range_list( &p_bt_range_download_info->_total_receive_range, &tmp_range_list );

	LOG_DEBUG( "brdi_adjust_resv_range_list end. _total_receive_range" );
	out_put_range_list( &p_bt_range_download_info->_total_receive_range );
	
	range_list_clear( &tmp_range_list );
}

BOOL brdi_is_all_resv( BT_RANGE_DOWNLOAD_INFO *p_bt_range_download_info )
{
	return range_list_is_contained2( &p_bt_range_download_info->_total_receive_range,
		&p_bt_range_download_info->_cur_need_download_range );
}


#endif /* #ifdef ENABLE_BT */


