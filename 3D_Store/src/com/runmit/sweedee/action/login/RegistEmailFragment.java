package com.runmit.sweedee.action.login;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Paint;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.InputFilter;
import android.text.Selection;
import android.text.Spannable;
import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.text.method.TransformationMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.action.login.RegistBaseFragment.RegistBundle;
import com.runmit.sweedee.manager.RegistManager;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.util.Util;

public class RegistEmailFragment extends RegistBaseFragment implements View.OnClickListener {

	private View mRootView;

	private EditText editTextEmail;

	private EditText editTextPassword;

	private LinearLayout btnPasswordExpressly;

	private Button btnSubmit;

	private TextView agree_btn;

	private ProgressDialog mProgressDialog;

	private InputMethodManager imInputMethodManager;

	private EditText editTextNick;

	private RelativeLayout select_location_layout;
	
	private Handler mHandler = new Handler() {

		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case RegistManager.MSG_REGISTER:
				Util.dismissDialog(mProgressDialog);
				if (msg.arg1 == 0 || msg.arg1 == RegistManager.REG_ACCOUT_ERROR_ACTIVE_FAIL) {// 注册成功
					if(msg.arg1 == RegistManager.REG_ACCOUT_ERROR_ACTIVE_FAIL){
						Util.showToast(getActivity(), getString(R.string.regist_success_and_send_email_fail), Toast.LENGTH_SHORT);
					}
					editTextPassword.requestFocus();
					imInputMethodManager.hideSoftInputFromWindow(editTextPassword.getWindowToken(), InputMethodManager.RESULT_UNCHANGED_SHOWN);
					Util.showToast(getActivity(), getString(R.string.regist_success), Toast.LENGTH_SHORT);
					Intent data = new Intent();
					RegistBundle mRegistBundle = new RegistBundle();
					mRegistBundle.isPhoneRegist = false;
					mRegistBundle.userName = editTextEmail.getText().toString();
					mRegistBundle.userPassword = editTextPassword.getText().toString();
					mRegistBundle.countrycode = getCurrentLocaleInfo().countrycode;
					mRegistBundle.language = getCurrentLocaleInfo().language;
					data.putExtra(RegistBundle.class.getSimpleName(), mRegistBundle);
					
					getActivity().setResult(0, data);
					getActivity().finish();
				} else {
					// 注册失败
					switch (msg.arg1) {
//					case RegistManager.REG_ACCOUT_ERROR_ACTIVE_FAIL:
//						Util.showToast(getActivity(), "注册失败，发送激活邮件失败，错误码：" + msg.arg1, Toast.LENGTH_SHORT);
//						break;
					case RegistManager.REG_ACCOUT_ERROR_VERIFYCODETIMEOUT:
						Util.showToast(getActivity(), getString(R.string.regist_fail_verifycode_timeout) + msg.arg1, Toast.LENGTH_SHORT);
						break;
					case RegistManager.REG_ACCOUT_ERROR_DEVICE_LIMIT:
						Util.showToast(getActivity(), getString(R.string.regist_fail_device_limit) + msg.arg1, Toast.LENGTH_SHORT);
						break;
					case RegistManager.REG_ACCOUT_ERROR_EMAIL_EXIT:
						Util.showToast(getActivity(), getString(R.string.regist_fail_email_already_regist) + msg.arg1, Toast.LENGTH_SHORT);
						break;
					case RegistManager.REG_ACCOUT_ERROR_EMAIL_INVALIDATE:
						Util.showToast(getActivity(), getString(R.string.regist_fail_email_invalidate) + msg.arg1, Toast.LENGTH_SHORT);
						break;
					case RegistManager.REG_ACCOUT_ERROR_VERIFYCODE_ERROR:
						Util.showToast(getActivity(), getString(R.string.regist_fail_verifycode_error) + msg.arg1, Toast.LENGTH_SHORT);
						break;
					case RegistManager.REG_ACCOUT_ERROR_BIND_ERROR:
						Util.showToast(getActivity(), getString(R.string.regist_fail_bind_device_error) + msg.arg1, Toast.LENGTH_SHORT);
						break;
					case ExceptionCode.CertificateException:
						Util.showToast(getActivity(), getString(R.string.regist_ssl_error_toast), Toast.LENGTH_SHORT);
						break;
					default:
						Util.showToast(getActivity(), getString(R.string.regist_fail_default) + msg.arg1, Toast.LENGTH_SHORT);
						break;
					}
				}
				break;

			default:
				break;
			}
		}
	};

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		mRootView = inflater.inflate(R.layout.register_email_item, null);
		initUI();
		return mRootView;
	}

	private void initUI() {
		imInputMethodManager = (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);
		mProgressDialog = new ProgressDialog(getActivity(),R.style.ProgressDialogDark);
		editTextEmail = (EditText) mRootView.findViewById(R.id.editTextEmail);
		editTextPassword = (EditText) mRootView.findViewById(R.id.editTextPassword);
		editTextNick = (EditText) mRootView.findViewById(R.id.editTextNick);
		btnSubmit = (Button) mRootView.findViewById(R.id.btnSubmit);
		agree_btn = (TextView) mRootView.findViewById(R.id.agree_btn);
		agree_btn.getPaint().setFlags(Paint.UNDERLINE_TEXT_FLAG); // 下划线
        agree_btn.getPaint().setAntiAlias(true);// 抗锯齿
        
		select_location_layout = (RelativeLayout) mRootView.findViewById(R.id.select_location_layout);
		btnPasswordExpressly = (LinearLayout) mRootView.findViewById(R.id.psw_switch_layout);
		textviewLocation = (TextView) mRootView.findViewById(R.id.textviewLocation);
		
		btnPasswordExpressly.setOnClickListener(this);
		btnSubmit.setOnClickListener(this);
		agree_btn.setOnClickListener(this);
		select_location_layout.setOnClickListener(this);
		
        editTextNick.setFilters(new InputFilter[]{new InputFilter.LengthFilter(16)});
        editTextPassword.setFilters(new InputFilter[]{new InputFilter.LengthFilter(16)});
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.btnSubmit:
			checkAndRegistCount();
			break;
		case R.id.select_location_layout:
			selectLocation();
			break;
		case R.id.agree_btn:
			Intent intent = new Intent(getActivity(), AgreementActivity.class);
			startActivity(intent);
			break;
		case R.id.psw_switch_layout:
			TransformationMethod method = editTextPassword.getTransformationMethod();
			if (method != PasswordTransformationMethod.getInstance()) {
				btnPasswordExpressly.setBackgroundResource(R.drawable.ic_eye_off);
				editTextPassword.setTransformationMethod(PasswordTransformationMethod.getInstance()); // 设置密码类型
			} else {
				btnPasswordExpressly.setBackgroundResource(R.drawable.ic_eye_on);
				editTextPassword.setTransformationMethod(HideReturnsTransformationMethod.getInstance()); // 设置明文
			}
			CharSequence text = editTextPassword.getText();
			if (text instanceof Spannable) {
				Spannable spanText = (Spannable) text;
				Selection.setSelection(spanText, text.length());
			}
			break;
		default:
			break;
		}
	}



	private void checkAndRegistCount() {
		String accountEmail = editTextEmail.getText().toString();
		if (!isEmailValidate(accountEmail)) {// 邮箱不合法
			Util.showToast(getActivity(), getString(R.string.regist_invalidate_email), Toast.LENGTH_SHORT);
			return;
		} else {
			String password = editTextPassword.getText().toString();
			if (isContainChinese(password)) {// 含中文
				Util.showToast(getActivity(), getString(R.string.regist_psw_contain_chinese), Toast.LENGTH_SHORT);
			} else if(!isValidatePassword(password)){
				Util.showToast(getActivity(), getString(R.string.regist_psw_contain_special_word), Toast.LENGTH_SHORT);
			} else if (password.length() >= 6 && password.length() <= 16) {// 长度合法
				if(Util.isNetworkAvailable()){
					String nicky_name = editTextNick.getText().toString();
					int len = String_length(nicky_name);
					if(len >=4 && len <= 16){
						Util.showDialog(mProgressDialog, getString(R.string.regist_ing));
						RegistManager mRegistManager = new RegistManager();
						String location = getCurrentLocaleInfo().locale;
						mRegistManager.startRegister(mRegistSexy,1, accountEmail, password, "", nicky_name, location, mHandler);
					}else{
						Util.showToast(getActivity(), getString(R.string.regist_nicky_invalidate), Toast.LENGTH_SHORT);
					}
				}else{
					Util.showToast(getActivity(), getString(R.string.no_network_toast), Toast.LENGTH_SHORT);
				}
			} else {
				// 密码长度不合法
				Util.showToast(getActivity(), getString(R.string.regist_password_invalidate), Toast.LENGTH_SHORT);
			}
		}
	}

	/**
	 * 邮箱的正则表达式
	 * 
	 * @param email
	 * @return
	 */
	private boolean isEmailValidate(String email) {
		//String check = "^([a-z0-9A-Z]+[-|\\.]?)+[a-z0-9A-Z]@([a-z0-9A-Z]+(-[a-z0-9A-Z]+)?\\.)+[a-zA-Z]{2,}$";
		String check = "^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$";
		Pattern regex = Pattern.compile(check);
		Matcher matcher = regex.matcher(email);
		boolean isMatched = matcher.matches();
		return isMatched;
	}
}
