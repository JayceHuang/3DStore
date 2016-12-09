/**
 * XlMainView.java 
 * com.xunlei.share.XlMainView
 * @author: Administrator
 * @date: 2013-4-18 下午5:24:31
 */
package com.runmit.sweedee;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v4.view.ViewPager.OnPageChangeListener;
import android.support.v4.view.ViewPager.PageTransformer;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Interpolator;
import android.view.animation.LinearInterpolator;
import android.widget.AbsListView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Scroller;

import com.runmit.sweedee.action.game.AppFragment;
import com.runmit.sweedee.action.game.GameFragment;
import com.runmit.sweedee.action.home.HomeFragment;
import com.runmit.sweedee.action.more.UserAndSettingActivity;
import com.runmit.sweedee.action.movie.VideoFragment;
import com.runmit.sweedee.action.search.SearchActivity;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.ImageHomeScrollTabStrip;
import com.runmit.sweedee.view.AnimateViewPager;
import com.runmit.sweedee.view.ImageHomeScrollTabStrip.OnTabInteractListener;
import com.runmit.sweedee.view.animation.PtrFrameLayout;
import com.tdviewsdk.viewpager.transforms.DepthPageTransformer;

/**
 * @author Administrator 实现的主要功能。
 *         <p/>
 *         修改记录：修改者，修改日期，修改内容
 */
public class TdStoreMainView extends LinearLayout implements AnimateViewPager.PageTransitionCallback {
	
	final XL_Log log = new XL_Log(TdStoreMainView.class);

	public final static String TAB_HOMEPAGE = "tab_1";
	public final static String TAB_CHANNEL = "tab_2";
	public final static String TAB_MINESETTING = "tab_3";
	public final static String CURRENT_TAB_KEY = "current_tab_key";
	
	private static final int SCROLL_DURATION = 400;

	private final LayoutInflater inflater;

	private FragmentActivity mFragmentActivity;

	private List<BaseFragment> mFragmentsList = new ArrayList<BaseFragment>();

	private int lastFragmentIndex = 0;

	private AnimateViewPager mViewPager;

	private UIFragmentPageAdapter mUIFragmentPageAdapter;

	private ImageHomeScrollTabStrip mHomeScrollTabStrip;
	
	private SensorManager mSensorMgr = null;
	
	private Sensor mSensor = null; 
	
	private FixedSpeedScroller mFixedSpeedScroller;
	
	private PtrFrameLayout mPtrFrameLayout;
	
	private ImageView userInfoimageView;
	
	private View mHomeSearchView;
	
	private PageTransformer mTransformer = new DepthPageTransformer();

	/**
	 * @param context
	 */
	public TdStoreMainView(FragmentActivity context, Bundle savedInstanceState) {
		super(context);
		this.mFragmentActivity = context;
		inflater = LayoutInflater.from(context);
		inflater.inflate(R.layout.activity_main, this);
		mSensorMgr = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
		mSensor = mSensorMgr.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		
		onFinishInflate(savedInstanceState);
	}
	
