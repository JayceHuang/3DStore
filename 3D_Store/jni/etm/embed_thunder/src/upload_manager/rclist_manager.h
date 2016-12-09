#if !defined(__RCLIST_MANAGER_H_20081105)
#define __RCLIST_MANAGER_H_20081105


#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#ifdef UPLOAD_ENABLE

#include "utility/list.h"
#include "utility/map.h"
#include "utility/errcode.h"



////////////////////////////////////////////////////////////////////////////////
/*        ��Դ��¼��������3�ֲ�ѯ������ʵ���˿��ٲ��롢��ѯ��ɾ��           */
////////////////////////////////////////////////////////////////////////////////

//enum INFO_KEY {KEY_SIZE_TCID= 0, KEY_GCID};

typedef struct 
{
	_u64 _size; //�ļ���С
	_u8    _tcid[CID_SIZE]; //����cid	
	_u8    _gcid[CID_SIZE]; //ȫ��cid
	char     _path[MAX_FILE_PATH_LEN+1]; //�ļ�·��
	_u8    _chub; //HUB����
	_u16   _pading16;
	}ID_RC_INFO;

typedef struct 
{
	LIST		_list_rc;           // list of ID_RC_INFO
	MAP	_index_gcid;		// GCID����
	char				_rc_list_file_path[MAX_FULL_PATH_BUFFER_LEN];
	_u32				_file_modified_time;	// ��Դ�б��ļ����һ�α��޸ĵ�ʱ��
	BOOL					_list_modified_flag;		// �б����������Ƿ���Ĺ�	
}RCLIST_MANAGER;

typedef struct tag_store_head
{
	_u16 crc;
	_u16 ver;
	_u32  len;
} RCLIST_STORE_HEAD;


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for upload_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_rclist_manager(void);
_int32 uninit_rclist_manager(void);

// �����Դ
_int32 rclist_add_rc(_u64 size, const _u8 *tcid, const _u8 *gcid, _u8 chub, const char *path);
// ɾ����Դ
_int32 rclist_del_rc(const _u8 *gcid);
// �޸���Դ
_int32 rclist_change_rc_path( const _u8 *gcid,const char *new_path,_u32  new_path_len);
_int32 rclist_change_file_path(_u64 size, const char *old_path, const char *new_path);

// ��ѯ��Դ
ID_RC_INFO *	rclist_query( const _u8 * gcid );
ID_RC_INFO * rclist_query_by_size_and_path(_u64 size, const char *path);

_int32 rclist_set_rc_list_path( const char *path,_u32  path_len);

LIST *  rclist_get_rc_list(void);
/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 rclist_index_gcid_map_compare_fun( void *E1, void *E2 );
//_int32 rclist_get_rc_info(ID_RC_INFO * p_info, enum INFO_KEY key ); 
_int32 rclist_load_data(BOOL need_check_file);
_int32 rclist_flush(void);
_int32 rclist_insert( const ID_RC_INFO * p_info );
_int32 rclist_remove(ID_RC_INFO * p_info ); 
_int32 rclist_clear(void);
_int32 rclist_del_oldest_rc(void);
_int32 rclist_get_version(void);

#endif /* UPLOAD_ENABLE */

#ifdef __cplusplus
}
#endif


#endif // !defined(__RCLIST_MANAGER_H_20081105)
