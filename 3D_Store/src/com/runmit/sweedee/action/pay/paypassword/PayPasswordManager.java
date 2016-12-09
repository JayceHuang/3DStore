package com.runmit.sweedee.action.pay.paypassword;

import android.text.TextUtils;

import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.util.AESEncryptor;
import com.runmit.sweedee.util.MD5;
import com.runmit.sweedee.util.SharePrefence;

public class PayPasswordManager {

	private static final String PAY_PASSWORD_KEY = "4fe8b97cbede";
	
	public static void savePassword(String password){
		String mUserPayPasswordKey=PAY_PASSWORD_KEY+"_"+UserManager.getInstance().getUserId();
		SharePrefence mSharePrefence = new SharePrefence(PAY_PASSWORD_KEY);
		String mEncrytUserPayPasswordKey = null;
		try {
			mEncrytUserPayPasswordKey = AESEncryptor.encryptStringMD5(password);
			mSharePrefence.putString(mUserPayPasswordKey,mEncrytUserPayPasswordKey);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
			
	}
	
	public static String getPayPassword(){
		String mUserPayPasswordKey=PAY_PASSWORD_KEY+"_"+UserManager.getInstance().getUserId();
		SharePrefence mSharePrefence = new SharePrefence(PAY_PASSWORD_KEY);
		String password = mSharePrefence.getString(mUserPayPasswordKey, null);
		return password;
	}
	
	public static boolean checkPassword(String enterPassword){
		if(TextUtils.isEmpty(enterPassword)){
			return false;
		}
		try {
			enterPassword=AESEncryptor.encryptStringMD5(enterPassword);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			return false;
		}
		return enterPassword.equals(getPayPassword());
	}
	
	public static boolean isPayPasswordEmpty(){
		String mUserPayPasswordKey=PAY_PASSWORD_KEY+"_"+UserManager.getInstance().getUserId();
		SharePrefence mSharePrefence = new SharePrefence(PAY_PASSWORD_KEY);
		String password = mSharePrefence.getString(mUserPayPasswordKey, null);
		return TextUtils.isEmpty(password);
	}
}
