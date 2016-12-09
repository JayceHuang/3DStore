/*
 ============================================================================
 Name		: sd_wm_network.h
 Author	  : XunLei
 Copyright   : - C (copyright) - www.xunlei.com - This document is the proprietary of XunLei All rights reserved.Integral or partial reproduction prohibited without a written authorization from the permanent of the author rights.
 Description : Declares UI class for application.
 ============================================================================
 */

#ifndef __SD_WM_NETWORK_20100625_h__
#define __SD_WM_NETWORK_20100625_h__


#include "platform/wm_interface/sd_wm_network_interface.h"
//#include <stdafx.h>
#include <objbase.h>
#include <initguid.h>
#include <connmgr.h> //cellcore.lib
#include <winnt.h>
#include <winsock2.h>
//#include <afxcoll.h>
//#include <afxtempl.h>
//#include <windef.h>
//#include <string.h>


#include <e32base.h>
#include <e32std.h>
#include <F32FILE.H>
#include <d32dbms.h>
#include <BADESCA.H>
#include <Etel3rdParty.h>
#include <MTCLREG.H>
#include <es_sock.h>
#include <CommDbConnPref.h>
#include <COEMAIN.H>
#include <BAUTILS.H>
#include <apmrec.h> 
#include <apgcli.h>
#include <smut.h> 
#include<hal.h>


class CNetConnector : public CActive
{

public:
static CNetConnector* NewL();
static CNetConnector* NewLC();
~CNetConnector();

int do_connection(RConnection &aConnection,TCommDbConnPref &aConnectPref);
protected: 
void DoCancel();
void RunL();
private:
CNetConnector();
void ConstructL(void);
//private:


};



#endif
#endif // __SD_WM_NETWORK_20100625_h__
// End of File

