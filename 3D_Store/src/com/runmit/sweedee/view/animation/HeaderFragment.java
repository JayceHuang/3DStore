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
//package com.runmit.sweedee.view.animation;
//
//import android.animation.ValueAnimator;
//import android.animation.ValueAnimator.AnimatorUpdateListener;
//import android.annotation.TargetApi;
//import android.app.Activity;
//import android.graphics.Bitmap;
//import android.graphics.Canvas;
//import android.graphics.drawable.BitmapDrawable;
//import android.graphics.drawable.Drawable;
//import android.os.Build;
//import android.os.Bundle;
//import android.util.Log;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.view.ViewGroup;
//import android.view.ViewGroup.LayoutParams;
//import android.widget.AbsListView;
//import android.widget.FrameLayout;
//import android.widget.ImageView;
//import android.widget.LinearLayout;
//import android.widget.ListAdapter;
//import android.widget.ListView;
//import android.widget.RelativeLayout;
//import android.widget.Space;
//
//import com.runmit.sweedee.BaseFragment;
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.util.XL_Log;
//
//public abstract class HeaderFragment extends BaseFragment {
//	
//	XL_Log log = new XL_Log(HeaderFragment.class);
//
//	public static final int HEADER_BACKGROUND_SCROLL_NORMAL = 0;
//	
//	public static final int HEADER_BACKGROUND_SCROLL_PARALLAX = 1;
//	
//	public static final int HEADER_BACKGROUND_SCROLL_STATIC = 2;
//
//	protected FrameLayout mFrameLayout;
//	
//	/**Head 允许翻转的最大角度，可以调整*/
//	private final float MAX_ROTATION = 80f;
//
//	// header
//	private FrameLayout mHeader;// 此类内部生成的
//	private View mAppHeadLayout;// app传递给我们的head
//	private View mAppContentLayout;// app传递给我们的content
//	private int mHeaderHeight;
//	private int mHeaderScroll;
//
//	private ImageView mBlurredImageView;
//
//	/**
//	 * Down scale factor to reduce blurring time and memory allocation.
//	 */
//	private float mDownScaleFactor = 5.0f;
//
//	/**
//	 * Render flag
//	 * <p/>
//	 * If true we must render if false, we have already blurred the background
//	 */
//	private boolean prepareToRender = true;
//
//	private Space mFakeHeader;
//	private boolean isListViewEmpty;
//
//	private NotifyingScrollView mNotifyingScrollView;
//
//	private OnHeaderScrollChangedListener mOnHeaderScrollChangedListener;
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
//		log.debug("HeaderFragment onCreateView");
//		final Activity activity = getActivity();
//		assert activity != null;
//		mFrameLayout = new FrameLayout(activity);
//		mFrameLayout.setBackgroundResource(R.color.back_ground);
//		mAppHeadLayout = onCreateHeaderView(inflater, mFrameLayout);
//		assert mAppHeadLayout.getLayoutParams() != null;
//		mHeaderHeight = mAppHeadLayout.getLayoutParams().height;
//		log.debug("mHeaderHeight="+mHeaderHeight);
//		mHeader = new FrameLayout(getActivity());
//		mHeader.setLayoutParams(new ListView.LayoutParams(LayoutParams.MATCH_PARENT, mHeaderHeight));
//		mHeader.addView(mAppHeadLayout);
//		mBlurredImageView = new ImageView(getActivity());
//		RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
//
//		mBlurredImageView.setLayoutParams(params);
//		mBlurredImageView.setClickable(false);
//		mBlurredImageView.setVisibility(View.GONE);
//		mBlurredImageView.setScaleType(ImageView.ScaleType.FIT_XY);
//		mHeader.post(new Runnable() {
//			@Override
//			public void run() {
//				mHeader.addView(mBlurredImageView, 1);
//			}
//		});
//
//		mFakeHeader = new Space(activity);
//		mFakeHeader.setLayoutParams(new ListView.LayoutParams(0, mHeaderHeight));
//
//		mAppContentLayout = onCreateContentView(inflater, mFrameLayout);
//		View contentView;
//		log.debug("is ListView = "+ (mAppContentLayout instanceof ListView));
//		if (mAppContentLayout instanceof ListView) {
//			isListViewEmpty = true;
//
//			final ListView listView = (ListView) mAppContentLayout;
//			listView.addHeaderView(mFakeHeader);
//			listView.setOnScrollListener(new AbsListView.OnScrollListener() {
//
//				@Override
//				public void onScrollStateChanged(AbsListView absListView, int scrollState) {
//					if (scrollState == AbsListView.OnScrollListener.SCROLL_STATE_IDLE) {
//						final View child = absListView.getChildAt(0);
////						if (child != null && child == mFakeHeader) {
////							final int childTop = child.getTop();
////							final int childHeight = child.getHeight();
////							final int scrollOffset = Math.abs(childTop) > childHeight / 2 ? childTop + childHeight : childTop;
////							listView.post(new Runnable() {
////								public void run() {
////									if (Math.abs(childTop) > childHeight / 2) {
////										listView.smoothScrollBy(scrollOffset, 200);
////									} else {
////										listView.smoothScrollByOffset(scrollOffset);
////									}
////								}
////							});
////						} else {
////							handleRecycle();// 停止了
////						}
//						if (child != null && child == mFakeHeader && child.getTop() == 0){
//							handleRecycle();
//						}
//					} else {
//						render();
//					}
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
//			// Merge fake header view and content view.
//			final LinearLayout view = new LinearLayout(activity);
//			view.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
//			view.setOrientation(LinearLayout.VERTICAL);
//			view.addView(mFakeHeader);
//			view.addView(mAppContentLayout);
//
//			// Put merged content to ScrollView
//			mNotifyingScrollView = new NotifyingScrollView(activity);
//			mNotifyingScrollView.addView(view);
//			mNotifyingScrollView.setOnScrollChangedListener(new NotifyingScrollView.OnScrollChangedListener() {
//				@Override
//				public void onScrollChanged(ScrollView who, int l, int t, int oldl, int oldt) {	
//					log.debug("onScrollChanged t = "+t);
//					scrollHeaderTo(-t);
//				}
//
//				@Override
//				public void onScrollStateChanged(int scrollState) {
//					int scrollY = mNotifyingScrollView.getScrollY();
//					log.debug("scroll="+scrollY);
//					if (scrollState == AbsListView.OnScrollListener.SCROLL_STATE_IDLE) {
////						if (scrollY > 0 && scrollY < mHeader.getHeight()) {
////							int scrollTo = 0;
////							if (scrollY > mHeader.getHeight() / 2 && mAppContentLayout.getHeight() >= scrollView.getHeight()) {
////								scrollTo = mHeader.getHeight();
////							} else {
////								scrollTo = 0;
////							}
////							scrollView.smoothScrollTo(scrollView.getScrollX(), scrollTo);
////						} else {
////							handleRecycle();// 停止了
////						}
//						if(scrollY == 0){
//							handleRecycle();
//						}
//					} else {
//						render();
//					}
//				}
//			});
//			contentView = mNotifyingScrollView;
//		}
//
//		mFrameLayout.addView(contentView);
//		mFrameLayout.addView(mHeader);
//
//		// Post initial scroll
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
//			ValueAnimator mValueAnimator = ValueAnimator.ofFloat(mNotifyingScrollView.getScrollY(),mNotifyingScrollView.getScrollY()+y);
//			mValueAnimator.addUpdateListener(new AnimatorUpdateListener() {
//				
//				@Override
//				public void onAnimationUpdate(ValueAnimator animation) {
//					float value = (Float) animation.getAnimatedValue();
//					mNotifyingScrollView.setScrollY((int) value);
//				}
//			});
//			mValueAnimator.setDuration(400);
//			mValueAnimator.start();
//		}
//	}
//
//	private void render() {
//		if (prepareToRender) {
//			prepareToRender = false;
//			Bitmap bitmap = loadBitmapFromView(mAppHeadLayout);
//			bitmap = scaleBitmap(bitmap);
//			bitmap = Blur.fastblur(getActivity(), bitmap, 25);
//			mBlurredImageView.setVisibility(View.VISIBLE);
//			mBlurredImageView.setImageBitmap(bitmap);
//		}
//	}
//
//	private Bitmap scaleBitmap(Bitmap myBitmap) {
//		int width = (int) (myBitmap.getWidth() / mDownScaleFactor);
//		int height = (int) (myBitmap.getHeight() / mDownScaleFactor);
//		return Bitmap.createScaledBitmap(myBitmap, width, height, false);
//	}
//
//	private Bitmap loadBitmapFromView(View mView) {
//		Bitmap b = Bitmap.createBitmap(mView.getWidth(), mView.getHeight(), Bitmap.Config.ARGB_8888);
//		Canvas c = new Canvas(b);
//		mView.draw(c);
//		return b;
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
//		setViewTranslationY(mHeader, scrollTo);
//		notifyOnHeaderScrollChangeListener((float) -scrollTo / mHeaderHeight, mHeaderHeight, -scrollTo);
//	}
//
//	private void handleRecycle() {
//		log.debug("handleRecycle");
//		mBlurredImageView.setVisibility(View.GONE);
//		Drawable drawable = mBlurredImageView.getDrawable();
//		if (drawable instanceof BitmapDrawable) {
//			BitmapDrawable bitmapDrawable = ((BitmapDrawable) drawable);
//			Bitmap bitmap = bitmapDrawable.getBitmap();
//			if (bitmap != null){
//				bitmap.recycle();
//			}
//			mBlurredImageView.setImageBitmap(null);
//		}
//		prepareToRender = true;
//	}
//
//	private void setViewTranslationY(View view, float translationY) {
//		if (view != null) {
//			view.setTranslationY(translationY);
//			view.setPivotX(view.getWidth() / 2);
//			view.setPivotY(mHeaderHeight);	
//			float dH = mHeaderHeight + translationY; //Header滑动的高度差
//			float currentRatio = 1f - dH / mHeaderHeight;
//			float currentRotation = MAX_ROTATION * currentRatio;
//			log.debug("currentRotation = " + currentRotation+",mHeaderHeight="+mHeaderHeight+",translationY="+translationY);
//			view.setRotationX(currentRotation);
//			mBlurredImageView.setImageAlpha((int) (255 * currentRatio));
//		}
//	}
//	
//	private void notifyOnHeaderScrollChangeListener(float progress, int height, int scroll) {
//		if (mOnHeaderScrollChangedListener != null) {
//			mOnHeaderScrollChangedListener.onHeaderScrollChanged(progress, height, scroll);
//		}
//	}
//
//	public abstract View onCreateHeaderView(LayoutInflater inflater, ViewGroup container);
//
//	public abstract View onCreateContentView(LayoutInflater inflater, ViewGroup container);
//
//	public void setListViewAdapter(ListView listView, ListAdapter adapter) {
//		isListViewEmpty = adapter == null;
//		listView.setAdapter(null);
//		listView.removeHeaderView(mFakeHeader);
//		listView.addHeaderView(mFakeHeader);
//		listView.setAdapter(adapter);
//	}
//}
