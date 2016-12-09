package com.runmit.sweedee.view.animation;

import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.LinearInterpolator;
import android.widget.FrameLayout;

import com.runmit.sweedee.TdStoreMainView;
import com.runmit.sweedee.util.XL_Log;

public class PtrFrameLayout extends FrameLayout {

	private XL_Log log = new XL_Log(PtrFrameLayout.class);

	private View mHeaderView;

	private View mContent;

	private float mLastX = 0;

	private float mLastY = 0;

	private boolean isTouching = false;

	public PtrFrameLayout(Context context) {
		this(context, null);
	}

	public PtrFrameLayout(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
	}

	public PtrFrameLayout(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}

	public void setTdStoreMainView(TdStoreMainView tdStoreMainView) {
	}

	@Override
	public boolean dispatchTouchEvent(MotionEvent ev) {
		switch (ev.getAction()) {
		case MotionEvent.ACTION_DOWN:
			isTouching = true;
			mLastY = ev.getY();
			mLastX = ev.getX();
			break;
		case MotionEvent.ACTION_MOVE: {
			float x = ev.getX();
			float y = ev.getY();
			float offsetY = y - mLastY;
			float offsetX = x - mLastX;
			mLastX = x;
			mLastY = y;
			float translationY = mContent.getTranslationY();
			if (offsetY > 0 && Math.abs(offsetY) > Math.abs(offsetX)) {
				if (translationY < mHeaderView.getHeight()) {
					translationY += offsetY;
					translationY = Math.max(translationY, 0);
					translationY = Math.min(translationY, mHeaderView.getHeight());
					mHeaderView.setTranslationY(-mHeaderView.getHeight() + translationY);
					mContent.setTranslationY(translationY);
				}
			}
			if (offsetY < 0 && Math.abs(offsetY) > Math.abs(offsetX)) {
				if (translationY > 0) {
					translationY += offsetY;
					translationY = Math.max(translationY, 0);
					translationY = Math.min(translationY, mHeaderView.getHeight());
					mHeaderView.setTranslationY(-mHeaderView.getHeight() + translationY);
					mContent.setTranslationY(translationY);
				}
			}
			break;
		}

		case MotionEvent.ACTION_CANCEL:
		case MotionEvent.ACTION_UP:
			isTouching = false;
			float translationY = mContent.getTranslationY();
			if (translationY > 0 || translationY < mHeaderView.getHeight()) {
				boolean isUp = translationY > mHeaderView.getHeight() / 2;
				ObjectAnimator mObjectAnimator = null;
				if (isUp) {
					mObjectAnimator = ObjectAnimator.ofFloat(mContent, "translationY", translationY, mHeaderView.getHeight());
				} else {
					mObjectAnimator = ObjectAnimator.ofFloat(mContent, "translationY", translationY, 0);
				}
				mObjectAnimator.setInterpolator(new LinearInterpolator());
				mObjectAnimator.setDuration(200);
				mObjectAnimator.addUpdateListener(new MyUpdateListener(mHeaderView.getHeight(), mObjectAnimator));
				mObjectAnimator.start();
			}
			break;
		default:
			break;
		}
//		log.debug("not ontouch");
		return super.dispatchTouchEvent(ev);
	}

	public boolean dispatchTouchEventSupper(MotionEvent e) {
		return super.dispatchTouchEvent(e);
	}

	@Override
	protected void onFinishInflate() {
		mHeaderView = getChildAt(0);
		mContent = getChildAt(1);
	}

	public void open(boolean withAnimator) {
		int height = mHeaderView.getHeight();
		if (height == 0) {
			height = mHeaderView.getLayoutParams().height;
		}

		if (withAnimator) {
			float translationY = mContent.getTranslationY();
			ObjectAnimator mObjectAnimator = ObjectAnimator.ofFloat(mContent, "translationY", translationY, height);
			mObjectAnimator.setInterpolator(new LinearInterpolator());
			mObjectAnimator.setDuration(400);
			mObjectAnimator.addUpdateListener(new MyUpdateListener(mHeaderView.getHeight(), mObjectAnimator));
			mObjectAnimator.start();
		} else {
			mContent.setTranslationY(height);
		}
	}

	public void close(boolean withAnimator) {
		if (withAnimator) {
			float translationY = mContent.getTranslationY();
			ObjectAnimator mObjectAnimator = ObjectAnimator.ofFloat(mContent, "translationY", translationY, 0);
			mObjectAnimator.setInterpolator(new LinearInterpolator());
			mObjectAnimator.setDuration(400);
			mObjectAnimator.addUpdateListener(new MyUpdateListener(mHeaderView.getHeight(), mObjectAnimator));
			mObjectAnimator.start();
		} else {
			mContent.setTranslationY(0);
		}
	}

	private class MyUpdateListener implements AnimatorUpdateListener {
		private int height;
		private ObjectAnimator animator;

		public MyUpdateListener(int height, ObjectAnimator animator) {
			this.height = height;
			this.animator = animator;
		}

		@Override
		public void onAnimationUpdate(ValueAnimator value) {
			if (isTouching) {
				if (animator != null && animator.isRunning())
					animator.cancel();
			}
			float dy = -height + (Float) value.getAnimatedValue();
			mHeaderView.setTranslationY(dy);
		}
	}

}