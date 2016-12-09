package com.runmit.sweedee.action.myvideo;

import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v4.view.ViewPager.OnPageChangeListener;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.MenuItem;

import com.runmit.sweedee.R;
import com.runmit.sweedee.action.more.NewBaseRotateActivity;
import com.runmit.sweedee.action.space.VideoCacheFragment;
import com.runmit.sweedee.view.PagerSlidingTabStrip;

public class MyVideoActivity extends NewBaseRotateActivity {

    public static final int REG_REQUEST_CODE = 0;

    private ViewPager mPager;

    private PagerSlidingTabStrip regist_tabs;

    private int currentPage;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login_register);
        mPager = (ViewPager) findViewById(R.id.vPager);
        mPager.setAdapter(new RegistPageAdapter(getSupportFragmentManager()));
        currentPage = 0;
        mPager.setCurrentItem(currentPage);

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
        regist_tabs.setOnPageChangeListener(new OnPageChangeListener() {
            @Override
            public void onPageSelected(int positon) {
                currentPage = positon;
                invalidateOptionsMenu();
                RegistPageAdapter ad = (RegistPageAdapter) mPager.getAdapter();
                if(ad != null){
                	if(ad.mCacheRecordFragment != null){
                		ad.mCacheRecordFragment.setPageEnable(currentPage == 1);
                	}
                	if(ad.mPlayRecordFragment != null){
                		ad.mPlayRecordFragment.setPageEnable(currentPage == 0);
                	}
                }
            }
            @Override
            public void onPageScrolled(int arg0, float arg1, int arg2) {}
            @Override
            public void onPageScrollStateChanged(int positon) {}
        });
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setDisplayShowHomeEnabled(false);
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
    public void onBackPressed() {
        RegistPageAdapter ad = (RegistPageAdapter) mPager.getAdapter();
        boolean catched = false;
        if(ad != null){
        	switch (currentPage){
            case 0:
                catched = ad.mPlayRecordFragment != null ? ad.mPlayRecordFragment.onBackPressed() : false;
                break;
            case 1:
                catched = ad.mCacheRecordFragment != null ? ad.mCacheRecordFragment.onBackPressed() : false;
                break;
            default:
            	break;
        	}
        }
        
        if (!catched){
            finish();
        }
    }

    private class RegistPageAdapter extends FragmentPagerAdapter {

        private final String[] TITLES;

        PlayRecordFragment mPlayRecordFragment;
        VideoCacheFragment mCacheRecordFragment;

        public RegistPageAdapter(FragmentManager fm) {
            super(fm);
            TITLES = getResources().getStringArray(R.array.my_video_head_type);
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return TITLES[position];
        }

        @Override
        public android.support.v4.app.Fragment getItem(int positon) {
            switch (positon) {
                case 0:
                    if (mPlayRecordFragment == null) {
                    	mPlayRecordFragment = new PlayRecordFragment();
                    }
                    return mPlayRecordFragment;
                case 1:
                    if (mCacheRecordFragment == null) {
                    	mCacheRecordFragment = new VideoCacheFragment();
                    }
                    return mCacheRecordFragment;
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
