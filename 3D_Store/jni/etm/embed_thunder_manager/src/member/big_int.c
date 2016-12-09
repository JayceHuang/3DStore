#include "utility/define.h"

#ifdef ENABLE_MEMBER
#include "member/big_int.h"
#include "em_common/em_define.h"
#include "utility/string.h"
#include "em_common/em_errcode.h"
/****************************************************************************************
大数比较
调用方式：N.Cmp(A)
返回值：若N<A返回-1；若N=A返回0；若N>A返回1
****************************************************************************************/
_int32 cmp(BIG_INT* a, BIG_INT* b)
{
	_int32 i = 0;
	if(a->_len > b->_len) 
		return 1;
	if(a->_len < b->_len)
		return -1;
	for(i = a->_len - 1; i >= 0; --i)
	{
		if(a->_value[i] > b->_value[i])
			return 1;
		if(a->_value[i] < b->_value[i])
			return -1;
	}
	return 0;
}

/****************************************************************************************
大数赋值
调用方式：N.Mov(A)
返回值：无，N被赋值为A
****************************************************************************************/

void mov(BIG_INT* result, BIG_INT* a)
{
	_u32 i = 0;
	result->_len = a->_len;
   	for(i = 0; i < BI_MAXLEN; ++i)
		result->_value[i] = a->_value[i];
}

void mov_by_u64(BIG_INT* result, _u64 a)
{
	_u32 i = 0;
	if(a > 0xffffffff)
	{
		result->_len = 2;
		result->_value[1] = (_u32)(a >> 32);
		result->_value[0] = (_u32)a;
	}
	else
	{
		result->_len = 1;
		result->_value[0] = (_u32)a;
	}
	for(i = result->_len; i < BI_MAXLEN; ++i)
		result->_value[i] = 0;
}



/****************************************************************************************
大数相加
调用形式：N.Add(A)
返回值：N+A
****************************************************************************************/
void add(BIG_INT* result, BIG_INT* a, BIG_INT* b)
{
	_u32 carry = 0, i = 0;
	_u64 sum = 0;
	mov(result, a);
	if(result->_len < b->_len)
		result->_len = b->_len;
	for(i = 0; i < result->_len; ++i)
	{
		sum = b->_value[i];
		sum = sum + result->_value[i] + carry;
		result->_value[i] = (_u32)sum;
		carry = (_u32)(sum >> 32);
	}
	result->_value[result->_len] = carry;
	result->_len += carry;
	return;
}

void add_by_u32(BIG_INT* result, BIG_INT* a, _u32 b)
{
	_u64 sum;
	_u32 i = 0;
	mov(result, a);
	sum = result->_value[0];
	sum += b;
	result->_value[0] = (_u32) sum;
	if(sum > 0xffffffff)
	{
		i = 1;
		while(result->_value[i] == 0xffffffff)
		{
			result->_value[i] = 0;
			++i;
		}
		result->_value[i]++;
		if(a->_len == i)
			a->_len++;
	}
	return;
}

/****************************************************************************************
大数相减
调用形式：N.Sub(A)
返回值：N-A
****************************************************************************************/
void sub(BIG_INT* result, BIG_INT* a, BIG_INT* b)	
{
	_u32 carry = 0, i;
	_u64 num = 0;
	mov(result, a);
	if(cmp(result, b) <= 0)
	{
		mov_by_u64(result, 0);
		return;
	}
	for(i = 0; i < a->_len; ++i)
	{
		if(a->_value[i] > b->_value[i] || ((a->_value[i] == b->_value[i]) && (carry == 0)))
		{
			result->_value[i] = a->_value[i] - carry - b->_value[i];
			carry = 0;
		}
        else
        {
        	num = 0x100000000L + a->_value[i];
		result->_value[i] = (_u32)(num - carry - b->_value[i]);
		carry = 1;
        }
	}
	while(result->_value[result->_len - 1] == 0)
		result->_len--;
	return;
}
	
	
void sub_by_u32(BIG_INT* result, BIG_INT* a, _u32 b)
{
	_u32 i = 0;
	_u64 num = 0;
	mov(result, a);
	if(result->_value[0] >= b)
	{
		result->_value[0] -= b;
		return;
	}
	if(result->_len == 1)
	{
		mov_by_u64(result, 0);
		return;
	}
	num = 0x100000000L + result->_value[0];
	result->_value[0] = (_u32)(num - b);
	i = 1;
	while(result->_value[i] == 0)
	{
		result->_value[i] = 0xffffffff;
		++i;
	}
	--result->_value[i];
	if(result->_value[i] == 0)
		--result->_len;
	return;
}

