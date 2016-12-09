
/*******************************************************
 * 
 * Android 访问apk中的"asset"资源路径相关函数。
 *
 *******************************************************/

#ifndef __SD_ANDROID_ASSET_FILES_20100925_h__
#define __SD_ANDROID_ASSET_FILES_20100925_h__

#if defined(_ANDROID_LINUX)

#include "utility/define.h"
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

// ASSET_PATH 必须与 get_resource_path() 相同
#define ASSET_PATH "/sdcard/thunder/resource/"
#define ASSET_FD_BASE (0x70000000)
#define ASSET_FD_SIZE (128)

_int32 sd_asset_open_ex(char *filepath, _u32 *file_id);
_int32 sd_asset_read(_u32 file_id, char *buffer, _int32 size, _u32 *readsize);

// 注意，只能向前定位，不能向后定位。
_int32 sd_asset_setfilepos(_u32 file_id, _u64 filepos);

_int32 sd_asset_close_ex(_u32 file_id);
BOOL sd_asset_file_exist(char *filepath);
BOOL sd_asset_is_asset_fd(_u32 file_id);

#ifdef __cplusplus
}
#endif

#endif
#endif // __SD_ANDROID_ASSET_FILES_20100925_h__

