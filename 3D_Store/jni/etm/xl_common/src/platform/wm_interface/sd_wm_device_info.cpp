#include "utility/define.h"
#ifdef WINCE
#ifdef __cplusplus
extern "C" 
{
#endif
//////////////////////////////////////////////////////////////////////////

#include "platform/wm_interface/sd_wm_device_info.h"
#include "utility/string.h"
#include <windows.h>
#include <Iphlpapi.h>
#include <Tapi.h>
#include <extapi.h>
#include <tsp.h>
#include <Winuser.h>
#include "utility/define_const_num.h"


#define TAPI_API_LOW_VERSION    0x00010003 
#define TAPI_CURRENT_VERSION	0x00020000 
#define EXT_API_LOW_VERSION     0x00010000 
#define EXT_API_HIGH_VERSION    0x00010000 
static char version_name[MAX_OS_LEN];
static BOOL is_sys_info_ok = FALSE;
static BOOL is_screen_ok=FALSE;
static _int32  screen_x= 0;
static _int32  screen_y= 0;

const char * get_wm_system_info(void)
{
	OSVERSIONINFOW m_VersionInformation;
	
	if(is_sys_info_ok) return version_name;
	
	sd_memset(&m_VersionInformation,0x00,sizeof(m_VersionInformation));
	GetVersionEx(&m_VersionInformation); //取得与平台和操作系统有关的版本信息 
	sd_memset(version_name,0,MAX_OS_LEN);
	sd_snprintf(version_name, MAX_OS_LEN-1, "%s_%d.%d", "windows_mobile",m_VersionInformation.dwMajorVersion,m_VersionInformation.dwMinorVersion);
	is_sys_info_ok = TRUE;
	return version_name;
}

_int32 get_wm_screen_size(_int32 *x,_int32 *y)
{
	if(!is_screen_ok)
	{
		screen_x= GetSystemMetrics(SM_CXSCREEN);
		screen_y= GetSystemMetrics(SM_CYSCREEN);
		is_screen_ok = TRUE;
	}
	
	if(x)
		*x= screen_x;
	if(y)
		*y = screen_y;
	return SUCCESS;
}

const char * get_wm_imei(void)
{
	static BOOL s_imei_ready = FALSE;
	static char s_imei[IMEI_LEN + 1];
	if (s_imei_ready)
	{
		return s_imei;
	}

	_int32 ret = 0;
	LINEINITIALIZEEXPARAMS lineExtParams;
	lineExtParams.dwTotalSize = sizeof(lineExtParams);
	lineExtParams.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;
	DWORD dwAPIVersion = TAPI_CURRENT_VERSION;
	HLINEAPP hLineApp;
	DWORD dwNumDevs;

	ret = lineInitializeEx(&hLineApp, (HINSTANCE)NULL, (LINECALLBACK)NULL, _T("Mengge"), &dwNumDevs, &dwAPIVersion, &lineExtParams);
	if (ret != 0)
	{
		return (char *)NULL;
	}

	DWORD dwTSPILineDeviceID = 0;
	for (DWORD dwCurrentDevID = 0; dwCurrentDevID < dwNumDevs; dwCurrentDevID++)
	{
		LINEEXTENSIONID LineExtensionID;
		if (lineNegotiateAPIVersion(hLineApp, dwCurrentDevID, TAPI_API_LOW_VERSION, TAPI_CURRENT_VERSION, 
			&dwAPIVersion, &LineExtensionID) == 0)
		{
			LINEDEVCAPS LineDevCaps;
			LineDevCaps.dwTotalSize = sizeof(LineDevCaps);
			if (::lineGetDevCaps(hLineApp, dwCurrentDevID, dwAPIVersion, 0, &LineDevCaps) == 0)
			{
				BYTE* pLineDevCapsBytes = new BYTE[LineDevCaps.dwNeededSize];
				if (NULL != pLineDevCapsBytes)  
				{
					LINEDEVCAPS* pLineDevCaps = (LINEDEVCAPS*)pLineDevCapsBytes;
					pLineDevCaps->dwTotalSize = LineDevCaps.dwNeededSize;
					if (::lineGetDevCaps(hLineApp, dwCurrentDevID, dwAPIVersion, 0, pLineDevCaps) == 0)
					{
						LPTSTR lstr = (TCHAR*)((BYTE*)pLineDevCaps+pLineDevCaps->dwLineNameOffset);
						if (!_tcscmp(CELLTSP_LINENAME_STRING, lstr))
						{
							dwTSPILineDeviceID = dwCurrentDevID;
							delete[] pLineDevCapsBytes;
							break;
						}
					}
					delete[] pLineDevCapsBytes;
				}
			}
		}
	}

	HLINE hLine = (HLINE)NULL;
	ret = ::lineOpen(hLineApp, dwTSPILineDeviceID, &hLine, dwAPIVersion, 0, 0,  
		LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_DATAMODEM, 0);
	if (ret != 0)
	{
		::lineShutdown(hLineApp);
		return (char *)NULL;
	}

	LINEGENERALINFO lviGeneralInfo;
	lviGeneralInfo.dwTotalSize = sizeof(lviGeneralInfo);
	ret = ::lineGetGeneralInfo(hLine, &lviGeneralInfo);
	if (ret != 0)
	{
		::lineClose(hLine);
		::lineShutdown(hLineApp);
		return (char *)NULL;
	}

	LPBYTE pLineGeneralInfoBytes = new BYTE[lviGeneralInfo.dwNeededSize];
	if (pLineGeneralInfoBytes != NULL)
	{
		LPLINEGENERALINFO plviGeneralInfo = (LPLINEGENERALINFO)pLineGeneralInfoBytes;
		plviGeneralInfo->dwTotalSize = lviGeneralInfo.dwNeededSize;
		ret = ::lineGetGeneralInfo(hLine, plviGeneralInfo);
		if (ret != 0)
		{
			::lineClose(hLine);
			::lineShutdown(hLineApp);
			delete[] pLineGeneralInfoBytes;
			return (char *)NULL;
		}
		else
		{
			if (plviGeneralInfo->dwSerialNumberSize > 0)
			{
				LPTSTR lstr = (TCHAR*)((BYTE*)plviGeneralInfo + plviGeneralInfo->dwSerialNumberOffset);
				for (int i = 0; i < IMEI_LEN; ++i)
				{
					s_imei[i] = (char)lstr[i];
				}
				s_imei[IMEI_LEN] = '\0';
				s_imei_ready = TRUE;
			}
		}
		delete[] pLineGeneralInfoBytes;
	}

	::lineClose(hLine);
	::lineShutdown(hLineApp);
	return s_imei;
}

//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif


#endif /* win32 */
