#include "utility/define.h"
#include "platform/sd_fs.h"
#include "utility/time.h"
#include "platform/sd_customed_interface.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/sd_assert.h"
#include "utility/sd_iconv.h"
#include "utility/mempool.h"

#if defined(LINUX)
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>


#if defined(MACOS)
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif
#elif defined(WINCE)
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <windows.h>
#include <winnt.h>
#endif


#define LOGID LOGID_INTERFACE
#include "utility/logger.h"

#define DEFAULT_FILE_MODE	(0644)
#define DEFAULT_DIR_MODE	(0777)

#define MSDOS_SUPER_MAGIC   0x4d44

#if defined(WINCE)
_u16* sd_strrchr_ucs2(_u16* dest, _u16 ch);
_int32 sd_strlen_ucs2(const _u16 *str);
_int32 conv_filepath_wince(const char* filepath, _u32 filepath_len, _u16* ucs2_filepath, _u32* ucs2_filepath_len);
#endif

/////////////////////////////////////////////////////////
_int32 sd_filepos(_u32 file_id, _u64 *filepos)
{
	_int32 ret_val = SUCCESS;

	if (is_available_ci(CI_FS_IDX_FILEPOS))
		return ((et_fs_filepos)ci_ptr(CI_FS_IDX_FILEPOS))(file_id, filepos);

#ifdef LINUX

#if defined(MACOS)
	*filepos = lseek(file_id, 0, SEEK_CUR);
#else
	*filepos = lseek64(file_id, 0, SEEK_CUR);
#endif
	if (*filepos == (_u64)(-1))
	{
		ret_val = errno;
	}
#elif defined(WINCE)
	{
		_int32 pos_high;
		ret_val = SetFilePointer((HANDLE)file_id, 0, &pos_high, FILE_CURRENT);
		if(ret_val == (_u32)(-1) && GetLastError()!=NO_ERROR)
		{
			ret_val = GetLastError();
		}
		else
		{
			*filepos = (_u64)pos_high<<32|(_u64)ret_val;
			ret_val = SUCCESS;
		}
	}
#else
	sd_assert(FALSE);
	ret_val = NOT_IMPLEMENT;
#endif

	return ret_val;
}

_int32 sd_setfilepos(_u32 file_id, _u64 filepos)
{
	_int32 ret_val = SUCCESS;

	if (is_available_ci(CI_FS_IDX_SETFILEPOS))
		return ((et_fs_setfilepos)ci_ptr(CI_FS_IDX_SETFILEPOS))(file_id, filepos);

#ifdef LINUX

#if defined(MACOS)
	if(lseek( file_id, filepos, SEEK_SET) == (_u64)(-1))
#else
	if (lseek64(file_id, filepos, SEEK_SET) == (_u64)(-1))
#endif
	{
		ret_val = errno;
	}
#elif defined(WINCE)
	{
		_int32 pos_high;
		_int32 pos_low;
		pos_low = (_int32)filepos;
		pos_high = (_int32)(filepos>>32);
		ret_val = SetFilePointer((HANDLE)file_id, pos_low, &pos_high, FILE_BEGIN);
		if(ret_val == (_u32)(-1) && GetLastError()!=NO_ERROR)
		{
			LOG_ERROR("SetFilePointer: fail h%d l%d", pos_high, pos_low);
			ret_val = GetLastError();
		}
		else
		{
			ret_val = SUCCESS;
		}
	}
#else
	sd_assert(FALSE);
	ret_val = NOT_IMPLEMENT;
#endif

	return ret_val;
}

_int32 sd_filesize(_u32 file_id, _u64 *filesize)
{
	_int32 ret_val = SUCCESS;
#if defined(LINUX)
	struct stat file_stat;
#endif
	if (is_available_ci(CI_FS_IDX_FILESIZE))
		return ((et_fs_filesize)ci_ptr(CI_FS_IDX_FILESIZE))(file_id, filesize);

#if defined(LINUX)

	*filesize = 0;
	ret_val = fstat(file_id, &file_stat);
	if (ret_val == -1)
	{
		ret_val = errno;
	}
	else
	{
		*filesize = file_stat.st_size;
		ret_val = 0;
	}
#elif defined(WINCE)
	{
		_u32 size_low, size_high;
		*filesize = 0; 
		size_low = GetFileSize((HANDLE)file_id, &size_high);
		if(size_low == -1 && GetLastError() != NO_ERROR)
		{
			ret_val = GetLastError();
		}
		else
		{
			*filesize = (_u64)size_high<<32|(_u64)size_low;
			printf("GetFileSize =%llu\n", *filesize);
			ret_val = SUCCESS;
		}
	}
#else
	sd_assert(FALSE);
	ret_val = NOT_IMPLEMENT;
#endif

	return ret_val;
}

#ifdef ENABLE_ETM_MEDIA_CENTER
#include <sys/vfs.h> // 为sd_enlarge_file中检测文件系统类型(fstatfs)做准备
#define ADFS_SUPER_MAGIC	  0xadf5
#define AFFS_SUPER_MAGIC	  0xADFF
#define BEFS_SUPER_MAGIC	  0x42465331
#define BFS_MAGIC			  0x1BADFACE
#define CIFS_MAGIC_NUMBER	  0xFF534D42
#define CODA_SUPER_MAGIC	  0x73757245
#define COH_SUPER_MAGIC 	  0x012FF7B7
#define CRAMFS_MAGIC		  0x28cd3d45
#define DEVFS_SUPER_MAGIC	  0x1373
#define EFS_SUPER_MAGIC 	  0x00414A53
#define EXT_SUPER_MAGIC 	  0x137D
#define EXT2_OLD_SUPER_MAGIC  0xEF51
#define EXT2_SUPER_MAGIC	  0xEF53
#define EXT3_SUPER_MAGIC	  0xEF53
#define EXT4_SUPER_MAGIC	  0xEF53
#define HFS_SUPER_MAGIC 	  0x4244
#define HPFS_SUPER_MAGIC	  0xF995E849
#define HUGETLBFS_MAGIC 	  0x958458f6
#define ISOFS_SUPER_MAGIC	  0x9660
#define JFFS2_SUPER_MAGIC	  0x72b6
#define JFS_SUPER_MAGIC 	  0x3153464a
#define MINIX_SUPER_MAGIC	  0x137F /* orig. minix */
#define MINIX_SUPER_MAGIC2	  0x138F /* 30 char minix */
#define MINIX2_SUPER_MAGIC	  0x2468 /* minix V2 */
#define MINIX2_SUPER_MAGIC2   0x2478 /* minix V2, 30 char names */
#define MSDOS_SUPER_MAGIC	  0x4d44
#define NCP_SUPER_MAGIC 	  0x564c
#define NFS_SUPER_MAGIC 	  0x6969
#define NTFS_SB_MAGIC		  0x5346544e
#define OPENPROM_SUPER_MAGIC  0x9fa1
#define PROC_SUPER_MAGIC	  0x9fa0
#define QNX4_SUPER_MAGIC	  0x002f
#define REISERFS_SUPER_MAGIC  0x52654973
#define ROMFS_MAGIC 		  0x7275
#define SMB_SUPER_MAGIC 	  0x517B
#define SYSV2_SUPER_MAGIC	  0x012FF7B6
#define SYSV4_SUPER_MAGIC	  0x012FF7B5
#define TMPFS_MAGIC 		  0x01021994
#define UDF_SUPER_MAGIC 	  0x15013346
#define UFS_MAGIC			  0x00011954
#define USBDEVICE_SUPER_MAGIC 0x9fa2
#define VXFS_SUPER_MAGIC	  0xa501FCF5
#define XENIX_SUPER_MAGIC	  0x012FF7B4
#define XFS_SUPER_MAGIC 	  0x58465342
#define _XIAFS_SUPER_MAGIC	  0x012FD16D

#ifdef _USE_REALTEK_UFSD_IOCTL
#ifndef _USE_FTRUNCATE
#define _USE_FTRUNCATE
#endif
///////////////////////////////////////////////////////////
// ufsd_ioctl -- provided by realtek
//
// This function sends ioctl to UFSD driver
///////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

static int
_ufsd_ioctl(
int FileNumber,
size_t IoControlCode,
const void* InputBuffer,
size_t InputBufferSize,
void* OutputBuffer,
size_t OutputBufferSize,
size_t* BytesReturned
)
{
	typedef struct _ufsd_ioctl_params
	{
		size_t IoControlCode;
		const void* InputBuffer;
		size_t InputBufferSize;
		void* OutputBuffer;
		size_t OutputBufferSize;
		size_t* BytesReturned;
	}ufsd_ioctl_params;

#define UFSD_IOCTL 13868

	ufsd_ioctl_params params =
	{
		IoControlCode,
		InputBuffer, InputBufferSize,
		OutputBuffer, OutputBufferSize, BytesReturned
	};

	return ioctl(FileNumber, UFSD_IOCTL, &params);
#undef UFSD_IOCTL
}

#define IOCTL_SET_VALID_DATA2  91

///////////////////////////////////////////////////////////
// Common structure for some iocodes
///////////////////////////////////////////////////////////
union UFSD_HANDLE
{

	struct
	{
		int type;         // Type of Name must be StrASCII (1) or StrUNICODE (2)
		const void* name;// Zero terminated name of file/directory
	}name;

	void* fs_obj;             // Handle of file/directory (CFSObject*)

};

struct UFSD_SET_VALID_DATA
{
	union UFSD_HANDLE handle;
	// New valid data length for the file.
	// This parameter must be a positive value that is greater than
	// the current valid data length, but less than the current file size.
	_u64 valid_size;
};

#elif _USE_TUXERA_IOCTL_FALLOCATE
#ifndef _USE_FTRUNCATE
#define _USE_FTRUNCATE
#endif
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TUXERA_IOCTL_FALLOCATE_FL_KEEP_SIZE 0x0001
#define TUXERA_IOCTL_FALLOCATE_SKIP_ZEROING 0x1000

typedef struct _TUXERA_IOCTL_FALLOCATE_ARGS {
	_u64 offset;
	_u64 length;
	_u32 mode;
} __attribute__ ((packed)) TUXERA_IOCTL_FALLOCATE_ARGS;

#define TUXERA_IOCTL_FALLOCATE  _IOW('N', 213, TUXERA_IOCTL_FALLOCATE_ARGS)

static _int32 _TUXERA_preallocate(_u32 fd, _u64 cur_offset, _u64 expect_filesize)
{
	_int32 ret;
	TUXERA_IOCTL_FALLOCATE_ARGS fa;
	LOG_URGENT("_TUXERA_preallocate(%u, %llu, %llu)", fd, cur_offset, expect_filesize);
	LOG_RUNNING("_TUXERA_preallocate(%u, %llu, %llu)", fd, cur_offset, expect_filesize);
	fa = (TUXERA_IOCTL_FALLOCATE_ARGS)
	{
		.offset = cur_offset,	/* offset in bytes */
			.length = expect_filesize - cur_offset,	/* length in bytes */
			.mode = TUXERA_IOCTL_FALLOCATE_SKIP_ZEROING,
			// Can specify 0, or just one of the
			// flags as desired...
	};
	ret = ioctl(fd, TUXERA_IOCTL_FALLOCATE, &fa);
	if(-1 == ret) return errno;
	return SUCCESS;
}
#endif

