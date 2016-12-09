///*
// * Copyright (C) 2013 AChep@xda <artemchep@gmail.com>
// *
// * Licensed under the Apache License, Version 2.0 (the "License");
// * you may not use this file except in compliance with the License.
// * You may obtain a copy of the License at
// *
// *      http://www.apache.org/licenses/LICENSE-2.0
// *
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// */
//
//package com.runmit.sweedee.action.home;
//
//import android.animation.ObjectAnimator;
//import android.app.Activity;
//import android.os.Bundle;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.view.ViewGroup;
//import android.widget.AbsListView;
//import android.widget.FrameLayout;
//import android.widget.LinearLayout;
//import android.widget.ListView;
//import android.widget.Space;
//
//import com.runmit.sweedee.BaseFragment;
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.util.XL_Log;
//import com.runmit.sweedee.view.animation.NotifyingScrollView;
//import com.runmit.sweedee.view.animation.ScrollView;
//import com.runmit.sweedee.view.folding.BaseFoldingLayout;
//import com.runmit.sweedee.view.folding.BaseFoldingLayout.Orientation;
//
//public abstract class SlidHeaderFragment extends BaseFragment {
//	
//	XL_Log log = new XL_Log(SlidHeaderFragment.class);
//
//	protected FrameLayout mFrameLayout;
//
//	// header
//	private View mAppHeadLayout;// app传递给我们的head
//	private View mAppContentLayout;// app传递给我们的content
//	private int mHeaderHeight;
//	private int mHeaderScroll;
//
//	private BaseFoldingLayout mFoldingLayout;
//
//	private Space mFakeHeader;
//	
//	private boolean isListViewEmpty;
//
//	private NotifyingScrollView mNotifyingScrollView;
//
//	private OnHeaderScrollChangedListener mOnHeaderScrollChangedListener;
//	
//	private boolean isHomeFragment = false;
//
//	public void setHomeFragment(boolean isHomeFragment) {
//		this.isHomeFragment = isHomeFragment;
//	}
//	private int actionbarHeight;
//
//	public interface OnHeaderScrollChangedListener {
//		public void onHeaderScrollChanged(float progress, int height, int scroll);
//	}
//
//	public void setOnHeaderScrollChangedListener(OnHeaderScrollChangedListener listener) {
//		mOnHeaderScrollChangedListener = listener;
//	}
//
//	@Override
//	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
//		if (mFrameLayout != null) {
//			return onlyOneOnCreateView(mFrameLayout);
//		}
//		final Activity activity = getActivity();
//		actionbarHeight = getResources().getDimensionPixelOffset(R.dimen.actionbar_height);
//		
//		mFrameLayout = new FrameLayout(activity);
//		mFrameLayout.setBackgroundResource(R.color.back_ground);
//		mAppHeadLayout = onCreateHeaderView(inflater, mFrameLayout);
//		
//		mFoldingLayout = (BaseFoldingLayout) mAppHeadLayout.findViewById(R.id.foldlayout);
//		mFoldingLayout.setAnchorFactor(1);
//		
//		if(mFoldingLayout == null){
//			throw new RuntimeException("mFoldingLayout is null you shod set the mFoldingLayout id == R.id.foldlayout");
//		}
//		mHeaderHeight = mAppHeadLayout.getLayoutParams().height;
//
//		mFakeHeader = new Space(activity);
//		mFakeHeader.setLayoutParams(new ListView.LayoutParams(0, mHeaderHeight));
//
//		mAppContentLayout = onCreateContentView(inflater, mFrameLayout);
//		View contentView;
//		if (mAppContentLayout instanceof ListView) {
//			isListViewEmpty = true;
//
//			final ListView listView = (ListView) mAppContentLayout;
//			listView.addHeaderView(mFakeHeader);
//			listView.setOnScrollListener(new AbsListView.OnScrollListener() {
//
//				@Override
//				public void onScrollStateChanged(AbsListView absListView, int scrollState) {
//				}
//
//				@Override
//				public void onScroll(AbsListView absListView, int firstVisibleItem, int visibleItemCount, int totalItemCount) {
//					if (isListViewEmpty) {
//						scrollHeaderTo(0);
//					} else {
//						final View child = absListView.getChildAt(0);
//						assert child != null;
//						scrollHeaderTo(child == mFakeHeader ? child.getTop() : -mHeaderHeight);
//					}
//				}
//			});
//			contentView = listView;
//		} else {
//
//			final LinearLayout view = new LinearLayout(activity);
//			view.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
//			view.setOrientation(LinearLayout.VERTICAL);
//			view.addView(mFakeHeader);
//			view.addView(mAppContentLayout);
//
//			mNotifyingScrollView = new NotifyingScrollView(activity);
//			mNotifyingScrollView.addView(view);
//			mNotifyingScrollView.setOnScrollChangedListener(new NotifyingScrollView.OnScrollChangedListener() {
//				@Override
//				public void onScrollChanged(ScrollView who, int l, int t, int oldl, int oldt) {	
//					scrollHeaderTo(-t);
//				}
//
//				@Override
//				public void onScrollStateChanged(int scrollState) {
//				}
//			});
//			contentView = mNotifyingScrollView;
//		}
//
//		mFrameLayout.addView(contentView);
//		mFrameLayout.addView(mAppHeadLayout);
//
//		mFrameLayout.post(new Runnable() {
//			@Override
//			public void run() {
//				scrollHeaderTo(0, true);
//			}
//		});
//
//		return mFrameLayout;
//	}
//	
//	public boolean needHeadScroll(int topBarHeight){
//		if(mNotifyingScrollView != null){
//			int mScrollY = mNotifyingScrollView.getScrollY();
//			return mScrollY < topBarHeight;
//		}
//		return false;
//	}
//	
//	public boolean needScrollToHead(int topBarMargin){
//		if(mNotifyingScrollView!=null){
//			int mScrollY = mNotifyingScrollView.getScrollY();
//			return mScrollY + topBarMargin == 0;
//		}
//		return false;
//	}
//
//	public void smoothScrollBy(int y){
//		if(mNotifyingScrollView!=null){
//			ObjectAnimator mValueAnimator = ObjectAnimator.ofInt(mNotifyingScrollView,"ScrollY",mNotifyingScrollView.getScrollY(),mNotifyingScrollView.getScrollY()+y);
//			mValueAnimator.setDuration(400);
//			mValueAnimator.start();
//		}
//	}
//
//	private void scrollHeaderTo(int scrollTo) {
//		scrollHeaderTo(scrollTo, false);
//	}
//
//	private void scrollHeaderTo(int scrollTo, boolean forceChange) {
//		scrollTo = Math.min(Math.max(scrollTo, -mHeaderHeight), 0);		
//		if (mHeaderScroll == (mHeaderScroll = scrollTo) & !forceChange) {
//			return;
//		}
//		mAppHeadLayout.setTranslationY(scrollTo);
//		float factor;
//		if(isHomeFragment){//如果是首页则需要减去actionbar的高度
//			log.debug("scrollTo="+scrollTo+",actionbarHeight="+actionbarHeight+",mFoldingLayout="+mFoldingLayout.getHeight());
//			if(Math.abs(scrollTo) <= actionbarHeight){
//				factor = 0;
//			}else{
//				factor = Math.abs((float) (scrollTo + actionbarHeight) / (float) (mFoldingLayout.getHeight()));
//			}
//		}else{
//			factor = Math.abs((float) (scrollTo) / (float) (mFoldingLayout.getHeight()));
//		}
//		
//		mFoldingLayout.setFoldFactor(factor);
//		
//		notifyOnHeaderScrollChangeListener((float) -scrollTo / mHeaderHeight, mHeaderHeight, -scrollTo);
//	}
//
//	
//	private void notifyOnHeaderScrollChangeListener(float progress, int height, int scroll) {
//		if (mOnHeaderScrollChangedListener != null) {
//			mOnHeaderScrollChangedListener.onHeaderScrollChanged(progress, height, scroll);
//		}
//	}
//	public abstract View onCreateHeaderView(LayoutInflater inflater, ViewGroup container);
//	public abstract View onCreateContentView(LayoutInflater inflater, ViewGroup container);
//}