/****************************************************************************************
大数相乘
调用形式：N.Mul(A)
返回值：N*A
****************************************************************************************/
void mul(BIG_INT* result, BIG_INT* a, BIG_INT* b)
{
	_u64 sum = 0, mul = 0, carry = 0;
	_u32 i = 0, j = 0;
	if(b->_len == 1)
	{
		mul_by_u32(result, a, b->_value[0]);
		return;
	}
	result->_len = a->_len + b->_len - 1;
	for(i = 0; i < result->_len; ++i)
	{
		sum = carry;
		carry = 0;
		for(j = 0; j < b->_len; ++j)
		{
			if(i - j >= 0 && i - j < a->_len)
			{
				mul = a->_value[i - j];
				mul *= b->_value[j];
				carry += mul >> 32;
				mul = mul & 0xffffffff;
				sum += mul;
			}
		}
		carry += sum >> 32;
		result->_value[i] = (_u32)sum;
	}
	if(carry)
	{
		++result->_len;
		result->_value[result->_len - 1] = (_u32)carry;
	}
	return;
}

void mul_by_u32(BIG_INT* result, BIG_INT* a, _u32 b)
{
	_u64 mul;
	_u32 carry = 0, i = 0;
	mov(result, a);
	for(i = 0; i < a->_len; ++i)
	{
		mul = a->_value[i];
		mul = mul * b + carry;
		result->_value[i] = (_u32)mul;
		carry = (_u32)(mul >> 32);
	}
	if(carry)
	{
		result->_len++;
		result->_value[result->_len - 1] = carry;
	}
	return;
}


/****************************************************************************************
大数相除
调用形式：N.Div(A)
返回值：N/A
****************************************************************************************/
void div1(BIG_INT* result, BIG_INT* a, BIG_INT* b)
{
	BIG_INT x, y, value, sub_value;
	_u32 i, len;
	_u64 num, div;
	if(b->_len == 1)
	{
		div_by_u32(result, a, b->_value[0]);
		return;
	}
	mov(&x, a);
	while(cmp(&x, b) >= 0)
	{
		div = x._value[x._len -1];
		num = b->_value[b->_len - 1];
		len = x._len - b->_len;
		if(div == num && len == 0)
		{
			add_by_u32(&value, result, 1);
			mov(result, &value);
			break;
		}
		if(div <= num && len)
		{
			--len;
			div = (div << 32) + x._value[x._len - 2];
		}
		div = div / (num + 1);
		mov_by_u64(&y, div);
		if(len)
		{
			y._len += len;
			for(i = y._len - 1; i >= len; --i)
				y._value[i] = y._value[i - len];
			for(i = 0; i < len; ++i)
				y._value[i] = 0;
		}
		add(&value, result, &y);
		mov(result, &value);
		mul(&value, b, &y);
		sub(&sub_value, &x, &value);
		mov(&x, &sub_value);
	}
	return;
}

void div_by_u32(BIG_INT* result, BIG_INT* a, _u32 b)
{
	_u64 div, mul;
	_u32 carry = 0;
	_int32 index = 0;
	mov(result, a);
	if(result->_len == 1)
	{
		result->_value[0] = result->_value[0] / b;
		return;
	}
	for(index = result->_len - 1; index >= 0; --index)
	{
		div = carry;
		div = (div << 32) + result->_value[index];
		result->_value[index] = (_u32)(div / b);
		mul = (div / b) * b;
		carry = (_u32)(div - mul);
	}
	if(result->_value[result->_len - 1] == 0)
		--result->_len;
	return;
}

/****************************************************************************************
大数求模
调用形式：N.Mod(A)
返回值：N%A
****************************************************************************************/
void mod(BIG_INT* result, BIG_INT* a, BIG_INT* b)
{
	BIG_INT x, value;
	_u64 div, num;
	_u32 carry = 0, i, len;
	mov(result, a);
	while(cmp(result, b) >= 0)
	{
		div = result->_value[result->_len - 1];
		num = b->_value[b->_len - 1];
		len = result->_len - b->_len;
		if((div == num) &&( len == 0))
		{
			sub(&value, result, b);
			mov(result, &value);
			break;
		}
		if((div <= num) && len)
		{
			--len;
			div = (div << 32) + result->_value[result->_len - 2];
		}
		div = div / (num + 1);
		mov_by_u64(&x, div);
		mul(&value, b, &x);
		mov(&x, &value);
		if(len)
		{
			x._len += len;
			for(i = x._len - 1; i >= len; --i)
			{
				x._value[i] = x._value[i - len];
			}
			for(i = 0; i < len; ++i)
			{
				x._value[i] = 0;
			}
		}
		sub(&value, result, &x);
		mov(result, &value);
	}
	return;
}

