package com.tdviewsdk.activity.transforms;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;

/**
 * 
 * @author Sven.Zhan
 * 根据动态点逐渐放大的动画
 */
public class ZoomOutEffect extends BaseEffect {
	
	private View mDstView;
	
	private float pivotX = 0;
    
	private float pivotY = 0;
	
	@Override
	public void prepareAnimation(Activity destActivity) {
		mDstView = destActivity.getWindow().getDecorView();		
	}	

	@Override
	public void setClickPoint(float x, float y) {
		this.pivotX = x;
		this.pivotY = y;
	}
		
	@Override
	public void animate(final Activity destActivity) {
		mDstView.setPivotX(pivotX);
		mDstView.setPivotY(pivotY);		
		AnimatorSet animSet = new AnimatorSet();
//		ObjectAnimator objAnimx = ObjectAnimator.ofFloat(mDstView, "scaleX", 0f, 1.02f,1f,1.01f,1f);
//		ObjectAnimator objAnimy = ObjectAnimator.ofFloat(mDstView, "scaleY", 0f, 1.02f,1f,1.01f,1f);
		ObjectAnimator objAnimx = ObjectAnimator.ofFloat(mDstView, "scaleX", 0f, 1f);
		ObjectAnimator objAnimy = ObjectAnimator.ofFloat(mDstView, "scaleY", 0f, 1f);
		animSet.playTogether(objAnimx,objAnimy);
		animSet.setInterpolator(new AccelerateDecelerateInterpolator());
		animSet.setDuration(duration);
		animSet.setStartDelay(16);
		animSet.start();
	}

	@Override
	public void reverse(final ReverseListener rel) {
		
		if(mDstView == null){
			rel.onReversed();
			return;
		}
		
		AnimatorSet animSet = new AnimatorSet();
//		ObjectAnimator objAnimx = ObjectAnimator.ofFloat(mDstView, "scaleX", 1f, 0f);
//		ObjectAnimator objAnimy = ObjectAnimator.ofFloat(mDstView, "scaleY", 1f, 0f);
		ObjectAnimator objAnimAlpha = ObjectAnimator.ofFloat(mDstView, "alpha", 1f, 0f);
		animSet.playTogether(objAnimAlpha);
		animSet.setInterpolator(new AccelerateDecelerateInterpolator());
		animSet.setDuration(duration/2);	
		animSet.addListener(new MyAnimatorListener() {			
			@Override
			public void onEnd() {
				rel.onReversed();
				mDstView = null;
			}
		});
		animSet.start();
	}
	
	
	abstract class MyAnimatorListener implements AnimatorListener{

		@Override
		public void onAnimationStart(Animator animation) {
			
		}

		@Override
		public void onAnimationEnd(Animator animation) {
			onEnd();
		}

		@Override
		public void onAnimationCancel(Animator animation) {
			onEnd();
		}

		@Override
		public void onAnimationRepeat(Animator animation) {
			
		}		
		public abstract void onEnd();		
	}

}
