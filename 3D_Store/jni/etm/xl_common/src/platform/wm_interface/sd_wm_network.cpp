#include "utility/define.h"
#if defined( WINCE)
#include "utility/sd_iconv.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "platform/sd_socket.h"
#include "utility/utility.h"

#include "platform/wm_interface/sd_wm_network.h"
//#include "platform/wm_interface/sd_wm_http_interface.h"
//#include "platform/symbian_interface/sd_symbian_http_engine.h"
//#include "platform/wm_interface/sd_wm_os.h"
#include <stdlib.h>
#include <windows.h>

#include <connmgr.h>
#include <Connmgr_status.h>
#include <Cfgmgrapi.h>
#include <strsafe.h>
#include <winsock2.h>

/* log */
#include "utility/logid.h"
#define	 LOGID	LOGID_SOCKET_PROXY
#include "utility/logger.h"

#define EXPORT_C  

#ifdef __cplusplus
extern "C" 
{
#endif


/////////////////////////////////////////
#define SD_WAIT_CON_TIME_OUT 100
static _u32 g_net_iap_id = MAX_U32;
static _u32 g_net_type = 0;
//static RSocketServ iSocketServ;
//static RConnection iConnection;
//static TCommDbConnPref iConnectPref;
static SD_NET_STATUS g_net_status = SNS_DISCNT;
//static CNetConnector* iNetConnector=NULL;
static SD_NOTIFY_NET_CONNECT_RESULT g_notify_net_call_back_func=(SD_NOTIFY_NET_CONNECT_RESULT)NULL;
static char g_net_iap_name[MAX_IAP_NAME_LEN];
static _u32 g_proxy_ip = 0;
static _u16 g_proxy_port = 0;


static _u32 g_wait_con_timer_id = 0;
static _u32 g_wait_con_count = 0;
 static    HANDLE   g_hConnMgr = NULL;
 static HANDLE m_hConnection = NULL;
// EXPORT_C _int32 sd_net_connected_timeout(void);


//检查是否有可用连接
 BOOL sd_GetConnMgrAvailable()
{
    HANDLE hConnMgr = ConnMgrApiReadyEvent ();
    BOOL bAvailbale = FALSE;
    DWORD dwResult = WaitForSingleObject ( hConnMgr, 2000 );
    if ( dwResult == WAIT_OBJECT_0 )
    {
        bAvailbale = TRUE;
    }
    // 关闭
    if ( hConnMgr ) CloseHandle ( hConnMgr );

	LOG_DEBUG("sd_GetConnMgrAvailable:%d",bAvailbale);
	
    return bAvailbale;
}
 //枚举所有可用连接
void sd_EnumNetIdentifier ( /* CStringArray &StrAry*/ )
{
    CONNMGR_DESTINATION_INFO networkDestInfo = {0};
	DWORD dwEnumIndex=0;
    // 得到网络列表
    for (  dwEnumIndex=0; ; dwEnumIndex++ )
    {
        memset ( &networkDestInfo, 0, sizeof(CONNMGR_DESTINATION_INFO) );
        if ( ConnMgrEnumDestinations ( dwEnumIndex, &networkDestInfo ) == E_FAIL )
        {
            break;
        }
/*
        //StrAry.Add ( networkDestInfo.szDescription );
		{
			TCHAR szSend[100];
			ZeroMemory(szSend, 100);
			Sleep(1000);
			wsprintf(szSend, L"sd_EnumNetIdentifier[%d] = %s", dwEnumIndex,networkDestInfo.szDescription);
			MessageBox(NULL, szSend, L"msg", MB_OK);
		}
 */
   }
}
int sd_GetNetIdentifierByGUID (GUID guid )
{
    CONNMGR_DESTINATION_INFO networkDestInfo = {0};
	DWORD dwEnumIndex=0;
    // 得到网络列表
    for (  dwEnumIndex=0; ; dwEnumIndex++ )
    {
        memset ( &networkDestInfo, 0, sizeof(CONNMGR_DESTINATION_INFO) );
        if ( ConnMgrEnumDestinations ( dwEnumIndex, &networkDestInfo ) == E_FAIL )
        {
            break;
        }
        //StrAry.Add ( networkDestInfo.szDescription );
        	if(sizeof(networkDestInfo.guid)!=sizeof(GUID)) return -1;
		if(sd_memcmp(&networkDestInfo.guid,&guid,sizeof(GUID))==0)
		{
			LOG_DEBUG("sd_GetNetIdentifierByGUID:%d",dwEnumIndex);
			return dwEnumIndex;
		}
    }

	return -1;
}
//
//   FUNCTION: GetAPNFromEntryName(LPCTSTR szEntryName, LPTSTR szAPN, int cchAPN)
//
//   PURPOSE: Get the GPRS Access Point Name form the Entry Name
//
HRESULT GetAPNFromEntryName(LPCTSTR szEntryName, LPTSTR szAPN, int cchAPN)
{
    // parm query formating string of "CM_GPRSEntries Configuration Service Provider"
    LPCTSTR szFormat =    TEXT("<wap-provisioningdoc>")
                        TEXT("    <characteristic type=\"CM_GPRSEntries\">")
                        TEXT("        <characteristic type=\"%s\">")
                        TEXT("            <characteristic type=\"DevSpecificCellular\">")
                        TEXT("                <parm-query name=\"GPRSInfoAccessPointName\"/>")
                        TEXT("            </characteristic>")
                        TEXT("        </characteristic>")
                        TEXT("    </characteristic>")
                        TEXT("</wap-provisioningdoc>");
    HRESULT hr = E_FAIL;
    LPTSTR szOutput   = (LPTSTR)NULL;
	LPTSTR szInput ;

    if(NULL == szEntryName)
        return E_INVALIDARG;


    // prepare the query string with the special entry name
    szInput = new TCHAR[_tcslen(szFormat) + _tcslen(szEntryName) + 10];
    if(NULL == szInput)
        return E_OUTOFMEMORY;

    _stprintf(szInput, szFormat, szEntryName);
   
    // Process the XML.
    hr = DMProcessConfigXML(szInput, CFGFLAG_PROCESS, &szOutput);

    if(S_OK == hr)
    {

        // find the value of GPRSInfoAccessPointName param
        LPTSTR szAPNStrStart = _tcsstr(szOutput, TEXT("value=\""));
        hr = E_FAIL;
       if(NULL != szAPNStrStart)
        {
			LPTSTR szAPNStrEnd ;
            szAPNStrStart += _tcslen(TEXT("value=\""));

            // find the end of value string
            szAPNStrEnd = _tcsstr(szAPNStrStart, TEXT("\""));
            if(NULL != szAPNStrEnd)
            {
                // set the null char at the end of the value string
                *szAPNStrEnd = TEXT('\0');

                // get the final Access Point Name string
                _tcsncpy(szAPN, szAPNStrStart, cchAPN);
                hr = S_OK;
            }
        }

    }

    // the caller must delete the XML returned from DMProcessConfigXML.
    delete[] szOutput;

    // clear the input string
    delete[] szInput;

    return hr;
}


//
// we will enum the all connected GPRS
// and display their entry name and APN on the message box
//
_u32 sd_GetDetailNetType(void)
{
    TCHAR szAPN[200];
	_u32 n_type = 0;
    DWORD dwSize = 0;
    HRESULT hr = E_FAIL;
	LPBYTE pBuffer ;

    //
    // Get the the required size of the buffer
    // with which the function needs to be called on the next attempt.
    //
    hr = ConnMgrQueryDetailedStatus((CONNMGR_CONNECTION_DETAILED_STATUS *)NULL, &dwSize);
    if(STRSAFE_E_INSUFFICIENT_BUFFER != hr)
        return 0;

    pBuffer = new BYTE[dwSize];
    if(NULL == pBuffer)
        return 0;

    //
    // Get the connection information
    //
    hr = ConnMgrQueryDetailedStatus((CONNMGR_CONNECTION_DETAILED_STATUS*)pBuffer, &dwSize);
    if(S_OK == hr)
    {

        //
        // Enum each connection entry
        //
        CONNMGR_CONNECTION_DETAILED_STATUS* cmStatus  = (CONNMGR_CONNECTION_DETAILED_STATUS*)pBuffer;
        while(NULL != cmStatus)
        {
		//	_stprintf(szAPN, TEXT("%d,%d,%d,%d,%d,%d,%d,%d,%s"), 
		//		cmStatus->dwVer,cmStatus->dwParams,cmStatus->dwType,cmStatus->dwSubtype,cmStatus->dwFlags,cmStatus->dwSecure,cmStatus->dwConnectionStatus,cmStatus->dwSignalQuality,cmStatus->szAdapterName);
           //MessageBox((HWND)NULL, szAPN, cmStatus->szDescription, MB_OK | MB_ICONINFORMATION);
           // find the connected GPRS entry
            if((cmStatus->dwParams & (CONNMGRDETAILEDSTATUS_PARAM_TYPE | CONNMGRDETAILEDSTATUS_PARAM_DESCRIPTION | CONNMGRDETAILEDSTATUS_PARAM_CONNSTATUS)) &&
                CM_CONNTYPE_CELLULAR == cmStatus->dwType &&
                CONNMGR_STATUS_CONNECTED == cmStatus->dwConnectionStatus &&
                NULL != cmStatus->szDescription)       
            {
                // get the connected GPRS APN
                if(S_OK == GetAPNFromEntryName(cmStatus->szDescription, szAPN, 200))
				{
                    //MessageBox((HWND)NULL, szAPN, cmStatus->szDescription, MB_OK | MB_ICONINFORMATION);
					if(0==_tcsicmp(szAPN, TEXT("CMNET")))
					{
						n_type = NT_CMNET;
					}
					else
					if(0==_tcsicmp(szAPN, TEXT("CMWAP")))
					{
						n_type = NT_CMWAP;
					}
					else
					if(0==_tcsicmp(szAPN, TEXT("CUINET")))
					{
						n_type = NT_CUINET;
					}
					else
					if(0==_tcsicmp(szAPN, TEXT("UNIWAP")))
					{
						n_type = NT_CUIWAP;
					}
					else
						n_type = NT_CMNET;
				}
            }

            // test the next one
            cmStatus = cmStatus->pNext;
        }
    }


    // clear the buffer
    delete pBuffer;


    return n_type;
}
 _int32 sd_test_connect(char * host)
{
	struct sockaddr_in os_addr;
	_int32 addr_len = sizeof(os_addr),ret_val = SUCCESS,last_err = SUCCESS;
	unsigned   long   ul   =   1; 

	_u32 sock = socket(SD_AF_INET, SD_SOCK_STREAM, IPPROTO_TCP);

	LOG_ERROR("sd_test_connect:%s",host);

	if(sock== (_u32)-1)
	{
		//sd_assert(FALSE);
		return -1;
	}

	//设置非阻塞方式连接 
	ret_val   =   ioctlsocket(sock,   FIONBIO,   (unsigned   long*)&ul); 
	if(ret_val==SOCKET_ERROR)
	{
		//sd_assert(FALSE);
		return -1;
	}
	
	os_addr.sin_family = AF_INET;
	os_addr.sin_port = htons(80);
	os_addr.sin_addr.s_addr=sd_inet_addr(host);  
	do
	{
		ret_val = connect(sock, (struct sockaddr*)&os_addr, addr_len);
		if(ret_val<0)
		{
			last_err = WSAGetLastError();
		}
	}while(ret_val < 0 && last_err== WSAEINTR);
	
	if(ret_val >= 0)
	{
		closesocket(sock); 
		LOG_ERROR("sd_test_connect:connect %s ok!",host);
		return SUCCESS;
	}
	else
	{
		ret_val = last_err;
		if(ret_val == WSAEWOULDBLOCK)
		{
			//select   模型，即设置超时 
			struct   timeval   timeout   ; 
			fd_set   r; 

			FD_ZERO(&r); 
			FD_SET(sock,   &r); 
			timeout.tv_sec   =   4;   //连接超时4秒 
			timeout.tv_usec   =0; 
			ret_val   =   select(0,   0,   &r,   0,   &timeout); 
			closesocket(sock); 
			if(   ret_val   >   0   ) 
			{ 
				/* ok,connected! */
				LOG_ERROR("sd_test_connect 2:connect %s ok!",host);
				return SUCCESS;
			}
		}
		else
		{
			LOG_ERROR("sd_test_connect :connect %s Failed!",host);
			closesocket(sock); 
		}
	}

	return -1;
}

 _u32 sd_test_proxy(_u32 *p_ip,_u16 *p_port)
{
	_u32 proxy_id = 0,net_type=0;
	_int32 ret_val = SUCCESS;

	LOG_ERROR("sd_test_proxy");
	*p_ip = 0;
	*p_port = 80;
	
	ret_val   =   sd_test_connect("8.8.8.8");  //google dns
	if(ret_val==SUCCESS)
	{
		net_type = NT_CMNET;
		return net_type;
	}
	
	ret_val   =   sd_test_connect("10.0.0.172");  //china mobile and china unicom
	if(ret_val==SUCCESS)
	{
		net_type = NT_CMWAP;
		*p_ip = 0xAC00000A; //10.0.0.172
		LOG_ERROR("sd_test_proxy,NT_CMWAP:10.0.0.172");
	}
	else
	{
		net_type = NT_CTWAP;
		*p_ip = 0xC800000A; //10.0.0.200
		LOG_ERROR("sd_test_proxy,NT_CTWAP:10.0.0.200");
	}

	return net_type;
}

_u32 sd_GetNetType (_u32 iap_id )
{
	GUID guid_DestNetInternet = IID_DestNetInternet;
	GUID guid_DestNetWAP = IID_DestNetWAP;
	GUID guid_DestNetCorp = IID_DestNetCorp;
    CONNMGR_DESTINATION_INFO DestInfo = {0};
	_u32 ntype=NT_CMNET; //NT_CMWAP;//
	_u32 local_ip = 0;
	BOOL is_gprs = FALSE;
	
	local_ip = get_local_ip();
	if((local_ip&0x000000FF)==0x0A)
	{
		is_gprs = TRUE;
	}
	
	ntype = sd_GetDetailNetType();
	if(ntype==0)
	{
		if ( SUCCEEDED(ConnMgrEnumDestinations(iap_id, &DestInfo)) )
		{
			if(sd_memcmp(&DestInfo.guid,&guid_DestNetInternet,sizeof(GUID))==0)
			{
				ntype =NT_CMNET;
			}
			else
			if(sd_memcmp(&DestInfo.guid,&guid_DestNetWAP,sizeof(GUID))==0)
			{
				ntype =NT_CMWAP;
			}
			else
			if(sd_memcmp(&DestInfo.guid,&guid_DestNetCorp,sizeof(GUID))==0)
			{
				ntype =NT_WLAN;
			}
			else
			{
				/*
				if((NULL!=wcsstr(DestInfo.szDescription,L"WAP"))||(NULL!=wcsstr(DestInfo.szDescription,L"wap")))
				{
					ntype =NT_CMWAP;
				}
				*/
				ntype = NT_WLAN;
			}
		}
	}

	if(is_gprs)
	{
		ntype = sd_test_proxy(&g_proxy_ip,&g_proxy_port);
		if(ntype == 0||ntype == NT_WLAN)
		{
			ntype = NT_CMNET;
		}
	}
	else
	{
		if(ntype != 0&&ntype != NT_WLAN)
		{
			ntype = NT_WLAN;
		}
	}
	LOG_DEBUG("sd_GetNetType:0x%X",ntype);
    return ntype;
}

//找到“Internet”这个连接，可用远程URL映射的方式来完成，这样可以让系统自动选取一个最好的连接
int sd_MapURLAndGUID ( LPCTSTR lpszURL, GUID *guidNetworkObject/*, OUT CString *pcsDesc=NULL*/ )
{
    int nIndex = 0;
	HRESULT hResult ;
    if ( !lpszURL || lstrlen(lpszURL) < 1 )
        return -1;

    memset ( guidNetworkObject, 0, sizeof(GUID) );
    hResult = ConnMgrMapURL ( lpszURL, guidNetworkObject, (DWORD*)&nIndex );
/*
	{
        TCHAR szSend[100];
        ZeroMemory(szSend, 100);
		Sleep(1000);
        wsprintf(szSend, L"sd_MapURLAndGUID = 0x%X,%d", hResult,nIndex);
        MessageBox(NULL, szSend, L"msg", MB_OK);
	}
*/
	
	
	if ( FAILED(hResult) )
    {
        DWORD dwLastError = GetLastError ();
		nIndex = -1;
       // AfxMessageBox ( _T("Could not map a request to a network identifier") );
    }
    else
    {
            CONNMGR_DESTINATION_INFO DestInfo = {0};
            if ( SUCCEEDED(ConnMgrEnumDestinations(nIndex, &DestInfo)) )
            {
                //*pcsDesc = DestInfo.szDescription;
				//if(DestInfo.guid!=*guidNetworkObject)
				if(sd_memcmp(&DestInfo.guid,guidNetworkObject,sizeof(GUID))!=0)
				{
					nIndex = sd_GetNetIdentifierByGUID (*guidNetworkObject );
/*
					{
						TCHAR szSend[100];
						ZeroMemory(szSend, 100);
						Sleep(1000);
						wsprintf(szSend, L"sd_GetNetIdentifierByGUID = %d", nIndex);
						MessageBox(NULL, szSend, L"msg", MB_OK);
					}
*/
				}
            }
			else
			{
/*
				TCHAR szSend[100];
				ZeroMemory(szSend, 100);
				Sleep(1000);
				wsprintf(szSend, L"ConnMgrEnumDestinations FAILED= %d", nIndex);
				MessageBox(NULL, szSend, L"msg", MB_OK);
*/

				nIndex = sd_GetNetIdentifierByGUID (*guidNetworkObject );
/*
				ZeroMemory(szSend, 100);
				Sleep(1000);
				wsprintf(szSend, L"sd_GetNetIdentifierByGUID = %d", nIndex);
				MessageBox(NULL, szSend, L"msg", MB_OK);
*/
			}
    }

	LOG_DEBUG("sd_MapURLAndGUID:%d",nIndex);
    return nIndex;
}
//释放连接
void sd_ReleaseConnection ()
{
	LOG_DEBUG("sd_ReleaseConnection");
    if ( m_hConnection )
    {
        ConnMgrReleaseConnection(m_hConnection, FALSE);
        m_hConnection = NULL;
    }
}

//启用指定编号的连接并检查连接状态
BOOL sd_EstablishConnection ( DWORD dwIndex )
{
     CONNMGR_DESTINATION_INFO DestInfo = {0};
	 HRESULT hResult ;
      BOOL bRet = FALSE;
 // 释放之前的连接
    sd_ReleaseConnection ();

    // 得到正确的连接信息
    hResult = ConnMgrEnumDestinations(dwIndex, &DestInfo);
    if(SUCCEEDED(hResult))
    {
         DWORD dwStatus = 0;
       // 初始化连接结构
        CONNMGR_CONNECTIONINFO ConnInfo;

        ZeroMemory(&ConnInfo, sizeof(ConnInfo));
        ConnInfo.cbSize = sizeof(ConnInfo);
        ConnInfo.dwParams = CONNMGR_PARAM_GUIDDESTNET;
        ConnInfo.dwFlags = CONNMGR_FLAG_PROXY_HTTP | CONNMGR_FLAG_PROXY_WAP | CONNMGR_FLAG_PROXY_SOCKS4 | CONNMGR_FLAG_PROXY_SOCKS5;
        ConnInfo.dwPriority = CONNMGR_PRIORITY_USERINTERACTIVE;
        ConnInfo.guidDestNet = DestInfo.guid;
        ConnInfo.bExclusive    = FALSE; 
        ConnInfo.bDisabled = FALSE;

       // hResult = ConnMgrEstablishConnectionSync(&ConnInfo, &m_hConnection, 10*1000, &dwStatus );
        hResult = ConnMgrEstablishConnection(&ConnInfo, &m_hConnection/*, 10*1000, &dwStatus */);
/*
		{
			TCHAR szSend[100];
			ZeroMemory(szSend, 100);
			Sleep(1000);
			wsprintf(szSend, L"sd_EstablishConnection = 0x%X,%s", hResult,DestInfo.szDescription);
			MessageBox(NULL, szSend, L"msg", MB_OK);
		}
*/
			
		if(FAILED(hResult))
        {
            m_hConnection = NULL;
        }
        else bRet = TRUE;
    }

	LOG_DEBUG("sd_EstablishConnection:%d",bRet);
    return bRet;
}
//检测连接状态
BOOL sd_CheckIfConnected (  DWORD *pdwStatus )
{
    BOOL bRet = FALSE;

        if ( m_hConnection )
        {
            DWORD dwStatus = 0;
            HRESULT hr = ConnMgrConnectionStatus ( m_hConnection, &dwStatus );

	
			if ( pdwStatus ) *pdwStatus = dwStatus;
            if ( SUCCEEDED(hr) )
            {
                if ( dwStatus == CONNMGR_STATUS_CONNECTED )
                {
                    bRet = TRUE;
                }
            }
        }

    return bRet;
}

EXPORT_C _int32 sd_init_network(_u32 iap_id,SD_NOTIFY_NET_CONNECT_RESULT call_back_func)
{
	int ret_val=SUCCESS,iap_index = -1;
	BOOL ret_b = FALSE;
	//CStringArray StrAry;
	//CString csDesc;
	GUID guidNetworkObject = {0};
	DWORD dwStatus = 0;

	LOG_ERROR("sd_init_network:g_net_status=%d,g_net_iap_id=%u,iap_id=%u",g_net_status,g_net_iap_id,iap_id);

	//sd_assert(g_net_status==SNS_DISCNT);
	if(g_net_status!=SNS_DISCNT)
	{
		return -1;
	}

	//iap_id = 7;
	iap_index = iap_id;
	g_net_iap_id = iap_id;
	g_notify_net_call_back_func = call_back_func;

	if(!sd_GetConnMgrAvailable()) return -1;

	//sd_EnumNetIdentifier (  );

	if(iap_id==-1)
	{
		iap_index = sd_MapURLAndGUID ( _T("http://www.runmit.com/"), &guidNetworkObject  );
	}
	
	if ( iap_index >= 0 )
	{
		ret_b = sd_EstablishConnection ( (DWORD ) iap_index);
		//ret_b = sd_EstablishConnection_by_guid(guidNetworkObject);
		if(ret_b)
		{
			g_net_iap_id = iap_index;
		}
	}
	
	LOG_ERROR("sd_init_network:%d,iap=%u",ret_b,g_net_iap_id);

	if(!ret_b) return -1;
	g_net_status=SNS_CNTING;
	return SUCCESS;
}

EXPORT_C _int32 sd_uninit_network(void)
{
	LOG_DEBUG("sd_uninit_network:%u",g_net_iap_id);
	sd_ReleaseConnection ();
	g_net_iap_id = MAX_U32;
	g_net_type = 0;
	g_net_status = SNS_DISCNT;
	sd_memset(g_net_iap_name,0x00,MAX_IAP_NAME_LEN);
	return SUCCESS;
}

EXPORT_C SD_NET_STATUS sd_get_network_status(void)
{
	sd_is_net_ok();
	
	return g_net_status;
}


EXPORT_C _int32 sd_get_network_iap(_u32 *iap_id)
{
	*iap_id = g_net_iap_id;
	return SUCCESS;
}
EXPORT_C const char* sd_get_network_iap_name(void)
{
	if(g_net_status==SNS_CNTING)
		return (char*)NULL;
	
	if(g_net_status==SNS_CNTED)
	{
		return g_net_iap_name;
	}
	else
	{
		return (char*)NULL;
	}
}

EXPORT_C void sd_check_net_connection_result(void)
{
	_int32 ret_val = SUCCESS;
		DWORD dwStatus  = 0;
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;
		
	if(g_net_status==SNS_DISCNT)
		return ;

    if(sd_CheckIfConnected(&dwStatus))
    {
		//wm_cancel_timer(g_wait_con_timer_id);
		//g_wait_con_timer_id = 0;
		g_wait_con_count = 0;
		g_net_status=SNS_CNTED;

		wVersionRequested = MAKEWORD( 2, 2 );
		 
		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 ) 
		{
		    /* Tell the user that we could not find a usable */
		    /* WinSock DLL.                                  */
		   LOG_ERROR("WSAStartup Error!");
		}

		g_net_type=sd_GetNetType (g_net_iap_id );

	    LOG_ERROR("The network is connected: iap=%u,net_type=0x%X", g_net_iap_id,g_net_type);
		((SD_NOTIFY_NET_CONNECT_RESULT)g_notify_net_call_back_func)(g_net_iap_id,SUCCESS,g_net_type);
    }
	else 
	{
		switch(dwStatus)
		{
			case CONNMGR_STATUS_DISCONNECTED:
			case CONNMGR_STATUS_CONNECTIONFAILED:
			case CONNMGR_STATUS_CONNECTIONDISABLED:
			case CONNMGR_STATUS_NOPATHTODESTINATION:
			case CONNMGR_STATUS_NORESOURCES:
			case CONNMGR_STATUS_CONNECTIONLINKFAILED:
			case CONNMGR_STATUS_AUTHENTICATIONFAILED:
			case CONNMGR_STATUS_WAITINGCONNECTIONABORT:
				//wm_cancel_timer(g_wait_con_timer_id);
				//g_wait_con_timer_id = 0;
				g_wait_con_count = 0;
				g_net_status=SNS_DISCNT;
	        		LOG_ERROR("The network connect failed 1: iap=%u,dwStatus=0x%X", g_net_iap_id,dwStatus);
			 	((SD_NOTIFY_NET_CONNECT_RESULT)g_notify_net_call_back_func)(g_net_iap_id,dwStatus,0);
				break;
			case CONNMGR_STATUS_CONNECTIONCANCELED:
				//wm_cancel_timer(g_wait_con_timer_id);
				//g_wait_con_timer_id = 0;
				g_wait_con_count = 0;
				g_net_status=SNS_DISCNT;
		        	LOG_ERROR("The network connect failed 2: iap=%u,dwStatus=0x%X", g_net_iap_id,dwStatus);
		 		((SD_NOTIFY_NET_CONNECT_RESULT)g_notify_net_call_back_func)(g_net_iap_id,USER_ABORT_NETWORK,0);
				break;
			case CONNMGR_STATUS_WAITINGCONNECTION:
				g_wait_con_count = 0;
				break;
			default:
				if(g_wait_con_count++>=20)
				{
					//wm_cancel_timer(g_wait_con_timer_id);
					//g_wait_con_timer_id = 0;
					g_wait_con_count = 0;
					g_net_status=SNS_DISCNT;
		        		LOG_ERROR("The network connect failed 3: iap=%u,dwStatus=0x%X", g_net_iap_id,dwStatus);
			 		((SD_NOTIFY_NET_CONNECT_RESULT)g_notify_net_call_back_func)(g_net_iap_id,dwStatus,0);
					//return;
				}
		}
	}
}


