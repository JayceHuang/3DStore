package com.runmit.sweedee.sdk.user.member.task;

import org.apache.http.Header;
import org.json.JSONException;
import org.json.JSONObject;

import com.loopj.android.http.TextHttpResponseHandler;
import com.runmit.sweedee.util.XL_Log;

public class UserPingTask {

    private final String CHECK_URL = "https://zeus.d3dstore.com/v1.5/home/check?token=";

    XL_Log log = new XL_Log(UserPingTask.class);

    public UserPingTask() {
    }

    public void execute(String token,final OnUserPingResultListener mOnUserPingResultListener) {
        UserUtil.getInstance().getHttpProxy().get(CHECK_URL + token,new TextHttpResponseHandler() {

            public void onSuccess(int responseCode, Header[] headers, String result) {
                log.debug("onSucce=" + result);
                try {
                    JSONObject jsonRespObj = new JSONObject(result);
                    int errorCode = jsonRespObj.getInt("rtn");
                    MemberInfo mPingMemberInfo = null;
                    if (errorCode == 0) {
                        JSONObject mUserInfoJsonObject = jsonRespObj.getJSONObject("userInfo");
                        mPingMemberInfo = MemberInfo.newInstance(mUserInfoJsonObject.toString());
                    }
                    if (mOnUserPingResultListener != null) {
                        mOnUserPingResultListener.onResult(errorCode, mPingMemberInfo);
                    }
                } catch (JSONException e) {
                    e.printStackTrace();
                    if (mOnUserPingResultListener != null) {
                        mOnUserPingResultListener.onResult(-100, null);
                    }
                }
            }

            @Override
            public void onFailure(int arg0, Header[] arg1, String arg2, Throwable error) {
                error.printStackTrace();
                log.debug("ping server onFailure=" + error.getMessage());
                if (mOnUserPingResultListener != null) {
                    mOnUserPingResultListener.onResult(-100, null);
                }
            }
        });
    }

    public interface OnUserPingResultListener {
        public void onResult(int rtn_code, MemberInfo mPingMemberInfo);
    }
}
