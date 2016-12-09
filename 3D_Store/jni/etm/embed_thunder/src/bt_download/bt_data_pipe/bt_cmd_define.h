/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/27
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_CMD_DEFINE_H_
#define _BT_CMD_DEFINE_H_
#ifdef __cplusplus
extern "C" 
{
#endif


#define		BT_HANDSHAKE		19		
#define		BT_CHOKE			0
#define		BT_UNCHOKE			1
#define		BT_INTERESTED		2	
#define		BT_NOT_INTERESTED	3
#define		BT_HAVE				4
#define		BT_BITFIELD			5
#define		BT_REQUEST			6
#define		BT_PIECE			7
#define		BT_CANCEL			8
#define		BT_PORT				9
#define		BT_A0				0xa0
#define		BT_A1				0xa1
#define		BT_A2				0xa2
#define		BT_A3				0xa3
#define		BT_A4				0xa4
#define		BT_AC				0xac			/*这个也是个扩展命令，不知是哪个BT版本的*/
#define		BT_FA				0xfa
#define		BT_FB				0xfb
#define		BT_KEEPALIVE		0xff			/*这个命令是自己扩张的，仅仅是编程上的需要*/
#define		BT_MAGNET		    0x14

#define		BT_PROTOCOL_STRING		"BitTorrent protocol"
#define		BT_PROTOCOL_STRING_LEN	19

#define		BT_HANDSHAKE_PRO_LEN	68

#ifdef __cplusplus
}
#endif

#endif

