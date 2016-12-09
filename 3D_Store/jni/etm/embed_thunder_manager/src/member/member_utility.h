
#include "em_common/em_define.h"

#ifdef ENABLE_MEMBER

_u32 get_member_account(_u32 level);

_u32 get_member_level(_u32 account);

_int32 member_double_md5(char md5_passwd[33], const char* password, _u32 password_len);

_int32 member_rsa(char rsa_pwd[65], char md5_passwd[33]);

_u32 get_update_days(_u32 account, _u32 vip_rank);

_int32 get_member_military_title(_u32 level, char title[16]);

#endif /* ENABLE_MEMBER */
