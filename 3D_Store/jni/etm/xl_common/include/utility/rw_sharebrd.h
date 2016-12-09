#ifndef _SD_RW_SHAREBRD_H_00138F8F2E70_200808262149
#define _SD_RW_SHAREBRD_H_00138F8F2E70_200808262149
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

typedef struct {
	_u16 _r_status;
	_u16 _w_status;
	void *_data;
}RW_SHAREBRD;

_int32 init_default_rw_sharebrd(void *data);
_int32 uninit_default_rw_sharebrd(void);

/* @Simple Function@
 * Description: lock data and return it for written.
 *	 if data has already be locked *data will equal to NULL
 * Return :  SUCCESS: success get lock     LOCK_SHAREBRD_FAILED: get lock failed
 */
_int32 begin_write_data(void **data);
/* unlock data for read */
_int32 end_write_data(void);


/* @Simple Function@
 * Description: lock data and return it for read.
 *	 if data has already be locked *data will equal to NULL
 * Return :  SUCCESS: success get lock     LOCK_SHAREBRD_FAILED: get lock failed
 */
_int32 begin_read_data(void **data);
/* return until finish lock data for read */
_int32 begin_read_data_block(void **data);
/* unlock data for write */
_int32 end_read_data(void);


/**************************************************************************/
/*            customed rw sharebrd */
/**************************************************************************/

/* customed rw_sharebrd, call this funtion to init this brd MUST IN DOWNLOAD-THREAD */
_int32 init_customed_rw_sharebrd(void *data, RW_SHAREBRD **brd);

_int32 uninit_customed_rw_sharebrd(RW_SHAREBRD *brd);

/* @Simple Function@
 * Description: lock data and return it for written.
 *	 if data has already be locked *data will equal to NULL
 * Return :  SUCCESS: success get lock     LOCK_SHAREBRD_FAILED: get lock failed
 */
_int32 cus_rws_begin_write_data(RW_SHAREBRD *brd, void **data);
/* unlock data for read */
_int32 cus_rws_end_write_data(RW_SHAREBRD *brd);


/* @Simple Function@
 * Description: lock data and return it for read.
 *	 if data has already be locked *data will equal to NULL
 * Return :  SUCCESS: success get lock     LOCK_SHAREBRD_FAILED: get lock failed
 */
_int32 cus_rws_begin_read_data(RW_SHAREBRD *brd, void **data);
/* return until finish lock data for read */
_int32 cus_rws_begin_read_data_block(RW_SHAREBRD *brd, void **data);
/* unlock data for write */
_int32 cus_rws_end_read_data(RW_SHAREBRD *brd);

#ifdef __cplusplus
}
#endif
#endif
