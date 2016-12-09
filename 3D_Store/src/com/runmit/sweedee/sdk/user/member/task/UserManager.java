package com.runmit.sweedee.sdk.user.member.task;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.report.sdk.ReportEventCode.UcReportCode;
import com.runmit.sweedee.sdk.user.member.task.MemberInfo.USERINFOKEY;
import com.runmit.sweedee.util.AESEncryptor;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.TDString;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

public class UserManager {

    XL_Log log = new XL_Log(UserManager.class);

    public static final String PREFERENCES = "ACOUNT_PREFERENCES";

    public enum LoginState {
        COMMON, // 普通模式,未登录
        LOGINING, // 正在登录
        LOGINED// 已经登录(成功)
    }

    public interface LoginStateChange {
        void onChange(LoginState lastState, LoginState currentState, Object userdata);
    }
    
    private static final String USER_INFOKEY = "abczz";

    private List<LoginStateChange> observes = new ArrayList<LoginStateChange>();

    private SharePrefence mSharedPreferences = null;

    private LoginState state = LoginState.COMMON;// 默认

    private MemberInfo mCurrentMemberInfo;

    public Object mSyncObj = new Object();

    private final static UserManager INSTANCE = new UserManager();

    private Handler mHandler = new Handler(StoreApplication.INSTANCE.getMainLooper());

    /**
     * 是否已经取消登陆
     */
    private boolean isCancelLogin = false;
    
    private LoginInfo mLoginInfo;

    // 此方法调用相当频繁，不应用同步方法实现，改用静态初始化和延迟加载的方法
    public static UserManager getInstance() {
        return INSTANCE;
    }

    private UserManager() {
        mSharedPreferences = SharePrefence.getInstance(StoreApplication.INSTANCE);
    }

    public boolean isLogined() {
        return state == LoginState.LOGINED;
    }

    public boolean isLogining() {
        return state == LoginState.LOGINING;
    }

    public synchronized MemberInfo getMemberInfo() {
        if (state == LoginState.LOGINED) {
            return mCurrentMemberInfo;
        } else {
            return null;
        }
    }

    /**
     * 获取用户id。未登录返回0
     *
     * @return
     */
    public long getUserId() {
        if (mCurrentMemberInfo != null) {
            return mCurrentMemberInfo.userid;
        } else if(mLoginInfo != null){
            return mLoginInfo.userid;
        }
		return 0;
    }

    public String getUserSessionId() {
        if (mCurrentMemberInfo != null) {
            return mCurrentMemberInfo.token;
        } else {
            return new String("0");
        }
    }

    public String getPeerId() {
        return Util.getPeerId();
    }

    /**
     * 由于onRestoreInstanceState会被Activity栈内的不同Activity调用，
     * 所以可能存在的问题是重复调用了onRestoreInstanceState
     * 但是致命的问题是我在UserinfoActivity调用注销之后回退到XlCloudplayActivity时候XlCloudplayActivity会再次调用这个函数
     * ， 他又把数据给还原了。所以此变量出现的意义是保证只初始化一次onRestoreInstanceState
     */
    private boolean hasRestoreInstanceState = false;

    public void onSaveInstanceState(Bundle outState) {
    	if(state == LoginState.LOGINED && mCurrentMemberInfo != null){
    		outState.putSerializable("loginstate", state);
            outState.putSerializable("MemberInfo", mCurrentMemberInfo);
    	}
        hasRestoreInstanceState = false;
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        if (hasRestoreInstanceState) {
            return;
        }
        hasRestoreInstanceState = true;
        if (savedInstanceState != null) {
            Serializable loginstate = savedInstanceState.getSerializable("loginstate");
            if (loginstate != null && loginstate instanceof LoginState) {
                state = (LoginState) loginstate;
            }
            savedInstanceState.remove("loginstate");
            Serializable mSerializableMemberInfo = savedInstanceState.getSerializable("MemberInfo");
            if (mSerializableMemberInfo != null && mSerializableMemberInfo instanceof MemberInfo) {
                mCurrentMemberInfo = (MemberInfo) mSerializableMemberInfo;
            }
            savedInstanceState.remove("MemberInfo");
        }
    }

    public void autoLogin() {
    	mLoginInfo = loadLoginInfo();
    	if(mLoginInfo != null && !TextUtils.isEmpty(mLoginInfo.username) && !TextUtils.isEmpty(mLoginInfo.userpassword)){
    		userLogin(mLoginInfo.countrycode,mLoginInfo.username, mLoginInfo.userpassword,true);
    	}
    }

