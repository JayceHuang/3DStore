package com.runmit.sweedee.sdk.user.member.task;

import android.content.Context;
import android.os.Bundle;

import com.runmit.sweedee.sdkex.httpclient.LoginAsyncHttpProxy;

/**
 * UserUtil 用户登录的接口类，单例，在使用前应该调用Init函数先初始化该单例，程序退出时，使用Uninit反初始化
 * 初始化UserUtil的init线程必须有Loop，否则会发生崩溃和其他未知情况。 监听者分为全局监听者和局部监听者
 * UserUtil注册的监听为全局监听者，调用业务接口时注册的监听为局部监听者。
 * 调用业务接口的线程没有Loop时局部监听者会在init线程回调，有Loop时直接回调。 全局监听者会一直在init线程中回调。 cacel接口暂未实现。
 *
 * @author xl
 */

public class UserUtil {

    private static final UserUtil s_instance = new UserUtil();

    private LoginAsyncHttpProxy mHttpProxy = LoginAsyncHttpProxy.getInstance();

    private Context mContext = null;

    private UserUtil() {
    }

    public static UserUtil getInstance() {
        return s_instance;
    }

    private boolean isInit = false;

    /**
     * 初始化登录模块，返回是否初始化成功
     *
     * @param context       Context对象
     * @param clientVersion ， 表示当前客户端版本号
     * @param businessType  ，取值见BusinessType中定义
     * @param peerid        ， 表示当前设备的peerid
     * @return 返回是否初始化成功
     */

    public boolean Init(Context context) {
        if (isInit == true) {
            return false;
        }
        isInit = true;
        mContext = context;
        mHttpProxy.init(context);
        return true;

    }

    public Context getContext() {
        return mContext;
    }

    public LoginAsyncHttpProxy getHttpProxy() {
        return mHttpProxy;
    }

    /**
     * 用户登录,返回该次登录任务的cookie
     *
     * @param account          ：登录账号
     * @param bUserID          ：account是否userid
     * @param password         ：登录密码，用户的明文密码，或登录成功后通过UserInfo.USERINFOKEY.
     *                         UIK_encryptedPassword获取的已加密密码（例如自动登录等业务）
     * @param passwordCheckNum ：登录密码的校验码，若password为明文密码，则其传空字串，若password为已加密处理的密码，则需要传递该校验码（
     *                         登录成功后通过UserInfo.USERINFOKEY.UIK_PasswordCheckNum获取）
     * @param verifyKey        ：验证码key，可以传空字串
     * @param verifyCode       ： 验证码,可以传空字串
     * @param listen           : OnUserListener类型的返回回调，可以传空（会回调至注册的全局监听接口）
     * @param userdata         : 用户自定义数据，会在相应的回调函数中回传回来，可以传null
     * @return 返回该次调用的cookie值，可以使用该值对当前任务进行取消操作
     */
    public int userLogin(/*final String countrycode,*/String account, String password, String verifyCode, OnUserListener listen, Object userdata) {
        UserLoginTask taskLogin = new UserLoginTask(this);
        taskLogin.initTask();
//        taskLogin.putCountryCode(countrycode);
        taskLogin.putUserName(account);
        taskLogin.putPassword(password);
        taskLogin.putUserData(userdata);
        taskLogin.putVerifyInfo(verifyCode);
        taskLogin.attachListener(listen);
        return commitTask(taskLogin);
    }

    private int commitTask(UserTask task) {
        task.execute();
        return task.getSequenceNo();
    }

    public void notifyListener(final UserTask task, final Bundle bundle) {
        task.fireListener(bundle);
    }
}
