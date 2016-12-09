#include "utility/define.h"
#ifdef ENABLE_EMULE
/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/14
-----------------------------------------------------------------------------------------------------------*/
#define	EMULE_PROTOCOL			0x01
#define	OP_EDONKEYHEADER		0xE3
#define	OP_EDONKEYPROT			OP_EDONKEYHEADER
#define	OP_EMULEPROT			0xC5
#define	OP_PACKEDPROT			0xD4



//////////////////////////////////////////////////////////////////////////
// 客户端与客户端通讯命令(TCP) edonkey标准协议
//////////////////////////////////////////////////////////////////////////
#define	OP_HELLO				0x01	// 0x10<HASH 16><ID 4><PORT 2><1 Tag_set>
#define	OP_SENDINGPART			0x46	// <HASH 16><von 4><bis 4><Daten len:(von-bis)>
#define	OP_REQUESTPARTS			0x47	// <HASH 16><von[3] 4*3><bis[3] 4*3>
#define	OP_FILEREQANSNOFIL		0x48	// <HASH 16>
#define	OP_HELLOANSWER			0x4C	// <HASH 16><ID 4><PORT 2><1 Tag_set><SERVER_IP 4><SERVER_PORT 2>
#define	OP_SETREQFILEID			0x4F	// <HASH 16>
#define	OP_FILESTATUS			0x50	// <HASH 16><count 2><status(bit array) len:((count+7)/8)>
#define	OP_HASHSETREQUEST		0x51	// <HASH 16>
#define	OP_HASHSETANSWER		0x52	// <count 2><HASH[count] 16*count>
#define	OP_STARTUPLOADREQ		0x54	// <HASH 16>
#define	OP_ACCEPTUPLOADREQ		0x55	// (null)
#define	OP_CANCELTRANSFER		0x56	// (null)	
#define	OP_OUTOFPARTREQS		0x57	// (null)
#define	OP_REQUESTFILENAME		0x58	// <HASH 16>	(more correctly file_name_request)
#define	OP_REQFILENAMEANSWER	0x59	// <HASH 16><len 2><NAME len>
#define	OP_QUEUERANK			0x5C	// <wert  4> (slot index of the request)
#define	OP_ASKSHAREDDIRS		0x5D	// (null)


//////////////////////////////////////////////////////////////////////////
// 客户端与客户端通讯(TCP) emule扩展协议
//////////////////////////////////////////////////////////////////////////
#define	OP_EMULEINFO			0x01	
#define	OP_EMULEINFOANSWER		0x02
//#define	OP_COMPRESSEDPART	0x40	// <HASH 16><von 4><size 4><Daten len:size>
#define	OP_QUEUERANKING			0x60	// <RANG 2>
#define	OP_REQUESTSOURCES		0x81	// <HASH 16>
#define	OP_ANSWERSOURCES		0x82	//
#define	OP_SECIDENTSTATE		0x87	// <state 1><rndchallenge 4>
#define	OP_SENDINGPART_I64		0xA2	// <HASH 16><von 8><bis 8><Daten len:(von-bis)>
#define	OP_REQUESTPARTS_I64		0xA3	// <HASH 16><von[3] 8*3><bis[3] 8*3>
#define	OP_SECALLBACK			0xF8	// <HASH 16><uint32><uint16>
#define	OP_TCPSENATREQ          0xFB	// <HASH 16><uint32><uint16>
#define OP_AICHFILEHASHANS		0x9D	
#define OP_AICHFILEHASHREQ		0x9E    


//////////////////////////////////////////////////////////////////////////
// 客户端与客户端通讯(UDP) 协议
//////////////////////////////////////////////////////////////////////////
#define	OP_REASKFILEPING		0x90	// <HASH 16>
#define	OP_REASKACK				0x91	// <RANG 2>
#define	OP_UDPTOUDPSENATREQ		0xF7	// <HASH 16><uint32><uint16>


//////////////////////////////////////////////////////////////////////////
// 客户端服务器通讯命令(TCP)
// client <-> server
//////////////////////////////////////////////////////////////////////////
#define	OP_LOGINREQUEST			0x01	//<HASH 16><ID 4><PORT 2><1 Tag_set>
#define	OP_GETSOURCES			0x19	// <HASH 16>
#define	OP_CALLBACKREQUEST		0x1C	// <ID 4>
#define	OP_GETSOURCES_OBFU		0x23
#define	OP_SERVERSTATUS			0x34	// <USER 4><FILES 4>
#define	OP_CALLBACKREQUESTED	0x35	// <IP 4><PORT 2>
#define	OP_SERVERMESSAGE		0x38	// <len 2><Message len>
#define	OP_IDCHANGE				0x40	// <NEW_ID 4><server_flags 4><primary_tcp_port 4 (unused)><client_IP_address 4>
#define	OP_FOUNDSOURCES			0x42	// <HASH 16><count 1>(<ID 4><PORT 2>)[count]
#define	OP_FOUNDSOURCES_OBFU	0x44    // <HASH 16><count 1>(<ID 4><PORT 2><obf settings 1>(UserHash16 if obf&0x08))[count]




//////////////////////////////////////////////////////////////////////////
// 客户端服务器通讯命令(UDP)
//client <-> UDP server
//////////////////////////////////////////////////////////////////////////
#define	OP_GLOBGETSOURCES		0x9A	// <HASH 16>
#define	OP_GLOBGETSOURCES2		0x94	// <HASH 16><FILESIZE 4>
#define	OP_GLOBFOUNDSOURCES		0x9B	//


//////////////////////////////////////////////////////////////////////////
// 客户端udt 协议
//////////////////////////////////////////////////////////////////////////
#define	OP_VC_NAT_HEADER		0xF1
#define	OP_NAT_SYNC				0xE1
#define	OP_NAT_PING				0xE2
#define	OP_NAT_REGISTER			0xE4
#define	OP_NAT_FAILED			0xE5
#define	OP_NAT_REPING			0xE8
#define	OP_NAT_SYNC2			0xE9
#define	OP_NAT_DATA				0xEA	// data segment with a part of the stream
#define	OP_NAT_ACK				0xEB	// data segment acknowledgement
#define	OP_NAT_RST				0xEF	// connection closed

#endif /* ENABLE_EMULE */

