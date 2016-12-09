package com.runmit.sweedee.action.login;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.login.RegetPassword.Status;
import com.runmit.sweedee.manager.RegistManager;
import com.runmit.sweedee.manager.ResetPassWord;
import com.runmit.sweedee.util.Util;

public class EmailRegetFragment extends BaseFragment {
	
	private RegetPassword mFragmentActivity;
	private Button next_btn;
	private EditText editTextLoginUsername;
	private Handler mHandler = new Handler(){
		public void handleMessage(Message msg) {
    		switch (msg.what) {
			case ResetPassWord.MSG_GET_VERIFYCODE:
				mFragmentActivity.hideProgress();
				if(msg.arg1 == 0){
					goSuccess();
				}
				else if(msg.arg1 == 1){
					Util.showToast(getActivity(), getActivity().getString(R.string.account_not_found), Toast.LENGTH_LONG);
				}
				else{
					Util.showToast(getActivity(), getActivity().getString(R.string.play_neterror_and_trylater), Toast.LENGTH_LONG); //"网络错误" 
				}
				break;
				default:break;
    		}
    	}
	};
	
	private String email;
    
    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        this.mFragmentActivity = (RegetPassword) activity;
    }
	
	protected void goSuccess() {
		mFragmentActivity.setValidAccount(email, 1);
		mFragmentActivity.setStatus(Status.Reset);
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		return inflater.inflate(R.layout.reget_pwd_email, container, false);
	}
	
	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {
		super.onViewCreated(view, savedInstanceState);
		editTextLoginUsername = (EditText)view.findViewById(R.id.editTextLoginUsername);
		next_btn = (Button)view.findViewById(R.id.next);
		next_btn.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				resetPwdEmail();				
			}
		});
	}

	protected void resetPwdEmail() {
		email = editTextLoginUsername.getText().toString();
		if (!isEmailValidate(email)) { //"邮箱不合法"
			Util.showToast(getActivity(), getString(R.string.regist_invalidate_email), Toast.LENGTH_SHORT);
		} else {// 长度合法
			if(Util.isNetworkAvailable()){
				(new ResetPassWord()).getVerifyCode(email, "", 1, mHandler);
				mFragmentActivity.showProgress(getString(R.string.sending_verify_code)); //"正在发送验证码"
			}else{
				Util.showToast(getActivity(), getString(R.string.no_network_toast), Toast.LENGTH_SHORT);
			}
		} 
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
