package com.runmit.sweedee.sdk.user.member.task;

import org.apache.http.Header;
import org.json.JSONException;
import org.json.JSONObject;

import android.os.Bundle;

import com.loopj.android.http.TextHttpResponseHandler;
import com.runmit.sweedee.util.XL_Log;

public class UserLogoutTask extends UserTask {

    private final String LOGIN_URL = "https://zeus.d3dstore.com/v1.5/home/logout?token=";

    XL_Log log = new XL_Log(UserLogoutTask.class);

    public UserLogoutTask(UserUtil util) {
        super(util);
    }

    public boolean fireEvent(OnUserListener listen, Bundle bundle) {
        if (bundle == null || bundle.getString("action") != "userLogoutTask")
            return false;
        return listen.onUserLogout(bundle.getInt("errorCode"), null, getUserData());
    }

    public boolean execute(long userid, String token, String devicehwid) {
        if (!UserManager.getInstance().isLogined()) {
            Bundle bundle = new Bundle();
            bundle.putString("action", "userLogoutTask");
            bundle.putInt("errorCode", UserErrorCode.OPERATION_INVALID);
            getUserUtil().notifyListener(this, bundle);
            return true;
        }
        String url = LOGIN_URL + token;
        getUserUtil().getHttpProxy().get(url, new TextHttpResponseHandler() {
			
			@Override
			public void onSuccess(int statusCode, Header[] headers, String content) {
                log.debug("onSucce=" + content);
                try {
                    JSONObject jsonRespObj = new JSONObject(content);
                    int errorCode = jsonRespObj.getInt("rtn");
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "userLogoutTask");
                    bundle.putInt("errorCode", errorCode);
                    getUserUtil().notifyListener(UserLogoutTask.this, bundle);
                } catch (JSONException e) {
                    e.printStackTrace();
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "userLogoutTask");
                    bundle.putInt("errorCode", UserErrorCode.OPERATION_INVALID);
                    getUserUtil().notifyListener(UserLogoutTask.this, bundle);
                }
            }

            @Override
            public void onFailure(int arg0, Header[] arg1, String arg2, Throwable error) {
                Bundle bundle = new Bundle();
                bundle.putString("action", "userLogoutTask");
                bundle.putInt("errorCode", UserErrorCode.OPERATION_INVALID);
                getUserUtil().notifyListener(UserLogoutTask.this, bundle);
            }
		});
        return true;
    }

//    /**
//     * 鉴权请求
//     *
//     * @param userid
//     * @param token
//     * @param devicehwid
//     * @param mOnUserPingResultListener
//     */
//    public void executeUnbindDevice(long userid, String token, String devicehwid) {
//        if (!UserManager.getInstance().isLogined()) {
//            Bundle bundle = new Bundle();
//            bundle.putString("action", "userLogoutTask");
//            bundle.putInt("errorCode", UserErrorCode.OPERATION_INVALID);
//            getUserUtil().notifyListener(this, bundle);
//            return;
//        }
//        JSONObject jsonRequObj = new JSONObject();
//        try {
//            jsonRequObj.put("userid", userid);
//            jsonRequObj.put("token", token);
//            jsonRequObj.put("devicehwid", Util.getPeerId());
//        } catch (JSONException e) {
//            e.printStackTrace();
//            return;
//        }
//        String jsonContent = jsonRequObj.toString();
//        log.debug("jsonContent="+jsonContent);
//        ByteArrayEntity byteEntity = new ByteArrayEntity(jsonContent.getBytes());
//        getUserUtil().getHttpProxy().post(DEL_DEVICE_URL, byteEntity, new LoginAsyncHttpProxyListener() {
//
//            @Override
//            public void onSuccess(int statusCode, Header[] headers, String content) {
//                log.debug("onSucce=" + content);
//                try {
//                    JSONObject jsonRespObj = new JSONObject(content);
//                    int errorCode = jsonRespObj.getInt("rtn");
//                    Bundle bundle = new Bundle();
//                    bundle.putString("action", "userLogoutTask");
//                    bundle.putInt("errorCode", errorCode);
//                    getUserUtil().notifyListener(UserLogoutTask.this, bundle);
//                } catch (JSONException e) {
//                    e.printStackTrace();
//                    Bundle bundle = new Bundle();
//                    bundle.putString("action", "userLogoutTask");
//                    bundle.putInt("errorCode", UserErrorCode.OPERATION_INVALID);
//                    getUserUtil().notifyListener(UserLogoutTask.this, bundle);
//                }
//            }
//
//            @Override
//            public void onFailure(Throwable error) {
//            	Bundle bundle = new Bundle();
//                bundle.putString("action", "userLogoutTask");
//                bundle.putInt("errorCode", UserErrorCode.OPERATION_INVALID);
//                getUserUtil().notifyListener(UserLogoutTask.this, bundle);
//            }
//
//        });
//    }

    @Override
    public boolean execute() {
        return false;
    }
}
