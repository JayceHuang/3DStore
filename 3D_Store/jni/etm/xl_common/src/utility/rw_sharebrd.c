#include "utility/rw_sharebrd.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "platform/sd_task.h"
#include "utility/mempool.h"

static RW_SHAREBRD g_default_rw_sharebrd;


/* customed rw_sharebrd, call this funtion to init this brd MUST IN DOWNLOAD-THREAD */
_int32 init_customed_rw_sharebrd(void *data, RW_SHAREBRD **brd)
{
    _int32 ret_val = SUCCESS;
    *brd = NULL;

    ret_val = sd_malloc(sizeof(RW_SHAREBRD), (void**)brd);
    CHECK_VALUE(ret_val);

    sd_memset(*brd, 0, sizeof(RW_SHAREBRD));
    (*brd)->_data = data;

    return ret_val;
}

_int32 uninit_customed_rw_sharebrd(RW_SHAREBRD *brd)
{
    _int32 ret_val = SUCCESS;

    ret_val = sd_free(brd);
	brd = NULL;
    CHECK_VALUE(ret_val);

    return ret_val;
}


_int32 cus_rws_begin_write_data(RW_SHAREBRD *brd, void **data)
{
	_int32 idx = 0;

	if(data)
		*data = NULL;

	/* try twice to assure write when read & write coming simultaneously
	    because operation of read not retry immediately */
	for(idx = 0;idx < 2; idx++)
	{
		if(brd->_r_status == 0)
		{
			brd->_w_status = 1;
			if(brd->_r_status == 1)
			{
				brd->_w_status = 0;
				continue;
			}

			if(data)
				*data = brd->_data;

			return SUCCESS;
		}
	}

	return LOCK_SHAREBRD_FAILED;
}

_int32 cus_rws_end_write_data(RW_SHAREBRD *brd)
{
	brd->_w_status = 0;
	return SUCCESS;
}

_int32 cus_rws_begin_read_data(RW_SHAREBRD *brd, void **data)
{
	if(data)
		*data = NULL;

	if(brd && brd->_w_status == 0)
	{
		brd->_r_status = 1;
		if(brd->_w_status == 1)
		{
			brd->_r_status = 0;
			return LOCK_SHAREBRD_FAILED;
		}
		if(data)
			*data = brd->_data;

		return SUCCESS;
	}

	return LOCK_SHAREBRD_FAILED;
}

/* return until finish to read */
_int32 cus_rws_begin_read_data_block(RW_SHAREBRD *brd, void **data)
{
	if(data)
		*data = NULL;

	while(1)
	{
		if(brd->_w_status == 0)
		{
			brd->_r_status = 1;
			if(brd->_w_status == 1)
			{
				brd->_r_status = 0;
			}
			else
			{
				if(data)
					*data = brd->_data;

				return SUCCESS;
			}
		}
		sd_sleep(1);
	}

	return LOCK_SHAREBRD_FAILED;
}

_int32 cus_rws_end_read_data(RW_SHAREBRD *brd)
{
	brd->_r_status = 0;
	return SUCCESS;
}



/**************************************************************************************/
/*                   */
/**************************************************************************************/

_int32 init_default_rw_sharebrd(void *data)
{
    sd_memset(&g_default_rw_sharebrd, 0, sizeof(g_default_rw_sharebrd));
    g_default_rw_sharebrd._data = data;
    return SUCCESS;
}

_int32 uninit_default_rw_sharebrd(void)
{
    	sd_memset(&g_default_rw_sharebrd, 0, sizeof(g_default_rw_sharebrd));
        return SUCCESS;
}

_int32 begin_write_data(void **data)
{
    return cus_rws_begin_write_data(&g_default_rw_sharebrd, data);
}

_int32 end_write_data(void)
{
    return cus_rws_end_write_data(&g_default_rw_sharebrd);
}


_int32 begin_read_data(void **data)
{
    return cus_rws_begin_read_data(&g_default_rw_sharebrd, data);
}

/* return until finish lock data for read */
_int32 begin_read_data_block(void **data)
{
    return cus_rws_begin_read_data_block(&g_default_rw_sharebrd, data);
}

/* unlock data for write */
_int32 end_read_data(void)
{
    return cus_rws_end_read_data(&g_default_rw_sharebrd);
}

