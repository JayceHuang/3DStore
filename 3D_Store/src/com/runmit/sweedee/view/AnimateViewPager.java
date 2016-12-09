package com.runmit.sweedee.view;

import java.util.HashMap;
import java.util.LinkedHashMap;

import com.nineoldandroids.view.ViewHelper;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.annotation.SuppressLint;
import android.content.Context;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.DecelerateInterpolator;

public class AnimateViewPager extends ViewPager {
	public static final String TAG = "AnimateViewPager";

	private HashMap<Integer, Object> mFragViewObjs = new LinkedHashMap<Integer, Object>();	// 保存 fragment view 对象

	private PageTransitionCallback mAniCallback;	// 动画结束回调对象
	
	private PageTransformer mTransformer;

	// 滑动状态;
	private int scrollState = SCROLL_STATE_IDLE;
	public void setScrollState(int newState) {
		scrollState = newState;
	}
	
	public AnimateViewPager(Context context) {
		this(context, null);
	}
	
	public AnimateViewPager(Context context, AttributeSet attrs) {
		super(context, attrs);
		setClipChildren(false);
	}
	
	
	/**
	 * 需要同时屏蔽 onTouchEvent & onInterceptTouchEvent
	 * viewpager才不会在动画的时候响应触摸
	 */
	@SuppressLint("ClickableViewAccessibility")
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		// 滑动的时候需要停止监听touch事件，以保证子viewpager能监听touch事件
		return true;
//		if(scrollState == 2)return true;
//		
//		if(getAnimateState()){
//			return true;
//		}
//		try{
//			return super.onTouchEvent(event);
//		} catch(IllegalArgumentException ex) {}  
//		return false; 
	}
	
	public boolean onInterceptTouchEvent(MotionEvent ev) {
		// 滑动的时候需要停止监听Intercept touch事件，以保证子viewpager能监听touch事件
		return false;
//		if(scrollState == 2)return true;
//		if(getAnimateState()){
//			return true;
//		}
//		try{
//			return super.onInterceptTouchEvent(ev);
//		} catch(IllegalArgumentException ex) {}  
//		return false; 
	}
		
	
	private void logState(View v, String title) {
//		Log.v(TAG, title + ": ROT (" + ViewHelper.getRotation(v) + ", " +
//				ViewHelper.getRotationX(v) + ", " +
//				ViewHelper.getRotationY(v) + "), TRANS (" +
//				ViewHelper.getTranslationX(v) + ", " +
//				ViewHelper.getTranslationY(v) + "), SCALE (" +
//				ViewHelper.getScaleX(v) + ", " + 
//				ViewHelper.getScaleY(v) + "), ALPHA " +
//				ViewHelper.getAlpha(v));
	}
	
	public void setObjectForPosition(Object obj, int position) {
		mFragViewObjs.put(Integer.valueOf(position), obj);
	}
	
	
	public View findViewFromObject(int position) {
		Object o = mFragViewObjs.get(Integer.valueOf(position));
		if (o == null) {
			return null;
		}
		PagerAdapter a = getAdapter();
		View v;
		for (int i = 0; i < getChildCount(); i++) {
			v = getChildAt(i);
			if (a.isViewFromObject(v, o)){
				v.setVisibility(View.VISIBLE);
				// Log.d("mzh", "foud view"+v.getLeft() + ",right="+ v.getRight());
				return v;
			}
		}
		return null;
	}
	
	public void setPageTransformer(boolean reverseDrawingOrder, PageTransformer transformer){
		mTransformer = transformer;
		super.setPageTransformer(reverseDrawingOrder, transformer);
	}
	
	/**
	 * 设置立即切换，不需要切换效果
	 */
	public void changePageimmediately(int pisition) {
		setPageTransformer(true, null);
		resetAllAlpha();
		setCurrentItem(pisition, false);
		setPageTransformer(true, mTransformer);
	}
		
	/**
	 * 在设置 DepthPageTransformer后页面会变透明！！！ 
	 */
	private void resetAllAlpha(){
		for(int i= getAdapter().getCount(); i >= 0; i--){
			View view = findViewFromObject(i);
			if(view != null){
				ViewHelper.setTranslationX(view, 0);
				view.setScaleX(1);
				view.setScaleY(1);
				view.setAlpha(1);
				//logState(view, "state");
			}
		}
	}
	
	public void addAnimationEndListener(PageTransitionCallback callback){
		mAniCallback = callback;
	}
	
	public static interface PageTransitionCallback {
		public void onAnimationEnd();
	}
	
	
	// ==============================================================================================================
	// ============================================= 自定义的切换效果 ================================================
	// ==============================================================================================================
	
	private View mTopView;
	private View mBtmView;
	private boolean isAnimating = false;
	private AnimatorSet animatorSetL;
	private AnimatorSet animatorSetR;
	
	private int width;
	private int height;
	private float currentX;
	
	/**
	 * false: left;  true: right
	 */
	private boolean dir;
	
	public boolean getAnimateState(){
		return isAnimating;
	}
	
	private void initTransform(){
		mTopView.setAlpha(1);
		mBtmView.setAlpha(1);
		ViewHelper.setScaleX(mTopView, 1);
		ViewHelper.setScaleY(mTopView, 1);
		ViewHelper.setScaleX(mBtmView, 1);
		ViewHelper.setScaleY(mBtmView, 1);
		
		width = mTopView.getMeasuredWidth();
		height = mTopView.getMeasuredHeight();
		ViewHelper.setPivotX(mTopView, width >> 1);
		ViewHelper.setPivotY(mTopView, height);
		
		currentX = (dir ?  -1 : 1) * width + getPageMargin();
		ViewHelper.setPivotX(mBtmView, width >> 1);
		ViewHelper.setPivotY(mBtmView, height);
		ViewHelper.setTranslationX(mBtmView, currentX);	
		
		mTopView.bringToFront();
	}
	
	public void bounceToLeft() {
		int pos = getCurrentItem();
		
		if(pos <= 0 || isAnimating){
			return;
		}
		
		setPageTransformer(true, null);
		
		mTopView = findViewFromObject(pos);
		mBtmView = findViewFromObject(pos-1);
		dir = false;
		initTransform();
		
		isAnimating = true;
		startAnimateL();
	}

	public void bounceToRight() {
		int pos = getCurrentItem();
		//Log.d("mzh", "pos:" + pos + "  count:" + getAdapter().getCount());
		if(pos >= getAdapter().getCount() -1 || isAnimating){
			return;
		}
		
		setPageTransformer(true, null);
		
		mTopView = findViewFromObject(pos);
		mBtmView = findViewFromObject(pos+1);
		dir = true;
		initTransform();
		
		isAnimating = true;
		startAnimateR();
		
	}
	
	private static final float RO_MAX = 5.0f;	// 动画幅度
	private void startAnimateR() {
		clearAll();
		
		float ratio1 = 0.6f;
		float ratio2 = 0.25f;
		float ratio3 = 0.8f;
		
		float moveDes = (float) ((width>>1) * Math.sin(RO_MAX * RAD));
		
		ObjectAnimator t_move_x1 = ObjectAnimator.ofFloat(mTopView, "translationX", 0, width * ratio1);
		ObjectAnimator t_move_y1 = ObjectAnimator.ofFloat(mTopView, "translationY", 0, moveDes);
		ObjectAnimator t_rotate = ObjectAnimator.ofFloat(mTopView, "rotation", 0, RO_MAX);
		ObjectAnimator t_move_x2 = ObjectAnimator.ofFloat(mTopView, "translationX", width * ratio1, width);
		
		ObjectAnimator b_rotate = ObjectAnimator.ofFloat(mBtmView, "rotation", 0, RO_MAX * ratio3);
		ObjectAnimator b_move_x1 = ObjectAnimator.ofFloat(mBtmView, "translationX", currentX, currentX + width * ratio2);
		ObjectAnimator b_move_y1 = ObjectAnimator.ofFloat(mBtmView, "translationY", 0, moveDes);
		ObjectAnimator b_rotate2 = ObjectAnimator.ofFloat(mBtmView, "rotation", RO_MAX * ratio3, 0);
		ObjectAnimator b_move_x2 = ObjectAnimator.ofFloat(mBtmView, "translationX", currentX + width * ratio2, currentX);
		ObjectAnimator b_move_y2 = ObjectAnimator.ofFloat(mBtmView, "translationY", moveDes, 0);
		
		b_rotate2.setStartDelay(50);
		b_move_x2.setStartDelay(50);
		b_move_y2.setStartDelay(50);
		
		animatorSetR.play(t_move_x1).with(t_rotate).with(t_move_y1).with(b_move_y1)		
				   .with(b_rotate).with(b_move_x1).before(t_move_x2);
		
		animatorSetR.play(t_move_x2).with(b_rotate2).with(b_move_x2).with(b_move_y2);
				
		animatorSetR.start();
		
	}
	
	private final double RAD = Math.PI/180.0;
	
	private void startAnimateL() {
		clearAll();
		float halfWidth = width>>1;
		ObjectAnimator t_move_x1 = ObjectAnimator.ofFloat(mTopView, "translationX", 0, - halfWidth);
		ObjectAnimator t_move_y1 = ObjectAnimator.ofFloat(mTopView, "translationY", 0, (float) (halfWidth * Math.sin(RO_MAX * RAD)));
		ObjectAnimator t_rotate = ObjectAnimator.ofFloat(mTopView, "rotation", 0, -RO_MAX);
		ObjectAnimator t_move_x2 = ObjectAnimator.ofFloat(mTopView, "translationX", -halfWidth, -width);
		
		float ratio1 = 0.6f;
		float ratio2 = 0.25f;
		ObjectAnimator b_rotate = ObjectAnimator.ofFloat(mBtmView, "rotation", 0, -RO_MAX * ratio1);
		ObjectAnimator b_move_x1 = ObjectAnimator.ofFloat(mBtmView, "translationX", currentX, currentX - width * ratio2);
		ObjectAnimator b_move_y1 = ObjectAnimator.ofFloat(mBtmView, "translationY", 0, (float) (halfWidth * Math.sin(RO_MAX * RAD)));
		ObjectAnimator b_rotate2 = ObjectAnimator.ofFloat(mBtmView, "rotation", -RO_MAX * ratio1, 0);
		ObjectAnimator b_move_x2 = ObjectAnimator.ofFloat(mBtmView, "translationX", currentX- width * ratio2, currentX);
		ObjectAnimator b_move_y2 = ObjectAnimator.ofFloat(mBtmView, "translationY", (float) (halfWidth * Math.sin(RO_MAX * RAD)), 0);
		
		animatorSetL.play(t_move_x1).with(t_rotate).with(t_move_y1)
				   .with(b_rotate).with(b_move_x1).with(b_move_y1).before(t_move_x2);
		
		animatorSetL.play(t_move_x2).with(b_rotate2).with(b_move_x2).with(b_move_y2);
		
		animatorSetL.start();
		
	}

	private final static int duration = 400; 
	private void clearAll() {
		if(dir){
			if(animatorSetR != null){
				animatorSetR.end();
			}
			animatorSetR = new AnimatorSet();
			animatorSetR.setDuration(duration);
			animatorSetR.setInterpolator(new DecelerateInterpolator());
			animatorSetR.addListener(animatorListener);
		}
		else{
			if(animatorSetL != null){
				animatorSetL.end();
			}
			animatorSetL = new AnimatorSet();
			animatorSetL.setDuration(duration);
			animatorSetL.setInterpolator(new DecelerateInterpolator());
			animatorSetL.addListener(animatorListener);
		}
	}
	
	private AnimatorListener animatorListener = new AnimatorListener() {
		
		@Override
		public void onAnimationStart(Animator arg0) {
			
		}
		
		@Override
		public void onAnimationRepeat(Animator arg0) {}
		
		@Override
		public void onAnimationEnd(Animator arg0) {
			//Log.d("mzh", "t_dx:" + ViewHelper.getTranslationX(mTopView) + " b_dx:"+ ViewHelper.getTranslationX(mBtmView));
			postDelayed(new Runnable() {
				@Override
				public void run() {
					ViewHelper.setRotation(mTopView, 0);
					ViewHelper.setRotation(mBtmView, 0);
					ViewHelper.setTranslationX(mTopView, 0);
					ViewHelper.setTranslationX(mBtmView, 0);
					ViewHelper.setTranslationY(mTopView, 0);
					ViewHelper.setTranslationY(mBtmView, 0);
					setCurrentItem(dir ? getCurrentItem()+1 : getCurrentItem()-1, false);
					isAnimating = false;
					
					if(mAniCallback != null){
						setPageTransformer(true, mTransformer);	// 动画结束后设置回原来的  transformer
						mAniCallback.onAnimationEnd();
					}
				}
			}, 20);
		}
		
		@Override
		public void onAnimationCancel(Animator arg0) {}
	};
	
}
