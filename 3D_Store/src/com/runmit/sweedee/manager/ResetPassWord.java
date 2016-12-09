package com.runmit.sweedee.manager;

import org.apache.http.Header;
import org.apache.http.entity.ByteArrayEntity;

import android.os.Handler;
import android.text.TextUtils;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.sdk.user.member.task.UserErrorCode;
import com.runmit.sweedee.sdk.user.member.task.UserUtil;
import com.runmit.sweedee.sdkex.httpclient.LoginAsyncHttpProxyListener;
import com.runmit.sweedee.util.XL_Log;

public class ResetPassWord {

	private XL_Log log = new XL_Log(ResetPassWord.class);

	public static final int MSG_GET_VERIFYCODE = 101;
	public static final int MSG_RESET_PASSWORD = MSG_GET_VERIFYCODE + 1;

	private final String url_get_verify = "https://zeus.d3dstore.com/v1.5/findpassword";

	private final String url_update_pwd = "https://zeus.d3dstore.com/v1.5/updatePwd";
	
	public static final int ERROR_CODE_ACCOUNT_ISNOT_EXIT = 20012;//该手机号不存在，请输入正确的手机号
	
	public static final int ERROR_CODE_INVALIDATE_VERIFYCODE = 20006;//短信验证码输入错误，请重新输入

	public ResetPassWord() {

	}

	/**
	 * 
	 * @param account
	 * @param findType找回方式，1:邮箱，2:手机
	 * @param mHandler
	 */
	public void getVerifyCode(String account,String countrycode, int findType, final Handler mHandler) {
		if (TextUtils.isEmpty(account)) {
			mHandler.obtainMessage(MSG_GET_VERIFYCODE, UserErrorCode.ACCOUNT_INVALID, 0).sendToTarget();
			return;
		}
		JsonObject jsonRequObj = new JsonObject();
		jsonRequObj.addProperty("account", account);
		jsonRequObj.addProperty("countrycode", countrycode);
		jsonRequObj.addProperty("findType", findType);

		String jsonContent = jsonRequObj.toString();
		ByteArrayEntity byteEntity = new ByteArrayEntity(jsonContent.getBytes());
		log.debug("execute jsonContent=" + jsonContent);
		UserUtil.getInstance().getHttpProxy().post(url_get_verify, byteEntity, new LoginAsyncHttpProxyListener() {
			public void onSuccess(int responseCode, Header[] headers, String result) {
				log.debug("url_get_verify onSuccess result=" + result);
				JsonObject jsonRespObj = new JsonParser().parse(result).getAsJsonObject();
				int errorCode = jsonRespObj.get("rtn").getAsInt();
				mHandler.obtainMessage(MSG_GET_VERIFYCODE, errorCode, 0).sendToTarget();
			}

			public void onFailure(Throwable error) {
				log.debug("url_get_verify onFailure error =" + error.getMessage());
				error.printStackTrace();
				int errorCode = ExceptionCode.getErrorCode(error);
				mHandler.obtainMessage(MSG_GET_VERIFYCODE, errorCode, 0).sendToTarget();
			}
		});

	}

	/**
	 * @param validphoneAccount
	 * @param password
	 * @param token
	 * @param mHandler
	 */
	public void resetPassWord(String validphoneAccount, String password, String verifycode, final Handler mHandler) {
		if (TextUtils.isEmpty(validphoneAccount) || TextUtils.isEmpty(password) || TextUtils.isEmpty(verifycode)) {
			mHandler.obtainMessage(MSG_RESET_PASSWORD, UserErrorCode.ACCOUNT_INVALID, 0).sendToTarget();
			return;
		}

		JsonObject jsonRequObj = new JsonObject();
		jsonRequObj.addProperty("phoneNumber", validphoneAccount);
		jsonRequObj.addProperty("verifycode", verifycode);
		jsonRequObj.addProperty("password", password);

		String jsonContent = jsonRequObj.toString();
		ByteArrayEntity byteEntity = new ByteArrayEntity(jsonContent.getBytes());
		log.debug("url_update_pwd execute jsonContent=" + jsonContent);
		UserUtil.getInstance().getHttpProxy().post(url_update_pwd, byteEntity, new LoginAsyncHttpProxyListener() {
			public void onSuccess(int responseCode, Header[] headers, String result) {
				log.debug("url_update_pwd onSuccess result=" + result);
				JsonObject jsonRespObj = new JsonParser().parse(result).getAsJsonObject();
				int errorCode = jsonRespObj.get("rtn").getAsInt();
				mHandler.obtainMessage(MSG_RESET_PASSWORD, errorCode, 0).sendToTarget();
			}

			public void onFailure(Throwable error) {
				log.debug("url_update_pwd onFailure error =" + error.getMessage());
				error.printStackTrace();
				int errorCode = ExceptionCode.getErrorCode(error);
				mHandler.obtainMessage(MSG_RESET_PASSWORD, errorCode, 0).sendToTarget();
			}
		});
	}
}
