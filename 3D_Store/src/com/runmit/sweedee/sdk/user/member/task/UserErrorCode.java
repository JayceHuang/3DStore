package com.runmit.sweedee.sdk.user.member.task;

public class UserErrorCode {
    /**
     * 服务器定义的错误码
     */
    /**
     * 表示无错误
     */
    public static final int SUCCESS = 0;
    public static final int SESSIONID_LLLEGAL = SUCCESS + 1;
    public static final int ACCOUNT_INVALID = SESSIONID_LLLEGAL + 1;
    public static final int PASSWORD_ERROR = ACCOUNT_INVALID + 1;


    
    /**
     * 用户定义错误码
     */

    /**
     * 无效的操作（没有登录时去拉取用户信息，没有登录时去注销登录等等）
     */
    public static final int OPERATION_INVALID = 1024;

}
