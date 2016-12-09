/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
 *  Copyright (C) 2001-2003  Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>

#include "utility/sha1.h"
#include "utility/string.h"
#include "platform/sd_mem.h"



#define GET_UINT32(n,b,i)                       \
{                                               \
    (n) = ( (unsigned int) (b)[(i)    ] << 24 )       \
        | ( (unsigned int) (b)[(i) + 1] << 16 )       \
        | ( (unsigned int) (b)[(i) + 2] <<  8 )       \
        | ( (unsigned int) (b)[(i) + 3]       );      \
}

#define PUT_UINT32(n,b,i)                       \
{                                               \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}



void sha1_initialize(ctx_sha1* p_ctx)
{
    p_ctx->_count[0] = 0;
    p_ctx->_count[1] = 0;

    p_ctx->_state[0] = 0x67452301;
    p_ctx->_state[1] = 0xEFCDAB89;
    p_ctx->_state[2] = 0x98BADCFE;
    p_ctx->_state[3] = 0x10325476;
    p_ctx->_state[4] = 0xC3D2E1F0;

	sd_memset(p_ctx->_inner_data, 0, sizeof(p_ctx->_inner_data));
}

void sha1_handle(unsigned int *state, const unsigned char data[64])
{
    unsigned int temp, W[16], A, B, C, D, E;

    GET_UINT32(W[ 0], data,  0);
    GET_UINT32(W[ 1], data,  4);
    GET_UINT32(W[ 2], data,  8);
    GET_UINT32(W[ 3], data, 12);
    GET_UINT32(W[ 4], data, 16);
    GET_UINT32(W[ 5], data, 20);
    GET_UINT32(W[ 6], data, 24);
    GET_UINT32(W[ 7], data, 28);
    GET_UINT32(W[ 8], data, 32);
    GET_UINT32(W[ 9], data, 36);
    GET_UINT32(W[10], data, 40);
    GET_UINT32(W[11], data, 44);
    GET_UINT32(W[12], data, 48);
    GET_UINT32(W[13], data, 52);
    GET_UINT32(W[14], data, 56);
    GET_UINT32(W[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                                            \
(                                                       \
    temp = W[(t -  3) & 0x0F] ^ W[(t - 8) & 0x0F] ^     \
           W[(t - 14) & 0x0F] ^ W[ t      & 0x0F],      \
    ( W[t & 0x0F] = S(temp,1) )                         \
)

#define P(a, b, c, d, e, x)                             \
{                                                       \
    e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);        \
}

    A = state[0];
    B = state[1];
    C = state[2];
    D = state[3];
    E = state[4];

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

    P(A, B, C, D, E, W[ 0]);
    P(E, A, B, C, D, W[ 1]);
    P(D, E, A, B, C, W[ 2]);
    P(C, D, E, A, B, W[ 3]);
    P(B, C, D, E, A, W[ 4]);
    P(A, B, C, D, E, W[ 5]);
    P(E, A, B, C, D, W[ 6]);
    P(D, E, A, B, C, W[ 7]);
    P(C, D, E, A, B, W[ 8]);
    P(B, C, D, E, A, W[ 9]);
    P(A, B, C, D, E, W[10]);
    P(E, A, B, C, D, W[11]);
    P(D, E, A, B, C, W[12]);
    P(C, D, E, A, B, W[13]);
    P(B, C, D, E, A, W[14]);
    P(A, B, C, D, E, W[15]);
    P(E, A, B, C, D, R(16));
    P(D, E, A, B, C, R(17));
    P(C, D, E, A, B, R(18));
    P(B, C, D, E, A, R(19));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0x6ED9EBA1

    P(A, B, C, D, E, R(20));
    P(E, A, B, C, D, R(21));
    P(D, E, A, B, C, R(22));
    P(C, D, E, A, B, R(23));
    P(B, C, D, E, A, R(24));
    P(A, B, C, D, E, R(25));
    P(E, A, B, C, D, R(26));
    P(D, E, A, B, C, R(27));
    P(C, D, E, A, B, R(28));
    P(B, C, D, E, A, R(29));
    P(A, B, C, D, E, R(30));
    P(E, A, B, C, D, R(31));
    P(D, E, A, B, C, R(32));
    P(C, D, E, A, B, R(33));
    P(B, C, D, E, A, R(34));
    P(A, B, C, D, E, R(35));
    P(E, A, B, C, D, R(36));
    P(D, E, A, B, C, R(37));
    P(C, D, E, A, B, R(38));
    P(B, C, D, E, A, R(39));

#undef K
#undef F

#define F(x,y,z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

    P(A, B, C, D, E, R(40));
    P(E, A, B, C, D, R(41));
    P(D, E, A, B, C, R(42));
    P(C, D, E, A, B, R(43));
    P(B, C, D, E, A, R(44));
    P(A, B, C, D, E, R(45));
    P(E, A, B, C, D, R(46));
    P(D, E, A, B, C, R(47));
    P(C, D, E, A, B, R(48));
    P(B, C, D, E, A, R(49));
    P(A, B, C, D, E, R(50));
    P(E, A, B, C, D, R(51));
    P(D, E, A, B, C, R(52));
    P(C, D, E, A, B, R(53));
    P(B, C, D, E, A, R(54));
    P(A, B, C, D, E, R(55));
    P(E, A, B, C, D, R(56));
    P(D, E, A, B, C, R(57));
    P(C, D, E, A, B, R(58));
    P(B, C, D, E, A, R(59));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0xCA62C1D6

    P(A, B, C, D, E, R(60));
    P(E, A, B, C, D, R(61));
    P(D, E, A, B, C, R(62));
    P(C, D, E, A, B, R(63));
    P(B, C, D, E, A, R(64));
    P(A, B, C, D, E, R(65));
    P(E, A, B, C, D, R(66));
    P(D, E, A, B, C, R(67));
    P(C, D, E, A, B, R(68));
    P(B, C, D, E, A, R(69));
    P(A, B, C, D, E, R(70));
    P(E, A, B, C, D, R(71));
    P(D, E, A, B, C, R(72));
    P(C, D, E, A, B, R(73));
    P(B, C, D, E, A, R(74));
    P(A, B, C, D, E, R(75));
    P(E, A, B, C, D, R(76));
    P(D, E, A, B, C, R(77));
    P(C, D, E, A, B, R(78));
    P(B, C, D, E, A, R(79));

#undef K
#undef F

    state[0] += A;
    state[1] += B;
    state[2] += C;
    state[3] += D;
    state[4] += E;
}

void sha1_update(ctx_sha1* p_ctx,const unsigned char *pdata, unsigned int count)
{
      unsigned int left,fill;
	  
       if(0 < count)
	{
	       left = p_ctx->_count[0] & 0x3F;
		fill = 64 - left;

		p_ctx->_count[0] += count;
		p_ctx->_count[0] &= 0xFFFFFFFF;

		if(p_ctx->_count[0] < count) p_ctx->_count[1] ++;

		if(0 != left && count >= fill)
		{
			sd_memcpy(p_ctx->_inner_data + left, pdata, fill);
			sha1_handle(p_ctx->_state, p_ctx->_inner_data);
			count -= fill;
			pdata += fill;
			left = 0;
		}

		while(64 <= count)
		{
			sha1_handle(p_ctx->_state, pdata);
			count -= 64;
			pdata += 64;
		}

		if(0 != count)
		{
			sd_memcpy(p_ctx->_inner_data + left, pdata, count);
		}
       }
}


void sha1_finish(ctx_sha1* p_ctx,unsigned char cid[20])
{
	
	static unsigned char PADDING[64] =
	{
		0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
    unsigned char ml[8];	
    unsigned int hi = ( p_ctx->_count[0] >> 29 ) | ( p_ctx->_count[1] <<  3 );
    unsigned int lo = ( p_ctx->_count[0] <<  3 );
    unsigned int last,padn; 	

    PUT_UINT32(hi, ml, 0);
    PUT_UINT32(lo, ml, 4);

    last = p_ctx->_count[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    sha1_update(p_ctx,PADDING, padn);
    sha1_update(p_ctx,ml, 8);

    PUT_UINT32(p_ctx->_state[0], cid,  0);
    PUT_UINT32(p_ctx->_state[1], cid,  4);
    PUT_UINT32(p_ctx->_state[2], cid,  8);
    PUT_UINT32(p_ctx->_state[3], cid, 12);
    PUT_UINT32(p_ctx->_state[4], cid, 16);

	/* Zeroize sensitive information.	*/
	sha1_initialize(p_ctx);
}

