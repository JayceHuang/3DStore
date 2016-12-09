package com.runmit.sweedee.action.login;

import java.text.MessageFormat;
import java.util.List;

import android.annotation.TargetApi;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.action.login.RegistBaseFragment.RegistBundle;
import com.runmit.sweedee.action.more.NewBaseRotateActivity;
import com.runmit.sweedee.manager.WalletManager;
import com.runmit.sweedee.manager.WalletManager.OnAccountChangeListener;
import com.runmit.sweedee.model.UserAccount;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.sdk.user.member.task.UserManager.LoginState;
import com.runmit.sweedee.sdk.user.member.task.UserManager.LoginStateChange;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.PagerSlidingTabStrip;
import com.umeng.analytics.MobclickAgent;

public class LoginActivity extends NewBaseRotateActivity implements LoginStateChange {
	XL_Log log = new XL_Log(LoginActivity.class);
	public static final int REG_REQUEST_CODE = 0;

	private ViewPager mPager;
	public SharePrefence mXLSharePrefence;
	private PagerSlidingTabStrip regist_tabs;

	private LoginPageAdapter mLoginPageAdapter;
	
	private boolean isRegistLogin = false;//是否注册成功后后回调的登陆，如果是，则需要查询金币

	/**
	 * 外部启动设置的，有些流程希望登录成功后不要toast提示，可以根据此值
	 */
	public static final String INTENT_DATA_SHOW_LOGINSUCESS_TOAST = "INTENT_DATA_SHOW_LOGINSUCESS_TOAST";

	private boolean mIntentDataShowLoinSucessToast = true;
	private View rootView;
	private PopupWindow mPopupWindow;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		rootView = LayoutInflater.from(getApplicationContext()).inflate(R.layout.activity_login_register, null);
		setContentView(rootView);
		mIntentDataShowLoinSucessToast = getIntent().getBooleanExtra(INTENT_DATA_SHOW_LOGINSUCESS_TOAST, true);
		mXLSharePrefence = SharePrefence.getInstance(getApplicationContext());
		mPager = (ViewPager) findViewById(R.id.vPager);
		mLoginPageAdapter = new LoginPageAdapter(getSupportFragmentManager());
		mPager.setAdapter(mLoginPageAdapter);

