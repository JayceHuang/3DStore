package com.runmit.sweedee.action.login;

import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.MenuItem;
import android.view.View;

import com.umeng.analytics.MobclickAgent;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.login.RegistBaseFragment.IUpdateSelectRegion;
import com.runmit.sweedee.model.OpenCountryCode;
import com.runmit.sweedee.view.PagerSlidingTabStrip;

public class RegisterActivity extends FragmentActivity {

    public static final int REG_REQUEST_CODE = 0;

    private ViewPager mPager;

    private PagerSlidingTabStrip regist_tabs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login_register);
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
        
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//去掉顶部标题分隔符
		regist_tabs.setVisibility(View.GONE);
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

        RegistEmailFragment mRegistEmailFragment;
        RegistPhoneFragment mRegistPhoneFragment;
        IUpdateSelectRegion updateSelectRegionListener = new IUpdateSelectRegion() {
			
			@Override
			public void onRegionSelected(String name, String code) {
				if(mRegistPhoneFragment != null)
					mRegistPhoneFragment.updateRegionText(name, code);
				
				if(mRegistEmailFragment != null)
					mRegistEmailFragment.updateRegionText(name, code);				
			}
		};
		
        public RegistPageAdapter(FragmentManager fm) {
            super(fm);
            TITLES = getResources().getStringArray(R.array.regist_head_type);
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return TITLES[position];
        }

        @Override
        public android.support.v4.app.Fragment getItem(int positon) {
            switch (positon) {
                case 0:
                    if (mRegistPhoneFragment == null) {
                        mRegistPhoneFragment = new RegistPhoneFragment();
                        mRegistPhoneFragment.addUpdateSelectRegionListener(updateSelectRegionListener);
                    }
                    return mRegistPhoneFragment;
                case 1:
                    if (mRegistEmailFragment == null) {
                        mRegistEmailFragment = new RegistEmailFragment();
                        mRegistEmailFragment.addUpdateSelectRegionListener(updateSelectRegionListener);
                    }
                    return mRegistEmailFragment;
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
