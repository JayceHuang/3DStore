package com.runmit.sweedee.view;
///*
// * Copyright (C) 2008 The Android Open Source Project
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
// * 
// * Modifications by: Alessandro Crugnola
// */
//
//package com.xunlei.cloud.view;
//
//import android.content.Context;
//import android.content.res.TypedArray;
//import android.graphics.Bitmap;
//import android.graphics.Canvas;
//import android.graphics.Rect;
//import android.location.Location;
//import android.os.Handler;
//import android.os.Message;
//import android.os.SystemClock;
//import android.util.AttributeSet;
//import android.util.Log;
//import android.view.MotionEvent;
//import android.view.SoundEffectConstants;
//import android.view.VelocityTracker;
//import android.view.View;
//import android.view.ViewGroup;
//import android.view.View.MeasureSpec;
//import android.view.accessibility.AccessibilityEvent;
//import android.widget.ListAdapter;
//import android.widget.ListView;
//import android.widget.RelativeLayout;
//import android.widget.TextView;
//
//import com.xunlei.cloud.R;
//import com.xunlei.cloud.util.XL_Log;
//
//public class MultiDirectionSlidingDrawer extends ViewGroup {
//
//	XL_Log log = new XL_Log(MultiDirectionSlidingDrawer.class);
//
//	// public static final int ORIENTATION_RTL = 0;
//	public static final int ORIENTATION_BTT = 1;
//	// public static final int ORIENTATION_LTR = 2;
//	public static final int ORIENTATION_TTB = 3;
//
//	private static final int TAP_THRESHOLD = 6;
//	private static final float MAXIMUM_TAP_VELOCITY = 100.0f;
//	private static final float MAXIMUM_MINOR_VELOCITY = 150.0f;
//	private static final float MAXIMUM_MAJOR_VELOCITY = 200.0f;
//	private static final float MAXIMUM_ACCELERATION = 2000.0f;
//	private static final int VELOCITY_UNITS = 1000;
//	private static final int MSG_ANIMATE = 1000;
//	private static final int ANIMATION_FRAME_DURATION = 1000 / 60;
//
//	private static final int EXPANDED_FULL_OPEN = -10001;
//	private static final int COLLAPSED_FULL_CLOSED = -10002;
//
//	private View mHandle;
//	private View mContent;
//
//	private final Rect mFrame = new Rect();
//	private final Rect mInvalidate = new Rect();
//	private boolean mTracking;
//	private boolean mLocked;
//
//	private VelocityTracker mVelocityTracker;
//
//	// private boolean mVertical;
//	private boolean mExpanded;
//	private int mBottomOffset;
//	private int mTopOffset;
//	private int mHandleHeight;
//	private int mHandleWidth;
//
//	private OnDrawerOpenListener mOnDrawerOpenListener;
//	private OnDrawerCloseListener mOnDrawerCloseListener;
//	private OnDrawerScrollListener mOnDrawerScrollListener;
//
//	private final Handler mHandler = new SlidingHandler();
//	private float mAnimatedAcceleration;
//	private float mAnimatedVelocity;
//	private float mAnimationPosition;
//	private long mAnimationLastTime;
//	private long mCurrentAnimationTime;
//	private int mTouchDelta;
//	private boolean mAnimating;
//	private boolean mAllowSingleTap;
//	private boolean mAnimateOnClick;
//
//	private final int mTapThreshold;
//	private final int mMaximumTapVelocity;
//	private int mMaximumMinorVelocity;
//	private int mMaximumMajorVelocity;
//	private int mMaximumAcceleration;
//	private final int mVelocityUnits;
//
//	/**
//	 * Callback invoked when the drawer is opened.
//	 */
//	public static interface OnDrawerOpenListener {
//
//		/**
//		 * Invoked when the drawer becomes fully open.
//		 */
//		public void onDrawerOpened();
//	}
//
//	/**
//	 * Callback invoked when the drawer is closed.
//	 */
//	public static interface OnDrawerCloseListener {
//
//		/**
//		 * Invoked when the drawer becomes fully closed.
//		 */
//		public void onDrawerClosed();
//	}
//
//	/**
//	 * Callback invoked when the drawer is scrolled.
//	 */
//	public static interface OnDrawerScrollListener {
//
//		/**
//		 * Invoked when the user starts dragging/flinging the drawer's handle.
//		 */
//		public void onScrollStarted();
//
//		/**
//		 * Invoked when the user stops dragging/flinging the drawer's handle.
//		 */
//		public void onScrollEnded();
//	}
//
//	/**
//	 * Creates a new SlidingDrawer from a specified set of attributes defined in
//	 * XML.
//	 * 
//	 * @param context
//	 *            The application's environment.
//	 * @param attrs
//	 *            The attributes defined in XML.
//	 */
//	public MultiDirectionSlidingDrawer(Context context, AttributeSet attrs) {
//		this(context, attrs, 0);
//	}
//
//	public interface ScrollingEndListener {
//		void onFinish();
//	}
//
//	private ScrollingEndListener scrollingEndListener;
//
//	public void setScrollingEndListener(
//			ScrollingEndListener scrollingEndListener) {
//		this.scrollingEndListener = scrollingEndListener;
//	}
//
//	/**
//	 * Creates a new SlidingDrawer from a specified set of attributes defined in
//	 * XML.
//	 * 
//	 * @param context
//	 *            The application's environment.
//	 * @param attrs
//	 *            The attributes defined in XML.
//	 * @param defStyle
//	 *            The style to apply to this widget.
//	 */
//	public MultiDirectionSlidingDrawer(Context context, AttributeSet attrs,
//			int defStyle) {
//		super(context, attrs, defStyle);
//		TypedArray a = context.obtainStyledAttributes(attrs,
//				R.styleable.MultiDirectionSlidingDrawer, defStyle, 0);
//
//		int orientation = a.getInt(
//				R.styleable.MultiDirectionSlidingDrawer_direction,
//				ORIENTATION_TTB);
//
//		mBottomOffset = (int) a.getDimension(
//				R.styleable.MultiDirectionSlidingDrawer_bottomOffset, 0.0f);
//		mTopOffset = (int) a.getDimension(
//				R.styleable.MultiDirectionSlidingDrawer_topOffset, 0.0f);
//		mAllowSingleTap = a.getBoolean(
//				R.styleable.MultiDirectionSlidingDrawer_allowSingleTap, true);
//		mAnimateOnClick = a.getBoolean(
//				R.styleable.MultiDirectionSlidingDrawer_animateOnClick, true);
//		final float density = getResources().getDisplayMetrics().density;
//		mTapThreshold = (int) (TAP_THRESHOLD * density + 0.5f);
//		mMaximumTapVelocity = (int) (MAXIMUM_TAP_VELOCITY * density + 0.5f);
//		mMaximumMinorVelocity = (int) (MAXIMUM_MINOR_VELOCITY * density + 0.5f);
//		mMaximumMajorVelocity = (int) (MAXIMUM_MAJOR_VELOCITY * density + 0.5f);
//		mMaximumAcceleration = (int) (MAXIMUM_ACCELERATION * density + 0.5f);
//		mVelocityUnits = (int) (VELOCITY_UNITS * density + 0.5f);
//
//		mMaximumAcceleration = -mMaximumAcceleration;
//		mMaximumMajorVelocity = -mMaximumMajorVelocity;
//		mMaximumMinorVelocity = -mMaximumMinorVelocity;
//
//		a.recycle();
//		setAlwaysDrawnWithCacheEnabled(false);
//	}
//
//	@Override
//	protected void onFinishInflate() {
//		mHandle = findViewById(R.id.handle);
//		if (mHandle == null) {
//			throw new IllegalArgumentException(
//					"The handle attribute is must refer to an"
//							+ " existing child.");
//		}
//
//		mContent = findViewById(R.id.content);
//		if (mContent == null) {
//			throw new IllegalArgumentException(
//					"The content attribute is must refer to an"
//							+ " existing child.");
//		}
//		log.debug("mContent=" + mContent.getClass().getName());
//		mContent.setVisibility(View.GONE);
//	}
//
//	public void setOnClick() {
//		mHandle.setOnClickListener(new DrawerToggler());
//	}
//
//	@Override
//	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
//		super.onMeasure(widthMeasureSpec, heightMeasureSpec);
//		int widthSpecMode = MeasureSpec.getMode(widthMeasureSpec);
//		int widthSpecSize = MeasureSpec.getSize(widthMeasureSpec);
//
//		int heightSpecMode = MeasureSpec.getMode(heightMeasureSpec);
//		int heightSpecSize = MeasureSpec.getSize(heightMeasureSpec);
//		if (widthSpecMode == MeasureSpec.UNSPECIFIED
//				|| heightSpecMode == MeasureSpec.UNSPECIFIED) {
//			throw new RuntimeException(
//					"SlidingDrawer cannot have UNSPECIFIED dimensions");
//		}
//
//		final View handle = mHandle;
//		measureChild(handle, widthMeasureSpec, heightMeasureSpec);
//
//		int height = heightSpecSize - handle.getMeasuredHeight() - mTopOffset;
//
//		mContent.measure(
//				MeasureSpec.makeMeasureSpec(widthSpecSize, MeasureSpec.EXACTLY),
//				MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY));
//		int mContentHeiht = mContent.getMeasuredHeight();
//
//		setMeasuredDimension(widthSpecSize, heightSpecSize);
//	}
//
//	@Override
//	protected void dispatchDraw(Canvas canvas) {
//		final long drawingTime = getDrawingTime();
//		final View handle = mHandle;
//		log.debug("dispatchDraw=" + (mTracking || mAnimating));
//		drawChild(canvas, handle, drawingTime);
//
//		if (mTracking || mAnimating) {
//			final Bitmap cache = mContent.getDrawingCache();
//			if (cache != null) {
//				canvas.drawBitmap(cache, 0,
//						handle.getTop() - mContent.getMeasuredHeight(), null);
//			} else {
//				canvas.save();
//				canvas.translate(
//						0,
//						handle.getTop() - mTopOffset
//								- mContent.getMeasuredHeight());
//				drawChild(canvas, mContent, drawingTime);
//				canvas.restore();
//			}
//			invalidate();
//		} else if (mExpanded) {
//			drawChild(canvas, mContent, drawingTime);
//		}
//	}
//
//	public static final String LOG_TAG = "Sliding";
//
//	@Override
//	protected void onLayout(boolean changed, int l, int t, int r, int b) {
//		if (mTracking) {
//			return;
//		}
//		log.debug("onLayout");
//		final int width = r - l;
//		final int height = b - t;
//		final View handle = mHandle;
//		int handleWidth = handle.getMeasuredWidth();
//		int handleHeight = handle.getMeasuredHeight();
//		int handleLeft;
//		int handleTop;
//		final View content = mContent;
//		handleLeft = (width - handleWidth);
//		handleTop = mExpanded ? mContent.getMeasuredHeight() - mBottomOffset
//				: mTopOffset;
//		content.layout(0, mTopOffset, content.getMeasuredWidth(), mTopOffset
//				+ content.getMeasuredHeight());
//		log.debug("handleTop=" + handleTop);
//		handle.layout(handleLeft, handleTop, handleLeft + handleWidth,
//				handleTop + handleHeight);
//		mHandleHeight = handle.getHeight();
//	}
//
//	@Override
//	public boolean onInterceptTouchEvent(MotionEvent event) {
//		log.debug("onInterceptTouchEvent start");
//
//		if (mLocked) {
//			return false;
//		}
//		requestFocus();
//		final int action = event.getAction();
//
//		float x = event.getX();
//		float y = event.getY();
//
//		final Rect frame = mFrame;
//		final View handle = mHandle;
//
//		handle.getHitRect(frame);
//		if (!mTracking && !frame.contains((int) x, (int) y)) {
//			log.debug("onInterceptTouchEvent mTracking=" + mTracking);
//			return false;
//		}
//
//		if (action == MotionEvent.ACTION_DOWN) {
//			mTracking = true;
//			handle.setPressed(true);
//			// Must be called before prepareTracking()
//			prepareContent();
//			// Must be called after prepareContent()
//			if (mOnDrawerScrollListener != null) {
//				mOnDrawerScrollListener.onScrollStarted();
//			}
//			final int top = mHandle.getTop();
//			mTouchDelta = (int) y - top;
//			prepareTracking(top);
//
//			if (mVelocityTracker != null) {
//				mVelocityTracker.recycle();
//				mVelocityTracker = null;
//			}
//			mVelocityTracker = VelocityTracker.obtain();
//			mVelocityTracker.addMovement(event);
//		}
//		log.debug("onInterceptTouchEvent return true");
//		return true;
//	}
//
//	@Override
//	public boolean onTouchEvent(MotionEvent event) {
//
//		if (mLocked) {
//			return true;
//		}
//		float x = event.getX();
//		float y = event.getY();
//		if (mExpanded) {
//			final Rect frame = new Rect();
//			final Rect contentR = new Rect();
//
//			mHandle.getHitRect(frame);
//			mContent.getHitRect(contentR);
//
//			if (!contentR.contains((int) x, (int) y)
//					&& !frame.contains((int) x, (int) y)) {
//				log.debug("onTouchEvent outside");
//				animateClose();
//				return false;
//			}
//		}
//		if (mTracking) {
//			mVelocityTracker.addMovement(event);
//			final int action = event.getAction();
//			switch (action) {
//			case MotionEvent.ACTION_MOVE:
//				moveHandle((int) (event.getY()) - mTouchDelta);
//				break;
//			case MotionEvent.ACTION_UP:
//			case MotionEvent.ACTION_CANCEL: {
//				final VelocityTracker velocityTracker = mVelocityTracker;
//				velocityTracker.computeCurrentVelocity(mVelocityUnits);
//
//				float yVelocity = velocityTracker.getYVelocity();
//				float xVelocity = velocityTracker.getXVelocity();
//				boolean negative;
//				log.debug("onTouchEvent start=" + event.getAction()
//						+ ",yVelocity=" + yVelocity + ",xVelocity=" + xVelocity);
//				negative = yVelocity < 0;
//				if (xVelocity < 0) {
//					xVelocity = -xVelocity;
//				}
//
//				if (xVelocity < mMaximumMinorVelocity) {
//					xVelocity = mMaximumMinorVelocity;
//				}
//
//				float velocity = (float) Math.hypot(xVelocity, yVelocity);// 滑动的直线距离
//				if (negative) {
//					velocity = -velocity;
//				}
//
//				final int handleTop = mHandle.getTop();
//				final int handleBottom = mHandle.getBottom();
//
//				if (Math.abs(velocity) < mMaximumTapVelocity) {
//					boolean c1;
//					boolean c2;
//
//					c1 = (mExpanded && (mContent.getHeight()+mHandleHeight - handleBottom) < mTapThreshold + mBottomOffset);
//					c2 = (!mExpanded && handleTop < mTopOffset + mHandleHeight - mTapThreshold);
//
//					log.debug("ACTION_UP: " + "c1: " + c1 + ", c2: " + c2
//							+ ",mAllowSingleTa=" + mAllowSingleTap
//							+ ",mExpanded=" + mExpanded);
//
//					if (c1 || c2) {
//						if (mAllowSingleTap) {
//							playSoundEffect(SoundEffectConstants.CLICK);
//							if (mExpanded) {
//								animateClose(handleTop);
//							} else {
//								animateOpen(handleTop);
//							}
//						} else {
//							performFling(handleTop, velocity, false);
//						}
//					} else {
//						performFling(handleTop, velocity, false);
//					}
//				} else {
//					performFling(handleTop, velocity, false);
//				}
//			}
//				break;
//			}
//		}
//		boolean ontouch = super.onTouchEvent(event);
//		log.debug("onTouchEvent mTracking=" + mTracking + ",mAnimating="
//				+ mAnimating + ",=ontouch" + ontouch);
//		return mTracking || mAnimating || ontouch;
//	}
//
//	private void animateClose(int position) {
//		prepareTracking(position);
//		performFling(position, mMaximumAcceleration, true);
//	}
//
//	private void animateOpen(int position) {
//		prepareTracking(position);
//		performFling(position, -mMaximumAcceleration, true);
//	}
//
//	private void performFling(int position, float velocity, boolean always) {
//		mAnimationPosition = position;
//		mAnimatedVelocity = velocity;
//		log.debug("position: " + position + ", velocity: " + velocity
//				+ ", mMaximumMajorVelocity: " + mMaximumMajorVelocity
//				+ ",always=" + always + ",mExpanded=" + mExpanded);
//		boolean c1;
//		boolean c2;
//		boolean c3;
//		if (mExpanded) {
//			int bottom = mContent.getHeight() + mHandleHeight;
//			c1 = velocity < mMaximumMajorVelocity;
//			c2 = (bottom - (position + mHandleHeight)) + mBottomOffset > mHandleHeight;
//			c3 = velocity < -mMaximumMajorVelocity;
//			if (always || (c1 || (c2 && c3))) {
//				// We are expanded, So animate to CLOSE!
//				mAnimatedAcceleration = mMaximumAcceleration;
//				if (velocity > 0) {
//					mAnimatedVelocity = 0;
//				}
//			} else {
//				// We are expanded, but they didn't move sufficiently to cause
//				// us to retract. Animate back to the expanded position. so
//				// animate BACK to expanded!
//				mAnimatedAcceleration = -mMaximumAcceleration;
//				if (velocity < 0) {
//					mAnimatedVelocity = 0;
//				}
//			}
//		} else {
//			// WE'RE COLLAPSED
//			c1 = velocity < mMaximumMajorVelocity;
//			c2 = position < (mContent.getHeight()) / 2;
//			c3 = velocity < -mMaximumMajorVelocity;
//			log.debug("c1=" + c1 + ",c2=" + c2 + ",c3=" + c3);
//			if (!always && (c1 || (c2 && c3))) {
//				mAnimatedAcceleration = mMaximumAcceleration;
//				if (velocity > 0) {
//					mAnimatedVelocity = 0;
//				}
//			} else {
//				mAnimatedAcceleration = -mMaximumAcceleration;
//				if (velocity < 0) {
//					mAnimatedVelocity = 0;
//				}
//			}
//		}
//		long now = SystemClock.uptimeMillis();
//		mAnimationLastTime = now;
//		mCurrentAnimationTime = now + ANIMATION_FRAME_DURATION;
//		mAnimating = true;
//		mHandler.removeMessages(MSG_ANIMATE);
//		mHandler.sendMessageAtTime(mHandler.obtainMessage(MSG_ANIMATE),
//				mCurrentAnimationTime);
//		// stopTracking();
//	}
//
//	private void prepareTracking(int position) {
//		mTracking = true;
//
//		boolean opening = !mExpanded;
//
//		if (opening) {
//			mAnimatedAcceleration = mMaximumAcceleration;
//			mAnimatedVelocity = mMaximumMajorVelocity;
//			mAnimationPosition = mTopOffset;
//			moveHandle((int) mAnimationPosition);
//			mAnimating = true;
//			mHandler.removeMessages(MSG_ANIMATE);
//			long now = SystemClock.uptimeMillis();
//			mAnimationLastTime = now;
//			mCurrentAnimationTime = now + ANIMATION_FRAME_DURATION;
//			mAnimating = true;
//		} else {
//			if (mAnimating) {
//				mAnimating = false;
//				mHandler.removeMessages(MSG_ANIMATE);
//			}
//			moveHandle(position);
//		}
//	}
//
//	private void moveHandle(int position) {
//		final View handle = mHandle;
//		log.debug("moveHandle=" + position);
//
//		if (position == -10002 && scrollingEndListener != null) {// 璇佹槑婊戝姩鍒板簳閮ㄤ簡
//			scrollingEndListener.onFinish();
//		}
//
//		if (position == EXPANDED_FULL_OPEN) {
//			log.debug("mBottomOffset=" + mBottomOffset + ",getBottom="
//					+ getBottom() + ",getTop=" + getTop() + ",mHandleHeight="
//					+ mHandleHeight);
//			handle.offsetTopAndBottom(mBottomOffset + handle.getBottom()
//					- handle.getTop() - mHandleHeight);
//			requestLayout();
//			invalidate();
//		} else if (position == COLLAPSED_FULL_CLOSED) {
//			handle.offsetTopAndBottom(mTopOffset - handle.getTop());
//			invalidate();
//		} else {
//			final int top = handle.getTop();
//			int deltaY = position - top;
//			if (position < mTopOffset) {
//				deltaY = mTopOffset - top;
//			} else if (deltaY > mBottomOffset + mContent.getMeasuredHeight()
//					- top) {
//				deltaY = mBottomOffset + mContent.getMeasuredHeight() - top;
//			}
//			handle.offsetTopAndBottom(deltaY);
//			log.debug("deltaY=" + deltaY + ",top=" + handle.getTop());
//
//			requestLayout();
//			invalidate();
//		}
//
//	}
//
//	private void prepareContent() {
//		if (mAnimating) {
//			return;
//		}
//
//		// Something changed in the content, we need to honor the layout request
//		// before creating the cached bitmap
//		final View content = mContent;
//		// if ( content.isLayoutRequested() ) {
//
//		final int handleHeight = mHandleHeight;
//		int height = getBottom() - getTop() - handleHeight - mTopOffset;
//		content.measure(MeasureSpec.makeMeasureSpec(getRight() - getLeft(),
//				MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(height,
//				MeasureSpec.EXACTLY));
//
//		content.layout(0, mTopOffset, content.getMeasuredWidth(), mTopOffset
//				+ content.getMeasuredHeight());
//
//		// }
//		// Try only once... we should really loop but it's not a big deal
//		// if the draw was cancelled, it will only be temporary anyway
//		content.getViewTreeObserver().dispatchOnPreDraw();
//		content.buildDrawingCache();
//
//		// content.setVisibility(View.GONE);
//	}
//
//	private void stopTracking() {
//		mHandle.setPressed(false);
//		mTracking = false;
//
//		if (mOnDrawerScrollListener != null) {
//			mOnDrawerScrollListener.onScrollEnded();
//		}
//
//		if (mVelocityTracker != null) {
//			mVelocityTracker.recycle();
//			mVelocityTracker = null;
//		}
//	}
//
//	private void doAnimation() {
//		log.debug("doAnimation=" + mAnimationPosition);
//		if (mAnimating) {
//			long now = SystemClock.uptimeMillis();
//			float t = (now - mAnimationLastTime) / 1000.0f; // ms -> s
//			final float position = mAnimationPosition;
//			final float v = mAnimatedVelocity; // px/s
//			final float a = mAnimatedAcceleration; // px/s/s
//			log.debug("mAnimationPosition v=" + v + ",t=" + t);
//
//			mAnimationPosition = position + (v * t) + (0.5f * a * t * t); // px
//			mAnimatedVelocity = v + (a * t); // px/s
//			mAnimationLastTime = now; // ms
//			int botton = mTopOffset + mContent.getMeasuredHeight() - 1;
//			log.debug("mAnimationPosition=" + mAnimationPosition
//					+ ",mTopOffset=" + mTopOffset + ",botton=" + botton);
//			if (mAnimationPosition < mTopOffset) {
//				mAnimating = false;
//				closeDrawer();
//				stopTracking();
//			} else if (mAnimationPosition >= mTopOffset
//					+ mContent.getMeasuredHeight() - 1) {
//				mAnimating = false;
//				openDrawer();
//				stopTracking();
//			} else {
//				moveHandle((int) mAnimationPosition);
//				mCurrentAnimationTime += ANIMATION_FRAME_DURATION;
//				mHandler.sendMessageAtTime(mHandler.obtainMessage(MSG_ANIMATE),
//						mCurrentAnimationTime);
//			}
//
//		}
//	}
//
//	/**
//	 * Toggles the drawer open and close. Takes effect immediately.
//	 * 
//	 * @see #open()
//	 * @see #close()
//	 * @see #animateClose()
//	 * @see #animateOpen()
//	 * @see #animateToggle()
//	 */
//	public void toggle() {
//		if (!mExpanded) {
//			openDrawer();
//		} else {
//			closeDrawer();
//		}
//		invalidateData();
//	}
//
//	public void enSureOpen() {
//		if (!mExpanded) {
//			openDrawer();
//		}
//		prepareContent();
//		invalidateData();
//	}
//
//	private void invalidateData() {
//		requestLayout();
//		invalidate();
//	}
//
//	/**
//	 * Toggles the drawer open and close with an animation.
//	 * 
//	 * @see #open()
//	 * @see #close()
//	 * @see #animateClose()
//	 * @see #animateOpen()
//	 * @see #toggle()
//	 */
//	public void animateToggle() {
//		if (!mExpanded) {
//			animateOpen();
//		} else {
//			animateClose();
//		}
//	}
//
//	/**
//	 * Opens the drawer immediately.
//	 * 
//	 * @see #toggle()
//	 * @see #close()
//	 * @see #animateOpen()
//	 */
//	public void open() {
//		openDrawer();
//		invalidate();
//		requestLayout();
//
//		sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED);
//	}
//
//	/**
//	 * Closes the drawer immediately.
//	 * 
//	 * @see #toggle()
//	 * @see #open()
//	 * @see #animateClose()
//	 */
//	public void close() {
//		closeDrawer();
//		invalidate();
//		requestLayout();
//	}
//
//	/**
//	 * Closes the drawer with an animation.
//	 * 
//	 * @see #close()
//	 * @see #open()
//	 * @see #animateOpen()
//	 * @see #animateToggle()
//	 * @see #toggle()
//	 */
//	public void animateClose() {
//		prepareContent();
//		final OnDrawerScrollListener scrollListener = mOnDrawerScrollListener;
//		if (scrollListener != null) {
//			scrollListener.onScrollStarted();
//		}
//		animateClose(mHandle.getTop());
//
//		if (scrollListener != null) {
//			scrollListener.onScrollEnded();
//		}
//	}
//
//	/**
//	 * Opens the drawer with an animation.
//	 * 
//	 * @see #close()
//	 * @see #open()
//	 * @see #animateClose()
//	 * @see #animateToggle()
//	 * @see #toggle()
//	 */
//	public void animateOpen() {
//		prepareContent();
//		final OnDrawerScrollListener scrollListener = mOnDrawerScrollListener;
//		if (scrollListener != null) {
//			scrollListener.onScrollStarted();
//		}
//		animateOpen(mHandle.getTop());
//
//		sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED);
//
//		if (scrollListener != null) {
//			scrollListener.onScrollEnded();
//		}
//	}
//
//	private void closeDrawer() {
//		log.debug("closeDrawer");
//		moveHandle(COLLAPSED_FULL_CLOSED);
//		mContent.setVisibility(View.GONE);
//		mContent.destroyDrawingCache();
//		if (!mExpanded) {
//			return;
//		}
//		mExpanded = false;
//		if (mOnDrawerCloseListener != null) {
//			mOnDrawerCloseListener.onDrawerClosed();
//		}
//	}
//
//	private void openDrawer() {
//		log.debug("openDrawer");
//		moveHandle(EXPANDED_FULL_OPEN);
//		mContent.destroyDrawingCache();
//		mContent.setVisibility(View.VISIBLE);
//		if (mExpanded) {
//			return;
//		}
//		mExpanded = true;
//
//		if (mOnDrawerOpenListener != null) {
//			mOnDrawerOpenListener.onDrawerOpened();
//		}
//	}
//
//	/**
//	 * Sets the listener that receives a notification when the drawer becomes
//	 * open.
//	 * 
//	 * @param onDrawerOpenListener
//	 *            The listener to be notified when the drawer is opened.
//	 */
//	public void setOnDrawerOpenListener(
//			OnDrawerOpenListener onDrawerOpenListener) {
//		mOnDrawerOpenListener = onDrawerOpenListener;
//	}
//
//	/**
//	 * Sets the listener that receives a notification when the drawer becomes
//	 * close.
//	 * 
//	 * @param onDrawerCloseListener
//	 *            The listener to be notified when the drawer is closed.
//	 */
//	public void setOnDrawerCloseListener(
//			OnDrawerCloseListener onDrawerCloseListener) {
//		mOnDrawerCloseListener = onDrawerCloseListener;
//	}
//
//	/**
//	 * Sets the listener that receives a notification when the drawer starts or
//	 * ends a scroll. A fling is considered as a scroll. A fling will also
//	 * trigger a drawer opened or drawer closed event.
//	 * 
//	 * @param onDrawerScrollListener
//	 *            The listener to be notified when scrolling starts or stops.
//	 */
//	public void setOnDrawerScrollListener(
//			OnDrawerScrollListener onDrawerScrollListener) {
//		mOnDrawerScrollListener = onDrawerScrollListener;
//	}
//
//	/**
//	 * Returns the handle of the drawer.
//	 * 
//	 * @return The View reprenseting the handle of the drawer, identified by the
//	 *         "handle" id in XML.
//	 */
//	public View getHandle() {
//		return mHandle;
//	}
//
//	/**
//	 * Returns the content of the drawer.
//	 * 
//	 * @return The View reprenseting the content of the drawer, identified by
//	 *         the "content" id in XML.
//	 */
//	public View getContent() {
//		return mContent;
//	}
//
//	/**
//	 * Unlocks the SlidingDrawer so that touch events are processed.
//	 * 
//	 * @see #lock()
//	 */
//	public void unlock() {
//		mLocked = false;
//	}
//
//	/**
//	 * Locks the SlidingDrawer so that touch events are ignores.
//	 * 
//	 * @see #unlock()
//	 */
//	public void lock() {
//		mLocked = true;
//	}
//
//	/**
//	 * Indicates whether the drawer is currently fully opened.
//	 * 
//	 * @return True if the drawer is opened, false otherwise.
//	 */
//	public boolean isOpened() {
//		return mExpanded;
//	}
//
//	/**
//	 * Indicates whether the drawer is scrolling or flinging.
//	 * 
//	 * @return True if the drawer is scroller or flinging, false otherwise.
//	 */
//	public boolean isMoving() {
//		return mTracking || mAnimating;
//	}
//
//	public class DrawerToggler implements OnClickListener {
//
//		public void onClick(View v) {
//			if (mLocked) {
//				return;
//			}
//			// mAllowSingleTap isn't relevant here; you're *always*
//			// allowed to open/close the drawer by clicking with the
//			// trackball.
//			log.debug("onClick=" + mAnimateOnClick);
//			if (mAnimateOnClick) {
//				animateToggle();
//			} else {
//				toggle();
//			}
//		}
//	}
//
//	private class SlidingHandler extends Handler {
//
//		public void handleMessage(Message m) {
//			switch (m.what) {
//			case MSG_ANIMATE:
//				doAnimation();
//				break;
//			}
//		}
//	}
//}
