#include "utility/define.h"
#if defined(MACOS)&&defined(MOBILE_PHONE)
#ifdef __cplusplus
extern "C" 
{
#endif
//////////////////////////////////////////////////////////////////////////

#include "platform/ios_interface/sd_ios_device_info.h"
#include "utility/string.h"
#include "utility/define_const_num.h"

#import  <UIKit/UIKit.h>
#import <SystemConfiguration/SCNetworkReachability.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import	<arpa/inet.h>
#import <netdb.h>
#include <sys/types.h>
#include <sys/sysctl.h>
	
static char ios_name[64]={0};
static char ios_version[64]={0};
static char hardware_version[64]={0};
static char version_name[MAX_OS_LEN]={0};
static BOOL is_sys_info_ok = FALSE;
static BOOL is_screen_ok=FALSE;
static _int32  screen_x= 0;
static _int32  screen_y= 0;
    
#define APP_ROOT_PATH_LEN   512
static char app_root_path[APP_ROOT_PATH_LEN]={0};

const char * get_ios_system_info(void)
{
	
	if(is_sys_info_ok) return version_name;
	
	sd_memset(version_name,0,MAX_OS_LEN);
	sd_snprintf(version_name, MAX_OS_LEN-1, "%s_%d.%d", "iPhone",1,1);
	is_sys_info_ok = TRUE;
	return version_name;
}

float get_ios_system_version(void)
{
    return [[[UIDevice currentDevice] systemVersion] floatValue];
}
                    

IPHONE_HARDWARE_VERSION get_ios_hardware_type(void)
{
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char *machine = malloc(size);
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    NSString *platform = [NSString stringWithCString:machine encoding:NSUTF8StringEncoding];
    free(machine);
    
    if (platform) 
    {
        if ([platform isEqualToString:@"iPhone1,1"])    return IPHONE_1G;
        if ([platform isEqualToString:@"iPhone1,2"])    return IPHONE_3G;
        if ([platform isEqualToString:@"iPhone2,1"])    return IPHONE_3GS;
        if ([platform isEqualToString:@"iPhone3,1"])    return IPHONE_4G;
        if ([platform isEqualToString:@"iPod1,1"])      return IPOD_TOUCH_1G;
        if ([platform isEqualToString:@"iPod2,1"])      return IPOD_TOUCH_2G;
        if ([platform isEqualToString:@"iPod3,1"])      return IPOD_TOUCH_3G;
        if ([platform isEqualToString:@"iPod4,1"])      return IPOD_TOUCH_4G;
        if ([platform isEqualToString:@"iPad1,1"])      return IPAD_2G;
        if ([platform isEqualToString:@"iPhone4,1"])    return IPHONE_4GS;
		if ([platform hasPrefix:@"iPad3"])		return IPAD_3G;
        if ([platform isEqualToString:@"i386"] || [platform isEqualToString:@"x86_64"])         
            return IPHONE_SIMULATOR;       
    }
    return IOS_HANDSET_UNKNOWN;
}
    
_int32 get_ios_screen_size(_int32 *x,_int32 *y)
{
       NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	if(!is_screen_ok)
	{
        UIScreen* screen = [UIScreen mainScreen];
		CGRect full_screen = [screen applicationFrame];		
        BOOL is_scale = [screen respondsToSelector:@selector(scale)];
		CGFloat scale = screen.scale;
		UIDevice *myDevice = [UIDevice currentDevice];
		UIDeviceOrientation deviceOrientation = [myDevice orientation];	
                
		if (get_ios_hardware_type() == IPAD_3G)
		{
			scale = 2.0f;
		}
		//
		if ( deviceOrientation == UIDeviceOrientationPortrait || deviceOrientation == UIDeviceOrientationPortraitUpsideDown)
		{
			if (is_scale) {
				screen_x= full_screen.size.width * scale;
				screen_y= full_screen.size.height * scale;
			}
		}
		else 
		{
			if (is_scale) {
				screen_x= full_screen.size.height * scale;
				screen_y= full_screen.size.width * scale;
			}		
		}

		
		is_screen_ok = TRUE;
	}
	
	if(x)
		*x= screen_x;
	if(y)
		*y = screen_y;

	[pool drain];
	return SUCCESS;
}

// 0: active; 1: inactive; 2: background
_int32 get_current_application_status(void)
{
    switch ([UIApplication sharedApplication].applicationState) 
    {
        case UIApplicationStateActive:
            return 0;
            break;
        case UIApplicationStateInactive:
            return 1;
            break;
        case UIApplicationStateBackground:
            return 2;
            break;
            
        default:
            break;
    };
    return 0;
}
    
const char * get_ios_imei(void)
{
//	NetworkController *ntc = [NetworkController sharedInstance];
//	NSString *imeistring = [ntc IMEI];
	static char s_imei[IMEI_LEN + 1] = "123456789012345";
	return s_imei;
}

_int32 check_network_connect(void)
{
    struct sockaddr_in zeroAddress;
    bzero(&zeroAddress, sizeof(zeroAddress));
    zeroAddress.sin_len = sizeof(zeroAddress);
    zeroAddress.sin_family = AF_INET;

   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
    SCNetworkReachabilityRef defaultRouteReachability = SCNetworkReachabilityCreateWithAddress(NULL, (struct sockaddr *)&zeroAddress);
    SCNetworkReachabilityFlags flags;
    
    BOOL didRetrieveFlags = SCNetworkReachabilityGetFlags(defaultRouteReachability, &flags);
    CFRelease(defaultRouteReachability);
    
    if (!didRetrieveFlags)
    {
        NSLog(@"Error. we lost the network connected");
        [pool drain];		
        return 0;
    }
    
    BOOL isReachable = flags & kSCNetworkFlagsReachable;
    BOOL needsConnection = flags & kSCNetworkFlagsConnectionRequired;
    [pool drain];
    return (isReachable&&!needsConnection)?1:0;
}
const char * get_ios_software_version(void)
{
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		if(sd_strlen(ios_version)==0)
		{
			NSString*  systemVersion=[[UIDevice currentDevice] systemVersion]; //系统版本号
			NSString*  model=[[UIDevice currentDevice] model];
			
			char *Char_systemVersion = [systemVersion UTF8String];
			char *Char_model = [model UTF8String];
			
			sd_memset(ios_version,0x00,64);
	    sd_snprintf(ios_version,63, "%s_%s",Char_model,Char_systemVersion);
		}

		[pool drain];
		return ios_version;
}
const char * get_ios_hardware_version(void)
{
      NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		if(sd_strlen(hardware_version)==0)
		{
			NSString*  model=[[UIDevice currentDevice] model];
			
			char *Char_model = [model UTF8String];

			unsigned long long  physicalMemory= [[NSProcessInfo processInfo] physicalMemory];
			int hver = 1;
			
			if(physicalMemory>300000000)
			{
				hver = 2;
			}
			
			sd_memset(hardware_version,0x00,64);
	    sd_snprintf(hardware_version,63, "%s%d",Char_model,hver);
		}
		[pool drain];
		return hardware_version;
}
const char * get_ios_name(void)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	if(sd_strlen(ios_name)==0)
	{
		//NSString*  deviceName= [[UIDevice currentDevice] uniqueIdentifier]; //唯一标识
		//NSString*  localizedModel=[[UIDevice currentDevice] localizedModel];
		//NSString*  systemVersion=[[UIDevice currentDevice] systemVersion]; //系统版本号
		//NSString*  systemName=[[UIDevice currentDevice] systemName]; //系统名字
		//NSString*  model=[[UIDevice currentDevice] model];
		NSString*  name=[[UIDevice currentDevice] name];

		//NSString*  hostName= [[NSProcessInfo processInfo] hostName];
		//NSString*  operatingSystemName= [[NSProcessInfo processInfo] operatingSystemName];
		//NSString*  operatingSystemVersionString= [[NSProcessInfo processInfo] operatingSystemVersionString];

		//unsigned long long  physicalMemory= [[NSProcessInfo processInfo] physicalMemory];

		//char *Char_deviceName = [deviceName UTF8String];
		//char *Char_localizedModel = [localizedModel UTF8String];
		//char *Char_systemVersion = [systemVersion UTF8String];
		//char *Char_systemName = [systemName UTF8String];
		//char *Char_model = [model UTF8String];
		char *Char_name = [name UTF8String];
		
		//char *Char_hostName = [hostName UTF8String];
		//char *Char_operatingSystemName = [operatingSystemName UTF8String];
		//char *Char_operatingSystemVersionString = [operatingSystemVersionString UTF8String];
		

		sd_memset(ios_name,0x00,64);
        sd_snprintf(ios_name,63, "%s",Char_name);
	}
	[pool drain];
	return ios_name;
}

const char * get_app_home_path(void)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(sd_strlen(app_root_path)==0)
	{
        sd_memset(app_root_path, 0, APP_ROOT_PATH_LEN);
        const char* pHome = [NSHomeDirectory() cStringUsingEncoding:NSUTF8StringEncoding];
        sd_memcpy(app_root_path, pHome, sd_strlen(pHome));
    }
    [pool drain];
    return app_root_path;
}
//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif


#endif /* win32 */