_int32 sd_enlarge_file(_u32 file_id, _u64 expect_filesize, _u64 *cur_filesize)
{
	_int32 ret_val = SUCCESS;
	_u64 offset = 0;
	char tmp_buf[2];
	_u32 buf_len = 2;
	BOOL need_ftruncate = TRUE;
	if (is_available_ci(CI_FS_IDX_ENLARGE_FILE))
		return ((et_fs_enlarge_file)ci_ptr(CI_FS_IDX_ENLARGE_FILE))(file_id, expect_filesize, cur_filesize);

	sd_assert(*cur_filesize < expect_filesize);

#ifdef _USE_FTRUNCATE
#ifdef _USE_REALTEK_UFSD_IOCTL  
	// 该宏控制快速创建大文件开关，由realtek提供，仅对ntfs有效
	struct statfs fs_info;
	struct UFSD_SET_VALID_DATA ufsd_valid;
#elif defined(_USE_TUXERA_IOCTL_FALLOCATE)
	struct statfs fs_info;
#endif
	offset = expect_filesize;
#else // NOT _USE_FTRUNCATE
	offset = *cur_filesize + FILE_ENLARGE_UNIT;
	if (offset > expect_filesize)
		offset = expect_filesize;
#endif /* end of _USE_FTRUNCATE */

#if defined(AMLOS) || defined(LINUX) || defined(SUNPLUS)
#ifdef _USE_FTRUNCATE
#ifdef _USE_REALTEK_UFSD_IOCTL
	while (1)
	{
#if defined(_HIMEDIA_ANDROID_ARM)
		ret_val = ftruncate64(file_id, offset);
#else
		ret_val = ftruncate(file_id, offset);
#endif
		LOG_URGENT("sd_enlarge_file[%u], !!!FTRUNCATE!!!", file_id);
		if(ret_val)
		{
			if(errno == EINTR)
			{
				LOG_URGENT("sd_enlarge_file, system call interrupted!! retry...");
			}
			ret_val = errno;
			LOG_ERROR("sd_enlarge_file, ftruncate: %d",ret_val);
			return ret_val;
		}
		else
		{
			break;
		}
	}

#if defined(_HIMEDIA_ANDROID_ARM)
#define UFSD_IOC_SETVALID         _IOW('f', 91, unsigned long long)

	ret_val = fstatfs(file_id, &fs_info);
	LOG_URGENT("sd_enlarge_file[%u], FS TYPE: 0x%X, NTFS_SB_MAGIC=0x%X, valid_size=%llu",
		file_id, fs_info.f_type, NTFS_SB_MAGIC, offset);
	LOG_RUNNING("sd_enlarge_file[%u], FS TYPE: 0x%X, NTFS_SB_MAGIC=0x%X, valid_size=%llu",
		file_id, fs_info.f_type, NTFS_SB_MAGIC, offset);
	if(SUCCESS == ret_val && fs_info.f_type == NTFS_SB_MAGIC)
	{
		ret_val = ioctl( file_id, UFSD_IOC_SETVALID, &offset);
		if(SUCCESS != ret_val)
		{
			ret_val = errno;
			LOG_URGENT("sd_enlarge_file, ioctl: %d", ret_val);
			LOG_RUNNING("sd_enlarge_file, ioctl: %d", ret_val);
			fprintf(stderr, "sd_enlarge_file, ioctl: %d", ret_val);
			sd_assert(FALSE);
		}
		else
		{
			need_ftruncate = FALSE;
		}
	}
#else
	ufsd_valid.valid_size = offset;
	ufsd_valid.handle.fs_obj = NULL;
	ret_val = fstatfs(file_id, &fs_info);
	LOG_URGENT("sd_enlarge_file[%u], FS TYPE: 0x%X, NTFS_SB_MAGIC=0x%X, valid_size=%llu",
		file_id, fs_info.f_type, NTFS_SB_MAGIC, ufsd_valid.valid_size);
	if(SUCCESS == ret_val && fs_info.f_type == NTFS_SB_MAGIC)
	{
		LOG_URGENT("_ufsd_ioctl(%u, %d, ufsd_valid.valid_size=%llu, %d, NULL, 0, NULL",
			file_id, IOCTL_SET_VALID_DATA2, ufsd_valid.valid_size, sizeof(ufsd_valid));
		ret_val = _ufsd_ioctl( file_id, IOCTL_SET_VALID_DATA2, &ufsd_valid,
			sizeof(ufsd_valid), NULL, 0, NULL );
		if(SUCCESS != ret_val)
		{
			ret_val = errno;
			LOG_URGENT("sd_enlarge_file, _ufsd_ioctl: %d", ret_val);
			fprintf(stderr, "sd_enlarge_file, _ufsd_ioctl: %d", ret_val);
			sd_assert(FALSE);
		}
		else
		{
			need_ftruncate = FALSE;
		}
	}
#endif

#ifdef _XUNLEI_CLOUD_BOX_REALTEK_1185
	else if(SUCCESS == ret_val && fs_info.f_type == EXT4_SUPER_MAGIC)
	{
		ret_val = posix_fallocate(file_id, 0, offset);
		if(SUCCESS != ret_val)
		{
			ret_val = errno;
			LOG_URGENT("sd_enlarge_file, posix_fallocate: %d", ret_val);
			sd_assert(FALSE);
		}
		else
		{
			LOG_URGENT("sd_enlarge_file, posix_fallocate(%u, 0, %llu): SUCCESS", file_id, offset);
			need_ftruncate = FALSE;
		}
	}
#endif

#elif defined(_USE_TUXERA_IOCTL_FALLOCATE)
	ret_val = fstatfs(file_id, &fs_info);
	LOG_URGENT("sd_enlarge_file[%u], FS TYPE: 0x%X, NTFS_SB_MAGIC=0x%X",
		file_id, fs_info.f_type, NTFS_SB_MAGIC);
	if(SUCCESS == ret_val && fs_info.f_type == NTFS_SB_MAGIC)
	{
		ret_val = _TUXERA_preallocate(file_id, *cur_filesize, offset);
		if(SUCCESS != ret_val)
		{
			LOG_ERROR("sd_enlarge_file, _TUXERA_preallocate: %d", ret_val);
			fprintf(stderr, "sd_enlarge_file, _TUXERA_preallocate: %d", ret_val);
			sd_assert(FALSE);
		}
		else
		{
			need_ftruncate = FALSE;
		}
	}
#elif defined(_TERRA_MASTER_F4_NAS)
	struct statfs fs_info;
	ret_val = fstatfs(file_id, &fs_info);
	if (ret_val == 0 && fs_info.f_type == EXT4_SUPER_MAGIC) {
		ret_val = posix_fallocate(file_id, *cur_filesize, expect_filesize - *cur_filesize);
		if(ret_val) {   
			LOG_ERROR("sd_enlarge_file: posix_fallocate: %d.\n", ret_val);
		} else {
			need_ftruncate = FALSE;
		}
	}
#endif
	if (need_ftruncate)
	{
		while (1)
		{
			ret_val = ftruncate(file_id, offset);
			LOG_URGENT("sd_enlarge_file[%u], !!!FTRUNCATE!!!", file_id);
			if (ret_val)
			{
				if (errno == EINTR)
				{
					LOG_URGENT("sd_enlarge_file, system call interrupted!! retry...");
				}
				ret_val = errno;
				LOG_ERROR("sd_enlarge_file, ftruncate: %d", ret_val);
				return ret_val;
			}
			else
			{
				break;
			}
		}
	}

	LOG_URGENT("sd_enlarge_file[%u], >>>>>>>>>>> %llu->%llu", file_id, *cur_filesize, offset);

	*cur_filesize = offset;
#else // NOT _USE_FTRUNCATE
	ret_val = sd_pwrite(file_id, tmp_buf, 1, offset - 1, &buf_len);
	if (ret_val == SUCCESS)
	{
		*cur_filesize = offset;
	}
#endif /* end of _USE_FTRUNCATE */

#elif defined(WINCE)
	{
		LONG lDistanceToMove;
		sd_assert(*cur_filesize <= expect_filesize);
		if(*cur_filesize > expect_filesize) return FILE_CANNOT_TRUNCATE;
		if(0xFFFFFFFF < expect_filesize) return FILE_TOO_BIG;
		lDistanceToMove = expect_filesize-*cur_filesize;
		if(*cur_filesize == 0)
		{
			ret_val = SetFilePointer ((HANDLE)file_id,lDistanceToMove, NULL, FILE_BEGIN);
		}
		else
		{
			ret_val = SetFilePointer ((HANDLE)file_id,lDistanceToMove, NULL, FILE_CURRENT);
		}

		if (ret_val == 0xFFFFFFFF)
		{
			ret_val = GetLastError();
			if(NO_ERROR==ret_val)
			{
				*cur_filesize+=ret_val;
			}
		}
		else
		{
			*cur_filesize+=ret_val;
			ret_val = NO_ERROR;
		}

		if(NO_ERROR==ret_val)
		{
			ret_val =SetEndOfFile((HANDLE)file_id);
			if(ret_val == 0)
			{
				ret_val= FILE_CANNOT_TRUNCATE;
			}
			else
			{
				ret_val= SUCCESS;
			}
		}
	}

#else
	sd_assert(FALSE);
	ret_val = NOT_IMPLEMENT;
#endif

	return ret_val;
}
#else
_int32 sd_enlarge_file(_u32 file_id, _u64 expect_filesize, _u64 *cur_filesize)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)
	_u64 offset = 0;
	char tmp_buf[2]={0};
	_u32 buf_len = 2;
#endif

	if(is_available_ci(CI_FS_IDX_ENLARGE_FILE))
		return ((et_fs_enlarge_file)ci_ptr(CI_FS_IDX_ENLARGE_FILE))(file_id, expect_filesize, cur_filesize);

#if defined(LINUX)
	sd_assert(*cur_filesize < expect_filesize);

	offset = *cur_filesize + FILE_ENLARGE_UNIT;
	if(offset > expect_filesize)
		offset = expect_filesize;

	ret_val = sd_pwrite(file_id, tmp_buf, 1, offset - 1, &buf_len);
	if(ret_val == SUCCESS)
	{
		*cur_filesize = offset;
	}

#elif defined(WINCE)
	{
		LONG lDistanceToMove;
		sd_assert(*cur_filesize <= expect_filesize);
		if(*cur_filesize > expect_filesize) return FILE_CANNOT_TRUNCATE;
		if(0xFFFFFFFF < expect_filesize) return FILE_TOO_BIG;
		lDistanceToMove = expect_filesize-*cur_filesize;
		if(*cur_filesize == 0)
		{
			ret_val = SetFilePointer ((HANDLE)file_id,lDistanceToMove, NULL, FILE_BEGIN) ; 
		}
		else
		{
			ret_val = SetFilePointer ((HANDLE)file_id,lDistanceToMove, NULL, FILE_CURRENT) ; 
		}

		if (ret_val == 0xFFFFFFFF)
		{ 
			ret_val = GetLastError() ; 
			if(NO_ERROR==ret_val)
			{
				*cur_filesize+=ret_val;
			}
		} 
		else
		{
			*cur_filesize+=ret_val;
			ret_val = NO_ERROR;
		}

		if(NO_ERROR==ret_val)
		{
			ret_val =SetEndOfFile((HANDLE)file_id);
			if(ret_val == 0)
			{
				ret_val= FILE_CANNOT_TRUNCATE;
			}
			else
			{
				ret_val= SUCCESS;
			}
		}
	}
#else
	sd_assert(FALSE);
	ret_val = NOT_IMPLEMENT;
#endif

	return ret_val;
}
#endif


_int32 sd_open_ex(const char *filepath, _int32 flag, _u32 *file_id)
{
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;
	_int32 os_flags = 0;
	BOOL open_again = FALSE;
	if (is_available_ci(CI_FS_IDX_OPEN))
	{
		ret_val = ((et_fs_open)ci_ptr(CI_FS_IDX_OPEN))((char*)filepath, flag, file_id);
		return ret_val;
	}

	if (NULL == filepath || sd_strlen(filepath) == 0 || sd_strlen(filepath) >= MAX_LONG_FULL_PATH_BUFFER_LEN || NULL == file_id)
	{
		return INVALID_ARGUMENT;
	}


	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	/* 和谐一下分隔符格式及编码方式 */
	ret_val = sd_format_conv_filepath(filepath, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

#if defined(LINUX)
	os_flags = O_RDWR;

	switch (flag & O_FS_MASK)
	{
	case O_FS_WRONLY:
		os_flags = O_WRONLY;
		break;
	case O_FS_RDONLY:
		os_flags = O_RDONLY;
		break;
	case O_FS_RDWR:
	default:
		os_flags = O_RDWR;
		break;
	}

	if ((flag & O_FS_MASK) & O_FS_CREATE)
	{
		os_flags |= O_CREAT;
	}

TRY_AGAIN:
	ret_val = open(formatted_conv_filepath, os_flags, DEFAULT_FILE_MODE);
	if (ret_val == -1)
	{
		LOG_DEBUG("sd_open_ex error, ret_val:%d, flag:0x%x, path:%s", ret_val, flag, filepath);
		ret_val = open(filepath, os_flags, DEFAULT_FILE_MODE);
	}

	if (ret_val == -1)
	{
		ret_val = errno;
		LOG_DEBUG("real error sd_open_ex error, ret_val:%d, flag:0x%x, path:%s", ret_val, flag, filepath);
	}
	else
	{
		sd_assert(ret_val != 0);
		if (ret_val == 0)
		{
			if (!open_again)
			{
				open_again = TRUE;
				goto TRY_AGAIN;
			}
			else
			{
				ret_val = UNKNOWN_OPENING_FILE_ERR;
			}
		}
		else
		{
			*file_id = ret_val;
			fchmod(ret_val, 0777);
			ret_val = SUCCESS;
		}
		LOG_DEBUG("success to open new file(ret_val=%d,id:%d, path:%s)", ret_val, *file_id, filepath);
	}

#elif defined(WINCE)
	{
		os_flags = OPEN_EXISTING;

		if(flag & O_FS_CREATE)
			os_flags = OPEN_ALWAYS;
		// 打开文件
		ret_val = (_u32)CreateFile( (LPCWSTR) formatted_conv_filepath, GENERIC_WRITE|GENERIC_READ,FILE_SHARE_WRITE|FILE_SHARE_READ
			,NULL,os_flags, FILE_FLAG_RANDOM_ACCESS, NULL);

		//printf("\n after sd_open_ex:%s ret_val=%d", filepath,ret_val);
		if(ret_val == -1)
		{
			ret_val = GetLastError();
		}
		else
		{
			*file_id = ret_val;
			ret_val = SUCCESS;
		}
	}
#else
	sd_assert(FALSE);
	ret_val = NOT_IMPLEMENT;
#endif
	return ret_val;
}

_int32 sd_close_ex(_u32 file_id)
{
	_int32 ret_val = SUCCESS;

	if (is_available_ci(CI_FS_IDX_CLOSE))
		return ((et_fs_close)ci_ptr(CI_FS_IDX_CLOSE))(file_id);

#if defined(LINUX)
	fchmod(file_id, 0777);
	close(file_id);
	LOG_DEBUG("success to close file(id:%d)", file_id);
#elif defined(WINCE)
	CloseHandle((HANDLE)file_id);
#endif

	return ret_val;
}

#ifdef ENABLE_ETM_MEDIA_CENTER
_int32 sd_read(_u32 file_id, char *buffer, _int32 size, _u32 *readsize)
{
	_int32 ret_val = SUCCESS;

	if (is_available_ci(CI_FS_IDX_READ))
		return ((et_fs_read)ci_ptr(CI_FS_IDX_READ))(file_id, buffer, size, readsize);

#if defined(LINUX)

	(*readsize) = 0;

	do
	{
		ret_val = read(file_id, buffer, size);
	} while (ret_val < 0 && errno == EINTR);

	if (ret_val >= 0)
	{
		*readsize = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
	}
#elif defined(WINCE)
	(*readsize) = 0;

	ret_val = ReadFile((HANDLE)file_id, buffer, size, readsize, NULL);

	if(ret_val == TRUE)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = GetLastError();
	}

#endif

	return ret_val;
}

_int32 sd_write(_u32 file_id, char *buffer, _int32 size, _u32 *writesize)
{
	_int32 ret_val = SUCCESS;

	if (is_available_ci(CI_FS_IDX_WRITE))
		return ((et_fs_write)ci_ptr(CI_FS_IDX_WRITE))(file_id, buffer, size, writesize);

#if defined(LINUX)

	*writesize = 0;

	do
	{
		ret_val = write(file_id, buffer, size);
	} while (ret_val < 0 && errno == EINTR);

	if (ret_val >= 0)
	{
		*writesize = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
	}
	//#ifndef _ANDROID_ARM
	//	fsync(file_id);
	//#endif

#elif defined(WINCE)
	*writesize = 0;
	ret_val = WriteFile((HANDLE)file_id, buffer, size, writesize, NULL);
	//printf("\n writefile ret=%d, file_id=%d, size=%d, writtensize=%d", ret_val, file_id, size,*writesize);
	if(ret_val >= 0)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = GetLastError();
	}

