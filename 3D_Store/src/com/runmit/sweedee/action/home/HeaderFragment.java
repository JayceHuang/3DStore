package com.runmit.sweedee.action.home;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.lucasr.twowayview.TwoWayLayoutManager.Orientation;
import org.lucasr.twowayview.widget.SpacingItemDecoration;
import org.lucasr.twowayview.widget.StaggeredGridLayoutManager;
import org.lucasr.twowayview.widget.TwoWayView;

import android.os.Bundle;
import android.os.Handler;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.AbsListView.OnScrollListener;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.Space;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.manager.InitDataLoader;
import com.runmit.sweedee.model.CmsModuleInfo;
import com.runmit.sweedee.model.CmsModuleInfo.CmsComponent;
import com.runmit.sweedee.model.CmsModuleInfo.CmsItem;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.EmptyView;
import com.runmit.sweedee.view.EmptyView.Status;

public abstract class HeaderFragment extends BaseFragment implements InitDataLoader.LoadListener {

	private XL_Log log = new XL_Log(HeaderFragment.class);

	private View mRootView;

	protected EmptyView mEmptyView;

	protected GuideGallery mGuideGallery;

	private CmsModuleInfo mCmsHomeDataInfo;

	protected Space mFakeHeader;

	private Handler mhandler = new Handler();

	private int mHeaderHeight;
	
	protected TwoWayView mainRecyclerView;

	public ListView getCurrentListView() {
		return null/*mMainListView*/;
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		if (mRootView != null) {
			return mRootView;
		}
        mRootView = inflater.inflate(R.layout.fragment_base_guidegallery_home, container, false);
        mEmptyView = (EmptyView) mRootView.findViewById(R.id.empty_view);
        mainRecyclerView = (TwoWayView) mRootView.findViewById(R.id.list);
        mainRecyclerView.setHasFixedSize(true);
        mainRecyclerView.setLongClickable(true);
        mainRecyclerView.setLayoutManager(new org.lucasr.twowayview.widget.StaggeredGridLayoutManager(Orientation.VERTICAL, 12, 3));
        mainRecyclerView.addItemDecoration(new SpacingItemDecoration(0, 15));
        mainRecyclerView.setOnScrollListener(new RecyclerView.OnScrollListener() {

			@Override
			public void onScrollStateChanged(RecyclerView recyclerView, int newState) {
				log.debug("scrollState=" + newState);
				switch (newState) {
				case OnScrollListener.SCROLL_STATE_TOUCH_SCROLL:
				case OnScrollListener.SCROLL_STATE_FLING:
					mGuideGallery.onStop();
					break;
				case OnScrollListener.SCROLL_STATE_IDLE:
					if(mGuideGallery.getRotationX() == 0){
						mGuideGallery.onResume();
					}
					break;
				default:
					break;
				}
			}

			@Override
			public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
				final View child = recyclerView.getChildAt(0);
				if(child != null){
					scrollHeaderTo(child == mFakeHeader ? child.getTop() : -mHeaderHeight);
				}
			}
		});
        
		mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPic(R.drawable.ic_failed);
		mEmptyView.setStatus(EmptyView.Status.Loading);
		mEmptyView.setEmptyTop(mFragmentActivity, 70);
		mEmptyView.setTextColor(getResources().getColor(R.color.white));

		mGuideGallery = (GuideGallery) mRootView.findViewById(R.id.guide_gallery);
		mHeaderHeight = mGuideGallery.getHeadHeight();
		log.debug("mHeaderHeight=" + mHeaderHeight);
		mFakeHeader = new Space(mFragmentActivity);
		mFakeHeader.setLayoutParams(new StaggeredGridLayoutManager.LayoutParams(0, mHeaderHeight));
		
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

	protected void setBackground(int resId) {
		mRootView.setBackgroundResource(resId);
	}