	private void onFinishInflate(Bundle savedInstanceState) {
		log.debug("onFinishInflate");
		mPtrFrameLayout = (PtrFrameLayout) findViewById(R.id.mainlayout);
		mPtrFrameLayout.open(false);
		mPtrFrameLayout.setTdStoreMainView(this);
		
		userInfoimageView = (ImageView) findViewById(R.id.userInfoimageView);
		userInfoimageView.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				mFragmentActivity.startActivity(new Intent(mFragmentActivity, UserAndSettingActivity.class));
			}
		});
		
		mHomeSearchView = (View)findViewById(R.id.searchImage);
        mHomeSearchView.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
            	HashMap<String, Integer> userinfo = new HashMap<String, Integer>();
            	userinfo.put(CURRENT_TAB_KEY, mUIFragmentPageAdapter.mLastSelectPager);
                SearchActivity.start(mFragmentActivity, userinfo);
            }
        });
        
		mHomeScrollTabStrip = (ImageHomeScrollTabStrip) findViewById(R.id.nav_tabs);
		
		if (savedInstanceState != null) {
			lastFragmentIndex = savedInstanceState.getInt("lastFragmentIndex");
		}

		mFragmentsList.add(new HomeFragment());
		mFragmentsList.add(new VideoFragment());
		if(!StoreApplication.isSnailPhone){
			mFragmentsList.add(new GameFragment());
		}
 		mFragmentsList.add(new AppFragment());

		mUIFragmentPageAdapter = new UIFragmentPageAdapter(mFragmentActivity.getSupportFragmentManager());
		mViewPager = (AnimateViewPager) findViewById(R.id.detail_viewPager);
		mViewPager.setPageTransformer(true, mTransformer);
		mViewPager.setOverScrollMode(View.OVER_SCROLL_NEVER);
		mViewPager.addAnimationEndListener(this);
		setViewPageScrollTime(mViewPager);
		mViewPager.setAdapter(mUIFragmentPageAdapter);
		mHomeScrollTabStrip.setViewPager(mViewPager);
		mViewPager.setCurrentItem(lastFragmentIndex);
		mHomeScrollTabStrip.setOnPageChangeListener(mUIFragmentPageAdapter);
		mHomeScrollTabStrip.setOnTabClickListener(new OnTabInteractListener() {
			
			@Override
			public void onclick(final int pisition) {

				if(mViewPager.getCurrentItem() == pisition){
					return;
				}
				mViewPager.changePageimmediately(pisition);
			}

			@Override
			public void onSwip(boolean isUp) {
				
			}
		});
	}

	//Only for PtrFrameLayout，other dont call me
	//我们需要从当前的fragemnt获取到一个listview，然后再传给PtrFrameLayout
	//这样子需要一层层获取，略坑
	public AbsListView getCurrentListView(){
		BaseFragment mlastFragment = mUIFragmentPageAdapter.getItem(mViewPager.getCurrentItem());
		log.debug("mlastFragment="+mlastFragment);
		if(mlastFragment != null){
			return mlastFragment.getCurrentListView();
		}
		return null;
	}
	
	private SensorEventListener mSensorListener = new SensorEventListener() {
		
		private final int MAX_VALUES = 10;
		private final int COMPUTE_SIZE = 6;
		private final int COMPUTE_SIZE_ACTIVE = 4;
		private final static float TO_ACC_LINE = 8.0f;
		private final static float TO_ACTIVE_LINE = 20.0f;
		
		private final static int READY_STATE = 0;
		private final static int ACC_STATE = 1;
		private final static int END_STATE = 2;
		
		private int state = READY_STATE;
		
		/*
		 * 使用平均量去触发动画
		 * 如果当前值大于平均值与一个设定的范围，就触发动画
		 */
		// 预备阶段
		private ArrayList<Float> nearValues = new ArrayList<Float>();
		// 加速阶段
		private ArrayList<Float> nearAccValues = new ArrayList<Float>();
		
		@Override
		public void onSensorChanged(SensorEvent event) {
			int currentSize = 0;
			float lastValue = event.values[0];

			float average = 0;
			switch(state){
				
			case READY_STATE:
				currentSize = nearValues.size();
				if (currentSize > COMPUTE_SIZE) {
					for (float f : nearValues) {
						average += f;
					}
					average = average / currentSize;
					
					if(Math.abs(lastValue - average) > TO_ACC_LINE){
						state = ACC_STATE;
						break;
					}
					
				}
				
				if(currentSize < MAX_VALUES) {
					nearValues.add(lastValue);
				}else {
					nearValues.remove(0);
					nearValues.add(lastValue);
				}
				break;
			case 1:
				currentSize = nearAccValues.size();
				if (currentSize > 0) {
					for (float f : nearAccValues) {
						average += f;
					}
					average = average / currentSize;
					if(Math.abs(lastValue - average) > TO_ACTIVE_LINE){
						if(average > 0){
							mViewPager.bounceToRight();
						}else{
							mViewPager.bounceToLeft();
						}
						state = END_STATE;
					}
					
				}
				
				if(currentSize < COMPUTE_SIZE_ACTIVE) {
					nearAccValues.add(lastValue);
					// print logs
					// String lll= "";
					// for(int j=0; j < nearAccValues.size(); j++){
					//	lll +=j+":" + nearAccValues.get(j)+",";
					// }
					// Log.d("mzh", lll);
				}
				else{
					state = END_STATE;
				}
				break;
			case END_STATE:
				nearValues.clear();
				nearAccValues.clear();
				state = READY_STATE;
				break;
			}
		}
		
		@Override
		public void onAccuracyChanged(Sensor sensor, int accuracy) {
			
		}
	};
	
	public void onSaveInstanceState(Bundle outState) {
		outState.putInt("lastFragmentIndex", mViewPager.getCurrentItem());
	}

	/**
	 * @return
	 */
	public boolean onBackPress() {
		Fragment mlastFragment = mUIFragmentPageAdapter.getItem(mViewPager.getCurrentItem());
		if (mlastFragment instanceof BaseFragment) {
			return ((BaseFragment) mlastFragment).onBackPress();
		}		
		return false;
	}
	
	public void onAnimationEnd(){
	}

	private class UIFragmentPageAdapter extends FragmentPagerAdapter implements OnPageChangeListener {

		private final String[] mHomeTasTitles;

		private int mLastSelectPager = 0;

		public UIFragmentPageAdapter(FragmentManager fm) {
			super(fm);
			mHomeTasTitles = getResources().getStringArray(R.array.home_tabs);
		}

		@Override
		public CharSequence getPageTitle(int position) {
			return mHomeTasTitles[position];
		}

		@Override
		public BaseFragment getItem(int positon) {
			return mFragmentsList.get(positon);
		}

		@Override
		public int getCount() {
			return mFragmentsList.size();
		}

		@Override
		public void onPageScrollStateChanged(int newState) {
			mViewPager.setScrollState(newState);
		}

		@Override
		public void onPageScrolled(int arg0, float arg1, int arg2) {

		}

		@Override
		public void onPageSelected(int position) {
			log.debug("onPageSelected=" + position);
			if (mLastSelectPager >= 0) {
				mUIFragmentPageAdapter.getItem(mLastSelectPager).onViewPagerStop();
			}
//			if(mLastSelectPager == 0 && position > 0){//从首页切换到其他页面
				mPtrFrameLayout.close(true);
//			}else if(position == 0 && mLastSelectPager > 0){
//				mPtrFrameLayout.open(true);
//			}
			mLastSelectPager = position;
			mUIFragmentPageAdapter.getItem(mLastSelectPager).onViewPagerResume();
		}
		
		@Override
		public Object instantiateItem(ViewGroup container, final int position) {
			Fragment fragment = (Fragment) super.instantiateItem(container, position);
			mViewPager.setObjectForPosition(fragment, position);
			return fragment;
		}
		
		@Override
		public void destroyItem(ViewGroup container, int position, Object object) {
			//重载该方法，防止其它视图被销毁，防止加载视图卡顿
			super.destroyItem(container, position, object);
		}
	}

	public void onResume() {		
		for (int i = 0, size = mFragmentsList.size(); i < size; i++) {
			BaseFragment mFragment = mFragmentsList.get(i);
			if (i == mUIFragmentPageAdapter.mLastSelectPager) {
				mFragment.onViewPagerResume();
			} else {
				mFragment.onViewPagerStop();
			}
		}
		mSensorMgr.registerListener(mSensorListener, mSensor, SensorManager.SENSOR_DELAY_UI);
		
	}

	public void onPause() {
		mUIFragmentPageAdapter.getItem(mUIFragmentPageAdapter.mLastSelectPager).onViewPagerStop();
		mSensorMgr.unregisterListener(mSensorListener);
	}
	
	private void setViewPageScrollTime(ViewPager viewpager){
		try {  
            Field mField = ViewPager.class.getDeclaredField("mScroller");  
            mField.setAccessible(true);  
            mFixedSpeedScroller = new FixedSpeedScroller(viewpager.getContext());  
            mField.set(viewpager, mFixedSpeedScroller);  
        } catch (Exception e) {  
            e.printStackTrace();  
        } 
	}
	
	/**此类用来修改ViewPager内部写死的滑动速率*/
	class FixedSpeedScroller extends Scroller {  
		  
	    private int mCustomDuration = SCROLL_DURATION;  
	    
	    /**ViewPager里面原有的Interpolator对象*/
		private Interpolator sInterpolator = new Interpolator() {
	        public float getInterpolation(float t) {
	            t -= 1.0f;
	            return t * t * t * t * t + 1.0f;
	        }
	    };
			  
	    public FixedSpeedScroller(Context context) {  
	        this(context,new LinearInterpolator());  
	    }  
	  
	    public FixedSpeedScroller(Context context, Interpolator interpolator) {  
	        super(context, interpolator);  
	    }  
	    
	    @Override
	    public boolean computeScrollOffset() {
	    	boolean isScroll = super.computeScrollOffset();
	    	if(!isScroll){
	    		postDelayed(new Runnable() {
					
					@Override
					public void run() {
						changeInterpolator(sInterpolator);
					}
				}, 50);
	    	}
	    	return isScroll;
	    }
	    
	    private void changeInterpolator(Interpolator interpolator){
	    	Field mField;
            try {
            	mField = Scroller.class.getDeclaredField("mInterpolator");
            	mField.setAccessible(true);  
				mField.set(this, interpolator);
			} catch (IllegalAccessException e) {
				e.printStackTrace();
			} catch (IllegalArgumentException e) {
				e.printStackTrace();
			} catch (NoSuchFieldException e) {
				e.printStackTrace();
			}  
	    }
	    
	    @Override  
	    public void startScroll(int startX, int startY, int dx, int dy, int duration) {  	       
	    	duration = Math.max(duration, mCustomDuration);
	        super.startScroll(startX, startY, dx, dy, mCustomDuration);  
	    } 
	}
}
