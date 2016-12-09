#ifndef SD_FS_H_00138F8F2E70_200806111928
#define SD_FS_H_00138F8F2E70_200806111928

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

/* ------------------------------------------------------ */

#define SD_MAX_PATH	(1024)

#define INVALID_FILE_ID	(0xFFFFFFFF)

/* whether if create file while it not exist. */
#define O_FS_CREATE		(0x1)
/* read and write (default) */
#define O_FS_RDWR		(0x0)
/* read only. */
#define O_FS_RDONLY		(0x2)
/* write only. */
#define O_FS_WRONLY		(0x4)
#define O_FS_MASK       (0xFF)

#if defined(LINUX)
#if defined(_ANDROID_LINUX)
#define FILE_ENLARGE_UNIT	(1024 * 1024)
#define FILE_ENLARGE_WRITE_UNIT	(1024 * 1024) 
#else
#define FILE_ENLARGE_UNIT	(1024 * 1024)
#endif
#elif defined(WINCE)
#define FILE_ENLARGE_UNIT	(1024 * 1024)
#else
#define FILE_ENLARGE_UNIT	(1024 * 1024)
#endif

/* 当前系统支持的目录分隔符 */
#if defined(WINCE) 
#define DIR_SPLIT_CHAR   '\\'
#define DIR_SPLIT_STRING "\\"
#else
#define DIR_SPLIT_CHAR   '/'
#define DIR_SPLIT_STRING "/"
#endif

char format_dir_split_char(char c);

/* ------------------------------------------------------ */

#define FILE_ATTRIB_READABLE (0x0001)
#define FILE_ATTRIB_WRITEABLE (0x0002)
#define FILE_ATTRIB_EXECABLE (0x0004)

typedef struct tag_file_attrib
{
	char _name[MAX_LONG_FILE_NAME_BUFFER_LEN];	
	BOOL _is_dir;
	_u32 _attrib;
#ifdef LINUX
	_u32 _is_lnk:1;
#endif	
} FILE_ATTRIB;

/* ------------------------------------------------------ */

_int32 sd_filepos(_u32 file_id, _u64 *filepos);
_int32 sd_setfilepos(_u32 file_id, _u64 filepos);
_int32 sd_filesize(_u32 file_id, _u64 *filesize);
_int32 sd_open_ex(const char *filepath, _int32 flag, _u32 *file_id);

/* sd_enlarge_file
 * 只允许扩大文件，调用此函数后，filepos位置不确定，请慎用！
 * （配合sd_pread/sd_pwrite使用，或者事先保存filepos）
 */
_int32 sd_enlarge_file(_u32 file_id, _u64 expect_filesize, _u64 *cur_filesize);
_int32 sd_close_ex(_u32 file_id);
_int32 sd_read(_u32 file_id, char *buffer, _int32 size, _u32 *readsize);
_int32 sd_write(_u32 file_id, char *buffer, _int32 size, _u32 *writesize);
_int32 sd_pread(_u32 file_id, char *buffer, _int32 size, _u64 filepos, _u32 *readsize);
_int32 sd_pwrite(_u32 file_id, char *buffer, _int32 size, _u64 filepos, _u32 *writesize);
_int32 sd_append(const char *filepath, char *buffer, _u32 size);
_int32 sd_copy_file(const char *filepath, const char *new_filepath);
_int32 sd_move_file(const char *filepath, const char *new_filepath);
_int32 sd_rename_file(const char *filepath, const char *new_filepath);
_int32 sd_delete_file(const char *filepath);
/* 删除整个目录 */
_int32 sd_delete_dir(const char *path);
_int32 sd_copy_dir(const char *src_path, const char *dest_path);

/* sd_file_exist
 * Return : whether if filepath exist
 */
BOOL sd_file_exist(const char *filepath);

/* sd_dir_exist
 * Return : whether if dirpath is a exist dir 
 */
BOOL sd_dir_exist(const char *dirpath);

_int32 sd_is_path_exist(const char* path);
_int32 sd_ensure_path_exist(const char* path);

_int32 recursive_mkdir(const char *dirpath);

/* create directory recursively */
_int32 sd_mkdir(const char *dirpath);

/* failed if the directory is not empty */
_int32 sd_rmdir(const char *dirpath);

/* failed if the file or directory in directory is in use */
_int32 sd_recursive_rmdir(const char *dirpath);

/* 获取子目录/文件的个数与名称。 
 * 如果sub_files为NULL，或sub_files_size=0，仅计算子目录的个数。
 */
