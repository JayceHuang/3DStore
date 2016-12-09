package com.runmit.sweedee.action.login;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.Util;

public class EmailLoginFragment extends LoginBaseFragment {
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		return inflater.inflate(R.layout.login, container, false);
	}

	@Override
	protected String getCountryCode() {
		return "";
	}

	@Override
	protected boolean checkisValidate() {
		boolean isEmailValidate = isEmailValidate(mEditUsername.getText().toString().trim());
		if(!isEmailValidate){
			Util.showToast(mFragmentActivity, getString(R.string.regist_invalidate_email), Toast.LENGTH_SHORT);
			mEditUsername.requestFocus();
			imInputMethodManager.toggleSoftInputFromWindow(mEditUsername.getWindowToken(), InputMethodManager.SHOW_IMPLICIT, InputMethodManager.HIDE_NOT_ALWAYS);
			
		}
		return isEmailValidate;
	}
	
	public static boolean isEmailValidate(String email) {
		//String check = "^([a-z0-9A-Z]+[-|\\.]?)+[a-z0-9A-Z]@([a-z0-9A-Z]+(-[a-z0-9A-Z]+)?\\.)+[a-zA-Z]{2,}$";
		String check = "^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$";
		Pattern regex = Pattern.compile(check);
		Matcher matcher = regex.matcher(email);
		boolean isMatched = matcher.matches();
		return isMatched;
	}
	
}
