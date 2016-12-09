package com.runmit.sweedee.action.more;

import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v4.view.ViewPager.OnPageChangeListener;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.MenuItem;
import android.view.View;

import com.runmit.sweedee.R;
import com.runmit.sweedee.action.more.NewBaseRotateActivity;
import com.runmit.sweedee.action.myvideo.PlayRecordFragment;
import com.runmit.sweedee.action.space.VideoCacheFragment;
import com.runmit.sweedee.view.BadgeView;
import com.runmit.sweedee.view.PagerSlidingTabStrip;

public class TaskAndUpdateActivity extends NewBaseRotateActivity {

    public static final int REG_REQUEST_CODE = 0;

    private ViewPager mPager;

    private PagerSlidingTabStrip regist_tabs;

    private int currentPage;
    
    private BadgeView badgeView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login_register);
        mPager = (ViewPager) findViewById(R.id.vPager);
        mPager.setAdapter(new TaskFragmentAdapter(getSupportFragmentManager()));
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
                MineSettingFragment.sendUserCenterBroadcast(TaskAndUpdateActivity.this);
                if(badgeView.isShown()){
                	badgeView.hide();
                }
            }
            @Override
            public void onPageScrolled(int arg0, float arg1, int arg2) {}
            @Override
            public void onPageScrollStateChanged(int positon) {}
        });
        badgeView = new BadgeView(this,regist_tabs);
        badgeView.setBackgroundResource(R.drawable.icon_badge);
		badgeView.setBadgePosition(BadgeView.POSITION_TOP_RIGHT);
		badgeView.setHeight(10);
		badgeView.setWidth(10);
		badgeView.setBadgeMargin(130,50);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setDisplayShowHomeEnabled(false);
        if(MineSettingFragment.isBadgeViewShow){
        	badgeView.show();	
        }else{
        	badgeView.hide();	
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
    
    @Override
    public void onBackPressed() {
    	TaskFragmentAdapter ad = (TaskFragmentAdapter) mPager.getAdapter();
        boolean catched = false;
        if(ad != null){
        	switch (currentPage){
            case 0:
                catched = ad.mTaskManagerFragment != null ? ad.mTaskManagerFragment.onBackPressed() : false;
                break;
            case 1:
                catched = ad.mTaskUpdateFragment != null ? ad.mTaskUpdateFragment.onBackPressed() : false;
                break;
            default:
            	break;
        	}
        }
        if (!catched){
            finish();
        }
    }

    private class TaskFragmentAdapter extends FragmentPagerAdapter {
        private final String[] TITLES;

        TaskManagerFragment mTaskManagerFragment;
        TaskUpdateFragment mTaskUpdateFragment;
        
        public TaskFragmentAdapter(FragmentManager fm) {
            super(fm);
            TITLES = getResources().getStringArray(R.array.task_tabs);
        }
        @Override
        public CharSequence getPageTitle(int position) {
            return TITLES[position];
        }

        @Override
        public android.support.v4.app.Fragment getItem(int positon) {
       
        	if(positon==0){
        	    if(mTaskManagerFragment==null){
        		    mTaskManagerFragment = new TaskManagerFragment();
        	    }	
        	    return mTaskManagerFragment;
        	}else{
        	    if(mTaskUpdateFragment==null){
        		    mTaskUpdateFragment = new TaskUpdateFragment();
                }	
                return mTaskUpdateFragment;
        	}

        }
        @Override
        public int getCount() {
            return TITLES.length;
        }
    }

	@Override
	protected void onSaveInstanceState(Bundle outState) {
		// TODO Auto-generated method stub
		//super.onSaveInstanceState(outState);
	}
    
    
}