    public void userLogin(final String countrycode ,final String user_name, final String user_password,final Boolean isAutoLogin) {
        log.debug("user_name="+user_name+",user_password="+user_password);
        new Thread(new Runnable() {
            @Override
            public void run() {
                if (user_name != null && user_password != null) {
                    log.debug("userLogin state=" + state);
                    while (state == LoginState.LOGINING) {
                        synchronized (mSyncObj) {
                            try {
                                mSyncObj.wait();
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                    if (state == LoginState.LOGINED) {
                        return;
                    }
                    mLoginInfo = new LoginInfo(user_name, user_password, countrycode, 0);
                    state = LoginState.LOGINING;
                    UserUtil.getInstance().userLogin(/*countrycode,*/user_name, user_password, "", mXLOnUserListener,isAutoLogin);
                }
            }
        }).start();

    }

    /**
     * @param isuserLogout :是否用户主动注销，如果是，则toast提示，并且清除sp 如果不是，比如账号被踢，则不用toast提示
     */
    public void userLogout(boolean isuserLogout) {
    	if(mCurrentMemberInfo != null){
    		UserLogoutTask task = new UserLogoutTask(UserUtil.getInstance());
            task.initTask();
            task.putUserData(isuserLogout);
            task.attachListener(mXLOnUserListener);
            task.execute(mCurrentMemberInfo.userid, mCurrentMemberInfo.token, mCurrentMemberInfo.token);
    	}
    }

//    public void unbindDevice(){
//    	UserLogoutTask task = new UserLogoutTask(UserUtil.getInstance());
//        task.initTask();
//        task.putUserData(true);
//        task.attachListener(mXLOnUserListener);
//        task.executeUnbindDevice(mCurrentMemberInfo.userid, mCurrentMemberInfo.token, mCurrentMemberInfo.token);
//    }
    
    public void addLoginStateChangeListener(final LoginStateChange observe) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (!observes.contains(observe)) {
                    observes.add(observe);
                }
            }
        });

    }

    public void removeLoginStateChangeListener(final LoginStateChange observe) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                observes.remove(observe);
            }
        });

    }
    
    private void clearAccount(){
    	if(mLoginInfo == null){
    		return;
    	}
    	mLoginInfo.userpassword = "";
    	try {
    		String json = mLoginInfo.toJsonString();
    		if(!TextUtils.isEmpty(json)){
    			String aesdata = AESEncryptor.encryptStringMD5(json);
    			mSharedPreferences.putString(USER_INFOKEY, aesdata);
    		}
        } catch (Exception e1) {
            e1.printStackTrace();
        }
    }

    private void saveAccountAndPassword() {
        try {
        	if(mLoginInfo != null){
        		mLoginInfo.userid = mCurrentMemberInfo.userid;
        		String json = mLoginInfo.toJsonString();
        		if(!TextUtils.isEmpty(json)){
        			String aesdata = AESEncryptor.encryptStringMD5(json);
        			mSharedPreferences.putString(USER_INFOKEY, aesdata);
        		}
        	}
        } catch (Exception e1) {
            e1.printStackTrace();
        }
    }

    // 获取登录状态
    public LoginState getLoginState() {
        return state;
    }

    /**
     * 获取保存sp的账号，如果没有获取到返回null
     *
     * @return
     */
    public String getAccount() {
    	if(mLoginInfo != null){
    		return mLoginInfo.username;
    	}
		return null;
    }
    
    public String getCountryCode() {
    	if(mLoginInfo != null){
    		return mLoginInfo.countrycode;
    	}
        return null;
    }
    
	public LoginInfo loadLoginInfo() {
		String aesdata = mSharedPreferences.getString(USER_INFOKEY, null);
		try {
			if (aesdata != null) {
				String jsondata = AESEncryptor.decryptStringMD5(aesdata);
				LoginInfo mLoginInfo = new LoginInfo(jsondata);
				return mLoginInfo;
			}
		} catch (Exception e1) {
			e1.printStackTrace();
		}
		return null;
	}

    /**
     * 获取保存sp的密码，如果没有获取到返回null
     *
     * @return
     */
    public String getPassword() {
    	if(mLoginInfo != null){
    		return mLoginInfo.userpassword;
    	}
        return null;
    }
    
    public void setPassword(String password) {
    	if(mLoginInfo != null){
    	   mLoginInfo.userpassword=password;
    	}
 
    }

    public void cancelLogin() {
        isCancelLogin = true;
        final LoginState lastState = state;
        state = LoginState.COMMON;
        postLoginChange(lastState, state, null);
        synchronized (mSyncObj) {
            mSyncObj.notifyAll();// 刷新所有等待的线程
        }

    }

    OnUserListener mXLOnUserListener = new OnUserListener() {
        public void loginErrorToast(int event, int errorCode) {
            switch (errorCode) {
                case 0:
                    // Util.showToast(XlShareApplication.INSTANCE, "登录成功",
                    // Toast.LENGTH_LONG);
                    break;
                case 20009:
                	Util.showToast(StoreApplication.INSTANCE, TDString.getStr(R.string.loginerr_password_account_error_with_tip), Toast.LENGTH_LONG);
                	break;
                case 20010:
                	Util.showToast(StoreApplication.INSTANCE, TDString.getStr(R.string.loginerr_user_toomuch_device_login), Toast.LENGTH_LONG);
                	break;
                case -418:
                    Util.showToast(StoreApplication.INSTANCE,  TDString.getStr(R.string.loginerr_timeout_with_retrytip) + errorCode, Toast.LENGTH_LONG);
                    break;
                case ExceptionCode.ERR_NO_NETWORK:
                	Util.showToast(StoreApplication.INSTANCE,  TDString.getStr(R.string.no_network_toast), Toast.LENGTH_LONG);
                	break;
                case ExceptionCode.CertificateException:
                	Util.showToast(StoreApplication.INSTANCE,  TDString.getStr(R.string.loginerr_ssl_error), Toast.LENGTH_LONG);
                	break;
                default:
                    Util.showToast(StoreApplication.INSTANCE,  TDString.getStr(R.string.loginerr_unknown_error) + errorCode, Toast.LENGTH_LONG);
                    break;
            }
        }

        @Override
        public boolean onUserLogin(int errorCode, final MemberInfo userInfo, final Object userdata) {
            log.debug("onUserLogin errorCode=" + errorCode);
            ReportActionSender.getInstance().error(ReportActionSender.MODULE_UC, UcReportCode.UC_REPORT_LOGIN, errorCode);
            final LoginState lastState = state;
            if (isCancelLogin) {// 已经取消登陆
                isCancelLogin = false;
                return true;
            }
            Boolean isAutoLogin = (Boolean) userdata;
            ReportActionSender.getInstance().login(errorCode, userInfo != null ? userInfo.userid : 0);
            ReportActionSender.getInstance().error(ReportActionSender.MODULE_UC, UcReportCode.UC_REPORT_LOGIN, errorCode);
            
            loginErrorToast(errorCode, errorCode);
            if (errorCode == 0) {// 登陆成功
                mCurrentMemberInfo = userInfo;
                log.debug("onUserLogin mCurrentMemberInfo=" + mCurrentMemberInfo);
                saveAccountAndPassword();
                log.debug("onUserLogin new session=" + getUserSessionId());
                state = LoginState.LOGINED;// 设置状态
            } else {
                state = LoginState.COMMON;
                if(!isAutoLogin){
                	clearAccount();//登录失败，清除登陆记录
                }
            }
            postLoginChange(lastState, state, userdata);
            synchronized (mSyncObj) {
                mSyncObj.notifyAll();// 刷新所有等待的线程
            }
            return true;
        }

        @Override
        public boolean onUserLogout(final int reson, final MemberInfo userInfo, final Object userdata) {
            log.debug("onUserLogout reson=" + reson);
            Boolean isUserLoginOut = (Boolean) userdata;
            if (isUserLoginOut) {// 不是取消登录就提示
                Util.showToast(StoreApplication.INSTANCE,  TDString.getStr(R.string.logout_succeed), Toast.LENGTH_SHORT);
            }
            clearAccount();
            mCurrentMemberInfo = new MemberInfo();
            LoginState oldState = state;
            state = LoginState.COMMON;
            postLoginChange(oldState, LoginState.COMMON, userdata);
            return true;
        }

        @Override
        public boolean onUserInfoCatched(int errorCode, List<USERINFOKEY> catchedInfoList, MemberInfo userInfo, Object userdata) {
            log.debug("onUserInfoCatched=" + errorCode + ",userInfo=" + userInfo);
            return true;
        }

        @Override
        public boolean onUserVerifyCodeUpdated(int errorCode, String verifyKey, int imageType, byte[] imageContent, Object userdata) {
            return false;
        }
    };

    private void postLoginChange(final LoginState lastState, final LoginState currentState, final Object userdata) {
        mHandler.post(new Runnable() {

            @Override
            public void run() {
                if (observes.size() > 0 && lastState != state) {
                    for (LoginStateChange observe : observes) {
                        observe.onChange(lastState, currentState, userdata);
                    }
                }
            }
        });
    }
    
    public static final class LoginInfo{
    	public String username;
    	public String userpassword;
    	public String countrycode;
    	public long userid;
    	
    	public LoginInfo(String jsondata){
    		try {
				JSONObject mJsonObject = new JSONObject(jsondata);
				this.username = mJsonObject.getString("username");
				this.userpassword = mJsonObject.getString("userpassword");
				this.countrycode = mJsonObject.getString("countrycode");
				this.userid = mJsonObject.getLong("userid");
			} catch (JSONException e) {
				e.printStackTrace();
			}
    	}
    	
    	public LoginInfo(String name,String psw,String code,long uid){
    		this.username = name;
    		this.userpassword = psw;
    		this.countrycode = code;
    		this.userid = uid;
    	}
    	
    	public String toJsonString(){
    		JSONObject mJsonObject = new JSONObject();
    		try {
				mJsonObject.put("username", this.username);
				mJsonObject.put("userpassword", this.userpassword);
	    		mJsonObject.put("countrycode", this.countrycode);
	    		mJsonObject.put("userid", this.userid);
	    		return mJsonObject.toString();
			} catch (JSONException e) {
				e.printStackTrace();
			}
    		return null;
    	}
    }
}
