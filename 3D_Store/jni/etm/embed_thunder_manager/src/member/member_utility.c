#include "utility/define.h"

#ifdef ENABLE_MEMBER

#include "member/member_utility.h"
#include "member/big_int.h"

#include "utility/md5.h"
#include "utility/time.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/url.h"

#include "em_common/em_define.h"
#include "em_common/em_errcode.h"
//经验等级
_u32 g_level_account[55] = 
{
	200,		500,		900,		1400,		2000,		2700,		3500,		4400,		5400,		6500,
	7700,		9000,		10400,		11900,		13500,		15200,		17000,		18900,		20900,		23000,
	25200,		27500,		29900,		32400,		35000,		37700,		40500,		43400,		46400,		49500,
	52700,		56000,		59400,		62900,		66500,		70200,		74000,		77900,		81900,		86000,
	90200,		94500,		98900,		103400,		108000,		112700,		117500,		122400, 	127400, 	132500,
	137700,		143000,		148400,		153900,		159500
};

char g_military_title[][18] = 
{
	"列兵", "少尉", "中尉", "上尉", "少校", "中校", "上校", "大校", "少将", "中将", 
	"上将", "大将", "元帅", "大元帅", "联军司令"
};



_u32 get_member_account(_u32 level)
{
	if(level > sizeof(g_level_account) / sizeof(_u32))
		return 99999999;
	return g_level_account[level - 1];
}

_u32 get_member_level(_u32 account)
{
	_u32 level = 1, index = 0;
	for(index =54; index >= 0; --index)
	{
		if(account >= g_level_account[index])
		{
			level = index + 1;
			break;
		}
	}
	return level;
}

_int32 get_member_military_title(_u32 level, char title[16])
{
	_u32 index = 0;
	if(level < 8)
		index = level - 1;
	else if(level < 12)
		index = 7;
	else if(level < 16)
		index = 8;
	else if(level < 32)
		index = 9;
	else if(level < 36)
		index = 10;
	else if(level < 42)
		index = 11;
	else if(level < 48)
		index = 12;
	else if(level < 55)
		index = 13;
	else
		index = 14;
	sd_strncpy(title, g_military_title[index], sd_strlen(g_military_title[index]) + 1);
	return SUCCESS;
}

_int32 member_double_md5(char md5_passwd[33], const char* password, _u32 password_len)
{
	ctx_md5 md5;
	_u16 hash[16];
	sd_assert(password_len == 32);
	md5_initialize(&md5);
	md5_update(&md5, (const _u8*)password, password_len);
	md5_finish(&md5,(unsigned char*)hash);
	str2hex((char*)hash, 16, md5_passwd, password_len);
	md5_passwd[32] = '\0';
	sd_string_to_low_case(md5_passwd);
	return SUCCESS;
}

_int32 member_rsa(char rsa_pwd[65], char md5_passwd[33])
{
	_int32 ret = SUCCESS, i = 0;
	char hex_str[] = "0123456789ABCDEF";
	char rand_str[16] = {0};
	char md5_pwd1[25] = {0}, md5_pwd2[25] = {0};
	char rsa_pwd1[33] = {0}, rsa_pwd2[33] = {0};
	char strN[] = "BA41A57F2AF7D7849EBBF54D742B8379";
	char strE[] = "10001";
	_u32 times = 0, index = 0;
	BIG_INT N, E, P, Q;
	//计算出一个16个字节的随机数
	sd_time(&times);
	sd_srand(times);
	for(index = 0; index < 16; ++index)
	{
		rand_str[index] = hex_str[sd_rand() % 16];
	}
	//生成两个md5_pwd
	sd_strncpy(md5_pwd1, md5_passwd, 16);
	sd_strncpy(md5_pwd1 + 16, rand_str, 8);
	sd_strncpy(md5_pwd2, md5_passwd + 16, 16);
	sd_strncpy(md5_pwd2 + 16, rand_str + 8, 8);
	//计算rsa
	get(&N, strN, sd_strlen(strN));
	get(&E, strE, sd_strlen(strE));
 	get(&P, md5_pwd1, sd_strlen(md5_pwd1));
	rsa(&Q, &P, &E, &N);
	ret = put(&Q, rsa_pwd1, 33);
	CHECK_VALUE(ret);
	get(&P, md5_pwd2, sd_strlen(md5_pwd2));
 	rsa(&Q, &P, &E, &N);
  	ret = put(&Q, rsa_pwd2, 33);
	//补0
	for (i = sd_strlen(rsa_pwd1); i < 32; ++i)
	{
		rsa_pwd1[i] = '0';
	}
	for(i = sd_strlen(rsa_pwd2); i < 32; ++i)
	{
		rsa_pwd2[i] = '0';
	}
	//结果
	sd_memset(rsa_pwd, 0, 65);
	sd_strcat(rsa_pwd, rsa_pwd1, sd_strlen(rsa_pwd1) + 1);
	sd_strcat(rsa_pwd, rsa_pwd2, sd_strlen(rsa_pwd2) + 1);
	return SUCCESS;
}


_u32 get_update_days(_u32 account, _u32 vip_rank)
{
	_u32 days = 0;
	_u32 max_score = 0;
	_u32 level = get_member_level(account);
	_u32 next_account = get_member_account(level + 1);
	switch(vip_rank)
	{
		case 0:
			max_score = 70;
			break;
		case 1:
			max_score = 80;
			break;
		case 2:
			max_score = 90;
			break;
		case 3:
			max_score = 100;
			break;
		case 4:
			max_score = 110;
			break;
		case 5:
			max_score = 120;
			break;
		case 6:
			max_score = 130;
			break;
		case 7:
			max_score = 140;
			break;
		default:
			sd_assert(FALSE);
			max_score = 140;
			break;
	}
	days = (next_account - account) / max_score;
	if((next_account - account) % max_score != 0)
		++days;
	return days;
}

#endif /* ENABLE_MEMBER */

