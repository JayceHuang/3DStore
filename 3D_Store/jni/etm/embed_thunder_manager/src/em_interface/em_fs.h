#ifndef EM_FS_H_00138F8F2E70_200806111928
#define EM_FS_H_00138F8F2E70_200806111928

#include "em_common/em_define.h"

#include "platform/sd_fs.h"

#define em_filepos	sd_filepos

#define em_setfilepos	sd_setfilepos
#define em_filesize	sd_filesize
#define em_open_ex	sd_open_ex
#define em_enlarge_file	sd_enlarge_file
#define em_close_ex	sd_close_ex
#define em_read	sd_read
#define em_write	sd_write
#define em_pread	sd_pread
#define em_pwrite	sd_pwrite
#define em_rename_file	sd_rename_file
#define em_copy_file	sd_copy_file
#define em_delete_file	sd_delete_file
#define em_file_exist	sd_file_exist
#define em_mkdir	sd_mkdir
#define em_rmdir	sd_rmdir
#define em_dir_exist	sd_dir_exist

#define em_get_file_size_and_modified_time	sd_get_file_size_and_modified_time
#define em_get_free_disk	sd_get_free_disk
#define em_is_file_readable	sd_is_file_readable

#define em_is_file_name_valid	sd_is_file_name_valid
#define em_write_save_to_buffer	sd_write_save_to_buffer
#define em_test_path_writable	sd_test_path_writable
#define em_get_sub_files	sd_get_sub_files
#define em_delete_dir	sd_delete_dir

#endif
