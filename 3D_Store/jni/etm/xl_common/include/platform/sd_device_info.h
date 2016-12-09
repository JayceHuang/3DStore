#ifndef _SD_DEVICE_INFO_H_
#define _SD_DEVICE_INFO_H_

#include "utility/define.h"
#include "utility/errcode.h"
#include "utility/define_const_num.h"

const char * get_system_info(void);
const char * get_imei(void);
void set_imei(const char * p_imei, _u32 len);

#endif
