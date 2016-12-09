#ifndef _EMULE_TAG_H_
#define _EMULE_TAG_H_

#ifdef ENABLE_EMULE

#include "utility/define.h"

#define	MAX_TAG_NAME_SIZE		5		
#define	TAGTYPE_INVALID		    0X00
#define	TAGTYPE_HASH			0x01
#define	TAGTYPE_STRING		    0x02
#define	TAGTYPE_UINT32		    0x03
#define	TAGTYPE_FLOAT32		    0x04
#define	TAGTYPE_BOOL			0x05
#define	TAGTYPE_BOOLARRAY	    0x06
#define	TAGTYPE_BLOB			0x07
#define	TAGTYPE_UINT16		    0x08
#define	TAGTYPE_UINT8			0x09
#define	TAGTYPE_BSOB			0x0A
#define	TAGTYPE_UINT64			0x0B

#define TAGTYPE_STR1			0x11
#define TAGTYPE_STR2			0x12
#define TAGTYPE_STR3			0x13
#define TAGTYPE_STR4			0x14
#define TAGTYPE_STR5			0x15
#define TAGTYPE_STR6			0x16
#define TAGTYPE_STR7			0x17
#define TAGTYPE_STR8			0x18
#define TAGTYPE_STR9			0x19
#define TAGTYPE_STR10			0x1A
#define TAGTYPE_STR11			0x1B
#define TAGTYPE_STR12			0x1C
#define TAGTYPE_STR13			0x1D
#define TAGTYPE_STR14			0x1E
#define TAGTYPE_STR15			0x1F
#define TAGTYPE_STR16			0x20
#define TAGTYPE_STR17			0x21
#define TAGTYPE_STR18			0x22
#define TAGTYPE_STR19			0x23
#define TAGTYPE_STR20			0x24
#define TAGTYPE_STR21			0x25
#define TAGTYPE_STR22			0x26

#define	CT_NAME					"\x01"
#define	CT_VERSION					"\x11"
#define	CT_SERVER_FLAGS			"\x20"
#define	CT_MOD_VERSION			"\x55"
#define	CT_EMULE_UDPPORTS		"\xf9"
#define	CT_EMULE_MISCOPTIONS1	"\xfa"
#define	CT_EMULE_MISCOPTIONS2	"\xfe"
#define	CT_EMULE_VERSION			"\xfb"

#define	ET_COMPRESSION			"\x20"
#define	ET_UDPPORT					"\x21"
#define	ET_UDPVER					"\x22"
#define	ET_SOURCEEXCHANGE		"\x23"
#define	ET_COMMENTS				"\x24"
#define	ET_EXTENDEDREQUEST		"\x25"
#define	ET_FEATURES				"\x27"

#define	FT_THUNDERPEERID			"\xd2"	 // <string> 迅雷peer id

//kad tag
#define	TAG_SOURCEPORT			"\xFD"	// <uint16>
#define	TAG_SOURCEIP				"\xFE"	// <uint32>


// values for CT_SERVER_FLAGS (server capabilities)
#define	SRVCAP_ZLIB				0x0001
#define	SRVCAP_NEWTAGS			0x0008
#define	SRVCAP_UNICODE			0x0010
#define	SRVCAP_LARGEFILES			0x0100
#define	SRVCAP_SUPPORTCRYPT     	0x0200
#define	SRVCAP_REQUESTCRYPT		0x0400
#define	SRVCAP_REQUIRECRYPT		0x0800

// server.met
#define	ST_SERVERNAME				"\x01"	// <string>
#define	ST_UDPFLAGS				"\x92"	// <uint32>
#define	ST_TCPPORTOBFUSCATION	"\x97"	// <uint16>


#define	EDONKEYVERSION			0x3C

typedef struct tagEMULE_TAG
{
	char _name[MAX_TAG_NAME_SIZE + 1];		//加上最后一个结束字符\0
	_u8 _tag_type;
	union
	{
		char* _val_str;
		_u32 _val_u32;
		_u16 _val_u16;
		_u8 _val_int8;
        _u64 _val_u64;
        BOOL _val_bool;
	}_value;
}EMULE_TAG;

_int32 emule_create_str_tag(EMULE_TAG** tag, char* name, char* value);
_int32 emule_create_u64_tag(EMULE_TAG** tag, char* name, _u64 value);
_int32 emule_create_u32_tag(EMULE_TAG** tag, char* name, _u32 value);
_int32 emule_create_u16_tag(EMULE_TAG** tag, char* name, _u16 value);
_int32 emule_create_u8_tag(EMULE_TAG** tag, char* name, _u8 value);

_int32 emule_destroy_tag(EMULE_TAG* tag);

//该函数获得把tag存到buffer中需要的buffer_size大小
_u32 emule_tag_size(EMULE_TAG* tag);

BOOL emule_tag_is_u32(const EMULE_TAG* tag);

BOOL emule_tag_is_str(const EMULE_TAG* tag);

_int32 emule_tag_from_buffer(EMULE_TAG** pp_tag, char** buffer, _int32* len);
_int32 emule_tag_to_buffer(EMULE_TAG* tag, char** buffer, _int32* len);
#endif /* ENABLE_EMULE */

#endif


