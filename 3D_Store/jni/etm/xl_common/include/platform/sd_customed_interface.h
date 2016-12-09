#ifndef SD_EMBEDTHUNDERFS_H_00138F8F2E70_200901131658
#define SD_EMBEDTHUNDERFS_H_00138F8F2E70_200901131658
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

_int32 set_customed_interface(_int32 fun_idx, void *fun_ptr);

BOOL is_available_ci(_int32 fun_idx);
void* ci_ptr(_int32 fun_idx);

#define CI_IDX_COUNT			    (25)

/* function index */
#define CI_FS_IDX_OPEN           (0)
#define CI_FS_IDX_ENLARGE_FILE   (1)
#define CI_FS_IDX_CLOSE          (2)
#define CI_FS_IDX_READ           (3)
#define CI_FS_IDX_WRITE          (4)
#define CI_FS_IDX_PREAD          (5)
#define CI_FS_IDX_PWRITE         (6)
#define CI_FS_IDX_FILEPOS        (7)
#define CI_FS_IDX_SETFILEPOS     (8)
#define CI_FS_IDX_FILESIZE       (9)
#define CI_FS_IDX_FREE_DISK      (10)

#define CI_SOCKET_IDX_SET_SOCKOPT  (11)

#define CI_MEM_IDX_GET_MEM           (12)
#define CI_MEM_IDX_FREE_MEM          (13)
#define CI_ZLIB_UNCOMPRESS          (14)
#define CI_FS_IDX_GET_FILESIZE_AND_MODIFYTIME   (15)
#define CI_FS_IDX_DELETE_FILE     (16)
#define CI_FS_IDX_RM_DIR              (17)
#define CI_FS_IDX_MAKE_DIR              (18)
#define CI_FS_IDX_RENAME_FILE              (19)
#define CI_SOCKET_IDX_CREATE  (20)
#define CI_SOCKET_IDX_CLOSE  (21)
#define CI_FS_IDX_FILE_EXIST          (22)
#define CI_DNS_GET_DNS_SERVER          (23)
#define CI_LOG_WRITE_LOG                (24)

typedef _int32 (*et_fs_open)(char *filepath, _int32 flag, _u32 *file_id);
typedef _int32 (*et_fs_enlarge_file)(_u32 file_id, _u64 expect_filesize, _u64 *cur_filesize);

typedef _int32 (*et_fs_close)(_u32 file_id);

typedef _int32 (*et_fs_read)(_u32 file_id, char *buffer, _int32 size, _u32 *readsize);
typedef _int32 (*et_fs_write)(_u32 file_id, char *buffer, _int32 size, _u32 *writesize);

typedef _int32 (*et_fs_pread)(_u32 file_id, char *buffer, _int32 size, _u64 filepos, _u32 *readsize);
typedef _int32 (*et_fs_pwrite)(_u32 file_id, char *buffer, _int32 size, _u64 filepos, _u32 *writesize);

typedef _int32 (*et_fs_filepos)(_u32 file_id, _u64 *filepos);
typedef _int32 (*et_fs_setfilepos)(_u32 file_id, _u64 filepos);

typedef _int32 (*et_fs_filesize)(_u32 file_id, _u64 *filesize);

typedef _int32 (*et_fs_get_free_disk)(const char *path, _u32 *free_size);


/* socket */
typedef _int32 (*et_socket_set_sockopt)(_u32 socket, _int32 socket_type);

typedef _int32 (*et_socket_create)(_int32 domain, _int32 type, _int32 protocol, _u32 *sock);
typedef _int32 (*et_socket_close)(_u32 socket);

/* memory */
typedef _int32 (*et_mem_get_mem)(_u32 memsize, void **mem);
typedef _int32 (*et_mem_free_mem)(void* mem, _u32 memsize);

typedef BOOL (*et_fs_file_exist)(const char *filepath);

typedef _int32 (*et_zlib_uncompress)( unsigned char *p_out_buffer, int *p_out_len, const unsigned char *p_in_buffer, int in_len );

typedef _int32 (*et_fs_get_file_size_and_modified_time)(const char * filepath,_u64 * p_file_size,_u32 * p_last_modified_time);

typedef _int32 (*et_fs_rmdir)(const char *dirpath);

typedef _int32 (*et_fs_makedir)(const char *dirpath);

typedef _int32 (*et_fs_delete_file)(const char *filepath);

typedef _int32 (*et_fs_rename_file)(const char *filepath, const char *new_filepath);


typedef _int32 (*et_dns_get_dns_server)(char *dns1, char* dns2);

typedef _int32 (*et_log_write_log)(_u32 file_id, char *buffer, _int32 size, _u32 *writesize);

#ifdef __cplusplus
}
#endif
#endif

