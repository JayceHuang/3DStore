#ifndef SD_DEFINE_H_00138F8F2E70_200806111929
#define SD_DEFINE_H_00138F8F2E70_200806111929

#ifdef HAVE_CONFIG_H
//#include "config.h"
#endif

#ifdef __cplusplus
extern "C" 
{
#endif

typedef unsigned char		_u8;
typedef unsigned short		_u16;
typedef unsigned int		_u32;

#ifndef WINCE
typedef char				_int8;
typedef short				_int16;
typedef int				_int32;
#endif

/****************** _u64  *********************/

 typedef unsigned long long	_u64;

/****************** End of _u64  *********************/


/****************** _int64  *********************/
#include <stdint.h>
#if defined(LINUX)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
typedef long long			_int64;
#elif defined(__SYMBIAN32__)
#include <e32def.h>
#ifndef _int64
typedef Int64			_int64;
#endif
#elif defined(WINCE)
#elif  defined(AMLOS)
#ifdef  NOT_SUPPORT_LARGE_INT_64 
    typedef		int		_int64;
#else
    typedef 	long long	_int64;
#endif
#elif  defined(MSTAR)||defined(SUNPLUS)
#ifdef  NOT_SUPPORT_LARGE_INT_64 
    typedef		int		_int64;
#else
    typedef 	long long	_int64;
#endif
#endif
/****************** End of _int64  *********************/

/****************** BOOL  *********************/
#ifdef MACOS
#ifdef __OBJC__
#import <Foundation/Foundation.h>
    #ifdef MOBILE_PHONE
        #import <UIKit/UIKit.h>
    #elif defined(OSX)
        #import <AppKit/AppKit.h>
    #endif
#endif

#ifndef __OBJC__
#ifndef TRUE
typedef _int32		BOOL;
#define TRUE	(1)
#define FALSE	(0)
#else 
    typedef _int32 BOOL;
#endif
#endif
//#endif /* OBJC_BOOL_DEFINED */
#else
#ifndef TRUE
typedef _int32		BOOL;
#define TRUE	(1)
#define FALSE	(0)
#else
#if defined(WINCE)
typedef _int32		BOOL;
#endif
#endif
#endif
/****************** End of BOOL  *********************/


/****************** NULL  *********************/
#ifndef NULL
#define NULL	((void*)(0))
#endif
/****************** End of NULL  *********************/


/****************** Compiling macro   *********************/
    
#define ENABLE_CDN 
#define ENABLE_COOKIES
#define ENABLE_LIXIAN
//#define _ENABLE_SSL
//#define EMBED_REPORT    
#define FAST_STOP_TASK  
#define _WRITE_BUFFER_MERGE  
//#define _USE_OS_MEM_ONLY  

#ifndef _USE_GLOBAL_MEM_POOL
#define _USE_GLOBAL_MEM_POOL  
#endif

#ifdef _DEBUG
#ifndef _LOGGER
#define _LOGGER   
#endif
#endif

//#define ENABLE_CHECK_NETWORK     /* �Զ��������״̬�Ƿ�����,�������,����ͣ���е�pipe,����ϵͳѹ��,��ֹ���������� */

#ifdef MOBILE_PHONE
	/**** �ֻ�Ѹ�ס�iPad������AndoridѸ��  ******/
	#ifndef _EXTENT_MEM_FROM_OS
	#define _EXTENT_MEM_FROM_OS  
	#endif
	
	// �����ϱ�ʹ��ѹ��
	#define ENABLE_ZIP    
	#define ENABLE_EMBED_REPORT_GZIP  
	
	#ifdef _ANDROID_LINUX
		
		#if defined(CLOUD_PLAY_PROJ)
			#define _USE_OS_MEM_ONLY 
			#define _MEMPOOL_10M
			
		#else
#error Compile error, must define PROJ1
		#endif /*  */
	#endif /* _ANDROID_LINUX */
	
	#ifdef MACOS
#error Compile error, must define OS
		#if defined(LIXIAN_PROJ)||defined(LIXIAN_IPHONE_PROJ)
			#define ENABLE_VOD  
			#define _VOD_NO_DISK
			#define _USE_OS_MEM_ONLY 
			#define ENABLE_MEMBER
			#define ENABLE_LIXIAN
			#define ENABLE_LX_XML_ZIP		
			#define ENABLE_BT  
			#define ENABLE_EMULE  
			#define _DK_QUERY  
			#define ENABLE_BT_PROTOCOL   
			#define ENABLE_EMULE_PROTOCOL  

			#define ENABLE_SNIFF_URL  
			#ifdef ENABLE_SNIFF_URL
				#define ENABLE_REGEX_DETECT/*������ʽ��̽*/
				#define ENABLE_STRING_DETECT/*�ַ�����λ��̽*/
			#endif/*ENABLE_SNIFF_URL*/

			#define ENABLE_LIXIAN_CACHE  
            #define LX_CREATE_LIXIAN_TASK
			#define OUT_OF_DATE   
			#define LX_LIST_CACHE
			#define ENABLE_JSON
			#define LX_GET_TASK_LIST_NEW
			#define ENABLE_LOGIN_REPORT  /*��¼�ϱ�*/
			#define CLOUD_PLAY_PROJ     	/* �Ʋ���Ŀ��� */
		#elif
#error Compile error, must define PROJ2
		#endif
	#endif /*MACOS*/
	
#else
#error Compile error, must define PROJ3
	/******* Ѹ�׺��ӡ�MACѸ��*******************/
	#define ENABLE_BT  
	#define ENABLE_EMULE  
	#define UPLOAD_ENABLE   
	#define _DK_QUERY  
	#define ENABLE_BT_PROTOCOL   
	#define ENABLE_EMULE_PROTOCOL  
	//#define AUTO_LAN   
	//#define ENABLE_DRM  

	#ifdef OSX
		/******* MAC����??****/
	 	#define _EXTENT_MEM_FROM_OS  
	 	#define _USE_OS_MEM_ONLY
        #define ENABLE_NULL_PATH
		
        #define ENABLE_CDN
        #define ENABLE_ZIP
        #define ENABLE_MEMBER
        #define ENABLE_LX_XML_ZIP
        #define ENABLE_BT
        #define ENABLE_EMULE
        #define ENABLE_HSC
        #define _DK_QUERY
        #define ENABLE_BT_PROTOCOL
        #define ENABLE_EMULE_PROTOCOL
        #define ENABLE_LIXIAN
        #define ENABLE_LIXIAN_CACHE
        #define ENABLE_SNIFF_URL
        #ifdef ENABLE_SNIFF_URL
        #define ENABLE_REGEX_DETECT/*��?����������??���C��?��*/
        #define ENABLE_STRING_DETECT/*?�¡�?��????a�C��?��*/
        #endif
		
	 	#ifdef ENABLE_CDN
	  		//#define ENABLE_HSC  
	  		//#define UI_PAY_HSC  
	 	#endif /* ENABLE_CDN */
	#else
	#define ENABLE_BT  
	#define ENABLE_EMULE  
	#define UPLOAD_ENABLE   
	#define _DK_QUERY  
	#define ENABLE_BT_PROTOCOL   
	#define ENABLE_EMULE_PROTOCOL  
	//#define AUTO_LAN   
	//#define ENABLE_DRM  

	 #define DISABLE_QUERY_VIP_HUB
	 #ifdef _DEBUG
	  #define ENABLE_CFG_FILE
	 #endif
        #define ENABLE_CDN
        #define ENABLE_ZIP
        #define ENABLE_MEMBER
        #define ENABLE_LX_XML_ZIP
        #define ENABLE_BT
        #define ENABLE_EMULE
        #define _DK_QUERY
        #define ENABLE_BT_PROTOCOL
        #define ENABLE_EMULE_PROTOCOL
        #define ENABLE_LIXIAN
		#define ENABLE_HSC
        #define ENABLE_LIXIAN_CACHE
        #define ENABLE_SNIFF_URL
        #ifdef ENABLE_SNIFF_URL
        #define ENABLE_REGEX_DETECT
        #define ENABLE_STRING_DETECT
        #endif
	#endif /* MACOS */
#endif /* MOBILE_PHONE */

#ifdef LINUX_TEST
	#define ENABLE_VOD  
	#define _VOD_NO_DISK
	#define ENABLE_HTTP_VOD   /* ����http�㲥������*/

	#ifndef ENABLE_CDN
		#define ENABLE_CDN
	#endif
    #ifndef ENABLE_HSC
        #define ENABLE_HSC  		/* ����ͨ�� */
    #endif
    #ifndef ENABLE_MEMBER
        #define ENABLE_MEMBER
    #endif

    #define ENABLE_KANKAN_CDN
    //#define UPLOAD_ENABLE
	
#endif //LINUX_TEST


#if  defined(ENABLE_EMULE) || defined(ENABLE_BT)
#define _DK_QUERY
#endif

/****** MSG POLL  ********/
#ifdef LINUX
  #ifdef MACOS

#define _KQUEUE   
  #else
    #define _EPOLL
  #endif
#elif defined(WINCE)
  #define _WSA_EVENT_SELECT
#endif /* LINUX */
/**** End of MSG POLL  ******/

/****** DNS_ASYNC  ********/
#ifdef LINUX
  #if   defined(_ANDROID_LINUX)
  	/* ��֧���첽DNS */
  #elif  defined(MACOS)
  	//#ifdef _DEBUG
	//#define DNS_ASYNC  
	//#endif
  #else
    //#define DNS_ASYNC  
  #endif
#endif /* LINUX */
/**** End of DNS_ASYNC  ******/

#define SD_UNUSED( var )

#define IMPORT_C 
/****************** End of compiling macro   *********************/

/****************** Momery  *********************/
#if  defined(_MEMPOOL_10M)
#define  MEMPOOL_10M
#elif defined(_MEMPOOL_8M)
#define  MEMPOOL_8M
#elif defined(_MEMPOOL_5M)
#define  MEMPOOL_5M
#elif defined(_MEMPOOL_3M)
#define  MEMPOOL_3M
#else
#if defined( MOBILE_PHONE)
#ifdef IPAD_KANKAN
#define  MEMPOOL_5M  
#else
#define  MEMPOOL_1M 
#endif
#else
#define  MEMPOOL_10M
#endif
#endif
/****************** End of Momery  *********************/


/****************** Constant  *********************/
/* timeout */
#define WAIT_INFINITE		(0xFFFFFFFF)
#define INFO_HASH_LEN 20
#define BT_PEER_ID_LEN 20

#ifdef AMLOS
#define WAIT_INTERVAL  (1)
#elif defined(WINCE)
#define WAIT_INTERVAL  (1)
#elif defined(MACOS)
#define WAIT_INTERVAL  (10)
#else
#define WAIT_INTERVAL  (10)
#endif

#ifdef _CONNECT_DETAIL
#define SERVER_INFO_NUM 5
#define PEER_INFO_NUM 10
#endif

#define MAX_U32 (0xFFFFFFFF)

#define SD_ERROR   (-1)

#define MAX_FILE_NAME_LENGTH    255
#define MAX_RELATION_FILE_NUM   (100)
typedef struct tagRELATION_BLOCK_INFO
{
	BOOL _block_info_valid;
	_u64 _origion_start_pos;
	_u64 _relation_start_pos;
	_u64 _length;
}RELATION_BLOCK_INFO;
/****************** End of constant  *********************/
#include "utility/define_const_num.h"

#ifdef __cplusplus
}
#endif
#endif