_int32 sd_get_sub_files(const char *dirpath, FILE_ATTRIB sub_files[], _u32 sub_files_size, _u32 *p_sub_files_count);


/* 获取目录内所有子文件(不进入子文件夹)的总大小 
 */
_int32 sd_get_sub_files_total_size(const char *dirpath,  _u64 * total_size);

/* Get file size and last modified time without opening the file */
_int32 sd_get_file_size_and_modified_time(const char * filepath,_u64 * p_file_size,_u32 * p_last_modified_time);

/* get free disk-space about path 
 *  unit of free_size : K (1024 bytes)
 */
_int32 sd_get_free_disk(const char *path, _u64 *free_size);

/* get total disk-space about path 
 *  unit of total_size : K (1024 bytes)
 */
_int32 sd_get_disk_space(const char *path, _u32 *total_size);

/* Check if the file is readable */
BOOL sd_is_file_readable(const char * filepath);
_int32 sd_test_path_writable(const char *path);

BOOL sd_is_file_name_valid(const char * filename);
char* sd_filter_valid_file_name(const char * filename);


/* Save the being writen data to buffer for reducing sd_write operations  */
_int32 sd_write_save_to_buffer(_u32 file_id, char *buffer, _u32 buffer_len,_u32 *buffer_pos, char * p_data,_u32 data_len);

/* 格式化文件路径中的\和/
 * 传入path编码必须兼容ASCII,例如GBK
**/
_int32 sd_format_filepath(const char *path, char *formated_path,
						  _u32 formated_path_size, _u32 *p_formated_path_len);

/* 格式化目录路径中的\和/，最后以分隔符结尾
* 传入path编码必须兼容ASCII,例如GBK
**/
_int32 sd_format_dirpath(const char *path, char *formated_path,
						 _u32 formated_path_size, _u32 *p_formated_path_len);

/* 对path进行转码，适应不同的平台
 * 其中symbian  -> GBK; WINCE ->Unicode; Android ->UTF8 , 其他不处理
**/
_int32 sd_conv_path(const char *input,_u32 input_len, char* output,_u32 *output_len);

/*格式化文件路径中的\和/，并针对不同平台进行转码
**/
_int32 sd_format_conv_filepath(const char *path, char *formated_path,
						  _u32 formated_path_size, _u32 *p_formated_path_len);

/* 格式化目录路径中的\和/，并针对不同平台进行转码
**/
_int32 sd_format_conv_dirpath(const char *path, char *formated_path,
						 _u32 formated_path_size, _u32 *p_formated_path_len);

_int32 sd_append_path(char *path, _u32 buff_len, const char *append);
/* ------------------------------------------------------ */
/* 文件虚拟路径
 * 虚路径格式: DIR_SPLIT_CHAR + 实路径, 如: "/a/b/c", "\c:\a\b\"。
 */

/* 是否真实文件路径 */
BOOL is_realdir(const char *dir);

BOOL sd_realpath(const char *path, char *resolved_path);

/* 文件虚路径转为实路径 */
_int32 vdir2realdir(const char *vdir, char *realdir, _u32 realdir_size, _u32 *p_realdir_len);

/* 文件实路径转为虚路径,不支持相对路径 */
_int32 realdir2vdir(const char *realdir, char *vdir, _u32 vdir_size, _u32 *p_vdir_len);

/* ------------------------------------------------------ */
/*	截断文件 
*/
_int32 sd_truncate(const char *filepath, _u64 length);

/* 检查路径下是否有足够的剩余空间,need_size的单位是KB */
_int32 sd_check_enough_free_disk(char * path, _u64 need_size);

/* ------------------------------------------------------ */

#include <stdio.h>
typedef FILE		_FILE;

/* ------------------------------------------------------ */
/*	mode : r,r+,w,w+,a,a+  or with 'b'
*/
_int32 sd_fopen(const char *filepath, const char * mode,_FILE ** fp);
_int32 sd_fclose(_FILE *fp);

/* 读取一行或n-1个字符 */
_int32 sd_fgets(char *buf, _int32 n, _FILE *fp);

/* 写入字符串 */
_int32 sd_fputs(const char *str, _FILE *fp);

_int32 sd_is_support_create_big_file(const char* path, BOOL* result);
BOOL sd_is_path_writable(const char* path);

_int32 sd_get_total_disk(const char *path, _u64 *total_size);

_int32 sd_recursive_rm_empty_dir(const char* dirpath);
#ifdef __cplusplus
}
#endif

#endif
