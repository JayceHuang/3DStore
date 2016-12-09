package com.runmit.sweedee.action.movie;

import java.util.ArrayList;
import java.util.List;

import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v4.view.ViewPager.OnPageChangeListener;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.InitDataLoader;
import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
import com.runmit.sweedee.model.CmsModuleInfo;
import com.runmit.sweedee.model.CmsModuleInfo.CmsComponent;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.EmptyView;
import com.runmit.sweedee.view.EmptyView.Status;
import com.runmit.sweedee.view.PagerSlidingTabStrip;
import com.runmit.sweedee.view.TriangleLineView;
import com.tdviewsdk.viewpager.transforms.DepthPageTransformer;

public class VideoFragment extends BaseFragment implements StoreApplication.OnNetWorkChangeListener {

	private XL_Log log = new XL_Log(VideoFragment.class);

	private PagerSlidingTabStrip mTabs;

	private ViewPager mViewPager;

	private MyAdapter mMyAdapter;

	private View mRootView;

	private EmptyView mEmptyView;

	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case CmsEventCode.EVENT_GET_HOME:
				if (msg.arg1 == 0) {
					CmsModuleInfo mCmsHomeDataInfo = (CmsModuleInfo) msg.obj;
					log.debug("mCmsHomeDataInfo.data=" + mCmsHomeDataInfo.data.size());
					ArrayList<CmsComponent> data = mCmsHomeDataInfo.data;
					if (data.size() > 0) {
						mEmptyView.setStatus(Status.Gone);
						mMyAdapter = new MyAdapter(getChildFragmentManager(), data);
						mViewPager.setAdapter(mMyAdapter);
						mTabs.setViewPager(mViewPager);
					} else {
						mEmptyView.setStatus(Status.Empty);
					}
				} else {// 失败了
					mEmptyView.setStatus(Status.Empty);
				}
				break;
			}
		}
	};

	public AbsListView getCurrentListView() {
		if (mMyAdapter != null && mViewPager != null) {
			BaseFragment f = mMyAdapter.getItem(mViewPager.getCurrentItem());
			if (f != null) {
				return f.getCurrentListView();
			}
		}
		return null;
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		if (mRootView != null) {
			return mRootView;
		}
		mRootView = inflater.inflate(R.layout.frg_home_page_video, container, false);
		mTabs = (PagerSlidingTabStrip) mRootView.findViewById(R.id.detail_tabs);
		mViewPager = (ViewPager) mRootView.findViewById(R.id.detail_viewPager);
		mViewPager.setPageTransformer(true, new DepthPageTransformer());
		DisplayMetrics dm = getResources().getDisplayMetrics();

		mTabs.setIndicatorColorResource(R.color.transparent);
		mTabs.setSelectedTextColorResource(R.color.thin_blue);
		mTabs.setDividerColor(Color.TRANSPARENT);
		mTabs.setUnderlineHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, dm));
		mTabs.setIndicatorHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 3, dm));
		mTabs.setTextSize((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 17, dm));
		mTabs.setTextColor(getResources().getColor(R.color.white_text_color));

		mEmptyView = (EmptyView) mRootView.findViewById(R.id.empty_view);
		mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPic(R.drawable.ic_failed);
		mEmptyView.setStatus(EmptyView.Status.Loading);
		mEmptyView.setEmptyTop(mFragmentActivity, 70);
		mEmptyView.setTextColor(getResources().getColor(R.color.white));
		
		TriangleLineView mTriangleLineView = (TriangleLineView) mRootView.findViewById(R.id.triangleline);
		mTriangleLineView.initTriangle(1);
		
		StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
		
		mTabs.setOnPageChangeListener(mMyAdapter);
		CmsServiceManager.getInstance().getModuleData(CmsServiceManager.MODULE_SHORTVIDEO, mHandler);

		return mRootView;
	}

	@Override
	public void onDestroyView() {
		super.onDestroyView();
        ViewGroup parent = (ViewGroup) mRootView.getParent();
        if (parent != null) {
            parent.removeView(mRootView);
        }
	}
	
	public void onViewPagerResume() {
		super.onViewPagerResume();
		if (mMyAdapter != null) {
			mMyAdapter.getItem(mViewPager.getCurrentItem()).onViewPagerResume();
		}
	}

	@Override
	public void onViewPagerStop() {
		super.onViewPagerStop();
		if (mMyAdapter != null) {
			mMyAdapter.getItem(mViewPager.getCurrentItem()).onViewPagerStop();
		}
	}

	class MyAdapter extends FragmentStatePagerAdapter implements OnPageChangeListener {

		private ArrayList<CmsComponent> mCmsComponents;

		private List<MovieStorageFragment> mCommonFragList;

		@Override
		public int getCount() {
			return mCmsComponents.size();
		}

		@Override
		public CharSequence getPageTitle(int position) {
			return mCmsComponents.get(position).title;
		}

		public MyAdapter(FragmentManager fm, ArrayList<CmsComponent> cmsComponents) {
			super(fm);
			mCommonFragList = new ArrayList<MovieStorageFragment>();
			for (CmsComponent item : cmsComponents) {
				log.debug("item.contents="+item.contents);
				MovieStorageFragment mMovieStorageFragment = new MovieStorageFragment(item.contents);
				mCommonFragList.add(mMovieStorageFragment);
			}
			
			this.mCmsComponents = cmsComponents;
		}

		@Override
		public BaseFragment getItem(int i) {
			log.debug("choose item = " + i);
			return mCommonFragList.get(i);
		}

		@Override
		public void onPageScrollStateChanged(int arg0) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onPageScrolled(int arg0, float arg1, int arg2) {
			// TODO Auto-generated method stub

		}

		private int mLastSelectPager = 0;

		@Override
		public void onPageSelected(int position) {
			if (mLastSelectPager >= 0) {
				mMyAdapter.getItem(mLastSelectPager).onViewPagerStop();
			}
			mLastSelectPager = position;
			mMyAdapter.getItem(mLastSelectPager).onViewPagerResume();
		}
	}

	@Override
	public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
		if(isNetWorkAviliable && isDataEmpty()){
			CmsServiceManager.getInstance().getModuleData(CmsServiceManager.MODULE_SHORTVIDEO, mHandler);
		}
	}

	private boolean isDataEmpty() {
		return !(mMyAdapter != null && mMyAdapter.getCount() > 0);
	}
}
