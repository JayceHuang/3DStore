/*
 ============================================================================
 Name		: platform/wm_interface/sd_wm_network_interface.h
 Author	  : XunLei
 Copyright   : - C (copyright) - www.xunlei.com - This document is the proprietary of XunLei All rights reserved.Integral or partial reproduction prohibited without a written authorization from the permanent of the author rights.
 Description : Declares UI class for application.
 ============================================================================
 */

#ifndef __SD_WM_NETWORK_INTERFACE_20100625_h__
#define __SD_WM_NETWORK_INTERFACE_20100625_h__
#include "utility/define.h"
#if defined( WINCE)

#include "platform/sd_network.h"


#ifdef __cplusplus
extern "C" 
{
#endif


IMPORT_C _u32 get_local_ip(void);
//PORT_C void sd_check_net_connection_result(void);

/* Get  RSocketServ * iSocketServ */
//EXPORT_C void* get_symbian_socket_serv(void);
/* Get  RConnection * iConnection  */
//EXPORT_C void* get_symbian_net_conection(void);

#ifdef __cplusplus
}
#endif

#endif
#endif // __SD_WM_NETWORK_INTERFACE_20100625_h__
// End of File

