#include <stdio.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"
#include <ooc/exception.h>

#include "PersistentTaskBtImpl.h"
#include "implement/PersistentTaskBtImpl.h"
#include "Task.h"
#include "download_manager/download_task_store.h"


AllocateClass( CPersistentTaskBtImpl, CPersistentTaskImpl );

/* Class virtual function prototypes
 */

static BOOL CPersistentTaskBtImpl_Serialize( CPersistentTaskBtImpl self, BOOL bRead, Object fileObj );

/* Class initializing
 */

static
void
CPersistentTaskBtImpl_initialize( Class this )
{
    CPersistentTaskBtImplVtable vtab = & CPersistentTaskBtImplVtableInstance;
    
    vtab->CPersistentTaskImpl.CPersistentImpl.IPersistent.Serialize    =     (BOOL (*)(Object, BOOL bRead, Object fileObj ))    CPersistentTaskBtImpl_Serialize;
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CPersistentTaskBtImpl_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CPersistentTaskBtImpl_constructor( CPersistentTaskBtImpl self, const void * params )
{
    sd_assert( ooc_isInitialized( CPersistentTaskImpl ) );
    
    chain_constructor( CPersistentTaskBtImpl, self, params );

}

/* Destructor
 */

static
void
CPersistentTaskBtImpl_destructor( CPersistentTaskBtImpl self, CPersistentTaskBtImplVtable vtab )
{
}

/* Copy constuctor
 */

static
int
CPersistentTaskBtImpl_copy( CPersistentTaskBtImpl self, const CPersistentTaskBtImpl from )
{
    

    return OOC_COPY_DONE;
}

/*    =====================================================
    Class member functions
 */


CPersistentTaskBtImpl
CPersistentTaskBtImpl_new( void )
{
    ooc_init_class( CPersistentTaskBtImpl );

    return ooc_new( CPersistentTaskBtImpl, NULL );
}

static
BOOL
CPersistentTaskBtImpl_Serialize( CPersistentTaskBtImpl self, BOOL bRead, Object fileObj )
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskBtImpl ) );

    LOG_DEBUG("CPersistentTaskBtImpl::Serialize begin");

    if (!CPersistentTaskBtImplParentVirtual( self )->CPersistentImpl.IPersistent.Serialize((Object)self, bRead, fileObj )) return FALSE;

    _int32 ret_val=SUCCESS;

    Object pData = ((CPersistentImpl)self)->m_pData;
    EM_TASK *p_task = CTaskVirtual((CTask)pData)->GetAttachedData(pData);
    TASK_INFO *_task_info = p_task->_task_info;

    LOG_DEBUG("CPersistentTaskBtImpl::Serialize:task_id=%u",p_task->_task_info->_task_id);
    EM_BT_TASK *p_task_ext = (EM_BT_TASK * )p_task->_task_info;

    if (bRead)
    {

    	_u32 ver_offset = 0;
        /* read file_path from file */
        ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_file_path, _task_info->_file_path_len+1, _task_info->_file_path_len, POS_PATH+ver_offset);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* read file_name from file */
        ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_file_name, _task_info->_file_name_len+1, _task_info->_file_name_len, POS_NAME+ver_offset);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* read seed path from file */
        ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_seed_file_path, _task_info->_ref_url_len_or_seed_path_len+1, _task_info->_ref_url_len_or_seed_path_len, POS_SEED_PATH+ver_offset);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* read user_data from file */
        if (_task_info->_have_user_data)
        {
            ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_user_data, _task_info->_user_data_len, _task_info->_user_data_len, POS_BT_USER_DATA+ver_offset);
            sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
        }

        /* read bt file index array from file */
        ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_dl_file_index_array, _task_info->_url_len_or_need_dl_num*sizeof(_u16), _task_info->_url_len_or_need_dl_num*sizeof(_u16), POS_INDEX_ARRAY+ver_offset);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* read bt file array from file */
        ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_file_array, _task_info->_url_len_or_need_dl_num*sizeof(BT_FILE), _task_info->_url_len_or_need_dl_num*sizeof(BT_FILE), POS_FILE_ARRAY+ver_offset);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* read _task_tag  from file  if is the new version*/
        _u16 ver = CPersistentTaskImplVirtual((CPersistentTaskImpl)self)->GetVersion(self);
          // read _task_tag  from file  if is the new version

 		if ( ver >= 3)
         {
			LOG_DEBUG("CPersistentTaskP2spImpl::Serialize: read task tag len");
			ret_val = Util_Read(fileObj, (void *)&p_task_ext->_task_tag_len, sizeof(_u32), POS_BT_TASK_TAG_LEN+ver_offset);
			sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

		    if (p_task_ext->_task_tag_len>0)
		    {
		    	//if (p_task_ext->_task_tag)
		    	//{
		    	//	SAFE_DELETE(p_task_ext->_task_tag);
		    	//}

				LOG_DEBUG("CPersistentTaskP2spImpl::Serialize: read task tag");
				ret_val = Util_AllocAndRead(fileObj, (void **)&p_task_ext->_task_tag, p_task_ext->_task_tag_len+1, p_task_ext->_task_tag_len, POS_BT_TASK_TAG+ver_offset);
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

        /* write seed path from file */
        ret_val = Util_Write(fileObj, (void *)p_task_ext->_seed_file_path, _task_info->_ref_url_len_or_seed_path_len, POS_SEED_PATH);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* write user_data from file */
        if (_task_info->_have_user_data)
        {
            ret_val = Util_Write(fileObj, (void *)p_task_ext->_user_data, _task_info->_user_data_len, POS_BT_USER_DATA);
            sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;
        }

        /* write bt file index array from file */
        ret_val = Util_Write(fileObj, (void *)p_task_ext->_dl_file_index_array, _task_info->_url_len_or_need_dl_num*sizeof(_u16), POS_INDEX_ARRAY);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

        /* write bt file array from file */
        ret_val = Util_Write(fileObj, (void *)p_task_ext->_file_array, _task_info->_url_len_or_need_dl_num*sizeof(BT_FILE), POS_FILE_ARRAY);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;


		/* write task_tag from file */
        ret_val = Util_Write(fileObj, (void *)&p_task_ext->_task_tag_len, sizeof(_u32), POS_BT_TASK_TAG_LEN);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

		/* write task_tag from file */
        ret_val = Util_Write(fileObj, (void *)p_task_ext->_task_tag, p_task_ext->_task_tag_len, POS_BT_TASK_TAG);
        sd_assert(ret_val==SUCCESS); if (SUCCESS != ret_val) return FALSE;

    }
    return TRUE;

}