#endif

	return ret_val;
}

_int32 sd_pread(_u32 file_id, char *buffer, _int32 size, _u64 filepos, _u32 *readsize)
{
	_int32 ret_val = SUCCESS;

	if ((NULL == buffer || NULL == readsize) && size != 0)
	{
		return INVALID_ARGUMENT;
	}
	if (is_available_ci(CI_FS_IDX_PREAD))
	{
		ret_val = ((et_fs_pread)ci_ptr(CI_FS_IDX_PREAD))(file_id, buffer, size, filepos, readsize);
		return ret_val;
	}

#if defined(LINUX)

	(*readsize) = 0;

	ret_val = sd_setfilepos(file_id, filepos);
	CHECK_VALUE(ret_val);
	ret_val = sd_read(file_id, buffer, size, readsize);

#elif defined(WINCE)
	(*readsize) = 0;

	ret_val = sd_setfilepos(file_id, filepos);
	CHECK_VALUE(ret_val);

	ret_val = ReadFile((HANDLE)file_id, buffer, size, readsize, NULL);

	if(ret_val == TRUE)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = GetLastError();
	}

#endif

	return ret_val;
}

_int32 sd_pwrite(_u32 file_id, char *buffer, _int32 size, _u64 filepos, _u32 *writesize)
{
	_int32 ret_val = SUCCESS;

	if ((NULL == buffer || NULL == writesize) && size != 0)
	{
		return INVALID_ARGUMENT;
	}

	if (is_available_ci(CI_FS_IDX_PWRITE))
	{
		ret_val = ((et_fs_pwrite)ci_ptr(CI_FS_IDX_PWRITE))(file_id, buffer, size, filepos, writesize);
		return ret_val;
	}

#if defined(LINUX)

	*writesize = 0;

	ret_val = sd_setfilepos(file_id, filepos);
	CHECK_VALUE(ret_val);
	ret_val = sd_write(file_id, buffer, size, writesize);
//	if (ret_val == SUCCESS)
//	{
//#if !defined(_ANDROID_LINUX)|| !defined(_ANDROID_ARM)
//		fsync(file_id);
//#endif
//	}
#elif defined(WINCE)
	*writesize = 0;
	ret_val = sd_setfilepos(file_id, filepos);
	if(ret_val != SUCCESS){
		LOG_DEBUG("sd_setfilepos failed to write file:pos(id:%d pos:%llu)", file_id, filepos);
		return ret_val;
	}

	ret_val = WriteFile((HANDLE)file_id, buffer, size, writesize, NULL);
	if(ret_val >= 0)
	{
		FlushFileBuffers(  (HANDLE)file_id ); 
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		*writesize = 0;
		ret_val = GetLastError();
		LOG_DEBUG("sd_pwrite failed to write file:pos(id:%d pos:%llu)", file_id, filepos);
	}
#endif

	return ret_val;
}
#else
_int32 sd_read(_u32 file_id, char *buffer, _int32 size, _u32 *readsize)
{
	_int32 ret_val = SUCCESS;

	if(is_available_ci(CI_FS_IDX_READ))
		return ((et_fs_read)ci_ptr(CI_FS_IDX_READ))(file_id, buffer, size, readsize);

#if defined(LINUX)

	(*readsize) = 0;

	do
	{
		ret_val = read(file_id, buffer, size);
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		*readsize = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
	}
#elif defined(WINCE)
	(*readsize) = 0;

	ret_val = ReadFile((HANDLE)file_id, buffer, size, readsize, NULL);

	if(ret_val == TRUE)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = GetLastError();
	}

#endif

	return ret_val;
}

_int32 sd_write(_u32 file_id, char *buffer, _int32 size, _u32 *writesize)
{
	_int32 ret_val = SUCCESS;

	if(is_available_ci(CI_FS_IDX_WRITE))
		return ((et_fs_write)ci_ptr(CI_FS_IDX_WRITE))(file_id, buffer, size, writesize);

#if defined(LINUX)

	*writesize = 0;

	do
	{
		ret_val = write(file_id, buffer, size);
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		*writesize = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
	}

	fsync(file_id);

#elif defined(WINCE)
	*writesize = 0;
	ret_val = WriteFile((HANDLE)file_id, buffer, size, writesize, NULL);
	//printf("\n writefile ret=%d, file_id=%d, size=%d, writtensize=%d", ret_val, file_id, size,*writesize);
	if(ret_val >= 0)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = GetLastError();
	}

#endif

	return ret_val;
}

_int32 sd_pread(_u32 file_id, char *buffer, _int32 size, _u64 filepos, _u32 *readsize)
{
	_int32 ret_val = SUCCESS;

	if ((NULL == buffer || NULL == readsize) && size != 0)
	{
		return INVALID_ARGUMENT;
	}
	if(is_available_ci(CI_FS_IDX_PREAD))
	{
		ret_val = ((et_fs_pread)ci_ptr(CI_FS_IDX_PREAD))(file_id, buffer, size, filepos, readsize);
		return ret_val;
	}

#if defined(LINUX)

	(*readsize) = 0;

	do
	{
#ifdef _ANDROID_LINUX	
		ret_val = sd_setfilepos(file_id, filepos);
		CHECK_VALUE(ret_val);
		return sd_read(file_id, buffer, size, readsize);
#elif defined(MACOS)
		ret_val = pread(file_id, buffer, size, filepos);
#else
		ret_val = pread64(file_id, buffer, size, filepos);
#endif
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		*readsize = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
	}
#elif defined(WINCE)
	(*readsize) = 0;

	ret_val = sd_setfilepos(file_id, filepos);
	CHECK_VALUE(ret_val);

	ret_val = ReadFile((HANDLE)file_id, buffer, size, readsize, NULL);

	if(ret_val == TRUE)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = GetLastError();
	}

#endif

	return ret_val;
}

_int32 sd_pwrite(_u32 file_id, char *buffer, _int32 size, _u64 filepos, _u32 *writesize)
{
	_int32 ret_val = SUCCESS;

	if ((NULL == buffer || NULL == writesize) && size != 0)
	{
		return INVALID_ARGUMENT;
	}

	if(is_available_ci(CI_FS_IDX_PWRITE))
	{
		ret_val = ((et_fs_pwrite)ci_ptr(CI_FS_IDX_PWRITE))(file_id, buffer, size, filepos, writesize);
		return ret_val;
	}

#if defined(LINUX)

	*writesize = 0;

	do
	{
#ifdef _ANDROID_LINUX
		ret_val = sd_setfilepos(file_id, filepos);		
		CHECK_VALUE(ret_val);
		return sd_write(file_id, buffer, size, writesize);
#elif defined(MACOS)
		ret_val = pwrite(file_id, buffer, size, filepos);
#else
		ret_val = pwrite64(file_id, buffer, size, filepos);
#endif
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		*writesize = ret_val;
#ifndef _ANDROID_LINUX
		fsync(file_id);
#endif
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
	}
#elif defined(WINCE)
	*writesize = 0;
	ret_val = sd_setfilepos(file_id, filepos);
	if(ret_val != SUCCESS){
		LOG_DEBUG("sd_setfilepos failed to write file:pos(id:%d pos:%llu)", file_id, filepos);
		return ret_val;
	}

	ret_val = WriteFile((HANDLE)file_id, buffer, size, writesize, NULL);
	if(ret_val >= 0)
	{
		FlushFileBuffers(  (HANDLE)file_id ); 
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		*writesize = 0;
		ret_val = GetLastError();
		LOG_DEBUG("sd_pwrite failed to write file:pos(id:%d pos:%llu)", file_id, filepos);
	}
#endif

	return ret_val;
}

#endif
// 写数据到文件最后。
_int32 sd_append(const char *filepath, char *buffer, _u32 size)
{
	_int32 ret = SUCCESS;
	_u32 file_id = 0;
	_u64 filesize = 0;
	_u32 writesize = 0, write_total_size = 0;

	ret = sd_open_ex(filepath, O_FS_WRONLY | O_FS_CREATE, &file_id);
	CHECK_VALUE(ret);

	ret = sd_filesize(file_id, &filesize);
	if (ret != SUCCESS)
	{
		sd_close_ex(file_id);
		return ret;
	}

	while (write_total_size < size)
	{
		ret = sd_pwrite(file_id, buffer, size, filesize, &writesize);
		if (ret != SUCCESS)
		{
			sd_close_ex(file_id);
			return ret;
		}
		write_total_size += writesize;
	}

	sd_close_ex(file_id);
	return SUCCESS;
}