	private void scrollHeaderTo(int scrollTo) {
		scrollTo = Math.min(Math.max(scrollTo, -mHeaderHeight), 0);
		MarginLayoutParams mGuideLayoutParams = (MarginLayoutParams) mGuideGallery.getLayoutParams();
		mGuideLayoutParams.topMargin = scrollTo;
		mGuideGallery.setLayoutParams(mGuideLayoutParams);
		
		mGuideGallery.setPivotX(mGuideGallery.getWidth() / 2);
		mGuideGallery.setPivotY(mGuideGallery.getHeight());
		float rotation = Math.max(-90 * scrollTo / mHeaderHeight, -90);
		mGuideGallery.setRotationX(rotation);
		if(rotation >= 30){//大于30度开始做透明度变化
			float alpha = -rotation /60 + 1.5f;
			mGuideGallery.setAlpha(alpha);
		}else{
			mGuideGallery.setAlpha(1.0f);//还原
		}
	}

	public void onViewPagerResume() {
		super.onViewPagerResume();
		if (mGuideGallery != null) {
			mGuideGallery.onResume();
		}
	}

	@Override
	public void onViewPagerStop() {
		super.onViewPagerStop();
		if (mGuideGallery != null) {
			mGuideGallery.onStop();
		}
	}

	@Override
	public boolean onLoadFinshed(final int ret, final boolean isCache, final Object result) {
		final boolean isOk = ret == 0;
		log.debug("onLoadFinshed=" + ret + ",isCache=" + isCache);
		mhandler.post(new Runnable() {

			@Override
			public void run() {
				if (isOk) {
					mCmsHomeDataInfo = (CmsModuleInfo) result;
					log.debug("mCmsHomeDataInfo.data=" + mCmsHomeDataInfo.data.size());
					ArrayList<CmsComponent> data = mCmsHomeDataInfo.data;
					if (data.size() > 0) {
						CmsComponent mTopCmsComponent = data.get(0);
						if(StoreApplication.isSnailPhone){//蜗牛手机删除游戏内容
							Iterator<CmsItem> mIterator = mTopCmsComponent.contents.iterator();
							while (mIterator.hasNext()) {
								CmsItem mCmsItem = mIterator.next();
								if(mCmsItem.type.value.equals(CmsItem.TYPE_GAME)){
									mIterator.remove();
								}
							}
						}
						mGuideGallery.setDataSource(mTopCmsComponent);
						
						List<CmsComponent> mSubList = data.subList(1, data.size());
						if(StoreApplication.isSnailPhone){//蜗牛手机删除所有游戏内容
							for(int i = 0,size = mSubList.size() ;i<size;i++){//第一步删除子类
								Iterator<CmsItem> mIterator = mSubList.get(i).contents.iterator();
								while (mIterator.hasNext()) {
									CmsItem mCmsItem = mIterator.next();
									if(mCmsItem.type.value.equals(CmsItem.TYPE_GAME)){
										mIterator.remove();
									}
								}
							}
							
							////第二步判断是否有空的组件
							Iterator<CmsComponent> mIterator = mSubList.iterator();
							while (mIterator.hasNext()) {
								CmsComponent mCmsComponent = mIterator.next();
								if(mCmsComponent.contents.size() == 0){
									mIterator.remove();
								}
							}
						}
						
						
						mainRecyclerView.setAdapter(new HomeRecycleAdapter(mFragmentActivity, mSubList,mFakeHeader));
						mEmptyView.setStatus(Status.Gone);
					} else {
						mEmptyView.setStatus(Status.Empty);
					}
				} else {// 失败了
					if (!isCache && mCmsHomeDataInfo == null) {// 不是cache并且mCmsHomeDataInfo为空
						mEmptyView.setStatus(Status.Empty);
					}
				}
			}
		});
		return true;
	}
	
	protected boolean isDataEmpty(){
		return !(mCmsHomeDataInfo != null && mCmsHomeDataInfo.data != null && mCmsHomeDataInfo.data.size() > 0);
	}
}