_u32 mod_by_u32(BIG_INT* a, _u32 b)
{
	_u64 div = 0;
	_u32 carry = 0;
	_int32 i = 0;
	if(a->_len == 1)
		return a->_value[0] % b;
	for(i = a->_len - 1; i >= 0; --i)
	{
		div = a->_value[i];
		div += carry * 0x100000000L;
		carry = (_u32)(div % b);
	}
	return carry;
}

/****************************************************************************************
调用格式：N.Get(str,sys)
返回值：N被赋值为相应大数
sys暂时只能为10或16
****************************************************************************************/
//void BigInt::Get(string& str)
void get(BIG_INT* result, const char* str, _u32 str_len)
{
	BIG_INT value;
	_u32 k, i;
	mov_by_u64(result, 0);
	for(i = 0; i < str_len; ++i)
	{
		mul_by_u32(&value, result, 16);
		mov(result, &value);
		if(str[i] >= '0' && str[i] <= '9')
			k = str[i] - 48;
		else if(str[i] >= 'A' && str[i] <= 'F')
			k = str[i] - 55;
		else if(str[i] >= 'a' && str[i] <= 'f')
			k = str[i] - 87;
		else
			k = 0;
		add_by_u32(&value, result, k);
		mov(result, &value);
	}
}

/****************************************************************************************
调用格式：N.Put(str,sys)
返回值：无，参数str被赋值为N的sys进制字符串
sys暂时只能为10或16
****************************************************************************************/
_int32 put(BIG_INT* a, char* str, _u32 str_len)
{
	BIG_INT value, value1;
	_u32 i = 0;
	char* t = "0123456789ABCDEF";
	if(str_len <= a->_len * 8)
		return -1;
	sd_memset(str, 0, (_int32)str_len);
	if(a->_len == 1 && a->_value[0] == 0)
	{
		str[0] = '0';
		return SUCCESS;
	}
	mov(&value, a);
	while(value._value[value._len - 1] > 0)
	{
		for(i = sd_strlen(str); i > 0; --i)
		{
			str[i] = str[i - 1];
		}
		str[0] =  t[mod_by_u32(&value, 16)];
		div_by_u32(&value1, &value, 16);
		mov(&value, &value1);
	}
	return SUCCESS;
}


/****************************************************************************************
求乘方的模
调用方式：N.RsaTrans(A,B)
返回值：X=N^A MOD B
****************************************************************************************/
void rsa(BIG_INT* result, BIG_INT* a, BIG_INT* b, BIG_INT* c)
{
	BIG_INT y, value, value1;
	_int32 i, j, k;
	_u32 n, num;
	k = b->_len * 32 - 32;
	num = b->_value[b->_len - 1];
	while(num)
	{
		num = num >> 1;
		++k;
	}
	mov(result, a);
	for(i =  k -2; i >= 0; --i)	
	{
		mul_by_u32(&value, result, result->_value[result->_len - 1]);
		mov(&y, &value);
		mod(&value, &y, c);
		mov(&y, &value);
		for(n = 1; n < result->_len; ++n)
		{
			for(j = y._len; j > 0; --j)
				y._value[j]=y._value[j - 1];
			y._value[0] = 0;
			y._len++;

			mul_by_u32(&value, result, result->_value[result->_len -n - 1]);
			add(&value1, &y, &value);
			mov(&y, &value1);

			mod(&value, &y, c);
			mov(&y, &value);
		}
		mov(result, &y);
		if((b->_value[i >> 5] >> (i & 31)) & 1)
		{
			mul_by_u32(&value, a, result->_value[result->_len - 1]);
			mov(&y, &value);

			mod(&value, &y, c);
			mov(&y, &value);

			for(n = 1; n < result->_len; ++n)
			{
				for(j = y._len; j > 0; --j)
					y._value[j] = y._value[j - 1];
				y._value[0] = 0;
				++y._len;
				mul_by_u32(&value, a, result->_value[result->_len - n - 1]);
				add(&value1, &y, &value);
				mov(&y, &value1);

				mod(&value, &y, c);
				mov(&y, &value);
			}
			mov(result, &y);
		}
	}
	return;
}

#endif

