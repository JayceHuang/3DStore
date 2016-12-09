package com.runmit.sweedee.action.login;

import java.text.MessageFormat;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Paint;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.FragmentActivity;
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
import com.runmit.sweedee.manager.RegistManager;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.util.StaticHandler;
import com.runmit.sweedee.util.StaticHandler.MessageListener;
import com.runmit.sweedee.util.Util;

public class RegistPhoneFragment extends RegistBaseFragment implements View.OnClickListener,MessageListener{

    private View mRootView;

    private EditText editTextPhone;

    private Button btn_send_verification;

    private EditText editText_erification;

    private EditText editTextPassword;

	private EditText editTextNick;

    private LinearLayout btnPasswordExpressly;

    private Button btnSubmit;

    private TextView agree_btn;
    
    private Timer mTimer;
    
    private VerifyCodeTimerTask mVerifyCodeTimerTask;
    
	private RelativeLayout select_location_layout;

	private ProgressDialog mProgressDialog;
	
    private InputMethodManager imInputMethodManager;
    
    private Activity mActivity;
    
    public void handleMessage(Message msg) {
		switch (msg.what) {
		case RegistManager.MSG_GET_VERIFYCODE:
			if(isDetached()){
				return;
			}
			if(msg.arg1==0){
				btn_send_verification.setClickable(false);
				mTimer = new Timer();//开启验证码倒计时
				if(mVerifyCodeTimerTask != null){
					mVerifyCodeTimerTask.reset();
				}
				mVerifyCodeTimerTask = new VerifyCodeTimerTask();
		        mTimer.schedule(mVerifyCodeTimerTask, 0, 1000);
			}else{
				btn_send_verification.setClickable(true);
				if(msg.arg1 == ExceptionCode.CertificateException){
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_fail_get_verifycode_error_ssl_error), Toast.LENGTH_LONG);
				}else if(msg.arg1==RegistManager.REG_ACCOUNT_ERROR_PHONE_EXIT){
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_fail_phonenumber_already_regist), Toast.LENGTH_LONG);
				}else{
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_fail_get_verifycode_error), Toast.LENGTH_LONG);
				}
			}
			break;
		case RegistManager.MSG_REGISTER:
			Util.dismissDialog(mProgressDialog);
			if(isDetached()){
				return;
			}
			if (msg.arg1 == 0) {// 注册成功
				editTextPassword.requestFocus();
				imInputMethodManager.hideSoftInputFromWindow(editTextPassword.getWindowToken(), InputMethodManager.RESULT_UNCHANGED_SHOWN);
				Util.showToast(getActivity(), getString(R.string.regist_success), Toast.LENGTH_SHORT);
				Intent data = new Intent();
				
				RegistBundle mRegistBundle = new RegistBundle();
				mRegistBundle.isPhoneRegist = true;
				mRegistBundle.userName = editTextPhone.getText().toString();
				mRegistBundle.userPassword = editTextPassword.getText().toString();
				mRegistBundle.countrycode = getCurrentLocaleInfo().countrycode;
				mRegistBundle.language = getCurrentLocaleInfo().language;
				
				data.putExtra(RegistBundle.class.getSimpleName(), mRegistBundle);
				getActivity().setResult(0, data);
				getActivity().finish();
			} else {
				// 注册失败
				switch (msg.arg1) {
				case RegistManager.REG_ACCOUT_ERROR_ACTIVE_FAIL:
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_success_and_send_email_fail) + msg.arg1, Toast.LENGTH_SHORT);
					break;
				case RegistManager.REG_ACCOUT_ERROR_VERIFYCODETIMEOUT:
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_fail_verifycode_timeout) + msg.arg1, Toast.LENGTH_SHORT);
					break;
				case RegistManager.REG_ACCOUT_ERROR_DEVICE_LIMIT:
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_fail_device_limit) + msg.arg1, Toast.LENGTH_SHORT);
					break;
				case RegistManager.REG_ACCOUT_ERROR_VERIFYCODE_ERROR:
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_fail_verifycode_error) + msg.arg1, Toast.LENGTH_SHORT);
					break;
				case RegistManager.REG_ACCOUT_ERROR_BIND_ERROR:
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_fail_bind_device_error) + msg.arg1, Toast.LENGTH_SHORT);
					break;
				case ExceptionCode.CertificateException:
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_ssl_error_toast), Toast.LENGTH_SHORT);
					break;
				default:
					Util.showToast(getActivity(), this.mActivity.getString(R.string.regist_fail_default) + msg.arg1, Toast.LENGTH_SHORT);
					break;
				}
			}
			break;
		default:
			break;
		}
	};
	
    private StaticHandler mHandler;
    
    private class TimeTaskRunnable implements Runnable{

    	int id=60;
    	
		@Override
		public void run() {
			try {
				if(id==0){
	        		btn_send_verification.setClickable(true);
	        		btn_send_verification.setText(R.string.regist_send_verify_code);
	        		if(mTimer != null){
	        			mTimer.cancel();
	            		mTimer=null;
	        		}
	        	}else{
	        		btn_send_verification.setText(MessageFormat.format(getString(R.string.reget_send_verify_code), ""+id--));
	        	}
			} catch (IllegalStateException e) {
				e.printStackTrace();
			}
        }
    	
    }
    
    private class VerifyCodeTimerTask extends TimerTask {
        
    	TimeTaskRunnable mVerifyCodeRunnable = new TimeTaskRunnable();
    	
    	public void run() {
            mHandler.post(mVerifyCodeRunnable);
        }
    	
    	public void reset(){
    		mHandler.removeCallbacks(mVerifyCodeRunnable);
    	}
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        this.mActivity = activity;
    }
    
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mRootView = inflater.inflate(R.layout.register_phone_item, null);
        mHandler = new StaticHandler(this);
        initUI();
        return mRootView;
    }
    
    @Override
    public void onDestroy() {
    	super.onDestroy();
    	if(mVerifyCodeTimerTask != null){
			mVerifyCodeTimerTask.reset();
		}
    	if(mTimer != null){
			mTimer.cancel();
    		mTimer=null;
		}
    }

    /**
     * 手机号是否正确的正则表达式
     *
     * @param phoneNumber
     * @return
     */
    private boolean isPhoneValidate(String phoneNumber) {
    	if(getCurrentLocaleInfo().locale.equalsIgnoreCase(Locale.CHINA.getCountry())){
          String check = "^((13[0-9])|(15[0-9])|(17[0-9])|(18[0-9]))\\d{8}$";
          Pattern regex = Pattern.compile(check);
          Matcher matcher = regex.matcher(phoneNumber);
          boolean isMatched = matcher.matches();
          return isMatched;
    	}else if(getCurrentLocaleInfo().locale.equalsIgnoreCase(Locale.TAIWAN.getCountry())){
    		String check = "^(09|9)\\d{8}$";
            Pattern regex = Pattern.compile(check);
            Matcher matcher = regex.matcher(phoneNumber);
            boolean isMatched = matcher.matches();
            return isMatched;
    	}else{
    		return phoneNumber != null && phoneNumber.length() > 0;
    	}
    }


    
    private void initUI() {
    	mProgressDialog = new ProgressDialog(getActivity(),R.style.ProgressDialogDark);
    	registCountrycode = (TextView) mRootView.findViewById(R.id.regist_countrycode);
    	registCountrycode.clearFocus();
        editTextPhone = (EditText) mRootView.findViewById(R.id.editTextPhone);
        editTextPassword = (EditText) mRootView.findViewById(R.id.editTextPassword);
        btn_send_verification = (Button) mRootView.findViewById(R.id.btn_send_verification);
        editText_erification = (EditText) mRootView.findViewById(R.id.editText_erification);
        btnSubmit = (Button) mRootView.findViewById(R.id.btnSubmit);
        agree_btn = (TextView) mRootView.findViewById(R.id.agree_btn);
        agree_btn.getPaint().setFlags(Paint.UNDERLINE_TEXT_FLAG); // 下划线
        agree_btn.getPaint().setAntiAlias(true);// 抗锯齿
		
        editTextNick = (EditText) mRootView.findViewById(R.id.editTextNick);
        btnPasswordExpressly = (LinearLayout) mRootView.findViewById(R.id.psw_switch_layout);
        
        select_location_layout = (RelativeLayout) mRootView.findViewById(R.id.select_location_layout);
        textviewLocation = (TextView) mRootView.findViewById(R.id.textviewLocation);
		
        btn_send_verification.setOnClickListener(this);
        btnSubmit.setOnClickListener(this);
        agree_btn.setOnClickListener(this);
        select_location_layout.setOnClickListener(this);
        btnPasswordExpressly.setOnClickListener(this);

        imInputMethodManager = (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);
        editTextNick.setFilters(new InputFilter[]{new InputFilter.LengthFilter(16)});
        editTextPassword.setFilters(new InputFilter[]{new InputFilter.LengthFilter(16)});
        editText_erification.setFilters(new InputFilter[]{new InputFilter.LengthFilter(10)});
        editTextPhone.setFilters(new InputFilter[]{new InputFilter.LengthFilter(11)});
        registCountrycode.setFilters(new InputFilter[]{new InputFilter.LengthFilter(6)});
        
        registCountrycode.requestFocus();
		imInputMethodManager.hideSoftInputFromWindow(registCountrycode.getWindowToken(), InputMethodManager.RESULT_UNCHANGED_SHOWN);
    }

    private void checkAndRegist(){
    	String phoneNumber=editTextPhone.getText().toString();
    	if(isPhoneValidate(phoneNumber)){//手机号码正确
    		String verifyCode=editText_erification.getText().toString();
    		if(verifyCode!=null && verifyCode.length()>0){//验证码正确
    			String password = editTextPassword.getText().toString();
    			if (isContainChinese(password)) {// 含中文
    				Util.showToast(getActivity(), getString(R.string.regist_psw_contain_chinese), Toast.LENGTH_SHORT);
    			} else if(!isValidatePassword(password)){
    				Util.showToast(getActivity(), getString(R.string.regist_psw_contain_special_word), Toast.LENGTH_SHORT);
    			} else if (password.length() >= 6 && password.length() <= 16) {// 长度合法
    				String nicky_name = editTextNick.getText().toString();
    				int len = String_length(nicky_name);
    				if(len >= 4 && len <= 16){//nickyname合法
    					if(Util.isNetworkAvailable()){
    						Util.showDialog(mProgressDialog, getString(R.string.regist_ing));
            				RegistManager mRegistManager = new RegistManager();
            				String location = getCurrentLocaleInfo().locale;
            				mRegistManager.startRegister(mRegistSexy , 2, phoneNumber, password,verifyCode, nicky_name, location, mHandler);
    					}else{
    						Util.showToast(getActivity(), getString(R.string.no_network_toast), Toast.LENGTH_SHORT);
    					}
    				}else{
    					Util.showToast(getActivity(), getString(R.string.regist_nicky_invalidate), Toast.LENGTH_SHORT);
    				}
    			} else {
    				// 密码长度不合法
    				Util.showToast(getActivity(), getString(R.string.regist_password_invalidate), Toast.LENGTH_SHORT);
    			}
    		}else{
    			Util.showToast(getActivity(), getString(R.string.regist_invalidate_verifycode), Toast.LENGTH_LONG);
    		}
    	}else{
    		Util.showToast(getActivity(), getString(R.string.regist_invalidate_password), Toast.LENGTH_LONG);
    	}
    }
    
    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.btn_send_verification:
            	String phoneNumber=editTextPhone.getText().toString();
            	if(isPhoneValidate(phoneNumber)){
            		btn_send_verification.setClickable(false);
            		RegistManager.getInstance().getVerifyCode(phoneNumber, getCurrentLocaleInfo().countrycode,mHandler);
            	}else{
            		Util.showToast(getActivity(), getString(R.string.regist_invalidate_password), Toast.LENGTH_LONG);
            	}
                break;
            case R.id.btnSubmit:
            	checkAndRegist();
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
}
