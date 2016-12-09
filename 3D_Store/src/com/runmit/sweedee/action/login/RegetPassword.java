package com.runmit.sweedee.action.login;

import java.text.MessageFormat;

import android.app.ProgressDialog;
import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.manager.ResetPassWord;
import com.runmit.sweedee.model.OpenCountryCode;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.sdkex.httpclient.simplehttp.HttpManager;
import com.runmit.sweedee.sdkex.httpclient.simplehttp.HttpManager.OnLoadFinishListener;
import com.runmit.sweedee.util.MD5;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.view.PagerSlidingTabStrip;

public class RegetPassword extends FragmentActivity {
	
	private ViewPager mPager;
	private PagerSlidingTabStrip regist_tabs;
	
	private OpenCountryCode mOpenCountryCode;
	
	public enum Status {
		Tab,
		Reset,
		Success
	}
	
	private Status currentStatus;
	
	private RegetPWDPageAdapter mRegetPWDPageAdapter;
	private View page_tab;
	private View page_reset;
	private View page_Success;
	private EditText new_pwd1;
	private EditText new_pwd2;
	private Button reset_submit;
	private EditText editTextPhoneverify;
	private TextView postTips;
		
	private String validAccount;
	private ProgressDialog mProgressDialog;
	
	private Handler mHandler = new Handler(){
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case ResetPassWord.MSG_RESET_PASSWORD:
				//0-成功
				if(msg.arg1 == 0){
					UserManager.getInstance().setPassword(new_pwd1.getText().toString());
					setStatus(Status.Success);
				}else if(msg.arg1 == 1){//1-修改失败
					Util.showToast(RegetPassword.this, RegetPassword.this.getString(R.string.pwd_modify_error), Toast.LENGTH_LONG); //"修改密码失败"
				}else if(msg.arg1 == ResetPassWord.ERROR_CODE_INVALIDATE_VERIFYCODE){
					Util.showToast(RegetPassword.this, getString(R.string.verify_code_error_toast), Toast.LENGTH_LONG);
					editTextPhoneverify.requestFocus();
				}else{
					Util.showToast(RegetPassword.this, RegetPassword.this.getString(R.string.play_neterror_and_trylater), Toast.LENGTH_LONG); //"网络错误" 
				}
				break;
			default:break;
			}
		}
	};
		
	@Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_reget_password);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setDisplayShowHomeEnabled(false);
        
        initView();
        setStatus(Status.Tab);
        
        mOpenCountryCode = (OpenCountryCode) getIntent().getSerializableExtra("openCountyCode");
        if(mOpenCountryCode == null){
        	new HttpManager(this).loadCountryCode(new OnLoadFinishListener<OpenCountryCode>() {
    			
    			@Override
    			public void onLoad(final OpenCountryCode openCountryCode) {
    				mOpenCountryCode = openCountryCode;
    				setLocation();
    			}
    		});
        }else {
        	setLocation();
		}
    }
	
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }
	

	private void setLocation() {
		if(currentStatus == Status.Tab){
			if(mPager.getCurrentItem() == 0){
				PhoneRegetFragment frag = (PhoneRegetFragment) ((RegetPWDPageAdapter)mPager.getAdapter()).getItem(0);
				frag.setOpenCountryCode(mOpenCountryCode);
			}
		}
	}
	
	private void initView() {
		mProgressDialog = new ProgressDialog(this, R.style.ProgressDialogDark);
		
		page_tab = findViewById(R.id.status_1);
        mPager = (ViewPager) findViewById(R.id.vPager);
		mRegetPWDPageAdapter = new RegetPWDPageAdapter(getSupportFragmentManager());
		mPager.setAdapter(mRegetPWDPageAdapter);
		
		regist_tabs = (PagerSlidingTabStrip) findViewById(R.id.regist_tabs);
		regist_tabs.setViewPager(mPager);
		regist_tabs.setDividerColorResource(R.color.background_green);
		regist_tabs.setIndicatorColorResource(R.color.thin_blue);
		regist_tabs.setSelectedTextColorResource(R.color.thin_blue);
		regist_tabs.setDividerColor(Color.TRANSPARENT);
		regist_tabs.setVisibility(View.GONE);
		DisplayMetrics dm = getResources().getDisplayMetrics();
		regist_tabs.setUnderlineHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, dm));
		regist_tabs.setIndicatorHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 2, dm));
		regist_tabs.setTextSize((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 16, dm));
		regist_tabs.setTextColor(getResources().getColor(R.color.game_tabs_text_color));
		
		postTips = (TextView)findViewById(R.id.post_tips);
		editTextPhoneverify = (EditText)findViewById(R.id.editTextPhoneverify);
		
		page_reset = findViewById(R.id.status_2);
		new_pwd1 = (EditText) findViewById(R.id.new_pwd1);
		new_pwd2 = (EditText) findViewById(R.id.new_pwd2);
		reset_submit = (Button)findViewById(R.id.reset_submit);
		reset_submit.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				resetPassWordCommit();
			}
		});
		
		page_Success = findViewById(R.id.status_3);
	}
	
	public void showProgress(String tips){
		Util.showDialog(mProgressDialog, tips, false);
	}
	
	public void hideProgress(){
		Util.dismissDialog(mProgressDialog);
	}

	private void resetPassWordCommit() {
		String verifyStr = editTextPhoneverify.getText().toString();
		if(TextUtils.isEmpty(verifyStr) || verifyStr.length() != 6){//检测验证码是否正确
			Util.showToast(RegetPassword.this, getString(R.string.regist_invalidate_verifycode), Toast.LENGTH_LONG);
			return;
		}
		String password = new_pwd1.getText().toString();
		String password2 = new_pwd2.getText().toString();
		if(TextUtils.isEmpty(password)){
			Util.showToast(this, getString(R.string.pwd_new_tips), Toast.LENGTH_SHORT);
			return;
		}
		
		if(TextUtils.isEmpty(password2)){
			Util.showToast(this, getString(R.string.pwd_confirm_tips), Toast.LENGTH_SHORT);
			return;
		}
		
		if (isContainChinese(password)) {// 含中文
			Util.showToast(this, getString(R.string.regist_psw_contain_chinese), Toast.LENGTH_SHORT);
		} else if(!isValidatePassword(password)){
			Util.showToast(this, getString(R.string.regist_psw_contain_special_word), Toast.LENGTH_SHORT);
		} else if (password.length() >= 6 && password.length() <= 16) {// 长度合法
			if(!password.equals(password2)){
				Util.showToast(this, getString(R.string.pwd_not_match_error), Toast.LENGTH_SHORT); //密码不一致 
			}else{
				if(!Util.isNetworkAvailable()){
					Util.showToast(this, getString(R.string.no_network_toast), Toast.LENGTH_SHORT);
				}else {
					(new ResetPassWord()).resetPassWord(validAccount, MD5.encrypt(password), verifyStr, mHandler);
				}
			}
		}else {
			Util.showToast(this, getString(R.string.regist_password_invalidate), Toast.LENGTH_SHORT);
		}
	}
	
	// 检查一串字符中是否含有中文
	private boolean isContainChinese(String string) {
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
	private boolean isChinese(char c) {
        Character.UnicodeBlock ub = Character.UnicodeBlock.of(c);
        if (ub == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS || ub == Character.UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS || ub == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A
                || ub == Character.UnicodeBlock.GENERAL_PUNCTUATION || ub == Character.UnicodeBlock.CJK_SYMBOLS_AND_PUNCTUATION || ub == Character.UnicodeBlock.HALFWIDTH_AND_FULLWIDTH_FORMS) {
            return true;
        }
        return false;
    } 
	
	private boolean isValidatePassword(String password) {
        for (int i = 0; i < password.length(); i++) { //循环遍历字符串   
            if (Character.isDigit(password.charAt(i)) || Character.isLetter(password.charAt(i))) {//如果是数字或者字母
                continue;
            } else {
                return false;
            }
        }
        return true;
    }

	public void setValidAccount(String acc, int from){
		String tips;
		if(from == 0){
			tips = acc.substring(0,3) + "****" + acc.substring(acc.length()-4);
		} else {
			tips = acc.substring(acc.indexOf("@")+1);
		}
		tips = MessageFormat.format(getString(R.string.send_phone_msg), tips);
		validAccount = acc;
		postTips.setText(tips);
	}

	public void setStatus(Status status){
		currentStatus = status;
		updateStatus();
	}
	
	private void updateStatus() {
		page_tab.setVisibility(currentStatus == Status.Tab ? View.VISIBLE : View.GONE);
		page_reset.setVisibility(currentStatus == Status.Reset ? View.VISIBLE : View.GONE);
		page_Success.setVisibility(currentStatus == Status.Success ? View.VISIBLE : View.GONE);	
		
		switch(currentStatus){
		case Tab:
			break;
		case Reset:
			editTextPhoneverify.requestFocus();
			break;
		case Success:
			setTitle(R.string.close);
			InputMethodManager imInputMethodManager = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
			imInputMethodManager.hideSoftInputFromWindow(new_pwd2.getWindowToken(), InputMethodManager.RESULT_UNCHANGED_SHOWN);
			break;
		}
	}
	
	private class RegetPWDPageAdapter extends FragmentPagerAdapter {

		private final String[] TITLES;

		PhoneRegetFragment mPhoneRegetFragment;
		EmailRegetFragment mEmailRegetFragment;

		public RegetPWDPageAdapter(FragmentManager fm) {
			super(fm);
			TITLES = getResources().getStringArray(R.array.reget_pwd_type);
		}

		@Override
		public CharSequence getPageTitle(int position) {
			return TITLES[position];
		}

		@Override
		public BaseFragment getItem(int positon) {
			switch (positon) {
			case 0:
				if (mPhoneRegetFragment == null) {
					mPhoneRegetFragment = new PhoneRegetFragment();
				}
				return mPhoneRegetFragment;
			case 1:
				if (mEmailRegetFragment == null) {
					mEmailRegetFragment = new EmailRegetFragment();
				}
				return mEmailRegetFragment;
			default:
				break;
			}
			return null;
		}

		@Override
		public int getCount() {
			return 1; //TITLES.length;
		}
	}
}
