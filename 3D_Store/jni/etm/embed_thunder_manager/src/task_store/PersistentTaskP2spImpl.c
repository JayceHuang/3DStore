#include <stdio.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"

#include <ooc/exception.h>

#include "PersistentTaskP2spImpl.h"
#include "implement/PersistentTaskP2spImpl.h"
#include "Task.h"

#include "download_manager/download_task_store.h"


AllocateClass( CPersistentTaskP2spImpl, CPersistentTaskImpl );

/* Class virtual function prototypes
 */

static BOOL CPersistentTaskP2spImpl_Serialize( CPersistentTaskP2spImpl self, BOOL bRead, Object fileObj );

/* Class initializing
 */

static
void
CPersistentTaskP2spImpl_initialize( Class this )
{
    CPersistentTaskP2spImplVtable vtab = & CPersistentTaskP2spImplVtableInstance;
    
    vtab->CPersistentTaskImpl.CPersistentImpl.IPersistent.Serialize    =     (BOOL (*)(Object, BOOL bRead, Object fileObj ))    CPersistentTaskP2spImpl_Serialize;
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CPersistentTaskP2spImpl_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CPersistentTaskP2spImpl_constructor( CPersistentTaskP2spImpl self, const void * params )
{
    sd_assert( ooc_isInitialized( CPersistentTaskImpl ) );
    
    chain_constructor( CPersistentTaskP2spImpl, self, params );

}

/* Destructor
 */

static
void
CPersistentTaskP2spImpl_destructor( CPersistentTaskP2spImpl self, CPersistentTaskP2spImplVtable vtab )
{
}

/* Copy constuctor
 */

static
int
CPersistentTaskP2spImpl_copy( CPersistentTaskP2spImpl self, const CPersistentTaskP2spImpl from )
{
    

    return OOC_COPY_DONE;
}

/*    =====================================================
    Class member functions
 */

CPersistentTaskP2spImpl
CPersistentTaskP2spImpl_new( void )
{
    ooc_init_class( CPersistentTaskP2spImpl );

    return ooc_new( CPersistentTaskP2spImpl, NULL );
}

static
BOOL
CPersistentTaskP2spImpl_Serialize( CPersistentTaskP2spImpl self, BOOL bRead, Object fileObj )
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskP2spImpl ) );

    LOG_DEBUG("CPersistentTaskP2spImpl::Serialize begin");

    if (!CPersistentTaskP2spImplParentVirtual( self )->CPersistentImpl.IPersistent.Serialize((Object)self, bRead, fileObj )) return FALSE;

    _int32 ret_val=SUCCESS;

    Object pData = ((CPersistentImpl)self)->m_pData;
    EM_TASK *p_task = CTaskVirtual((CTask)pData)->GetAttachedData(pData);
    TASK_INFO *_task_info = p_task->_task_info;

    EM_P2SP_TASK * p_task_ext=NULL;

    LOG_DEBUG("CPersistentTaskP2spImpl::Serialize:task_id=%u",p_task->_task_info->_task_id);
    p_task_ext = (EM_P2SP_TASK * )p_task->_task_info;

    if (bRead)
    {
    	_u32 ver_offset = 0;

        /* read file_path from file */
    	_u32 task_info_len = sizeof(TASK_INFO);
        LOG_DEBUG("CPersistentTaskP2spImpl::Serialize: read file path");
        ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_file_path, _task_info->_file_path_len+1, _task_info->_file_path_len, POS_PATH+ver_offset );
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* read file_name from file */
        LOG_DEBUG("CPersistentTaskP2spImpl::Serialize: read file name");
        ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_file_name, _task_info->_file_name_len+1, _task_info->_file_name_len, POS_NAME+ver_offset );
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* read url from file */
        LOG_DEBUG("CPersistentTaskP2spImpl::Serialize: read url");
        ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_url, _task_info->_url_len_or_need_dl_num+1, _task_info->_url_len_or_need_dl_num, POS_URL+ver_offset );
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* read ref_url from file */
        if (_task_info->_have_ref_url)
        {
            LOG_DEBUG("CPersistentTaskP2spImpl::Serialize: read ref url");
            ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_ref_url, _task_info->_ref_url_len_or_seed_path_len+1, _task_info->_ref_url_len_or_seed_path_len, POS_REF_URL+ver_offset );
            sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
        }

        /* read user_data from file */
        if (_task_info->_have_user_data)
        {
            LOG_DEBUG("CPersistentTaskP2spImpl::Serialize: read user data");
            ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_user_data, _task_info-> _user_data_len, _task_info->_user_data_len, POS_P2SP_USER_DATA+ver_offset);
            sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
        }

        /* read tcid from file */
        if (_task_info->_have_tcid)
        {
            LOG_DEBUG("CPersistentTaskP2spImpl::Serialize: tcid");
            ret_val = Util_Read(fileObj, (void *)p_task_ext->_tcid, CID_SIZE, POS_TCID + ver_offset);
            sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
        }
       
        _u16 ver = CPersistentTaskImplVirtual((CPersistentTaskImpl)self)->GetVersion(self);

		if ( ver >= 3)
        {

		  ret_val = Util_Read(fileObj, (void *)&p_task_ext->_task_tag_len, sizeof(_u32), POS_P2SP_TASK_TAG_LEN+ver_offset );
		  sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
		  LOG_DEBUG("read_task_tag:task_tag_len:%d", p_task_ext->_task_tag, p_task_ext->_task_tag_len);
		 
		  if (p_task_ext->_task_tag_len>0)
		  {
		    	if (p_task_ext->_task_tag)
		    	{
		    		//SAFE_DELETE(p_task_ext->_task_tag);
		    	}

			  LOG_DEBUG("CPersistentTaskP2spImpl::Serialize: read task tag");
			  ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_task_tag, p_task_ext->_task_tag_len+1, p_task_ext->_task_tag_len, POS_P2SP_TASK_TAG+ver_offset );
			  LOG_DEBUG("read_task_tag:task_task=%s", p_task_ext->_task_tag);
	          sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

		  }

		 }


		 _task_info->_full_info = TRUE;
    }
    else
    {
        /* write file_path from file */
        ret_val = Util_Write(fileObj, (void *)p_task_ext->_file_path, _task_info->_file_path_len, POS_PATH);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* write file_name from file */
        ret_val = Util_Write(fileObj, (void *)p_task_ext->_file_name, _task_info->_file_name_len, POS_NAME);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* write url from file */
        ret_val = Util_Write(fileObj, (void *)p_task_ext->_url, _task_info->_url_len_or_need_dl_num, POS_URL);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* write ref_url from file */
        if (_task_info->_have_ref_url)
        {
            ret_val = Util_Write(fileObj, (void *)p_task_ext->_ref_url, _task_info->_ref_url_len_or_seed_path_len, POS_REF_URL);
            sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
        }

        /* write user_data from file */
        if (_task_info->_have_user_data)
        {
            ret_val = Util_Write(fileObj, (void *)p_task_ext->_user_data, _task_info->_user_data_len, POS_P2SP_USER_DATA);
            sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
        }

        /* write tcid from file */
        if (_task_info->_have_tcid)
        {
            ret_val = Util_Write(fileObj, (void *)p_task_ext->_tcid, CID_SIZE, POS_TCID);
            sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
        }

		 //write task_tag from file
        _int32 pos = (POS_TCID+ (p_task->_task_info->_have_tcid!=0?(CID_SIZE*sizeof(_u8)):0));
		 
		 LOG_DEBUG("write_task_tag:task_tag_len:%d", p_task_ext->_task_tag_len);
		 ret_val = Util_Write(fileObj, (void *)&p_task_ext->_task_tag_len, sizeof(_u32), POS_P2SP_TASK_TAG_LEN);
		 sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

		 if (p_task_ext->_task_tag_len>0)
		 {
			 // write task_tag from file
			 sd_assert(p_task_ext->_task_tag!=NULL);
			LOG_DEBUG("write_task_tag:task_task=%s, task_tag_len:%d", p_task_ext->_task_tag, p_task_ext->_task_tag_len);
			ret_val = Util_Write(fileObj, (void *)p_task_ext->_task_tag, p_task_ext->_task_tag_len, POS_P2SP_TASK_TAG);
			sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
		 }

    }
    return TRUE;
    
}
