/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/24
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_DEFINE_H_
#define _BT_DEFINE_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

#ifdef ENABLE_BT 
#define		BT_PEERID_LEN			20
#define		BT_INFO_HASH_LEN		20
#define		BT_MAX_PACKET_SIZE		(256*1024)
#define		BT_PRO_HEADER_LEN		4
#define		BT_MAX_REQUEST_PIECE	3
#define		BT_PEERID_HEADER		"-UT3000-"
#define		BT_MAX_DOWNLOAD_FILES	(1000)
#endif

#ifdef __cplusplus
}
#endif
#endif