_int32 sd_rename_file(const char *filepath, const char *new_filepath)
{

	/* 和谐一下分隔符格式及编码方式 */
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_filepath_len = 0;
	_u32 new_formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;
	char new_formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 new_formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

	if (is_available_ci(CI_FS_IDX_RENAME_FILE))
		return ((et_fs_rename_file)ci_ptr(CI_FS_IDX_RENAME_FILE))(filepath, new_filepath);

#if defined(LINUX)

	// 如果new_filepath文件已经存在，返回错误。以便和其他平台保持一致性
	if (sd_file_exist(new_filepath))
		return -1;

#endif

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(filepath, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

	sd_memset(new_formatted_conv_filepath, 0, new_formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(new_filepath, new_formatted_conv_filepath, new_formatted_conv_filepath_buflen, &new_formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

#if defined(LINUX)

	if (rename(formatted_conv_filepath, new_formatted_conv_filepath) == -1)
	{
		ret_val = errno;
	}
#elif defined(WINCE)
	if(MoveFile((LPCWSTR)formatted_conv_filepath, (LPCWSTR)new_formatted_conv_filepath) == FALSE)
	{
		ret_val = GetLastError();
	}
#endif

	return ret_val;

}

_int32 sd_copy_file(const char *filepath, const char *new_filepath)
{
	/* 和谐一下分隔符格式及编码方式 */
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_filepath_len = 0;
	_u32 new_formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;
	char new_formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 new_formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(filepath, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

	sd_memset(new_formatted_conv_filepath, 0, new_formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(new_filepath, new_formatted_conv_filepath, new_formatted_conv_filepath_buflen, &new_formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

	LOG_DEBUG("sd_copy_file, from %s to %s", filepath, new_filepath);
#if defined(WINCE)
	{
		if (CopyFile((LPCWSTR)formatted_conv_filepath, (LPCWSTR)new_formatted_conv_filepath, TRUE) == FALSE)
			ret_val = GetLastError();
	}
#elif defined(LINUX)
	{
#define MAX_CPFILE_DATA_BUF_LEN (1024)
		struct stat stat_buf;
		char databuf[MAX_CPFILE_DATA_BUF_LEN];
		_int32 fd1 = -1;
		_int32 fd2 = -1;
		_u32 size = 0;

		sd_memset(&stat_buf, 0, sizeof (stat_buf));
		ret_val = lstat(formatted_conv_filepath, &stat_buf);
		if (ret_val != SUCCESS)
		{
			LOG_DEBUG("sd_copy_file, lstat fail");
			return ret_val;
		}
		if (sd_strcmp(formatted_conv_filepath, new_formatted_conv_filepath) == 0)
		{
			LOG_DEBUG("sd_copy_file, from is same as to");
			return -1;
		}
		if ((ret_val = open(formatted_conv_filepath, O_RDONLY)) == -1)
		{
			LOG_DEBUG("sd_copy_file, open from file fail");
			return ret_val;
		}
		fd1 = ret_val;
		if (SUCCESS != sd_open_ex(new_formatted_conv_filepath, O_FS_CREATE, (_u32*)&ret_val))
		{
			LOG_DEBUG("sd_copy_file, open to file fail");
			return ret_val;
		}

		fd2 = ret_val;

		ret_val = SUCCESS;
		while ((size = read(fd1, databuf, MAX_CPFILE_DATA_BUF_LEN)) != 0)
		{
			if (write(fd2, databuf, size) != size)
			{
				LOG_DEBUG("sd_copy_file, write to file fail");
				ret_val = -1;
				break;
			}
		}
		close(fd1);
		close(fd2);
#undef MAX_CPFILE_DATA_BUF_LEN 
	}

#endif
	return ret_val;
}

_int32 sd_move_file(const char *filepath, const char *new_filepath)
{
	/* 和谐一下分隔符格式及编码方式 */
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_filepath_len = 0;
	_u32 new_formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;
	char new_formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 new_formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(filepath, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

	sd_memset(new_formatted_conv_filepath, 0, new_formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(new_filepath, new_formatted_conv_filepath, new_formatted_conv_filepath_buflen, &new_formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

#if defined(WINCE)
	//{
	//	if (CopyFile((LPCWSTR)formatted_conv_filepath, (LPCWSTR)new_formatted_conv_filepath, TRUE) == FALSE)
	//		ret_val = GetLastError();
	//}
	ret_val = NOT_IMPLEMENT; 
#elif defined(LINUX)
	{
#define MAX_CPFILE_DATA_BUF_LEN (1024)
		struct stat stat_buf;

		sd_memset(&stat_buf, 0, sizeof (stat_buf));
		ret_val = lstat(formatted_conv_filepath, &stat_buf);
		if (ret_val != SUCCESS)
		{
			return ret_val;
		}
		if (sd_strcmp(formatted_conv_filepath, new_formatted_conv_filepath) == 0)
		{
			return -1;
		}
		ret_val = rename(formatted_conv_filepath, new_formatted_conv_filepath);
#undef MAX_CPFILE_DATA_BUF_LEN 
	}

#endif
	return ret_val;
}



_int32 sd_delete_file(const char *filepath)
{

	/* 和谐一下分隔符格式及编码方式 */
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

	LOG_DEBUG("sd_delete_file begin");

	if (is_available_ci(CI_FS_IDX_DELETE_FILE))
		return ((et_fs_delete_file)ci_ptr(CI_FS_IDX_DELETE_FILE))(filepath);

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(filepath, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

	LOG_DEBUG("sd_delete_file do, filepath = %s", filepath);
#if defined(LINUX)

	LOG_DEBUG("sd_delete_file(path:%s)", filepath);
	if (unlink(formatted_conv_filepath) == -1)
		ret_val = errno;
#elif defined(WINCE)
	{
		LOG_DEBUG("sd_delete_file(path:%s)", filepath);

		ret_val = DeleteFile((LPCWSTR)formatted_conv_filepath);
		if(TRUE == ret_val)
		{
			ret_val = SUCCESS;
		}
		else
		{
			ret_val = GetLastError();
			if(ret_val==0) ret_val = 2;
		}
	}

#endif

	return ret_val;

}


/**
* 如果传入is_dir为TRUE，检查filepath是否为已存在的目录
* 否则仅检查filepath是否存在，不关心其是文件还是目录
*/
static BOOL sd_file_exist_ex(const char *filepath, const BOOL is_dir)
{
	/* 和谐一下格式 */
	BOOL ret_val = TRUE;
	_int32 ret = 0;
	_u32 formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

#if defined(LINUX) //&& !defined(_ANDROID_LINUX)
	struct stat file_stat;
#endif


	if (is_available_ci(CI_FS_IDX_FILE_EXIST))
		return ((et_fs_file_exist)ci_ptr(CI_FS_IDX_FILE_EXIST))(filepath);

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	ret = sd_format_conv_filepath(filepath, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	sd_assert(ret == SUCCESS);
	if (ret != SUCCESS)
	{
		return FALSE;
	}

#if defined(LINUX) //&& !defined(_ANDROID_LINUX)


	if ((lstat(formatted_conv_filepath, &file_stat) == -1)/* && errno == ENOENT*/)
	{
		ret_val = FALSE;
	}
	else
	{
		ret_val = is_dir ? S_ISDIR(file_stat.st_mode) : TRUE;
	}


	if (!ret_val)
	{

		// 不编码重试一次.....
		if ((lstat(filepath, &file_stat) == -1)/* && errno == ENOENT*/)
		{
			ret_val = FALSE;
		}
		else
		{
			ret_val = is_dir ? S_ISDIR(file_stat.st_mode) : TRUE;
		}
	}

#elif defined(WINCE)
	{
		_u32 rv = 0 ;

		rv = GetFileAttributes((LPCWSTR)formatted_conv_filepath);
		if(rv == (_u32)-1 )
		{		
			ret_val = FALSE;
		} 
		else
		{
			ret_val = is_dir ? (rv & FILE_ATTRIBUTE_DIRECTORY)!=0 : TRUE;
		}
		printf("sd_file_exist +++++++++++++++ file: %s, ret_val: %d \n", filepath, ret_val);
	}

#endif

	return ret_val;

}

BOOL sd_file_exist(const char *filepath)
{
	BOOL ret = FALSE;
	ret = sd_file_exist_ex(filepath, FALSE);
	//LOG_DEBUG("sd_file_exist, (%d):%s", ret, filepath);
	return ret;
}

BOOL sd_dir_exist(const char *dirpath)
{
	BOOL ret = FALSE;

	ret = sd_file_exist_ex(dirpath, TRUE);
	//LOG_DEBUG("sd_dir_exist: (%d):%s", ret, dirpath);
	return ret;
}

_int32 sd_is_path_exist(const char* path)
{
	_int32 ret_val = SUCCESS;

	/* 和谐一下格式，但是不能转码，因为recursive_mkdir递归内部无和谐且需要gbk编码*/
	char formatted_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_filepath_len = 0;

	sd_memset(formatted_filepath, 0, MAX_LONG_FULL_PATH_BUFFER_LEN);
	ret_val = sd_format_dirpath(path, formatted_filepath, MAX_LONG_FULL_PATH_BUFFER_LEN, &formatted_filepath_len);
	CHECK_VALUE(ret_val);

	if (sd_file_exist(formatted_filepath))
	{
		return SUCCESS;
	}

	LOG_DEBUG("sd_is_path_exist: path %s , not exist.", formatted_filepath);

	return TM_ERR_INVALID_FILE_PATH;
}

_int32 sd_ensure_path_exist(const char* path)
{
	_int32 ret_val = SUCCESS;

	/* 和谐一下格式，但是不能转码，因为recursive_mkdir递归内部无和谐且需要gbk编码*/
	char formatted_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_filepath_len = 0;

	sd_memset(formatted_filepath, 0, MAX_LONG_FULL_PATH_BUFFER_LEN);
	ret_val = sd_format_dirpath(path, formatted_filepath, MAX_LONG_FULL_PATH_BUFFER_LEN, &formatted_filepath_len);
	CHECK_VALUE(ret_val);

	if (sd_file_exist(formatted_filepath))
	{
		return SUCCESS;
	}
	ret_val = recursive_mkdir((char*)formatted_filepath);


	LOG_DEBUG("sd_ensure_path_exist: (%d):%s", ret_val, formatted_filepath);
	return ret_val;
}

//非递归创建dir，dirpath格式已经和谐，编码未和谐
static _int32 pure_mkdir(const char* dirpath)
{
	_int32 ret_val = SUCCESS;
#if defined(WINCE)
	char unicode_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 unicode_len = MAX_LONG_FULL_PATH_BUFFER_LEN;

	sd_memset(unicode_filepath, 0x00, MAX_LONG_FULL_PATH_BUFFER_LEN);

	ret_val = sd_conv_path(dirpath, sd_strlen(dirpath), unicode_filepath, &unicode_len);
	CHECK_VALUE(ret_val);

	ret_val = CreateDirectory((LPCWSTR)unicode_filepath, NULL);
	if(ret_val != TRUE)
	{
		ret_val = GetLastError();

		if(ret_val == ERROR_PATH_NOT_FOUND)
		{
		}
		else if(ret_val == ERROR_FILE_EXISTS)
		{
			ret_val = SUCCESS;
		}
		else if(ERROR_ALREADY_EXISTS == ret_val)
		{
			ret_val = SUCCESS;
		}
	}
	else
	{
		ret_val = SUCCESS;
	}

#endif

	return ret_val;

}

//注意本函数要求dirpath格式已经和谐，编码未和谐；因为用到了sd_strrchr，同时内部调用了pure_mkdir
_int32 recursive_mkdir(const char *dirpath)
{
#if defined(LINUX)
	_int32 ret_val = TRUE;
	char *ppos = NULL;

	char formatted_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_filepath_len = MAX_LONG_FULL_PATH_BUFFER_LEN;

	ppos = sd_strrchr(dirpath, DIR_SPLIT_CHAR);
	if (!ppos)
		return BAD_DIR_PATH;

	sd_memset(formatted_filepath, 0x00, formatted_filepath_len);
	ret_val = sd_conv_path(dirpath, sd_strlen(dirpath), formatted_filepath, &formatted_filepath_len);
	CHECK_VALUE(ret_val);

	*ppos = 0;

	ret_val = mkdir(formatted_filepath, DEFAULT_DIR_MODE);

	if (ret_val < 0)
	{
		ret_val = errno;
		if (ret_val == ENOENT)
		{
			ret_val = recursive_mkdir(dirpath);
			if (ret_val == SUCCESS)
			{
				ret_val = mkdir(formatted_filepath, DEFAULT_DIR_MODE);

				if (ret_val < 0)
				{
					ret_val = errno;
					if (ret_val == EEXIST)
						ret_val = SUCCESS;
				}
			}
		}
	}
	*ppos = DIR_SPLIT_CHAR;

	return ret_val;

#elif defined(WINCE)
	_int32 ret_val = SUCCESS;
	char *ppos = NULL;

	ppos = (char*)sd_strrchr(dirpath, DIR_SPLIT_CHAR);
	if(!ppos)
		return BAD_DIR_PATH;

	*ppos = 0;

	ret_val = pure_mkdir(dirpath);
	if(SUCCESS != ret_val )
	{
		if(ret_val == ERROR_PATH_NOT_FOUND) //如果是路径不存在
		{
			ret_val = recursive_mkdir(dirpath);
			if(ret_val == SUCCESS)
			{
				ret_val = pure_mkdir(dirpath);
			}
		}
	}
	else
	{
		ret_val = SUCCESS;
	}
	*ppos = DIR_SPLIT_CHAR;

	return ret_val;

#endif

}

_int32 sd_mkdir(const char *dirpath)
{

	/* 和谐一下格式，但是不能转码，因为recursive_mkdir递归内部无和谐且需要gbk编码*/
	_int32 ret_val = SUCCESS;
	char formatted_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_filepath_len = 0;
	char tmp_path[MAX_LONG_FULL_PATH_BUFFER_LEN];

#if defined(LINUX)
	char converted_path[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 converted_path_len = MAX_LONG_FULL_PATH_BUFFER_LEN;
#endif

	if (is_available_ci(CI_FS_IDX_MAKE_DIR))
		return ((et_fs_makedir)ci_ptr(CI_FS_IDX_MAKE_DIR))(dirpath);

	sd_memset(formatted_filepath, 0, MAX_LONG_FULL_PATH_BUFFER_LEN);
	ret_val = sd_format_dirpath(dirpath, formatted_filepath, MAX_LONG_FULL_PATH_BUFFER_LEN, &formatted_filepath_len);
	CHECK_VALUE(ret_val);

	dirpath = formatted_filepath;

#if defined(LINUX)
	sd_memset(converted_path, 0x00, converted_path_len);
	ret_val = sd_conv_path(dirpath, sd_strlen(dirpath), converted_path, &converted_path_len);
	CHECK_VALUE(ret_val);

	ret_val = mkdir(converted_path, DEFAULT_DIR_MODE);

	if (ret_val < 0)
	{
		ret_val = errno;

		if (ret_val == ENOENT)
		{
			sd_strncpy(tmp_path, dirpath, MAX_LONG_FULL_PATH_BUFFER_LEN);
			ret_val = recursive_mkdir(tmp_path);
			if (ret_val == SUCCESS)
			{
				ret_val = mkdir(converted_path, DEFAULT_DIR_MODE);
				if (ret_val < 0)
				{
					ret_val = errno;
					if (sd_dir_exist(converted_path))
						ret_val = SUCCESS;
					else
						return BAD_DIR_PATH;
				}
			}
		}
		else if (ret_val == EEXIST)
		{
			if (sd_dir_exist(converted_path))
				ret_val = SUCCESS;
			else
				return BAD_DIR_PATH;
		}
	}

#elif defined(WINCE)
	ret_val = pure_mkdir(dirpath);
	if(SUCCESS != ret_val )
	{
		if(ret_val == ERROR_PATH_NOT_FOUND) //如果是路径不存在
		{
			sd_strncpy(tmp_path, dirpath, MAX_LONG_FULL_PATH_BUFFER_LEN);
			ret_val = recursive_mkdir(tmp_path);
			if(ret_val == SUCCESS)
			{
				ret_val = pure_mkdir(dirpath);
			}
		}
	}

#endif

	return ret_val;
}

_int32 sd_rmdir(const char *dirpath)
{

	/* 和谐一下分隔符格式及编码方式 */
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_dirpath_len = 0;
	char formatted_conv_dirpath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_dirpath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

	if (is_available_ci(CI_FS_IDX_RM_DIR))
		return ((et_fs_rmdir)ci_ptr(CI_FS_IDX_RM_DIR))(dirpath);

	sd_memset(formatted_conv_dirpath, 0, formatted_conv_dirpath_buflen);
	ret_val = sd_format_conv_dirpath(dirpath, formatted_conv_dirpath, formatted_conv_dirpath_buflen, &formatted_conv_dirpath_len);
	CHECK_VALUE(ret_val);

	dirpath = formatted_conv_dirpath;

#if defined(LINUX) 
	ret_val = rmdir(dirpath);

#elif defined(WINCE)
	{
		ret_val = RemoveDirectory((LPCWSTR)dirpath);
		if(ret_val != TRUE)
		{
			ret_val = GetLastError();
		}
		else
		{
			ret_val = SUCCESS;
		}
	}

#endif

	return ret_val;
}

#if defined(LINUX)
_int32 recursive_rmdir(char *dirpath, _u32 path_len, _u32 buf_len)
{
	_int32 ret_val = SUCCESS;
	_u32 new_path_len = path_len;

	DIR* p_dir = opendir(dirpath);
	struct dirent* p_dirent_item = NULL;

	if (NULL == p_dir)
	{
		LOG_ERROR("recursive_rmdir, open dir fail: %s", dirpath);
		return 1;
	}
	// 删除当前目录里面的所有文件
	while ((p_dirent_item = readdir(p_dir)) != NULL)
	{
		_u32 fname_len = 0;
		BOOL is_dir = FALSE;

		//过滤“.”、“..”文件
		if (p_dirent_item->d_name[0] == '.' &&
			(p_dirent_item->d_name[1] == '\0' || (p_dirent_item->d_name[1] == '.' && p_dirent_item->d_name[2] == '\0')))
		{
			continue;
		}

#ifdef _DIRENT_HAVE_D_NAMLEN
		fname_len = p_dirent_item->d_namlen;
#else
		fname_len = sd_strlen(p_dirent_item->d_name);
#endif
		//长度是否可接受
		if (fname_len > buf_len - path_len - 1)
		{
			//closedir(p_dir);
			ret_val = 2;// 超出buf长度，无法删除
			break;
		}
		new_path_len = path_len + fname_len;
#if defined(_ANDROID_LINUX)
		// 连接utf8串
		sd_memcpy(dirpath + path_len, p_dirent_item->d_name, fname_len);
		dirpath[new_path_len] = '\0';
#else // Linux   可能需要先把结果转码一下
		sd_memcpy(dirpath + path_len, p_dirent_item->d_name, fname_len);
		dirpath[new_path_len] = '\0';
#endif

		// 判断是否文件夹
#ifdef _DIRENT_HAVE_D_TYPE
		is_dir = (p_dirent_item->d_type == DT_DIR) ? (TRUE): (FALSE);
#else

		//查询stat
		struct stat stat_buf;

		sd_memset(&stat_buf, 0, sizeof (stat_buf));
		ret_val = lstat(dirpath, &stat_buf);
		if (ret_val != SUCCESS)
		{
			LOG_ERROR("recursive_rmdir, lstat file fail: %s", p_dirent_item->d_name);
			//closedir(p_dir);
			ret_val = 2;// 无法判断是文件还是路径，无法删除
			break;
		}
		is_dir = S_ISDIR(stat_buf.st_mode);
#endif
		// 删除当前项
		if (!is_dir)
		{
			if (unlink(dirpath) != 0)
			{
				//closedir(p_dir);
				ret_val = 2;// 删除文件失败
				break;
			}
		}
		else
		{
			dirpath[new_path_len++] = DIR_SPLIT_CHAR;
			dirpath[new_path_len] = '\0';
			ret_val = recursive_rmdir(dirpath, new_path_len, buf_len);
			if (ret_val != SUCCESS)
			{
				//closedir(p_dir);
				ret_val = ret_val + 1;// 删除第ret_val层目录失败
				break;
			}
		}
	}

	closedir(p_dir);

	// 删除当前目录
	if (SUCCESS == ret_val)
	{
		dirpath[path_len] = '\0';
		ret_val = rmdir(dirpath);
		if (ret_val != 0)
			ret_val = 1;// 删除当前目录失败返回1
	}
	return ret_val;

}
#endif

_int32 sd_recursive_rmdir(const char *dirpath)
{
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_path_len = 0;
	char formatted_conv_path[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_path_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;


	sd_memset(formatted_conv_path, 0, formatted_conv_path_buflen);
	ret_val = sd_format_conv_dirpath(dirpath, formatted_conv_path, formatted_conv_path_buflen, &formatted_conv_path_len);
	CHECK_VALUE(ret_val);

#if defined(WINCE)
	{
		SHFILEOPSTRUCT fileop;

		fileop.hwnd = NULL;  
		fileop.wFunc = FO_DELETE;
		fileop.pFrom = (LPCWSTR)formatted_conv_path;    
		fileop.pTo = NULL;
		fileop.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;
		ret_val = SHFileOperation(&fileop) ;
		if ( ret_val != SUCCESS )
		{
			ret_val = GetLastError();
		}
	}
#elif defined(LINUX)
	return recursive_rmdir(formatted_conv_path, formatted_conv_path_len, formatted_conv_path_buflen);
#endif

	return ret_val;
}

static inline BOOL _is_cur_or_parent_dir(const char* name)
{
	return (*name == '.' && name[1] == 0) || (*(_int16*)name == 0x2E2E && name[2] == 0);
}

static _int32 _sd_recursive_rm_empty_dir_impl(const char* dirpath){
#ifdef LINUX
	_int32 ret_val = 0; // return 0 if success
	BOOL is_empty = TRUE;
	struct dirent *p_dirent;
	DIR* p_dir = opendir(dirpath);
	if (!p_dir) return errno;

	while ((p_dirent = readdir(p_dir)))
	{
		if (p_dirent->d_type == DT_DIR)
		{
			if (!_is_cur_or_parent_dir(p_dirent->d_name))
			{
				chdir(dirpath);
				ret_val = _sd_recursive_rm_empty_dir_impl(p_dirent->d_name);
				if (ret_val != 0) is_empty = FALSE;
				chdir("..");
			}
		}
		else is_empty = FALSE;
	}

	closedir(p_dir);
	if (is_empty) return 0 == rmdir(dirpath) ? 0 : errno;
	return 0 == ret_val ? -1 : ret_val;
#else
	return SUCCESS;
#endif
}

//remove all empty directory recursively
_int32 sd_recursive_rm_empty_dir(const char* dirpath)
{
#ifdef LINUX
	_int32 ret_val = SUCCESS;
	char cur_dir[MAX_FILE_PATH_LEN];
	if (NULL == getcwd(cur_dir, MAX_FILE_PATH_LEN))
	{
		sd_assert(FALSE);
		return errno;
	}
	ret_val = _sd_recursive_rm_empty_dir_impl(dirpath);
	chdir(cur_dir);
	return ret_val;
#else
	return SUCCESS;
#endif
}

static long _sd_get_total_number_of_files_in_dir_impl(const char* dirpath)
{
#ifdef LINUX
	long file_num = 0;
	struct dirent* p_dirent;
	DIR* p_dir = opendir(dirpath);
	if (!p_dir) return 0;
	while ((p_dirent = readdir(p_dir)))
	{
		if (p_dirent->d_type == DT_DIR)
		{
			if (!_is_cur_or_parent_dir(p_dirent->d_name))
			{
				chdir(dirpath);
				file_num += _sd_get_total_number_of_files_in_dir_impl(p_dirent->d_name);
				chdir("..");
			}
		}
		else file_num++;
	}
	closedir(p_dir);
	return file_num;
#else
	return 0;
#endif
}

//计算dir_path内除目录之外的文件总个数
long sd_get_total_number_of_files_in_dir(const char* dirpath)
{
#ifdef LINUX
	long file_num = 0;
	char cur_dir[MAX_FILE_PATH_LEN];
	if (NULL == getcwd(cur_dir, MAX_FILE_PATH_LEN))
	{
		sd_assert(FALSE);
		return -1;
	}
	file_num = _sd_get_total_number_of_files_in_dir_impl(dirpath);
	chdir(cur_dir);
	return file_num;
#else
	return SUCCESS;
#endif
}

/* 获取子目录/文件的个数与名称。
* 如果sub_files为NULL，或sub_files_size=0，仅计算子目录的个数。
*/
_int32 sd_get_sub_files(const char *dirpath, FILE_ATTRIB sub_files[], _u32 sub_files_size, _u32 *p_sub_files_count)
{
	_int32 ret_val = SUCCESS;
	BOOL is_need_detail_info = TRUE;
	_u32 formatted_dirpath_len = 0;
	_u32 file_attrib_name_size = MAX_LONG_FILE_NAME_BUFFER_LEN;
	char formatted_dirpath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_dirpath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

#if defined(WINCE)
	char unicode_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 unicode_len = MAX_LONG_FULL_PATH_BUFFER_LEN;

	WIN32_FIND_DATA fd;
	HANDLE fileHand;
	_u32 sum = 0;
	BOOL is_dir_exist = FALSE;
#endif

#if defined(LINUX)
	DIR* p_dir = NULL;
	struct dirent* p_dirent_item = NULL;
	_u32 sum = 0;
#endif


	if (NULL == sub_files || 0 == sub_files_size)
	{
		is_need_detail_info = FALSE;
	}

	if (sub_files != NULL)
	{
		sd_memset(sub_files, 0, sizeof(FILE_ATTRIB)* sub_files_size);
	}

#if defined(WINCE) 
	/* 和谐一下分隔符格式，但是不处理编码，因为WIN32使用了sd_strcat */
	sd_memset(formatted_dirpath, 0, formatted_dirpath_buflen);
	ret_val = sd_format_dirpath(dirpath, formatted_dirpath, formatted_dirpath_buflen, &formatted_dirpath_len);
	CHECK_VALUE(ret_val);
#else
	/* 和谐一下分隔符格式及编码*/
	sd_memset(formatted_dirpath, 0, formatted_dirpath_buflen);
	ret_val = sd_format_conv_dirpath(dirpath, formatted_dirpath, formatted_dirpath_buflen, &formatted_dirpath_len);
	CHECK_VALUE(ret_val);
#endif

#if defined(WINCE)

	// 需要判断是否空目录的情况，与其他平台保持一致
	ret_val = sd_conv_path(formatted_dirpath, formatted_dirpath_len, unicode_filepath, &unicode_len);
	CHECK_VALUE(ret_val);
	ret_val = GetFileAttributes((LPCWSTR)unicode_filepath);
	if(ret_val != (_u32)-1 && (ret_val & FILE_ATTRIBUTE_DIRECTORY)!=0 )
	{
		is_dir_exist = TRUE;
	}
	else
	{
		return -1;
	}

	// 查找目录下任意文件和目录
	if(formatted_dirpath_len > formatted_dirpath_buflen-2)
		return -1;
	sd_strcat(formatted_dirpath, "*", sd_strlen("*"));

	sd_memset(unicode_filepath, 0x00, MAX_LONG_FULL_PATH_BUFFER_LEN);
	unicode_len = MAX_LONG_FULL_PATH_BUFFER_LEN;
	ret_val = sd_conv_path(formatted_dirpath, sd_strlen(formatted_dirpath), unicode_filepath, &unicode_len);
	CHECK_VALUE(ret_val);

	fileHand = FindFirstFile((LPCWSTR)unicode_filepath, &fd );
	if(fileHand == INVALID_HANDLE_VALUE)
	{
		// 如果是存在的空目录，返回SUCCESS
		if (is_dir_exist)
		{
			*p_sub_files_count = 0;
			return SUCCESS;
		}
		ret_val = GetLastError();
		return (ret_val != 0) ? ret_val : -1;
	}
	do
	{
		//过滤“.”、“..”文件
		if( fd.cFileName[0] == '.' &&
			(fd.cFileName[1] == '\0'|| (fd.cFileName[1] == '.' && fd.cFileName[2] == '\0')) )
		{
			continue;
		}
		//如果文件数大于空间数，返回错误
		if (sum >= sub_files_size && is_need_detail_info)
		{
			FindClose(fileHand);
			return BUFFER_OVERFLOW;
		}		
		if (is_need_detail_info) //是否只计算文件个数
		{
			// unicode -> gbk
			_u32 in_len = 0, out_len = formatted_dirpath_buflen;
			while (fd.cFileName[in_len] != '\0')
			{
				in_len ++;
			}

			ret_val = sd_unicode_2_gbk((const _u16 *)fd.cFileName, in_len, sub_files[sum]._name, &out_len);
			CHECK_VALUE(ret_val);
			if (ret_val != SUCCESS)
			{
				// 如果转码失败，原样返回
				out_len = (in_len*2<formatted_dirpath_buflen-1) ? in_len*2 : (formatted_dirpath_buflen-1);
				sd_memcpy(sub_files[sum]._name, (const _u8 *)fd.cFileName, out_len);
			}
			if (out_len >= formatted_dirpath_buflen)
			{
				FindClose(fileHand);
				return BUFFER_OVERFLOW;
			}
			sub_files[sum]._name[out_len] = 0;

			// 判断是否是文件夹
			sub_files[sum]._is_dir = ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )? TRUE: FALSE;
		}
		sum++;
	} while( FindNextFile( fileHand, &fd ) );
	*p_sub_files_count = sum;
	FindClose(fileHand);
	ret_val = SUCCESS;
#elif defined(LINUX)

	p_dir = opendir(formatted_dirpath);
	if (NULL == p_dir)
	{
		LOG_ERROR("sd_get_sub_files, open dir fail: %s", formatted_dirpath);
		return -1;
	}

	while ((p_dirent_item = readdir(p_dir)) != NULL)
	{
		//过滤“.”、“..”文件
		LOG_ERROR("sd_get_sub_files 1:d_name=%s", p_dirent_item->d_name);
		if (p_dirent_item->d_name[0] == '.' &&
			(p_dirent_item->d_name[1] == '\0' || (p_dirent_item->d_name[1] == '.' && p_dirent_item->d_name[2] == '\0')))
		{
			continue;
		}
		LOG_ERROR("sd_get_sub_files 2: sum=%u,sub_files_size=%u, is_need_detail_info=%d", sum, sub_files_size, is_need_detail_info);
		//如果文件数大于空间数，返回错误
		if (sum >= sub_files_size && is_need_detail_info)
		{
			ret_val = BUFFER_OVERFLOW;
			break;
		}
		if (is_need_detail_info) //是否只计算文件个数
		{
			_u32 in_len = 0, out_len = file_attrib_name_size;

#ifdef _DIRENT_HAVE_D_NAMLEN
			in_len = p_dirent_item->d_namlen;
#else
			in_len = sd_strlen(p_dirent_item->d_name);
#endif

			// 判断是否文件夹
#ifdef _DIRENT_HAVE_D_TYPE
			// 一些系统的d_type一直为DT_UNKNOWN
			if (p_dirent_item->d_type != DT_UNKNOWN)
			{ 
				sub_files[sum]._is_dir = (p_dirent_item->d_type == DT_DIR) ? (TRUE): (FALSE);
				sub_files[sum]._is_lnk = p_dirent_item->d_type == DT_LNK;
			}
			else
			{
#endif
				// 查询stat
				LOG_ERROR("sd_get_sub_files 4:in_len=%u,formatted_dirpath_buflen=%u,formatted_dirpath_len=%u", in_len, formatted_dirpath_buflen, formatted_dirpath_len);
				if (in_len < formatted_dirpath_buflen - formatted_dirpath_len - 1)
				{
					// 如果并且长度可接受
					struct stat stat_buf;

					// 连接utf8串
					sd_memcpy(formatted_dirpath + formatted_dirpath_len, p_dirent_item->d_name, in_len);
					formatted_dirpath[formatted_dirpath_len + in_len] = '\0';

					sd_memset(&stat_buf, 0, sizeof (stat_buf));
					ret_val = lstat(formatted_dirpath, &stat_buf);
					LOG_ERROR("sd_get_sub_files 5:ret_val=%d,st_mode=0x%X", ret_val, stat_buf.st_mode);
					if (ret_val != SUCCESS)
					{
						//lstat失败，默认非文件夹,已memset
						LOG_ERROR("sd_get_sub_files 6, lstat file fail: %s", p_dirent_item->d_name);
					}
					else
					{
						sub_files[sum]._is_dir = S_ISDIR(stat_buf.st_mode);
						sub_files[sum]._is_lnk = S_ISLNK(stat_buf.st_mode); //bool value
						LOG_ERROR("sd_get_sub_files 7:is_dir=%d, is_lnk=%d", sub_files[sum]._is_dir, sub_files[sum]._is_lnk);
					}
				}
				else
				{
					//文件名太长，不检查类型，默认非文件夹,已memset
					LOG_ERROR("sd_get_sub_files 8:FALSE");
				}
#ifdef _DIRENT_HAVE_D_TYPE
			}
#endif

			// anycode -> gbk 给上层返回gbk编码
#if defined(MACOS)&&defined(MOBILE_PHONE)
			ret_val = -1;
#else
			//			ret_val = sd_any_format_to_gbk(p_dirent_item->d_name, in_len, (_u8*)(sub_files[sum]._name), &out_len);
#endif
			// 转码失败的处理，原样返回 or 忽略
			out_len = (in_len < file_attrib_name_size - 1) ? in_len : (file_attrib_name_size - 1);
			sd_memcpy(sub_files[sum]._name, (_u8 *)p_dirent_item->d_name, out_len);
			LOG_ERROR("sd_get_sub_files 3:out_len=%u,file_attrib_name_size=%u", out_len, file_attrib_name_size);
			if (out_len >= file_attrib_name_size)
			{
				ret_val = BUFFER_OVERFLOW;
				break;
			}
			//已memset  sub_files[sum]._name[out_len] = 0;

		}
		sum++;
		ret_val = SUCCESS;
	}// end while 
	*p_sub_files_count = sum;
	closedir(p_dir);

#endif
	LOG_ERROR("sd_get_sub_files end! ret_val(%d) count(%d)", ret_val, *p_sub_files_count);
	return ret_val;
}

/* 获取目录内所有子文件(不进入子文件夹)的总大小
 */
_int32 sd_get_sub_files_total_size(const char *dirpath, _u64 * total_size)
{
	_int32 ret_val = SUCCESS;
	BOOL is_need_detail_info = TRUE;
	_u32 formatted_dirpath_len = 0;
	char formatted_dirpath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_dirpath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

#if defined(WINCE)
	char unicode_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 unicode_len = MAX_LONG_FULL_PATH_BUFFER_LEN;

	WIN32_FIND_DATA fd;
	HANDLE fileHand;
	_u32 sum = 0;
	BOOL is_dir_exist = FALSE;
#endif

#if defined(LINUX)
	DIR* p_dir = NULL;
	struct dirent* p_dirent_item = NULL;
	_u32 sum = 0;
#endif

	//	*total_size = 0;

#if defined(WINCE) 
	/* 和谐一下分隔符格式，但是不处理编码，因为WIN32使用了sd_strcat */
	sd_memset(formatted_dirpath, 0, formatted_dirpath_buflen);
	ret_val = sd_format_dirpath(dirpath, formatted_dirpath, formatted_dirpath_buflen, &formatted_dirpath_len);
	CHECK_VALUE(ret_val);
#else
	/* 和谐一下分隔符格式及编码*/
	sd_memset(formatted_dirpath, 0, formatted_dirpath_buflen);
	ret_val = sd_format_conv_dirpath(dirpath, formatted_dirpath, formatted_dirpath_buflen, &formatted_dirpath_len);
	CHECK_VALUE(ret_val);
#endif

#if defined(WINCE)

	// 需要判断是否空目录的情况，与其他平台保持一致
	ret_val = sd_conv_path(formatted_dirpath, formatted_dirpath_len, unicode_filepath, &unicode_len);
	CHECK_VALUE(ret_val);
	ret_val = GetFileAttributes((LPCWSTR)unicode_filepath);
	if(ret_val != (_u32)-1 && (ret_val & FILE_ATTRIBUTE_DIRECTORY)!=0 )
	{
		is_dir_exist = TRUE;
	}
	else
	{
		return -1;
	}

	// 查找目录下任意文件和目录
	if(formatted_dirpath_len > formatted_dirpath_buflen-2)
		return -1;
	sd_strcat(formatted_dirpath, "*", sd_strlen("*"));

	sd_memset(unicode_filepath, 0x00, MAX_LONG_FULL_PATH_BUFFER_LEN);
	unicode_len = MAX_LONG_FULL_PATH_BUFFER_LEN;
	ret_val = sd_conv_path(formatted_dirpath, sd_strlen(formatted_dirpath), unicode_filepath, &unicode_len);
	CHECK_VALUE(ret_val);

	fileHand = FindFirstFile((LPCWSTR)unicode_filepath, &fd );
	if(fileHand == INVALID_HANDLE_VALUE)
	{
		// 如果是存在的空目录，返回SUCCESS
		if (is_dir_exist)
		{
			*total_size = 0;
			return SUCCESS;
		}
		ret_val = GetLastError();
		return (ret_val != 0) ? ret_val : -1;
	}
	do
	{
		//过滤“.”、“..”文件
		if( fd.cFileName[0] == '.' &&
			(fd.cFileName[1] == '\0'|| (fd.cFileName[1] == '.' && fd.cFileName[2] == '\0')) )
		{
			continue;
		}
		//如果文件数大于空间数，返回错误
		if (is_need_detail_info) //是否只计算文件个数
		{
			*total_size+=(_u64)(fd.nFileSizeHigh)<<32|(_u64)(fd.nFileSizeLow);
		}
		sum++;
	} while( FindNextFile( fileHand, &fd ) );
	FindClose(fileHand);
	ret_val = SUCCESS;
#elif defined(LINUX)
	p_dir = opendir(formatted_dirpath);
	if (NULL == p_dir)
	{
		LOG_ERROR("sd_get_sub_files, open dir fail: %s", formatted_dirpath);
		return -1;
	}

	while ((p_dirent_item = readdir(p_dir)) != NULL)
	{
		//过滤“.”、“..”文件
		if (p_dirent_item->d_name[0] == '.' &&
			(p_dirent_item->d_name[1] == '\0' || (p_dirent_item->d_name[1] == '.' && p_dirent_item->d_name[2] == '\0')))
		{
			continue;
		}
		if (is_need_detail_info) //是否只计算文件个数
		{
			_u32 in_len = 0;

#ifdef _DIRENT_HAVE_D_NAMLEN
			in_len = p_dirent_item->d_namlen;
#else
			in_len = sd_strlen(p_dirent_item->d_name);
#endif
			// 查询stat
			if (ret_val == SUCCESS && in_len < formatted_dirpath_buflen - formatted_dirpath_len - 1)
			{
				// 如果长度可接受
				struct stat stat_buf;
#if defined(_ANDROID_LINUX)
				// 连接utf8串
				sd_memcpy(formatted_dirpath + formatted_dirpath_len, p_dirent_item->d_name, in_len);
				formatted_dirpath[formatted_dirpath_len + in_len] = '\0';
#else // Linux
				sd_memcpy(formatted_dirpath + formatted_dirpath_len, p_dirent_item->d_name, in_len);
				formatted_dirpath[formatted_dirpath_len + in_len] = '\0';
#endif
				/* 处理子目录文件 */
				if (DT_DIR == p_dirent_item->d_type)
				{
					sd_get_sub_files_total_size(formatted_dirpath, total_size);
				}
				sd_memset(&stat_buf, 0, sizeof (stat_buf));
				ret_val = lstat(formatted_dirpath, &stat_buf);
				if (ret_val != SUCCESS)
				{
					LOG_ERROR("sd_get_sub_files, lstat file fail: %s", p_dirent_item->d_name);
					//return ret_val;  
				}
				else
				{
					*total_size += stat_buf.st_size;
					LOG_DEBUG("filename = %s,filesize = %d,total_size = %llu", p_dirent_item->d_name, stat_buf.st_size, *total_size);
				}
			}
			else
			{
				// 文件名太长，不处理
			}
		}
		sum++;
		ret_val = SUCCESS;
	}// end while 
	closedir(p_dir);
#else
	ret_val = NOT_IMPLEMENT;
#endif

	return ret_val;
}

_int32 sd_get_file_size_and_modified_time(const char * filepath, _u64 * p_file_size, _u32 * p_last_modified_time)
{

	/* 和谐一下分隔符格式，但是不处理编码方式 */
	_int32 ret_val = SUCCESS;
	char formatted_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_filepath_len = 0;
#if defined(LINUX) 
	struct stat stat_buf;
	char converted_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 converted_filepath_len = MAX_LONG_FULL_PATH_BUFFER_LEN;
#elif defined(WINCE)
	_int32 file_id;
	BY_HANDLE_FILE_INFORMATION  FileInformation;
#endif

	if (is_available_ci(CI_FS_IDX_GET_FILESIZE_AND_MODIFYTIME))
		return ((et_fs_get_file_size_and_modified_time)ci_ptr(CI_FS_IDX_GET_FILESIZE_AND_MODIFYTIME))(filepath, p_file_size, p_last_modified_time);

	sd_memset(formatted_filepath, 0, MAX_LONG_FULL_PATH_BUFFER_LEN);
	ret_val = sd_format_filepath(filepath, formatted_filepath, MAX_LONG_FULL_PATH_BUFFER_LEN, &formatted_filepath_len);
	CHECK_VALUE(ret_val);

	filepath = formatted_filepath;
#if defined(LINUX)
	sd_memset(&stat_buf, 0, sizeof(stat_buf));
	sd_memset(converted_filepath, 0x00, converted_filepath_len);
	ret_val = sd_conv_path(filepath, sd_strlen(filepath), converted_filepath, &converted_filepath_len);
	CHECK_VALUE(ret_val);
	filepath = (char *)converted_filepath;

	/*获取文件大小和最后修改时间*/
	ret_val = stat(filepath, &stat_buf);

	// 如果获取成功
	if (ret_val == 0)
	{
		/*文件大小*/
		if (p_file_size != NULL)
			*p_file_size = stat_buf.st_size;
		/*文件最后一次更改时间*/
		if (p_last_modified_time != NULL)
			*p_last_modified_time = stat_buf.st_mtime;
		return SUCCESS;
	}
	return -1;
#elif defined(WINCE)

	sd_memset(&FileInformation,0,sizeof(FileInformation));

	ret_val = sd_open_ex(filepath,  O_FS_RDONLY , &file_id);
	if(SUCCESS != ret_val)
		return ret_val;
	/*获取文件大小和最后修改时间*/
	ret_val = GetFileInformationByHandle( (HANDLE)file_id, &FileInformation);
	sd_close_ex(file_id);
	// 如果获取成功
	if(ret_val==TRUE)
	{
		/*文件大小*/
		if(p_file_size!=NULL)
		{
			*p_file_size = (_u64)(FileInformation.nFileSizeHigh)<<32|(_u64)(FileInformation.nFileSizeLow);
		}

		/*文件最后一次更改时间*/
		if(p_last_modified_time!=NULL)
		{
			////*p_last_modified_time = stat_buf.st_mtim.tv_sec;
			//*p_last_modified_time = FileInformation.ftLastWriteTime.dwLowDateTime;
			ULARGE_INTEGER	uli;
			ULARGE_INTEGER	uli1970;
			FILETIME		ft1970;
			const SYSTEMTIME st1970 = {1970, 1,	4, 1, 0, 0, 0, 0};

			// convert to a FILETIME
			// Gives number of 100-nanosecond intervals since January 1, 1970 (UTC)
			//
			if(!SystemTimeToFileTime(&st1970, &ft1970))
			{
				return -1;
			}

			// Copy file time structures into ularge integer so we can do
			// the math more easily
			//
			memcpy(&uli, &FileInformation.ftLastWriteTime, sizeof(uli));
			memcpy(&uli1970, &ft1970, sizeof(uli1970));

			// Subtract the 1970 number of 100 ns value from the 1601 100 ns value
			// so we can get the number of 100 ns value between 1970 and now
			// then devide be 10,000,000 to get the number of seconds since 1970
			//
			uli.QuadPart = ((uli.QuadPart - uli1970.QuadPart) / 10000000);

			*p_last_modified_time = (_u32)uli.QuadPart;
		}
		return SUCCESS;
	}
	ret_val = GetLastError();
	return ret_val;
#endif
}

_int32 sd_is_support_create_big_file(const char* path, BOOL* result)
{
	/* 和谐一下分隔符格式及编码方式 */
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

#if defined(LINUX)
	struct statfs sf;
#endif

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(path, formatted_conv_filepath
		, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);
	path = formatted_conv_filepath;
	*result = TRUE;

#if defined(LINUX)

	if (statfs(path, &sf) < 0)
	{
		ret_val = errno;
	}
	else
	{
		LOG_DEBUG("statfs path = %s, f_type = %d", path, sf.f_type);
		if (sf.f_type == MSDOS_SUPER_MAGIC)
		{
			*result = FALSE;
		}
	}

#elif defined(WINCE)
	{
		*result = FALSE;
	}
#endif

	return ret_val;
}

_int32 sd_get_free_disk(const char *path, _u64 *free_size)
{
	/* 和谐一下分隔符格式及编码方式 */
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

#if defined(LINUX)
	struct statfs sf;
#endif

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(path, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);
	path = formatted_conv_filepath;

#if defined(LINUX)

	*free_size = 0;

	if (statfs(path, &sf) < 0)
	{
		ret_val = errno;
		LOG_DEBUG("sd_get_free_disk, path:%s, statfs fail errno = %d", path, ret_val);
	}
	else
	{
		if (sf.f_bsize >= 1024)
			sf.f_bsize /= 1024;
		else if (sf.f_bavail >= 1024)
			sf.f_bavail /= 1024;
		else
		{
			*free_size = sf.f_bsize * sf.f_bavail / 1024;
			return ret_val;
		}
		LOG_DEBUG("sd_get_free_disk, statfs path = %s, f_type = %d", path, sf.f_type);
		*free_size = sf.f_bsize * sf.f_bavail;
		LOG_DEBUG("sd_get_disk_space:free_size = %u", *free_size);
		#if  defined(_ANDROID_LINUX)
			if(*free_size == 0)
			{
				if(sd_android_utility_is_xiaomi())
				{
					get_android_available_space(free_size);
					LOG_DEBUG("sd_get_disk_space: get_android_available_space--free_size = %u", *free_size);
				}
			}
		#endif
	}

#elif defined(WINCE)
	{
		ULARGE_INTEGER nTotalBytes,nTotalFreeBytes,nTotalAvailableBytes;   

		LOG_ERROR("sd_get_free_disk_impl:%s", path);

		*free_size = 0;
		if( GetDiskFreeSpaceEx((LPCWSTR)path,&nTotalAvailableBytes,&nTotalBytes,&nTotalFreeBytes)==0){
			ret_val = GetLastError();
		}else{
			*free_size = nTotalAvailableBytes.LowPart/1024;
		}

		printf("******************* sd_get_free_disk : %llu\n", *free_size);
	}
#endif

	return ret_val;
}
/* get total disk-space about path
 *  unit of total_size : K (1024 bytes)
 */
_int32 sd_get_disk_space(const char *path, _u32 *total_size)
{

	/* 和谐一下分隔符格式及编码方式 */
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

#if defined(LINUX)
	struct statfs sf;
#endif

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(path, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);
	path = formatted_conv_filepath;

#if defined(LINUX)

	*total_size = 0;

	if (statfs(path, &sf) < 0)
		ret_val = errno;
	else
	{
		if (sf.f_bsize >= 1024)
			sf.f_bsize /= 1024;
		else if (sf.f_blocks >= 1024)
			sf.f_blocks /= 1024;
		else
		{
			*total_size = sf.f_bsize * sf.f_blocks / 1024;
			return ret_val;
		}

		*total_size = sf.f_bsize * sf.f_blocks;
	}

#elif defined(WINCE)
	{
		ULARGE_INTEGER nTotalBytes,nTotalFreeBytes,nTotalAvailableBytes;   

		LOG_ERROR("sd_get_disk_space:%s", path);

		*total_size = 0;
		if( GetDiskFreeSpaceEx((LPCWSTR)path,&nTotalAvailableBytes,&nTotalBytes,&nTotalFreeBytes)==0){
			ret_val = GetLastError();
		}else{
			*total_size = nTotalBytes.LowPart/1024;
		}

		printf("******************* sd_get_disk_space : %u\n", *total_size);
	}

#endif

	return ret_val;
}

BOOL sd_is_file_readable(const char * filepath)
{
	/* 和谐一下分隔符格式，但是不处理编码方式 */
	BOOL ret_val = TRUE;
	_int32 ret = 0;
	//andriod的lsata方法不可靠，统一使用sd_open_ex判断
	_int32 flag = O_FS_RDONLY;
	_u32 file_id = (_u32)NULL;
	ret = sd_open_ex(filepath, flag, &file_id);
	if (ret != SUCCESS)
	{
		ret_val = FALSE;
	}
	else
	{
		ret_val = (ret == SUCCESS);
		sd_close_ex(file_id);
	}

	return ret_val;
}
_int32 sd_test_path_writable(const char *path)
{
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0, cur_time = 0;
	char file_buffer[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_int32 path_len = sd_strlen(path);

	if (path == NULL || path_len == 0)
	{
		return -1;
	}
	sd_memset(file_buffer, 0, MAX_LONG_FULL_PATH_BUFFER_LEN);

	//sd_time_ms(&(cur_time));
	if (path[path_len] != '/')
	{
		sd_snprintf(file_buffer, MAX_LONG_FULL_PATH_BUFFER_LEN, "%s/etm_test_%u", path, cur_time);
	}
	else
	{
		sd_snprintf(file_buffer, MAX_LONG_FULL_PATH_BUFFER_LEN, "%setm_test_%u", path, cur_time);
	}

	ret_val = sd_open_ex(file_buffer, O_FS_CREATE, &file_id);
	if (ret_val != SUCCESS)
		return ret_val;

	sd_close_ex(file_id);
	sd_delete_file(file_buffer);
	return SUCCESS;
}

BOOL sd_is_file_name_valid(const char * filename)
{
	char reserve[] = "<>/\\|:\"*?\t\r\n";
	_int32 i = 0;

	if (filename == NULL || 0 == sd_strlen(filename))
		return FALSE;

	while (i < sizeof(reserve)-1)
	if (sd_strchr((char*)filename, reserve[i++], 0))
		return FALSE;


	return TRUE;
}

char* sd_filter_valid_file_name(const char * filename)
{
	char reserve[]="<>/\\|:\"*?\t\r\n";
	_int32 i = 0, j = 0;
	static char fixname[MAX_FILE_NAME_LEN] = {0};
	char *pos = NULL;
	if (filename == NULL || 0 == sd_strlen(filename)) 
		return NULL;
	else
		sd_strncpy(fixname, filename, MAX_FILE_NAME_LEN);
	while( fixname[i] != 0 )
	{
		j = 0;
		while( j < sizeof(reserve)-1 )
		{	
			if( fixname[i] == reserve[j] )
			{
				fixname[i] = '-';
				break;
			}
			j++;
		}
		i++;
	}
	return fixname;
}

_int32 sd_write_save_to_buffer(_u32 file_id, char *buffer, _u32 buffer_len, _u32 *buffer_pos, char * p_data, _u32 data_len)
{
	_int32 ret = SUCCESS;
	_u32 pos = *buffer_pos, write_size = 0;

	LOG_DEBUG("sd_write_save_to_buffer:buffer_len=%u,*buffer_pos=%u,data_len=%u", buffer_len, *buffer_pos, data_len);

	if (pos + data_len > buffer_len)
	{
		ret = sd_write(file_id, buffer, pos, &write_size);
		CHECK_VALUE(ret);
		sd_assert(pos == write_size);
		pos = 0;
	}

	if (data_len > buffer_len)
	{
		ret = sd_write(file_id, p_data, data_len, &write_size);
		CHECK_VALUE(ret);
		sd_assert(data_len == write_size);
	}
	else
	{
		sd_assert(pos + data_len <= buffer_len);
		sd_memcpy(buffer + pos, p_data, data_len);
		pos += data_len;
	}

	*buffer_pos = pos;

	return SUCCESS;
}

_int32 sd_format_filepath(const char *path, char *formated_path,
	_u32 formated_path_size, _u32 *p_formated_path_len)
{
	_int32 ret_val = SUCCESS;
	_u32 path_len = sd_strlen(path);
	_u32 i = 0, j = 0;

	if (path == NULL || formated_path == NULL
		|| path_len == 0 || path_len >= MAX_LONG_FULL_PATH_BUFFER_LEN || p_formated_path_len == 0)
	{
		return INVALID_ARGUMENT;
	}

	sd_memset(formated_path, 0, formated_path_size);
	for (i = 0; i < path_len; ++i)
	{
		if (formated_path_size < j + 1)
		{
			return BUFFER_OVERFLOW;
		}

		if (path[i] == '/' || path[i] == '\\')
		{
			formated_path[j] = DIR_SPLIT_CHAR;

			// 去掉重复的'/'
			while (i + 1 < path_len)
			{
				if (path[i + 1] == '/' || path[i + 1] == '\\')
				{
					i++;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			formated_path[j] = path[i];
		}

		j++;
	}

	*p_formated_path_len = j;


	return ret_val;
}

_int32 sd_format_dirpath(const char *path, char *formated_path,
	_u32 formated_path_size, _u32 *p_formated_path_len)
{
	_u32 format_len = 0;
	_int32 ret = sd_format_filepath(path, formated_path, formated_path_size, &format_len);
	if (ret != SUCCESS)
	{
		return ret;
	}

	if (format_len == 0)
	{
		return FILE_INVALID_PARA;
	}

	// 必须以 DIR_SPLIT_CHAR 结尾。
	if (formated_path[format_len - 1] != DIR_SPLIT_CHAR)
	{
		if (formated_path_size < format_len + 2)
		{
			return FILE_INVALID_PARA;
		}
		formated_path[format_len] = DIR_SPLIT_CHAR;
		formated_path[format_len + 1] = '\0';
		format_len++;
	}

	*p_formated_path_len = format_len;
	return SUCCESS;
}

// 对path进行转码，适应不同的平台
// 其中symbian  -> GBK; WINCE ->Unicode; Android ->UTF8
// 其他不处理
_int32 sd_conv_path(const char *p_input, _u32 input_len, char* p_output, _u32 *p_output_len)
{
	_int32 ret_val = SUCCESS;

#if defined(WINCE)
	/*WINCE ->Unicode */

	if (0 == *p_output_len)
	{
		return INVALID_ARGUMENT;
	}

	*p_output_len /= 2;
	// ret_val = sd_gbk_2_unicode(p_input, input_len, (_u16*)p_output, p_output_len);
	// 处理相对路径
	ret_val = conv_filepath_wince(p_input, input_len, (_u16*)p_output, p_output_len);
	*p_output_len *= 2;

#elif defined(_ANDROID_LINUX) ||defined(MACOS) || defined(_PC_LINUX)
	/*Android ->UTF8*/
	ret_val = sd_any_format_to_utf8(p_input, input_len, p_output, p_output_len);

#else
	/* 其他,保持GBK不变*/
	if (*p_output_len < input_len + 1)
	{
		return INVALID_MEMORY;
	}
	sd_memcpy(p_output, p_input, input_len);
	p_output[input_len] = '\0';
	*p_output_len = input_len;
#endif

	return ret_val;
}

_int32 sd_format_conv_filepath(const char *path, char *formated_conv_path,
	_u32 formated_conv_path_size, _u32 *p_formated_conv_path_len)
{
	_int32 ret = 0;
	_u32 path_len = sd_strlen(path);
	char formatted_path[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_path_len = 0;

	if (NULL == path || NULL == formated_conv_path
		|| 0 == path_len || path_len >= MAX_LONG_FULL_PATH_BUFFER_LEN || NULL == p_formated_conv_path_len)
	{
		return INVALID_ARGUMENT;
	}

	sd_memset(formatted_path, 0, MAX_LONG_FULL_PATH_BUFFER_LEN);
	//格式化路径分隔符
	ret = sd_format_filepath(path, formatted_path, MAX_LONG_FULL_PATH_BUFFER_LEN, &formatted_path_len);
	CHECK_VALUE(ret);

	// 按平台进行转码
	*p_formated_conv_path_len = formated_conv_path_size;
	ret = sd_conv_path(formatted_path, formatted_path_len, formated_conv_path, p_formated_conv_path_len);
	CHECK_VALUE(ret);

	return SUCCESS;
}

_int32 sd_format_conv_dirpath(const char *path, char *formated_conv_path,
	_u32 formated_conv_path_size, _u32 *p_formated_conv_path_len)
{
	_int32 ret = 0;
	_u32 path_len = sd_strlen(path);
	char formatted_path[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_path_len = 0;

	if (NULL == path || NULL == formated_conv_path
		|| 0 == path_len || path_len >= MAX_LONG_FULL_PATH_BUFFER_LEN || NULL == p_formated_conv_path_len)
	{
		return INVALID_ARGUMENT;
	}

	sd_memset(formatted_path, 0, MAX_LONG_FULL_PATH_BUFFER_LEN);
	//格式化路径分隔符
	ret = sd_format_dirpath(path, formatted_path, MAX_LONG_FULL_PATH_BUFFER_LEN, &formatted_path_len);
	CHECK_VALUE(ret);

	// 按平台进行转码
	*p_formated_conv_path_len = formated_conv_path_size;
	ret = sd_conv_path(formatted_path, formatted_path_len, formated_conv_path, p_formated_conv_path_len);
	CHECK_VALUE(ret);

	return SUCCESS;
}

_int32 sd_append_path(char *path, _u32 buff_len, const char *append)
{
	_int32 path_len = 0;
	path_len = sd_strlen(path);
	if (!path || !append || buff_len < path_len + sd_strlen(append)) return  BUFFER_OVERFLOW;

	if (path[path_len - 1] != DIR_SPLIT_CHAR)
	{
		sd_strcat(path, DIR_SPLIT_STRING, 1);
		path_len++;
	}
	sd_strcat(path, append, sd_strlen(append));
	return SUCCESS;
}

char format_dir_split_char(char c)
{
	if (c == '\\' || c == '/')
	{
		return DIR_SPLIT_CHAR;
	}
	else
	{
		return 0;
	}
}

/* 是否真实文件路径 */
BOOL is_realdir(const char *dir)
{
	sd_assert(dir != NULL);
	//目前仅仅Symbian平台下虚路径和实路径有区别	

	return TRUE;

}

BOOL sd_realpath(const char *path, char *resolved_path)
{
#ifdef LINUX_DEMO
	BOOL ret_val = FALSE;

	if( path != NULL && realpath( path, resolved_path ) != NULL )
	{
		ret_val = TRUE;
	}
	return ret_val;
#else
	return FALSE;
#endif

}


/* 文件虚路径转为实路径
* 在Symbian下，虚拟根目录'\\'特殊处理，不做转换
*/
_int32 vdir2realdir(const char *vdir, char *realdir, _u32 realdir_size, _u32 *p_realdir_len)
{
	_u32 vdirlen;

	if (NULL == vdir || NULL == realdir)
	{
		return INVALID_ARGUMENT;
	}
	vdirlen = sd_strlen(vdir);
	if (realdir_size < vdirlen + 1)
	{
		return INVALID_ARGUMENT;
	}

	//虚实路径相同,直接copy
	sd_memmove(realdir, vdir, vdirlen);
	realdir[vdirlen] = '\0';
	*p_realdir_len = vdirlen;

	return SUCCESS;
}

/* 文件实路径转为虚路径 */
_int32 realdir2vdir(const char *realdir, char *vdir, _u32 vdir_size, _u32 *p_vdir_len)
{
	_u32 rdirlen;

	if (NULL == vdir || NULL == realdir)
	{
		return INVALID_ARGUMENT;
	}
	rdirlen = sd_strlen(realdir);
	if (vdir_size < rdirlen + 1)
	{
		return INVALID_ARGUMENT;
	}

	//虚实路径相同,直接copy 
	sd_memmove(vdir, realdir, rdirlen);
	vdir[rdirlen] = '\0';
	*p_vdir_len = rdirlen;

	return SUCCESS;
}


#if defined(WINCE)
//获取UCS2字符串的长度
_int32 sd_strlen_ucs2(const _u16 *str)
{
	_int32 i = 0;
	if(str == NULL)
	{
		return 0;
	}

	while (str[i] != 0)
		i++;

	return i;
}

//在UCS2字符串中查找指定字符位置，从尾到头逆序查找
_u16* sd_strrchr_ucs2(_u16* dest, _u16 ch)
{
	_u32 i = 0;
	while (dest[i] != 0)
		i++;
	do
	if (dest[i] == ch)
		return (_u16 *) (dest + i);
	while (i-- != 0);
	return NULL;
}

_int32 conv_filepath_wince(const char* filepath, _u32 filepath_len, _u16* ucs2_filepath, _u32* ucs2_filepath_len)
{
	_int32 ret_val = SUCCESS;
	_u16* pslash = NULL;
	_int32 path_len = 0;

	// 判断是否相对路径
	if(  (filepath[1] == ':' && ( filepath[2] == DIR_SPLIT_CHAR)) || filepath[0] == DIR_SPLIT_CHAR)
	{
		// 绝对路径，转码到unicode_filepath
		ret_val = sd_any_format_to_unicode(filepath, filepath_len, ucs2_filepath, ucs2_filepath_len);
		CHECK_VALUE(ret_val);
	}
	else
	{

		// 相对路径，取工作路径到unicode_filepath
		GetModuleFileName(NULL, (LPWSTR)ucs2_filepath, MAX_LONG_FILE_NAME_BUFFER_LEN); 

		pslash = sd_strrchr_ucs2((_u16*)ucs2_filepath, (_u16)DIR_SPLIT_CHAR);
		if(pslash != NULL)
			*pslash = 0;
		path_len = sd_strlen_ucs2(ucs2_filepath);

		// 添加"\" 去".\", 并转码到unicode_filepath后面
		if(*(pslash-1) != DIR_SPLIT_CHAR && filepath[0] != DIR_SPLIT_CHAR)
		{
			*pslash = (_u16)DIR_SPLIT_CHAR;
			pslash += 1;
			path_len += 1;
		}
		if(filepath[0]=='.' && (filepath[1]==DIR_SPLIT_CHAR))
			filepath = filepath+2;
		*ucs2_filepath_len -= path_len;
		ret_val = sd_any_format_to_unicode(filepath, sd_strlen(filepath), pslash, ucs2_filepath_len);
		CHECK_VALUE(ret_val);
		*ucs2_filepath_len += path_len;
	}
	return SUCCESS;
}
#endif

_int32 sd_truncate(const char *filepath, _u64 length)
{
	_int32 ret_val = SUCCESS;
#if defined(LINUX)
	_u32 formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	/* 和谐一下分隔符格式及编码方式 */
	ret_val = sd_format_conv_filepath(filepath, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

	ret_val = truncate(formatted_conv_filepath, length);
#elif defined(WINCE)
	_u32 file_id = 0;
	ret_val = sd_open_ex(filepath, O_FS_RDWR, &file_id);
	CHECK_VALUE(ret_val);

	ret_val = sd_setfilepos( file_id, length);
	if(ret_val == SUCCESS)
	{
		ret_val = SetEndOfFile((HANDLE)file_id);
	}
	sd_close_ex(file_id);
	CHECK_VALUE(ret_val);
#else
	ret_val = NOT_IMPLEMENT;
#endif
	return ret_val;
}

#if defined(LINUX) 
_int32 sd_delete_dir_impl(char *formated_path, _u32 path_len)
{
	_int32 ret_val = SUCCESS;
	struct stat file_stat;
	DIR* p_dir = NULL;
	struct dirent* p_dirent_item = NULL;
	_u32 dir_len = 0, file_name_len = 0;

	dir_len = path_len;
	formated_path[path_len] = '\0';
	if ((lstat(formated_path, &file_stat) == -1)/* && errno == ENOENT*/)
	{
		LOG_ERROR("sd_delete_dir_impl, get lstat fail: %s", formated_path);
		ret_val = -1;
		//sd_assert(ret_val == SUCCESS);
	}
	else
	{
		if (S_ISDIR(file_stat.st_mode))
		{
			/* 目录 */
			p_dir = opendir(formated_path);
			if (NULL == p_dir)
			{
				LOG_ERROR("sd_delete_dir_impl, open dir fail: %s", formated_path);
				ret_val = -1;
				//sd_assert(ret_val == SUCCESS);
				return ret_val;
			}

			if (formated_path[path_len - 1] != DIR_SPLIT_CHAR)
			{
				formated_path[path_len] = DIR_SPLIT_CHAR;
				formated_path[path_len + 1] = '\0';
				dir_len++;
			}

			while ((p_dirent_item = readdir(p_dir)) != NULL)
			{
				//过滤“.”、“..”文件
				if (p_dirent_item->d_name[0] == '.' &&
					(p_dirent_item->d_name[1] == '\0' || (p_dirent_item->d_name[1] == '.' && p_dirent_item->d_name[2] == '\0')))
				{
					continue;
				}


				file_name_len = sd_strlen(p_dirent_item->d_name);
				sd_memcpy(formated_path + dir_len, p_dirent_item->d_name, file_name_len);
				formated_path[dir_len + file_name_len] = '\0';
				/* 递归调用,删除子文件夹 */
				ret_val = sd_delete_dir_impl(formated_path, dir_len + file_name_len);
				if (ret_val != SUCCESS)
				{
					LOG_ERROR("sd_delete_dir_impl,delete sub dir error:%d, %s", ret_val, formated_path);
				}
			}// end while 

			closedir(p_dir);

			formated_path[path_len] = '\0';
			if (ret_val == SUCCESS)
			{
				/* 将当前目录删除 */
				if (rmdir(formated_path) != 0)
				{
					ret_val = errno;
					LOG_ERROR("sd_delete_dir_impl, rmdir fail:%d, %s", ret_val, formated_path);
					//sd_assert(ret_val == SUCCESS);
				}
			}
		}
		else
		{
			/* 删除文件 */
			if (unlink(formated_path) != 0)
			{
				ret_val = errno;
				LOG_ERROR("sd_delete_dir_impl, unlink fail:%d, %s", ret_val, formated_path);
				//sd_assert(ret_val == SUCCESS);
			}
		}
	}
	return ret_val;
}
#endif
/* 删除整个目录 */
_int32 sd_delete_dir(const char *path)
{
	_int32 ret_val = SUCCESS;
#if defined(LINUX) 
	_u32 formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_LONG_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_LONG_FULL_PATH_BUFFER_LEN;

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	/* 和谐一下分隔符格式及编码方式 */
	ret_val = sd_format_conv_filepath(path, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);

	ret_val = sd_delete_dir_impl(formatted_conv_filepath, formatted_conv_filepath_len);
	//CHECK_VALUE(ret_val);
#else  
	ret_val = NOT_IMPLEMENT;
#endif 
	return ret_val;

}

_int32 sd_copy_dir(const char *src_path, const char *dest_path)
{
	_int32 ret_val = SUCCESS;
	FILE_ATTRIB *file;
	_u32 file_count = 0;
	_u32 i = 0;
	_int32 sub_src_len = 0;
	_int32 sub_dest_len = 0;

	if (!sd_dir_exist(src_path)) return BAD_DIR_PATH;
	if (!sd_dir_exist(dest_path))
	{
		ret_val = recursive_mkdir(dest_path);
		sd_assert(ret_val == SUCCESS);
		if (SUCCESS != ret_val) return ret_val;
	}

	ret_val = sd_get_sub_files(src_path, NULL, 0, &file_count);
	sd_assert(ret_val == SUCCESS);
	if (SUCCESS != ret_val) return ret_val;

	if (file_count == 0) return SUCCESS;
	ret_val = sd_malloc(sizeof(FILE_ATTRIB)*file_count, (void **)&file);
	sd_assert(ret_val == SUCCESS);
	if (SUCCESS != ret_val) return ret_val;

	ret_val = sd_get_sub_files(src_path, file, file_count, &file_count);
	sd_assert(ret_val == SUCCESS);
	if (SUCCESS != ret_val)
	{
		sd_free(file);
		return ret_val;
	}
	for (i = 0; i < file_count; i++)
	{
		char sub_src[MAX_FILE_PATH_LEN] = { 0 };
		char sub_dest[MAX_FILE_PATH_LEN] = { 0 };

		//初始化源文件或目录
		sub_src_len = sd_strlen(src_path);
		sd_strncpy(sub_src, src_path, sub_src_len);
		sd_append_path(sub_src, MAX_FILE_PATH_LEN, file[i]._name);

		//初始化目标文件或目录
		sub_dest_len = sd_strlen(dest_path);
		sd_strncpy(sub_dest, dest_path, sd_strlen(dest_path) + 1);
		sd_append_path(sub_dest, MAX_FILE_PATH_LEN, file[i]._name);

		if (file[i]._is_dir)
		{
			ret_val = sd_copy_dir(sub_src, sub_dest);
		}
		else
		{
			ret_val = sd_copy_file(sub_src, sub_dest);
		}
		sd_assert(ret_val == SUCCESS);
		if (SUCCESS != ret_val) break;
	}

	sd_free(file);

	return ret_val;
}

/* 检查路径下是否有足够的剩余空间,need_size的单位是KB */
_int32 sd_check_enough_free_disk(char * path, _u64 need_size)
{
	_int32 ret_val = SUCCESS;
	_u64 free_size = 0;

	ret_val = sd_get_free_disk(path, &free_size);
	if (SUCCESS != ret_val)
	{
		LOG_ERROR("sd_check_enough_free_disk, sd_get_free_disk fail:result:%d, path: %s", ret_val, path);
		return ret_val;
	}
	//CHECK_VALUE(ret_val);

	if (free_size<MIN_FREE_DISK_SIZE) return INSUFFICIENT_DISK_SPACE;  //小于100MB 

	if (need_size + MIN_FREE_DISK_SIZE>free_size)  //100 MB 空余
	{
		return INSUFFICIENT_DISK_SPACE;
	}

	return SUCCESS;
}


/* ------------------------------------------------------ */
/*	mode : r,r+,w,w+,a,a+  or with 'b'
*/
_int32 sd_fopen(const char *filepath, const char * mode, _FILE ** fp)
{
	_int32 ret_val = SUCCESS;
#ifdef LINUX
	*fp = fopen(filepath, mode);
	if (*fp == NULL)
	{
		ret_val = errno;
	}
#else
	ret_val = NOT_IMPLEMENT;
#endif
	return ret_val;
}
_int32 sd_fclose(_FILE *fp)
{
	_int32 ret_val = SUCCESS;
#ifdef LINUX
	ret_val = fclose(fp);
	if (ret_val == EOF)
	{
		ret_val = errno;
	}
#else
	ret_val = NOT_IMPLEMENT;
#endif
	return ret_val;
}

/* 读取一行或n-1个字符 */
_int32 sd_fgets(char *buf, _int32 n, _FILE *fp)
{
	_int32 ret_val = SUCCESS;
#ifdef LINUX
	if (NULL == fgets(buf, n, fp))
	{
		ret_val = -1;
	}
#else
	ret_val = NOT_IMPLEMENT;
#endif
	return ret_val;
}

/* 写入字符串 */
_int32 sd_fputs(const char *str, _FILE *fp)
{
	_int32 ret_val = SUCCESS;
#ifdef LINUX
	ret_val = fputs(str, fp);
	if (ret_val == EOF || ret_val < 0)
	{
		ret_val = errno;
	}
	else
	{
		ret_val = SUCCESS;
	}
#else
	ret_val = NOT_IMPLEMENT;
#endif
	return ret_val;
}


BOOL sd_is_path_writable(const char* path)
{
	_int32 ret_val = SUCCESS;
#if 0
	_u32 file_id=0,cur_time=0;
	char file_buffer[MAX_FULL_PATH_BUFFER_LEN]=
	{};
	_u32 path_len = sd_strlen(path);

	if(path_len == 0) return -1;

	//sd_time_ms(&(cur_time));
	sd_snprintf(file_buffer, MAX_FULL_PATH_BUFFER_LEN,
		path[path_len-1]=='/' ? "%setm_test_%u" : "%s/etm_test_%u", path, cur_time);
	ret_val=sd_open_ex(file_buffer, O_FS_CREATE, &file_id);
	if (ret_val != SUCCESS)
		return ret_val;

	sd_close_ex(file_id);
	sd_delete_file(file_buffer);
#endif
	struct stat st_info;
	mode_t mode;
	ret_val = stat(path, &st_info);
	if (ret_val)
		return errno;
	if (st_info.st_mode & S_IWUSR)
	{
		return TRUE;
	}
	mode = st_info.st_mode;
	mode |= S_IWUSR;
	ret_val = chmod(path, mode);
	if (ret_val)
	{
		LOG_URGENT("sd_is_path_writable:%s, error:%d", path, errno);
		return FALSE;
	}
	return TRUE;
}

//total_size单位为KB
_int32 sd_get_total_disk(const char *path, _u64 *total_size)
{
#if defined(AMLOS) || defined(MSTAR) || defined(SUNPLUS)
	return sd_get_free_disk_old(path, free_size);
#else
	// __SYMBIAN32__ WINCE LINUX _ANDROID_LINUX

	/* 和谐一下分隔符格式及编码方式 */
	_int32 ret_val = SUCCESS;
	_u32 formatted_conv_filepath_len = 0;
	char formatted_conv_filepath[MAX_FULL_PATH_BUFFER_LEN];
	_u32 formatted_conv_filepath_buflen = MAX_FULL_PATH_BUFFER_LEN;

	struct statfs sf;

	sd_memset(formatted_conv_filepath, 0, formatted_conv_filepath_buflen);
	ret_val = sd_format_conv_filepath(path, formatted_conv_filepath, formatted_conv_filepath_buflen, &formatted_conv_filepath_len);
	CHECK_VALUE(ret_val);
	path = formatted_conv_filepath;

	*total_size = 0;

	if (statfs(path, &sf) < 0)
		ret_val = errno;
	else
	{
		*total_size = (sf.f_bsize * sf.f_blocks) >> 10;
	}

	return ret_val;
#endif
}


