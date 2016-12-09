package com.runmit.sweedee.action.login;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.login.RegistBaseFragment.RegistBundle;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.sdk.user.member.task.UserManager.LoginState;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

public abstract class LoginBaseFragment extends BaseFragment implements OnClickListener{
	
	XL_Log log = new XL_Log(LoginBaseFragment.class);

    protected EditText mEditUsername;
    private EditText mEditPassword;
    private Button mBtnLogin;
    private String mAccount;
    private String mPassword;
    private ProgressDialog mDialog;
	public SharePrefence mXLSharePrefence;
    
    protected InputMethodManager imInputMethodManager;
    
    private View mRootView;
    
    private Runnable runnable;
    
    public Handler loginHandler = new Handler();
    
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    	return super.onCreateView(inflater, container, savedInstanceState);
    }
    
    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
    	super.onViewCreated(view, savedInstanceState);
    	mRootView = view;
    	initView();
    }

    private void initView() {
        imInputMethodManager = (InputMethodManager) mFragmentActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
		mXLSharePrefence = SharePrefence.getInstance(getActivity().getApplicationContext());
        mDialog = new ProgressDialog(mFragmentActivity, R.style.ProgressDialogDark);
        mDialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
            @Override
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_BACK) {// 返回取消登录
                    if (UserManager.getInstance().isLogined()) {
                        return false;
                    }
                    UserManager.getInstance().cancelLogin();
                }
                return false;
            }
        });
        mEditUsername = (EditText) mRootView.findViewById(R.id.editTextLoginUsername);
        mEditPassword = (EditText) mRootView.findViewById(R.id.editTextLoginPassword);

        mEditUsername.addTextChangedListener(new TextWatcher() {
            private CharSequence temp;
            private int num = 40;
            private int selectionStart;
            private int selectionEnd;

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                temp = s;
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                selectionStart = mEditUsername.getSelectionStart();
                selectionEnd = mEditUsername.getSelectionEnd();
                if (temp != null && temp.length() > 50) {
                    Util.showToast(mFragmentActivity, getString(R.string.login_username_limit), Toast.LENGTH_SHORT);
                    mEditUsername.setText("");
                    return;
                }
                if (temp != null && temp.length() > num) {
                    s.delete(selectionStart - 1, selectionEnd);
                    int tempSelection = selectionStart;
                    mEditUsername.setText(s);
                    mEditUsername.setSelection(tempSelection);// 设置光标在最后
                }
            }
        });

        mEditPassword.addTextChangedListener(new TextWatcher() {
            private CharSequence temp;
            private int num = 16;
            private int selectionStart;
            private int selectionEnd;

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                temp = s;
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                selectionStart = mEditPassword.getSelectionStart();
                selectionEnd = mEditPassword.getSelectionEnd();
                if (temp != null && temp.length() > 18) {
                    Util.showToast(mFragmentActivity, getString(R.string.login_password_limit), Toast.LENGTH_SHORT);
                    mEditPassword.setText("");
                    return;
                }
                if (temp != null && temp.length() > num) {
                    s.delete(selectionStart - 1, selectionEnd);
                    int tempSelection = selectionStart;
                    mEditPassword.setText(s);
                    mEditPassword.setSelection(tempSelection);// 设置光标在最后
                }
            }
        });

        mRootView.findViewById(R.id.regetPwd).setOnClickListener(this);
        
        mBtnLogin = (Button) mRootView.findViewById(R.id.buttonLogin);
        mBtnLogin.setOnClickListener(this);

        mAccount = mFragmentActivity.getIntent().getStringExtra("username");
        mPassword = mFragmentActivity.getIntent().getStringExtra("password");
        if (!TextUtils.isEmpty(mPassword) && !TextUtils.isEmpty(mAccount)) {
            mEditUsername.setText(mAccount);
            mEditPassword.setText(mPassword);
            initLogin();
        } else {
            mAccount = UserManager.getInstance().getAccount();
            mPassword = UserManager.getInstance().getPassword();
            
            // 判断账号格式
            boolean isEmailType = mAccount != null && EmailLoginFragment.isEmailValidate(mAccount);
            if(this instanceof EmailLoginFragment){
            	if(!isEmailType){
            		mEditUsername.requestFocus();
            		showImm();
                	return;
            	}
            }else{
            	if(isEmailType){
            		mEditUsername.requestFocus();
            		showImm();
                	return;
            	}
            }

            if (!TextUtils.isEmpty(mAccount)) {
                mEditUsername.setText(mAccount);
                mEditUsername.setSelection(mEditUsername.length());
            }
            mEditUsername.requestFocus();
            showImm();
            if (!TextUtils.isEmpty(mPassword) && !TextUtils.isEmpty(mAccount)) {
                mEditPassword.setText(mPassword);
                mEditPassword.setSelection(mEditPassword.length());
            }
        }
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.buttonLogin:
                mEditPassword.requestFocus();
                showImm();
                imInputMethodManager.toggleSoftInput(InputMethodManager.SHOW_IMPLICIT, InputMethodManager.HIDE_NOT_ALWAYS);
                initLogin();
                break;
            case R.id.regetPwd:
            	startRegetPage();
            	break;
            default:
                break;
        }
    }

    protected void startRegetPage() {
    	Intent intent = new Intent(getActivity(), RegetPassword.class);
		getActivity().startActivity(intent);
	}

	void onActivityFinish() {
        if (runnable != null) {
            loginHandler.removeCallbacks(runnable);
        }
        log.debug("imInputMethodManager="+imInputMethodManager+",mEditPassword="+mEditPassword);
        if(imInputMethodManager != null && mEditPassword != null){
        	imInputMethodManager.hideSoftInputFromWindow(mEditPassword.getWindowToken(), InputMethodManager.RESULT_UNCHANGED_SHOWN);
        }
    }

    void onLoginChange(LoginState lastState, LoginState currentState, final Object userdata){
    	 if (mDialog != null) {
             mDialog.dismiss();
         }
    }

    private void initLogin() {
        if (mEditUsername.getText().length() < 1 || mEditUsername.getText().equals(" ")) {
            Util.showToast(mFragmentActivity, getResources().getString(R.string.please_enter_account), Toast.LENGTH_SHORT);
            mEditUsername.requestFocus();
            showImm();
            return;
        }
        if (mEditPassword.getText().length() < 1) {
            Util.showToast(mFragmentActivity, getResources().getString(R.string.please_enter_account), Toast.LENGTH_SHORT);
            mEditPassword.requestFocus();
            showImm();
            return;
        }
        if(checkisValidate()){
        	if (Util.isNetworkAvailable(mFragmentActivity)) {
                Util.showDialog(mDialog, getString(R.string.login_ing));
                UserManager.getInstance().userLogin(getCountryCode(),mEditUsername.getText().toString().trim(), mEditPassword.getText().toString().trim(),false);
            } else {
                Util.showToast(mFragmentActivity, getString(R.string.no_network_toast), 1000);
            }
        }
    }
    
    private void showImm() {
//    	mRootView.postDelayed(new Runnable() {
//			
//			@Override
//			public void run() {
//				boolean isShowGiftToast = mXLSharePrefence.getBoolean("REGIST_GIFT_ISTOAST", true);
//		    	if (isShowGiftToast) {
//		    		imInputMethodManager.toggleSoftInput(InputMethodManager.SHOW_IMPLICIT, InputMethodManager.HIDE_NOT_ALWAYS);
//		    	} else {
//		    		imInputMethodManager.hideSoftInputFromWindow(mEditPassword.getWindowToken(), InputMethodManager.RESULT_UNCHANGED_SHOWN);
//		    	}
//			}
//		}, 200);
    	
    }

	public void onActivityResult(RegistBundle mRegistBundle) {
		if(mRegistBundle != null && mEditUsername != null && mEditPassword != null && mBtnLogin != null){
			mEditUsername.setText(mRegistBundle.userName);
	        mEditPassword.setText(mRegistBundle.userPassword);
	        mBtnLogin.performClick();
		}
	}
	
	protected abstract String getCountryCode();
	
	protected abstract boolean checkisValidate();
}
