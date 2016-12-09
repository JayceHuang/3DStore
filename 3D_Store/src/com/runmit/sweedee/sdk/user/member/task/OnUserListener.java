package com.runmit.sweedee.sdk.user.member.task;

import java.util.List;

import com.runmit.sweedee.sdk.user.member.task.MemberInfo.USERINFOKEY;

/**
 * errorCode 定义在类XLErrorCode中
 * errorCode:SUCCESS表示XLErrorCode.SUCCESS
 * 后续errorCode依次类推
 *
 * @author lzl
 */

public abstract class OnUserListener {
    /**
     * 用户登录通知
     *
     * @param errorCode = SUCCESS表明登录成功， user不为空，否则登录失败
     * @param userInfo  可以从该对象中获取用户信息
     * @param userdata  用户在调用登录时传入的自定义数据
     * @return 如果返回true，则继续往下传递该事件，若返回false，则其后注册的监听者将收不到该事件回调(包括全局注册的监听者)
     */
    public boolean onUserLogin(int errorCode, MemberInfo userInfo, Object userdata) {
        return true;
    }

    /**
     * 用户登出通知
     *
     * @param errorCode = SUCCESS:用户正常登出, SESSIONID_KICKOUT：被踢（其他终端登录）,SESSIONID_TIMEOUT：超时（session过期）, SERVER_DISCONNENT：服务器连接断开, SERVER_ABNORMAL: 服务器出现异常
     * @param userInfo  可以从该对象中获取用户信息
     * @param userdata  用户在调用登录时传入的自定义数据
     * @return 如果返回true，则继续往下传递该事件，若返回false，则其后注册的监听者将收不到该事件回调(包括全局注册的监听者)
     */
    public boolean onUserLogout(int errorCode, MemberInfo userInfo, Object userdata) {
        return true;
    }

    /**
     * 获取到用户详细资料时的通知
     *
     * @param errorCode    SUCCESS成功
     * @param catchedList: 哪些数据被成功获取
     * @param userInfo：    可以获取更新后数据
     * @return 如果返回true，则继续往下传递该事件，若返回false，则其后注册的监听者将收不到该事件回调(包括全局注册的监听者)
     */
    public boolean onUserInfoCatched(int errorCode, List<USERINFOKEY> catchedInfoList, MemberInfo userInfo, Object userdata) {
        return true;
    }

    /**
     * 用户获取验证通知
     *
     * @param errorCode    = SUCCESS 表明获取成功
     * @param verifyKey    验证码图片的key，再次登录时需要传入此值
     * @param userdata     用户在调用登录时传入的自定义数据
     * @param imageType    验证码图片类型 VERIFY_JPEG_IMAGE=1 VERIFY_PNG_IMAGE=2 VERIFY_BMP_IMAGE=3 VERIFY_UNKNOWN_IMAGE=4
     * @param imageContent 验证码图片二进制数据
     * @return 如果返回true，则继续往下传递该事件，若返回false，则其后注册的监听者将收不到该事件回调(包括全局注册的监听者)
     */
    public boolean onUserVerifyCodeUpdated(int errorCode, String verifyKey, int imageType, final byte[] imageContent, Object userdata) {

        return true;
    }
}
