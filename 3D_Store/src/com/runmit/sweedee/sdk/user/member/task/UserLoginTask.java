package com.runmit.sweedee.sdk.user.member.task;

import org.apache.http.Header;
import org.apache.http.entity.ByteArrayEntity;
import org.json.JSONException;
import org.json.JSONObject;

import android.os.Bundle;
import android.text.TextUtils;

import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.sdkex.httpclient.LoginAsyncHttpProxyListener;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.MD5;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

public class UserLoginTask extends UserTask {

    XL_Log log = new XL_Log(UserLoginTask.class);

    public static final String PUBKEY = "pubKey";
    private String sRequName = "";
    private String sRequPwd = "";
//    private String sRequCountryCode = "";
    private String sVerifyCode = "";
    private String verifytoken;

    private final String LOGIN_URL = "https://zeus.d3dstore.com/v1.5/login";

    public UserLoginTask(UserUtil util) {
        super(util);
    }

    public void initTask() {
        super.initTask();
        sRequName = "";
        sRequPwd = "";
        sVerifyCode = "";
    }

//    public void putCountryCode(String countrycode) {
//    	sRequCountryCode = countrycode;
//    }
    
    public void putUserName(String name) {
        sRequName = name;
    }

    public void putPassword(String pwd) {
        sRequPwd = MD5.encrypt(pwd);
    }

    public void putVerifyInfo(String verifyCode) {
        sVerifyCode = verifyCode;
    }

    public boolean fireEvent(OnUserListener listen, Bundle bundle) {
        if (bundle == null || bundle.getString("action") != "userLoginTask")
            return false;
        return listen.onUserLogin(bundle.getInt("errorCode"), (MemberInfo) bundle.getSerializable(MemberInfo.class.getSimpleName()), getUserData());
    }

    public boolean execute() {
        if (TASKSTATE.TS_CANCELED == getTaskState()) {
            return false;
        }
        if(!Util.isNetworkAvailable()){
        	 Bundle bundle = new Bundle();
             bundle.putString("action", "userLoginTask");
             bundle.putInt("errorCode", ExceptionCode.ERR_NO_NETWORK);
             getUserUtil().notifyListener(this, bundle);
             putTaskState(TASKSTATE.TS_DONE);
             return false;
        }
        log.debug("sRequName="+sRequName+",sRequPwd="+sRequPwd);
        if (sRequName == null || sRequName.length() == 0) {
            Bundle bundle = new Bundle();
            bundle.putString("action", "userLoginTask");
            bundle.putInt("errorCode", UserErrorCode.ACCOUNT_INVALID);
            getUserUtil().notifyListener(this, bundle);
            putTaskState(TASKSTATE.TS_DONE);
            return false;
        }
        if (sRequPwd.compareTo(MD5.encrypt("")) == 0) {
            Bundle bundle = new Bundle();
            bundle.putString("action", "userLoginTask");
            bundle.putInt("errorCode", UserErrorCode.PASSWORD_ERROR);
            getUserUtil().notifyListener(this, bundle);
            putTaskState(TASKSTATE.TS_DONE);
            return false;
        }
        putTaskState(TASKSTATE.TS_DOING);
        JSONObject jsonRequObj = new JSONObject();
        try {
            jsonRequObj.put("account", sRequName);
            jsonRequObj.put("password", sRequPwd);// 一次MD5后
//            jsonRequObj.put("countrycode", sRequCountryCode);
            if(!TextUtils.isEmpty(sVerifyCode)){
            	jsonRequObj.put("verifycode", sVerifyCode);
            }
            if(!TextUtils.isEmpty(verifytoken)){
            	jsonRequObj.put("verifytoken", verifytoken);
            }
        } catch (JSONException e) {
            e.printStackTrace();
            Bundle bundle = new Bundle();
            bundle.putString("action", "userLoginTask");
            bundle.putInt("errorCode", ExceptionCode.JSONException);
            getUserUtil().notifyListener(this, bundle);
            putTaskState(TASKSTATE.TS_DONE);
            return false;
        }

        String jsonContent = jsonRequObj.toString();
        ByteArrayEntity byteEntity = new ByteArrayEntity(jsonContent.getBytes());
        log.debug("execute jsonContent=" + jsonContent);
        getUserUtil().getHttpProxy().post(LOGIN_URL, byteEntity, new LoginAsyncHttpProxyListener() {

            public void onSuccess(int responseCode, Header[] headers, String result) {
                log.debug("Login onSuccess result=" + result);
                try {
                    JSONObject jsonRespObj = new JSONObject(result);
                    int errorCode = jsonRespObj.getInt("rtn");
                    if (errorCode == UserErrorCode.SUCCESS) {
                        JSONObject mUserInfoJsonObject = jsonRespObj.getJSONObject("userInfo");
                        MemberInfo mPingMemberInfo = MemberInfo.newInstance(mUserInfoJsonObject.toString());
                        mPingMemberInfo.token = jsonRespObj.getString("token");
                        Bundle bundle = new Bundle();
                        bundle.putString("action", "userLoginTask");
                        bundle.putInt("errorCode", UserErrorCode.SUCCESS);
                        bundle.putSerializable(MemberInfo.class.getSimpleName(), mPingMemberInfo);
                        getUserUtil().notifyListener(UserLoginTask.this, bundle);
                    } else {
                        Bundle bundle = new Bundle();
                        bundle.putString("action", "userLoginTask");
                        bundle.putInt("errorCode", errorCode);
                        getUserUtil().notifyListener(UserLoginTask.this, bundle);
                    }
                } catch (JSONException e) {
                    e.printStackTrace();
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "userLoginTask");
                    bundle.putInt("errorCode", ExceptionCode.JsonParseException);
                    getUserUtil().notifyListener(UserLoginTask.this, bundle);
                }
                putTaskState(TASKSTATE.TS_DONE);
            }

            public void onFailure(Throwable error) {
                error.printStackTrace();
                Bundle bundle = new Bundle();
                bundle.putString("action", "userLoginTask");
                int errorCode = ExceptionCode.getErrorCode(error);
                bundle.putInt("errorCode",errorCode);
                getUserUtil().notifyListener(UserLoginTask.this, bundle);
                putTaskState(TASKSTATE.TS_DONE);
            }
        });
        return true;
    }
}
