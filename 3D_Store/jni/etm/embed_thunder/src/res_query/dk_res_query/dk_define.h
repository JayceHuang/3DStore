/*****************************************************************************
 *
 * Filename: 			dk_define.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/12/16
 *	
 * Purpose:				define the dht and kad control parameters
 *
 *****************************************************************************/
	 

#if !defined(__DK_DEFINE_20091216)
#define __DK_DEFINE_20091216

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

#ifdef _DK_QUERY
//////////////////////////////////////////////////////////////////////////
#define OP_EDONKEYHEADER		0xE3
#define OP_KADEMLIAHEADER		0xE4
#define OP_KADEMLIAPACKEDPROT	0xE5
#define OP_EDONKEYPROT			OP_EDONKEYHEADER
#define OP_PACKEDPROT			0xD4
#define OP_EMULEPROT			0xC5
#define OP_UDPRESERVEDPROT1		0xA3	// reserved for later UDP headers (important for EncryptedDatagramSocket)
#define OP_UDPRESERVEDPROT2		0xB2	// reserved for later UDP headers (important for EncryptedDatagramSocket)
#define OP_MLDONKEYPROT			0x00
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// KADÍøÂçÍ¨Ñ¶ÃüÁî
// KADEMLIA (opcodes) (udp)
//////////////////////////////////////////////////////////////////////////

#define KADEMLIA_BOOTSTRAP_REQ			0x00	// <PEER (sender) [25]>
#define KADEMLIA2_BOOTSTRAP_REQ			0x01	//

#define KADEMLIA_BOOTSTRAP_RES			0x08	// <CNT [2]> <PEER [25]>*(CNT)
#define KADEMLIA2_BOOTSTRAP_RES			0x09	//

#define KADEMLIA_HELLO_REQ	 			0x10	// <PEER (sender) [25]>
#define KADEMLIA2_HELLO_REQ				0x11	//

#define KADEMLIA_HELLO_RES     			0x18	// <PEER (receiver) [25]>
#define KADEMLIA2_HELLO_RES				0x19	//

#define KADEMLIA_REQ		   			0x20	// <TYPE [1]> <HASH (target) [16]> <HASH (receiver) 16>
#define KADEMLIA2_REQ					0x21	//

#define KADEMLIA_RES					0x28	// <HASH (target) [16]> <CNT> <PEER [25]>*(CNT)
#define KADEMLIA2_RES					0x29	//

#define KADEMLIA_SEARCH_REQ				0x30	// <HASH (key) [16]> <ext 0/1 [1]> <SEARCH_TREE>[ext]
//#define UNUSED						0x31	// Old Opcode, don't use.
#define KADEMLIA_SEARCH_NOTES_REQ		0x32	// <HASH (key) [16]>
#define KADEMLIA2_SEARCH_KEY_REQ		0x33	//
#define KADEMLIA2_SEARCH_SOURCE_REQ		0x34	//
#define KADEMLIA2_SEARCH_NOTES_REQ		0x35	//

#define KADEMLIA_SEARCH_RES				0x38	// <HASH (key) [16]> <CNT1 [2]> (<HASH (answer) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
//#define UNUSED						0x39	// Old Opcode, don't use.
#define KADEMLIA_SEARCH_NOTES_RES		0x3A	// <HASH (key) [16]> <CNT1 [2]> (<HASH (answer) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
#define KADEMLIA2_SEARCH_RES			0x3B	//

#define KADEMLIA_PUBLISH_REQ			0x40	// <HASH (key) [16]> <CNT1 [2]> (<HASH (target) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
//#define UNUSED						0x41	// Old Opcode, don't use.
#define KADEMLIA_PUBLISH_NOTES_REQ		0x42	// <HASH (key) [16]> <HASH (target) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
#define	KADEMLIA2_PUBLISH_KEY_REQ		0x43	//
#define	KADEMLIA2_PUBLISH_SOURCE_REQ	0x44	//
#define KADEMLIA2_PUBLISH_NOTES_REQ		0x45	//

#define KADEMLIA_PUBLISH_RES			0x48	// <HASH (key) [16]>
//#define UNUSED						0x49	// Old Opcode, don't use.
#define KADEMLIA_PUBLISH_NOTES_RES		0x4A	// <HASH (key) [16]>
#define	KADEMLIA2_PUBLISH_RES			0x4B	//

#define KADEMLIA_FIREWALLED_REQ			0x50	// <TCPPORT (sender) [2]>
#define KADEMLIA_FINDBUDDY_REQ			0x51	// <TCPPORT (sender) [2]>
#define KADEMLIA_CALLBACK_REQ			0x52	// <TCPPORT (sender) [2]>

#define KADEMLIA_FIREWALLED_RES			0x58	// <IP (sender) [4]>
#define KADEMLIA_FIREWALLED_ACK_RES		0x59	// (null)
#define KADEMLIA_FINDBUDDY_RES			0x5A	// <TCPPORT (sender) [2]>

// KADEMLIA (parameter)
#define KADEMLIA_FIND_VALUE				0x02
#define KADEMLIA_STORE					0x04
#define KADEMLIA_FIND_NODE				0x0B

//////////////////////////////////////////////////////////////////////////
#define OP_VC_NAT_HEADER		0xf1

#define OP_NAT_SYNC				0xE1
#define OP_NAT_PING				0xE2

#define OP_NAT_REGISTER			0xE4
#define OP_NAT_FAILED			0xE5


#define OP_NAT_REPING			0xE8
#define OP_NAT_SYNC2			0xE9

#define OP_NAT_DATA				0xEA	// data segment with a part of the stream
#define OP_NAT_ACK				0xEB	// data segment acknowledgement

#define OP_NAT_RST				0xEF	// connection closed

// MOD Note: Do not change this part - Merkur
#define	EMULE_PROTOCOL					0x01
// MOD Note: end
#define	EDONKEYVERSION					0x3C
#define KADEMLIA_VERSION1_46c			0x01 /*45b - 46c*/
#define KADEMLIA_VERSION2_47a			0x02 /*47a*/
#define KADEMLIA_VERSION3_47b			0x03 /*47b*/
#define KADEMLIA_VERSION				0x04 /*47c*/
#define PREFFILE_VERSION				0x14	//<<-- last change: reduced .dat, by using .ini
#define PARTFILE_VERSION				0xe0
#define PARTFILE_SPLITTEDVERSION		0xe1
#define PARTFILE_VERSION_LARGEFILE		0xe2

#define CREDITFILE_VERSION		0x12
#define CREDITFILE_VERSION_29	0x11

////////////////////////////////////////////////////////////////////////////
// statistic

#define  FT_USER_COUNT			 0xF4	// <uint32>
#define  FT_FILE_COUNT			 0xF5	// <uint32>

#define  FT_FILESIZE			 0x02	// <uint32> (or <uint64> when supported)
#define  FT_SOURCETYPE			 0xFF	// <uint8>
#define  FT_SOURCEPORT			 0xFD	// <uint16>


// kad find node type
#define KAD_FIND_NODE		        0
#define KAD_FIND_NODECOMPLETE		1
#define KAD_FIND_KEYWORD		    2
#define KAD_FIND_NOTES		    3
#define KAD_FIND_STOREFILE		4
#define KAD_FIND_STOREKEYWORD		5
#define KAD_FIND_STORENOTES		6
#define KAD_FIND_FINDBUDDY		7
#define KAD_FIND_FINDSOURCE		8

// DHT find node type
#define DHT_FIND_SOURCE		        9

#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__DK_DEFINE_20091216)