		regist_tabs = (PagerSlidingTabStrip) findViewById(R.id.regist_tabs);
		regist_tabs.setViewPager(mPager);
		regist_tabs.setDividerColorResource(R.color.background_green);
		regist_tabs.setIndicatorColorResource(R.color.thin_blue);
		regist_tabs.setSelectedTextColorResource(R.color.thin_blue);
		regist_tabs.setDividerColor(Color.TRANSPARENT);
		DisplayMetrics dm = getResources().getDisplayMetrics();
		regist_tabs.setUnderlineHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, dm));
		regist_tabs.setIndicatorHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 2, dm));
		regist_tabs.setTextSize((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 16, dm));
		regist_tabs.setTextColor(getResources().getColor(R.color.game_tabs_text_color));

		getActionBar().setDisplayHomeAsUpEnabled(true);
		getActionBar().setDisplayShowHomeEnabled(false);

		UserManager.getInstance().addLoginStateChangeListener(this);
		WalletManager.getInstance().addAccountChangeListener(mAccountChangeListener);
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//去掉顶部标题分隔符
		regist_tabs.setVisibility(View.GONE);
	}

	private boolean hasShow = false;
	
	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if(hasFocus && !hasShow){
			hasShow = true;
			if (TextUtils.isEmpty(UserManager.getInstance().getAccount())) {// 如果获取到的账号为空，则表示没有注册登录过账号
				mXLSharePrefence.putBoolean("REGIST_GIFT_ISTOAST", true);
				final View popupView = View.inflate(this, R.layout.popup_regist_toast_layout, null);
				
				Rect frame = new Rect();   
				getWindow().getDecorView().getWindowVisibleDisplayFrame(frame);   
				int statusBarHeight = frame.top; 
				
				View gift = popupView.findViewById(R.id.img_gift);
				LayoutParams layoutParams = (LayoutParams) gift.getLayoutParams();
				layoutParams.topMargin = statusBarHeight;
				gift.setLayoutParams(layoutParams);
				
				mPopupWindow = new PopupWindow(popupView, WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT);
				mPopupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
				
				mPopupWindow.showAtLocation(rootView, Gravity.TOP, 0, 0);
				mPopupWindow.setFocusable(true);
				popupView.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						if (mPopupWindow != null) {
							mPopupWindow.dismiss();
						}
					}
				});
			}
		}
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.login_activity_menu, menu);
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case android.R.id.home:
			finishActivity();
			break;
		case R.id.regist_account:
			Intent intent = new Intent(this, RegisterActivity.class);
			this.startActivityForResult(intent, RegisterActivity.REG_REQUEST_CODE);
			break;
		default:
			break;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			if (mPopupWindow != null && mPopupWindow.isShowing()) {
				mPopupWindow.dismiss();
				return true;
			}
			finishActivity();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	private void finishActivity() {
		mLoginPageAdapter.getItem(mPager.getCurrentItem()).onActivityFinish();
		finish();
	}

	private OnAccountChangeListener mAccountChangeListener = new OnAccountChangeListener() {
		
		@Override
		public void onAccountChange(int rtnCode, List<UserAccount> mAccounts) {
			if(isRegistLogin){//只在注册登录时候才有效
				UserAccount mVirtual = WalletManager.getInstance().getVirtualAccount();
				if(rtnCode == 0 && mVirtual != null && mVirtual.amount > 0){//服务器返回成功并且虚拟币不为空
					showVirtualDialog(mVirtual.amount);
				}else{//否则结束
					setFinish();
				}
			}
		}
	};
	
	@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1) private void showVirtualDialog(final float amount){
		AlertDialog.Builder mBuilder = new AlertDialog.Builder(LoginActivity.this);
		mBuilder.setTitle(R.string.login_success);
		mBuilder.setMessage(MessageFormat.format(getString(R.string.regist_and_login_success_send_virtual_money_msg), amount));
		mBuilder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
			
			@Override
			public void onClick(DialogInterface dialog, int which) {
				mIntentDataShowLoinSucessToast = false;
				setFinish();
			}
		});
		mBuilder.setOnCancelListener(new OnCancelListener() {
			
			@Override
			public void onCancel(DialogInterface dialog) {
				mIntentDataShowLoinSucessToast = false;
				setFinish();
			}
		});
		AlertDialog mAlertDialog = mBuilder.create();
		mAlertDialog.show();
	}
	
	private void setFinish(){
		if (mIntentDataShowLoinSucessToast) {
			Util.showToast(LoginActivity.this, getString(R.string.login_success), Toast.LENGTH_LONG);
		}
		setResult(0);
		finishActivity();
	}
	
	@Override
	public void onChange(LoginState lastState, LoginState currentState, final Object userdata) {
		mLoginPageAdapter.getItem(mPager.getCurrentItem()).onLoginChange(lastState, currentState, userdata);
		if (currentState == LoginState.LOGINED) {
			if(isRegistLogin){
				////此处等待金币回调
			}else{
				setFinish();
			}
		}
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (resultCode == 0 && data != null) {
			isRegistLogin = true;
			RegistBundle mRegistBundle = (RegistBundle) data.getSerializableExtra(RegistBundle.class.getSimpleName());
			if (mRegistBundle.isPhoneRegist) {
				mPager.setCurrentItem(0);
			} else {
				mPager.setCurrentItem(1);
			}
			int page = mRegistBundle.isPhoneRegist ? 0 : 1; 
			mLoginPageAdapter.getItem(page).onActivityResult(mRegistBundle);
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		MobclickAgent.onResume(this);
	}

	@Override
	protected void onPause() {
		super.onPause();
		MobclickAgent.onPause(this);
	}

	protected void onDestroy() {
		super.onDestroy();
		UserManager.getInstance().removeLoginStateChangeListener(this);
		WalletManager.getInstance().removeAccountChangeListener(mAccountChangeListener);
	};

	private class LoginPageAdapter extends FragmentPagerAdapter {

		private final String[] TITLES;

		PhoneLoginFragment mPhoneLoginFragment;
		EmailLoginFragment mEmailLoginFragment;

		public LoginPageAdapter(FragmentManager fm) {
			super(fm);
			TITLES = getResources().getStringArray(R.array.login_head_type);
		}

		@Override
		public CharSequence getPageTitle(int position) {
			return TITLES[position];
		}

		@Override
		public LoginBaseFragment getItem(int positon) {
			switch (positon) {
			case 0:
				if (mPhoneLoginFragment == null) {
					mPhoneLoginFragment = new PhoneLoginFragment();
				}
				return mPhoneLoginFragment;
			case 1:
				if (mEmailLoginFragment == null) {
					mEmailLoginFragment = new EmailLoginFragment();
				}
				return mEmailLoginFragment;
			default:
				break;
			}
			return null;
		}

		@Override
		public int getCount() {
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//viewpager只显示一个
			return 1/*TITLES.length*/;
		}
	}
}
