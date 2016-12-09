package com.runmit.sweedee.action.login;

import java.io.Serializable;
import java.util.Locale;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.runmit.sweedee.R;
import com.runmit.sweedee.model.OpenCountryCode;
import com.runmit.sweedee.model.OpenCountryCode.CountryInfo;
import com.runmit.sweedee.sdkex.httpclient.simplehttp.HttpManager;
import com.runmit.sweedee.sdkex.httpclient.simplehttp.HttpManager.OnLoadFinishListener;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;

public class RegistBaseFragment extends Fragment {

	protected int selectPosition;

	private AlertDialog mAlertDialog = null;

	protected TextView textviewLocation;

	protected TextView registCountrycode;

	private OpenCountryCode mOpenCountryCode;

	public RadioButton radioButton_male,radioButton_female,radioButton_unknow; 

	protected int mRegistSexy = 0;// 注册性别

	private RadioGroup.OnCheckedChangeListener mChangeRadio = new RadioGroup.OnCheckedChangeListener() {
		
		@Override
		public void onCheckedChanged(RadioGroup group, int checkedId) {
			if (checkedId == R.id.radioButton_male) {
				mRegistSexy = 0;
			} else if (checkedId == R.id.radioButton_female) {
				mRegistSexy = 1;
			} else{
				mRegistSexy = 2;
			}
		}
	};

	protected void selectLocation() {
		MySimpleArrayAdapter mySimpleArrayAdapter = new MySimpleArrayAdapter(getActivity(), mOpenCountryCode.data);
		int position = SharePrefence.getInstance(getActivity()).getInt(SharePrefence.REGIST_LOCATION, selectPosition);
		mAlertDialog = new AlertDialog.Builder(getActivity()).setTitle(Util.setColourText(getActivity(), R.string.select_location, Util.DialogTextColor.BLUE))
				.setSingleChoiceItems(mySimpleArrayAdapter, position, new DialogInterface.OnClickListener() {

					@Override
					public void onClick(DialogInterface dialog, int which) {
						selectPosition = which;
						SharePrefence.getInstance(getActivity()).putInt(SharePrefence.REGIST_LOCATION, selectPosition);

						if (onUpdateRegionListener != null)
							onUpdateRegionListener.onRegionSelected(getCurrentLocaleInfo().name, getCurrentLocaleInfo().countrycode);

						mAlertDialog.dismiss();
					}
				}).setPositiveButton(R.string.cancel, new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int whichButton) {

					}
				}).create();
		mAlertDialog.show();
	}

	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {
		super.onViewCreated(view, savedInstanceState);
		((RadioGroup)view.findViewById(R.id.radioGroupSelectSexy)).setOnCheckedChangeListener(mChangeRadio);
		new HttpManager(getActivity()).loadCountryCode(new OnLoadFinishListener<OpenCountryCode>() {

			@Override
			public void onLoad(final OpenCountryCode openCountryCode) {
				mOpenCountryCode = openCountryCode;
				setLocation();
			}
		});
	}

	private void setLocation() {
		SharePrefence mSharePrefence = SharePrefence.getInstance(getActivity());
		if (mSharePrefence.containKey(SharePrefence.REGIST_LOCATION)) {
			selectPosition = mSharePrefence.getInt(SharePrefence.REGIST_LOCATION, selectPosition);
		} else {
			Locale defaultLocale = Locale.getDefault();
			for (int i = 0, size = mOpenCountryCode.data.size(); i < size; i++) {
				CountryInfo mCountryInfo = mOpenCountryCode.data.get(i);
				if (mCountryInfo.locale.toLowerCase().equals(defaultLocale.getCountry().toLowerCase())) {
					selectPosition = i;
				}
			}
		}
		textviewLocation.setText(getCurrentLocaleInfo().name);
		if (registCountrycode != null) {
			registCountrycode.setText(getCurrentLocaleInfo().countrycode);
		}
	}

	protected CountryInfo getCurrentLocaleInfo() {
		try {
			return mOpenCountryCode.data.get(selectPosition);
		} catch (IndexOutOfBoundsException e) {
			e.printStackTrace();
			selectPosition = 0;
			SharePrefence mSharePrefence = SharePrefence.getInstance(getActivity());
			mSharePrefence.remove(SharePrefence.REGIST_LOCATION);
			return mOpenCountryCode.data.get(0);
		}
	}

	protected boolean isValidatePassword(String password) {
		for (int i = 0; i < password.length(); i++) { // 循环遍历字符串
			if (Character.isDigit(password.charAt(i)) || Character.isLetter(password.charAt(i))) {// 如果是数字或者字母
				continue;
			} else {
				return false;
			}
		}
		return true;
	}

	// 检查一串字符中是否含有中文
	protected boolean isContainChinese(String string) {
		boolean res = false;
		char[] cTemp = string.toCharArray();
		for (int i = 0; i < string.length(); i++) {
			if (isChinese(cTemp[i])) {
				res = true;
				break;
			}
		}
		return res;
	}

	// 判定是否是中文字符
	protected boolean isChinese(char c) {
		Character.UnicodeBlock ub = Character.UnicodeBlock.of(c);
		if (ub == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS || ub == Character.UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS || ub == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A
				|| ub == Character.UnicodeBlock.GENERAL_PUNCTUATION || ub == Character.UnicodeBlock.CJK_SYMBOLS_AND_PUNCTUATION || ub == Character.UnicodeBlock.HALFWIDTH_AND_FULLWIDTH_FORMS) {
			return true;
		}
		return false;
	}

	public static int String_length(String value) {
		int valueLength = 0;
		String chinese = "[\u4e00-\u9fa5]";
		for (int i = 0; i < value.length(); i++) {
			String temp = value.substring(i, i + 1);
			if (temp.matches(chinese)) {
				valueLength += 2;
			} else {
				valueLength += 1;
			}
		}
		return valueLength;
	}

	public static class RegistBundle implements Serializable {
		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;

		public boolean isPhoneRegist;
		public String userName;
		public String userPassword;
		public String countrycode;
		public String language;
	}

	IUpdateSelectRegion onUpdateRegionListener;

	public void addUpdateSelectRegionListener(IUpdateSelectRegion listener) {
		onUpdateRegionListener = listener;
	}

	public interface IUpdateSelectRegion {
		void onRegionSelected(String name, String code);
	}

	public void updateRegionText(String name, String code) {
		textviewLocation.setText(name);
		if (registCountrycode != null) {
			registCountrycode.setText(code);
		}
	}
}