//EXPORT_C _int32 sd_net_connected_timeout(void)
EXPORT_C BOOL sd_is_net_ok(void)
{
	if(g_net_status!=SNS_CNTED)
		return FALSE;

	return sd_CheckIfConnected( (DWORD *)NULL);
	
		return SUCCESS;
}

EXPORT_C _u32 sd_get_net_type(void)
{
	if(g_net_status!=SNS_CNTED)
		return 0;
	
	return g_net_type;
}
EXPORT_C _int32 sd_set_net_type(_u32 net_type)
{
	g_net_type = net_type;
	return SUCCESS;
}

_u32 get_local_ip(void)
{
	char host_name[64];
	struct hostent* phe;
	_u32 local_ip = 0,l_ip = 0;
	LOG_ERROR("get_local_ip");
	if (gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR)
	{
		LOG_ERROR("gethostname Error!");
		return 0;
	}

	LOG_ERROR("Host name is %s",host_name);

	phe = gethostbyname(host_name);
	if (phe == 0) 
	{
		LOG_ERROR("gethostbyname Error !");
		return 0;
	}

	for (int i = 0; phe->h_addr_list[i] != 0; ++i) 
	{
		//struct in_addr addr;
		//memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		sd_memcpy((void*)&l_ip, (void*)((_u32*)phe->h_addr_list[i]), sizeof(_u32));
		sd_memset(host_name,0x00,64);
		sd_inet_ntoa(l_ip, host_name, 64);
		//LOG_ERROR("Local Address[%d]:0x%X=%s",i,l_ip,inet_ntoa(addr));
		LOG_ERROR("Local Address[%d]:0x%X=%s",i,l_ip,host_name);
		if(l_ip!=0)
		{
			local_ip = l_ip;
		}
	}
    
	return local_ip;
}


EXPORT_C _int32 sd_set_proxy(_u32 ip,_u16 port)
{
	g_proxy_ip = ip;
	g_proxy_port = port;

	return SUCCESS;
}
EXPORT_C _int32 sd_get_proxy(_u32 *p_ip,_u16 *p_port)
{
	if(g_proxy_ip==0)
	{
		if(sd_get_net_type() ==(NT_GPRS_WAP|CHINA_TELECOM))
		{
			/* 电信WAP */
			g_proxy_ip = 0xC800000A; //10.0.0.200
		}
		else
		{
			g_net_type = sd_test_proxy(&g_proxy_ip,&g_proxy_port);
		}
		g_proxy_port = 80;
	}
	
	if(p_ip)
		*p_ip = g_proxy_ip;
	
	if(p_port)
		*p_port = g_proxy_port;
	return SUCCESS;
}


#ifdef __cplusplus
}
#endif


#endif 
