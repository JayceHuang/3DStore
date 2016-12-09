package com.runmit.sweedee.action.pay;

import java.text.MessageFormat;
import java.util.List;
import java.util.Locale;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.runmit.sweedee.R;
import com.runmit.sweedee.action.more.NewBaseRotateActivity;
import com.runmit.sweedee.action.pay.paypassword.PayPasswordTypeActivity;
import com.runmit.sweedee.manager.WalletManager;
import com.runmit.sweedee.manager.WalletManager.OnAccountChangeListener;
import com.runmit.sweedee.model.UserAccount;
import com.runmit.sweedee.sdk.user.member.task.MemberInfo;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.view.PagerSlidingTabStrip;
import com.umeng.analytics.MobclickAgent;

public class MyWalletActivity extends NewBaseRotateActivity {

    public static final int REG_REQUEST_CODE = 0;

    private ViewPager mPager;

    private PagerSlidingTabStrip regist_tabs;
    
    private TextView mTvUserName;
    
    private TextView tv_account_balance;
    
    private TextView tv_virtual_balance;
    
    private Button buttonRecharge;
    
	private OnAccountChangeListener mAccountChangeListener = new OnAccountChangeListener() {
		
		@Override
		public void onAccountChange(int rtnCode, List<UserAccount> mAccounts) {
			setWalletAccount();
		}
	};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my_wallet);
        
        mTvUserName = (TextView) findViewById(R.id.tv_userInfo);
        setUserName();
        
        tv_account_balance = (TextView) findViewById(R.id.tv_account_balance);
        tv_virtual_balance = (TextView) findViewById(R.id.tv_virtual_balance);
        buttonRecharge = (Button) findViewById(R.id.buttonRecharge);
        buttonRecharge.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				UserAccount mUserAccount = WalletManager.getInstance().getReadAccount();
				if(mUserAccount != null){
					Intent intent = new Intent(MyWalletActivity.this, RechargeActivity.class);
					intent.putExtra(UserAccount.class.getSimpleName(), mUserAccount);
					startActivity(intent);
				}
			}
		});
        
        mPager = (ViewPager) findViewById(R.id.vPager);
        mPager.setAdapter(new RegistPageAdapter(getSupportFragmentManager()));
        mPager.setCurrentItem(0);

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
        WalletManager.getInstance().reqGetAccount();
        WalletManager.getInstance().addAccountChangeListener(mAccountChangeListener);
        setWalletAccount();
    }
    
    @Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.menu_mywallet, menu);
		return true;
	}
    
    private void setWalletAccount(){
    	UserAccount mUserAccount = WalletManager.getInstance().getVirtualAccount();
        if(mUserAccount != null){
        	int amount = (int) mUserAccount.amount;
        	tv_virtual_balance.setText(Integer.toString(amount));
        }
        
        mUserAccount = WalletManager.getInstance().getReadAccount();
        if(mUserAccount != null){
        	int amount = (int) (mUserAccount.amount/100);
        	tv_account_balance.setText(Integer.toString(amount));
        }
    }
    
    @Override
    protected void onDestroy() {
    	super.onDestroy();
    	WalletManager.getInstance().removeAccountChangeListener(mAccountChangeListener);
    }
    
    private void setUserName() {
        String accountName = "";
        MemberInfo mem = UserManager.getInstance().getMemberInfo();
        if (mem != null) {
            accountName = mem.nickname;
        }
        mTvUserName.setText(accountName);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                break;
            case R.id.manager_pay_password:
            	startActivity(new Intent(MyWalletActivity.this, PayPasswordTypeActivity.class));
            	break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
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

    private class RegistPageAdapter extends FragmentPagerAdapter {

        private final String[] TITLES;

        PurchaseRecordFragment mPurchaseRecordFragment;
        RechargeRecordFragment mRechargeRecordFragment;
        GiveRecordFragment mGiveRecordFragment;
        
        public RegistPageAdapter(FragmentManager fm) {
            super(fm);
            TITLES = getResources().getStringArray(R.array.my_wallet_head_type);
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return TITLES[position];
        }

        @Override
        public android.support.v4.app.Fragment getItem(int positon) {
            switch (positon) {
                case 0:
                    if (mPurchaseRecordFragment == null) {
                    	mPurchaseRecordFragment = new PurchaseRecordFragment();
                    }
                    return mPurchaseRecordFragment;
                case 1:
                	if (mRechargeRecordFragment == null) {
                    	mRechargeRecordFragment = new RechargeRecordFragment();
                    }
                    return mRechargeRecordFragment;
                case 2:
                    if(mGiveRecordFragment == null){
                    	mGiveRecordFragment = new GiveRecordFragment();
                    }
                    return mGiveRecordFragment;
                default:
                    break;
            }
            return null;
        }

        @Override
        public int getCount() {
            return TITLES.length;
        }
    }
}
