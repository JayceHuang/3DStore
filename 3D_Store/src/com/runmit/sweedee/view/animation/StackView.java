///* Copyright (C) 2010 The Android Open Source Project
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
//import java.lang.ref.WeakReference;
//
//import android.animation.Animator;
//import android.animation.Animator.AnimatorListener;
//import android.animation.ObjectAnimator;
//import android.animation.PropertyValuesHolder;
//import android.animation.ValueAnimator;
//import android.animation.ValueAnimator.AnimatorUpdateListener;
//import android.content.Context;
//import android.graphics.Rect;
//import android.util.AttributeSet;
//import android.util.Log;
//import android.view.MotionEvent;
//import android.view.VelocityTracker;
//import android.view.View;
//import android.view.ViewConfiguration;
//import android.view.ViewParent;
//import android.view.animation.LinearInterpolator;
//import android.view.animation.OvershootInterpolator;
//import android.widget.Adapter;
//import android.widget.FrameLayout;
//
///**
// * A view that displays its children in a stack and allows users to discretely swipe
// * through the children.
// */
//public class StackView extends AdapterViewAnimator {
//    
//	private final String TAG = "StackView";
//
//    /**
//     * Default animation parameters
//     */
//    private static final int DEFAULT_ANIMATION_DURATION = 400;
//    
//    private static final int STACK_RELAYOUT_DURATION = 200;
//
//    /**
//     * Parameters effecting the perspective visuals
//     */
//    private static final float PERSPECTIVE_SHIFT_FACTOR_Y = 0.3f;
//
//    private float mPerspectiveShiftY;
//    
//    private float mNewPerspectiveShiftY;
//
//    private static final float PERSPECTIVE_SCALE_FACTOR = 0.3f;
//
//    /**
//     * These specify the different gesture states
//     */
//    private static final int GESTURE_NONE = 0;
//    private static final int GESTURE_SLIDE_LEFT = 1;
//    private static final int GESTURE_SLIDE_RIGHT = 2;
//
//    /**
//     * Sentinel value for no current active pointer.
//     * Used by {@link #mActivePointerId}.
//     */
//    private static final int INVALID_POINTER = -1;
//
//    private static final int FRAME_PADDING = 0;
//
//    private final Rect mTouchRect = new Rect();
//
//    /**
//     * These variables are all related to the current state of touch interaction
//     * with the stack
//     */
//    private float mInitialY;
//    
//    private float mInitialX;
//    
//    private int mActivePointerId;
//
//    private int mSwipeGestureType = GESTURE_NONE;
//    
//    private int mTouchSlop;
//    
//    private int mMaximumVelocity;
//    
//    private int mMinimumVelocity;
//    
//    private VelocityTracker mVelocityTracker;
//
//    private boolean mFirstLayoutHappened = false;
//    
//    private int mFramePadding;
//
//    private View mSwipView;
//    
//    private boolean isBeginTranslation = false;
//    
//    private int mCameraDistance;
//    
//    /**
//     * {@inheritDoc}
//     */
//    public StackView(Context context) {
//        this(context, null);
//    }
//
//    /**
//     * {@inheritDoc}
//     */
//    public StackView(Context context, AttributeSet attrs) {
//        this(context, attrs, 0);
//    }
//
//    /**
//     * {@inheritDoc}
//     */
//    public StackView(Context context, AttributeSet attrs, int defStyleAttr) {
//        super(context, attrs, defStyleAttr);
//        initStackView();
//    }
//
//    private void initStackView() {
//        setStaticTransformationsEnabled(true);
//        final ViewConfiguration configuration = ViewConfiguration.get(getContext());
//        mTouchSlop = configuration.getScaledTouchSlop();
//        mMaximumVelocity = configuration.getScaledMaximumFlingVelocity();
//        mMinimumVelocity = configuration.getScaledMinimumFlingVelocity();
//        
//        mActivePointerId = INVALID_POINTER;
//
//        setClipChildren(false);
//        setClipToPadding(false);
//
//        // This is a flag to indicate the the stack is loading for the first time
//        mWhichChild = -1;
//
//        // Adjust the frame padding based on the density, since the highlight changes based
//        // on the density
//        final float density = getContext().getResources().getDisplayMetrics().density;
//        mFramePadding = (int) Math.ceil(density * FRAME_PADDING);
//        
//        mCameraDistance = getContext().getResources().getDisplayMetrics().heightPixels * 5;
//    }
//
//    /**
//     * Animate the views between different relative indexes within the {@link AdapterViewAnimator}
//     */
//    void transformViewForTransition(int fromIndex, int toIndex, final View view, boolean animate) {
//        if (!animate) {
//            ((StackFrame) view).cancelSliderAnimator();
//        }
//        Log.d(TAG, "fromIndex="+fromIndex+",toIndex="+toIndex);
//        if (fromIndex == -1 && toIndex == getNumActiveViews() -1) {
//            transformViewAtIndex(toIndex, view, false);
//        } else if (fromIndex == 1 && toIndex == 0) {
//            // Slide item out
//            ((StackFrame) view).cancelSliderAnimator();
//        } else if (toIndex == 0) {
//            // Make sure this view that is "waiting in the wings" is invisible
//            view.setVisibility(INVISIBLE);
//        }
//        // Implement the faked perspective
//        if (toIndex != -1) {
//            transformViewAtIndex(toIndex, view, animate);
//        }
//    }
//
//    private void transformViewAtIndex(int index, final View view, boolean animate) {
//        final float maxPerspectiveShiftY = mPerspectiveShiftY;
//        Log.d(TAG, "transformViewAtIndex="+index+",mMaxNumActiveViews="+mMaxNumActiveViews+",animate="+animate);
//        index = mMaxNumActiveViews - index - 1;
//        if (index == mMaxNumActiveViews - 1) {
//        	index--;
//        }
//        Log.d(TAG, "transformViewAtIndex="+index);
//        float r = (index * 1.0f) / (mMaxNumActiveViews - 2);
//        final float scale = 1 - PERSPECTIVE_SCALE_FACTOR * (1 - r);
//        float perspectiveTranslationY = r * maxPerspectiveShiftY;
//        float scaleShiftCorrectionY = (scale - 1) * (getMeasuredHeight() * (1 - PERSPECTIVE_SHIFT_FACTOR_Y) / 2.0f);
//        final float transY = perspectiveTranslationY + scaleShiftCorrectionY;
//
//        // If this view is currently being animated for a certain position, we need to cancel
//        // this animation so as not to interfere with the new transformation.
//        if (view instanceof StackFrame) {
//            ((StackFrame) view).cancelTransformAnimator();
//        }
//        Log.d(TAG, "transY="+transY+",scale="+scale+",index="+index+",animate="+animate);
//        if (animate) {
//        	if(index == 0){//最后一个
//        		PropertyValuesHolder translationY = PropertyValuesHolder.ofFloat("x", getWidth(),0);
//                ObjectAnimator oa = ObjectAnimator.ofPropertyValuesHolder(view,translationY);
//                oa.setDuration(STACK_RELAYOUT_DURATION);
//                if (view instanceof StackFrame) {
//                    ((StackFrame) view).setTransformAnimator(oa);
//                }
//                oa.start();
//        	}else{
//        		PropertyValuesHolder translationY = PropertyValuesHolder.ofFloat("translationY", transY);
//                PropertyValuesHolder scalePropX = PropertyValuesHolder.ofFloat("scaleX", scale);
//                PropertyValuesHolder scalePropY = PropertyValuesHolder.ofFloat("scaleY", scale);
//
//                ObjectAnimator oa = ObjectAnimator.ofPropertyValuesHolder(view, scalePropX, scalePropY,translationY);
//                oa.setDuration(STACK_RELAYOUT_DURATION);
//                oa.setInterpolator(new OvershootInterpolator(6f));
//                if (view instanceof StackFrame) {
//                    ((StackFrame) view).setTransformAnimator(oa);
//                }
//                oa.start();
//        	}
//        } else {
//            view.setTranslationY(transY);
//            view.setScaleX(scale);
//            view.setScaleY(scale);
//        }
//    }
//
//    void showOnly(int childIndex, boolean animate) {
//        super.showOnly(childIndex, animate);
//
//        // Here we need to make sure that the z-order of the children is correct
//        for (int i = mCurrentWindowEnd; i >= mCurrentWindowStart; i--) {
//            int index = modulo(i, getWindowSize());
//            ViewAndMetaData vm = mViewsMap.get(index);
//            if (vm != null) {
//                View v = mViewsMap.get(index).view;
//                if (v != null) v.bringToFront();
//            }
//        }
//    }
//
//    private void updateChildTransforms() {
//        for (int i = 0; i < getNumActiveViews(); i++) {
//            View v = getViewAtRelativeIndex(i);
//            if (v != null) {
//                transformViewAtIndex(i, v, false);
//            }
//        }
//    }
//
//    private static class StackFrame extends FrameLayout {
//        WeakReference<ObjectAnimator> transformAnimator;
//        WeakReference<ObjectAnimator> sliderAnimator;
//
//        public StackFrame(Context context) {
//            super(context);
//        }
//
//        void setTransformAnimator(ObjectAnimator oa) {
//            transformAnimator = new WeakReference<ObjectAnimator>(oa);
//        }
//
//        boolean cancelTransformAnimator() {
//            if (transformAnimator != null) {
//                ObjectAnimator oa = transformAnimator.get();
//                if (oa != null) {
//                    oa.cancel();
//                    return true;
//                }
//            }
//            return false;
//        }
//
//        boolean cancelSliderAnimator() {
//            if (sliderAnimator != null) {
//                ObjectAnimator oa = sliderAnimator.get();
//                if (oa != null) {
//                    oa.cancel();
//                    return true;
//                }
//            }
//            return false;
//        }
//    }
//
//    @Override
//    FrameLayout getFrameForChild() {
//        StackFrame fl = new StackFrame(getContext());
//        fl.setPadding(mFramePadding, mFramePadding, mFramePadding, mFramePadding);
//        return fl;
//    }
//
//    private void onLayout() {
//        if (!mFirstLayoutHappened) {
//            mFirstLayoutHappened = true;
//            updateChildTransforms();
//        }
//        if (Float.compare(mPerspectiveShiftY, mNewPerspectiveShiftY) != 0) {
//            mPerspectiveShiftY = mNewPerspectiveShiftY;
//            updateChildTransforms();
//        }
//    }
//
//    /**
//     * {@inheritDoc}
//     */
//    @Override
//    public boolean onInterceptTouchEvent(MotionEvent ev) {
//        int action = ev.getAction();
//        switch(action & MotionEvent.ACTION_MASK) {
//            case MotionEvent.ACTION_DOWN: {
//                if (mActivePointerId == INVALID_POINTER) {
//                    mInitialX = ev.getX();
//                    mInitialY = ev.getY();
//                    mActivePointerId = ev.getPointerId(0);
//                }
//                break;
//            }
//            case MotionEvent.ACTION_MOVE: {
//                int pointerIndex = ev.findPointerIndex(mActivePointerId);
//                if (pointerIndex == INVALID_POINTER) {
//                    Log.d(TAG, "Error: No data for our primary pointer.");
//                    return false;
//                }
//                float newY = ev.getY(pointerIndex);
//                float newX = ev.getX(pointerIndex);
//                
//                float deltaY = newY - mInitialY;
//                float deltaX = newX - mInitialX;
//                if(deltaY > deltaX && (int) Math.abs(deltaY) > mTouchSlop){
//                	isBeginTranslation = true;
//                	ViewParent parent = getParent();
//                    if (parent != null) {
//                        parent.requestDisallowInterceptTouchEvent(true);
//                    }
//                    return true;
//                }
//                break;
//            }
//            case MotionEvent.ACTION_POINTER_UP: {
//                onSecondaryPointerUp(ev);
//                break;
//            }
//            case MotionEvent.ACTION_UP:
//            case MotionEvent.ACTION_CANCEL: {
//                mActivePointerId = INVALID_POINTER;
//                mSwipeGestureType = GESTURE_NONE;
//            }
//        }
//
//        return mSwipeGestureType != GESTURE_NONE;
//    }
//
//    private void beginGestureIfNeeded(float deltaY) {
//    	Log.d(TAG, "beginGestureIfNeeded deltaX="+deltaY +",mTouchSlop="+mTouchSlop);
//        if ((int) Math.abs(deltaY) > mTouchSlop) {
//            final int swipeGestureType = deltaY < 0 ? GESTURE_SLIDE_LEFT : GESTURE_SLIDE_RIGHT;
//            cancelLongPress();
//            requestDisallowInterceptTouchEvent(true);
//            if (mAdapter == null) {
//            	return;
//            }
//            mSwipView = getCurrentView();
//            if (mSwipView == null) {
//            	return;
//            }
//            Log.d(TAG, "mSwipView deltaX="+mSwipView.getPaddingBottom()+",paddingTop="+mSwipView.getPaddingTop());
//            mSwipView.setCameraDistance(mCameraDistance);
//            float rotation = -deltaY/mSwipView.getHeight(); 
//            mSwipView.setPivotX(mSwipView.getWidth()/2);
//            mSwipView.setPivotY(mSwipView.getHeight() - mSwipView.getPaddingBottom() - mSwipView.getPaddingTop());
//            mSwipView.setRotationX(rotation*90);
//            mSwipeGestureType = swipeGestureType;
//            cancelHandleClick();
//        }
//    }
//
//    @Override
//    public void setAdapter(Adapter adapter) {
//    	configureViewAnimator(adapter.getCount() + 1, 1);
//    	super.setAdapter(adapter);
//    }
//    
//    @Override
//    public boolean onTouchEvent(MotionEvent ev) {
//        int action = ev.getAction();
//        int pointerIndex = ev.findPointerIndex(mActivePointerId);
//        if (pointerIndex == INVALID_POINTER) {
//            Log.d(TAG, "Error: No data for our primary pointer.");
//            return false;
//        }
//        if (mVelocityTracker == null) {
//            mVelocityTracker = VelocityTracker.obtain();
//        }
//        mVelocityTracker.addMovement(ev);
//
//        switch (action & MotionEvent.ACTION_MASK) {
//            case MotionEvent.ACTION_MOVE: {
//            	float newY = ev.getY(pointerIndex);
//            	float deltaY = newY - mInitialY;
//            	if(isBeginTranslation){
//            		beginGestureIfNeeded(deltaY);
//            		return true;
//            	}
//                break;
//            }
//            case MotionEvent.ACTION_UP: {
//                handlePointerUp(ev);
//                break;
//            }
//            case MotionEvent.ACTION_POINTER_UP: {
//                onSecondaryPointerUp(ev);
//                isBeginTranslation = false;
//                break;
//            }
//            case MotionEvent.ACTION_CANCEL: {
//            	isBeginTranslation = false;
//                mActivePointerId = INVALID_POINTER;
//                mSwipeGestureType = GESTURE_NONE;
//                break;
//            }
//        }
//        return super.onTouchEvent(ev);
//    }
//
//    private void onSecondaryPointerUp(MotionEvent ev) {
//        final int activePointerIndex = ev.getActionIndex();
//        final int pointerId = ev.getPointerId(activePointerIndex);
//        if (pointerId == mActivePointerId) {
//
//            int activeViewIndex = (mSwipeGestureType == GESTURE_SLIDE_RIGHT) ? 0 : 1;
//
//            View v = getViewAtRelativeIndex(activeViewIndex);
//            if (v == null) return;
//
//            // Our primary pointer has gone up -- let's see if we can find
//            // another pointer on the view. If so, then we should replace
//            // our primary pointer with this new pointer and adjust things
//            // so that the view doesn't jump
//            for (int index = 0; index < ev.getPointerCount(); index++) {
//                if (index != activePointerIndex) {
//
//                    float x = ev.getX(index);
//                    float y = ev.getY(index);
//
//                    mTouchRect.set(v.getLeft(), v.getTop(), v.getRight(), v.getBottom());
//                    if (mTouchRect.contains(Math.round(x), Math.round(y))) {
//                        float oldX = ev.getX(activePointerIndex);
//                        float oldY = ev.getY(activePointerIndex);
//
//                        // adjust our frame of reference to avoid a jump
//                        mInitialY += (y - oldY);
//                        mInitialX += (x - oldX);
//
//                        mActivePointerId = ev.getPointerId(index);
//                        if (mVelocityTracker != null) {
//                            mVelocityTracker.clear();
//                        }
//                        // ok, we're good, we found a new pointer which is touching the active view
//                        return;
//                    }
//                }
//            }
//            // if we made it this far, it means we didn't find a satisfactory new pointer :(,
//            // so end the gesture
//            handlePointerUp(ev);
//        }
//    }
//
//    private void handlePointerUp(MotionEvent ev) {
//        System.currentTimeMillis();
//        int mYVelocity = 0;
//        if (mVelocityTracker != null) {
//            mVelocityTracker.computeCurrentVelocity(1000, mMaximumVelocity);
//            mYVelocity = (int) mVelocityTracker.getYVelocity(mActivePointerId);
//        }
//        if (mVelocityTracker != null) {
//            mVelocityTracker.recycle();
//            mVelocityTracker = null;
//        }
//       
//        if(mSwipView != null){
//        	float mRotationX=mSwipView.getRotationX();
//            Log.d(TAG, "mSwipView="+mSwipView+",mRotationX="+mRotationX+",width="+getWidth());
//        	boolean needSwip = Math.abs(mYVelocity) > mMinimumVelocity || (mRotationX < 0 && mRotationX > -90);
//        	if(needSwip){//SWIP TO NEXT
//        		showNextWithAnimation(mRotationX > 45, mRotationX);
//        	}
//        }
//
//        mActivePointerId = INVALID_POINTER;
//        mSwipeGestureType = GESTURE_NONE;
//    }
//    
//    public void swipRight(){
//    	mSwipView = getCurrentView();
//    	showNextWithAnimation(true,0);
//    }
//    
//    public void swipLeft(){
//    	mSwipView = getCurrentView();
//    	showNextWithAnimation(false,0);
//    }
//    
//    private void showNextWithAnimation(boolean isRight,float mTranslationX){
//    	if(mSwipView == null){
//    		return;
//    	}
//    	mSwipView.setPivotX(mSwipView.getWidth()/2);
//        mSwipView.setPivotY(mSwipView.getHeight());
//        
//        float endPosition = isRight ? 0 : -90;
//    	float starPosition = mSwipView.getRotationX();
//    	Log.d(TAG, "endPosition="+endPosition+",starPosition="+starPosition);
//		final ObjectAnimator pa = ObjectAnimator.ofFloat(mSwipView, "rotationX", starPosition , endPosition);
//        pa.setDuration(DEFAULT_ANIMATION_DURATION);
//        pa.setInterpolator(new LinearInterpolator());
//        pa.addListener(new AnimatorListener() {
//			
//			@Override
//			public void onAnimationStart(Animator animation) {
//			}
//			
//			@Override
//			public void onAnimationRepeat(Animator animation) {
//			}
//			
//			@Override
//			public void onAnimationEnd(Animator animation) {
//				showNext();
//			}
//			
//			@Override
//			public void onAnimationCancel(Animator animation) {
//			}
//		});
//        pa.addUpdateListener(new AnimatorUpdateListener() {
//			
//			@Override
//			public void onAnimationUpdate(ValueAnimator animation) {
//				Float value = Math.abs((Float) animation.getAnimatedValue());
//				float alpha = 1-value/180;
//				Log.d(TAG, "alpha="+alpha);
//	            mSwipView.setAlpha(alpha);
//			}
//		});
//        pa.start();
//    }
//    
//    @Override
//    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
//        checkForAndHandleDataChanged();
//
//        final int childCount = getChildCount();
//        for (int i = 0; i < childCount; i++) {
//            final View child = getChildAt(i);
//            int childRight = getPaddingLeft()  + child.getMeasuredWidth();
//            int childBottom = getPaddingTop() + child.getMeasuredHeight();
//            child.layout(getPaddingLeft(), getPaddingTop(),childRight, childBottom);
//        }
//        onLayout();
//    }
//
//    private void measureChildren() {
//        final int count = getChildCount();
//
//        final int measuredWidth = getMeasuredWidth();
//        final int measuredHeight = getMeasuredHeight();
//
//        final int childWidth = measuredWidth - getPaddingLeft()- getPaddingRight();
//        final int childHeight = Math.round(measuredHeight*(1-PERSPECTIVE_SHIFT_FACTOR_Y)) - getPaddingTop() - getPaddingBottom();
//
//        for (int i = 0; i < count; i++) {
//            final View child = getChildAt(i);
//            child.measure(MeasureSpec.makeMeasureSpec(childWidth, MeasureSpec.AT_MOST),MeasureSpec.makeMeasureSpec(childHeight, MeasureSpec.AT_MOST));
//        }
//
//        mNewPerspectiveShiftY = PERSPECTIVE_SHIFT_FACTOR_Y * measuredHeight;
//    }
//
//    @Override
//    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
//        int widthSpecSize = MeasureSpec.getSize(widthMeasureSpec);
//        int heightSpecSize = MeasureSpec.getSize(heightMeasureSpec);
//        final int widthSpecMode = MeasureSpec.getMode(widthMeasureSpec);
//        final int heightSpecMode = MeasureSpec.getMode(heightMeasureSpec);
//
//        boolean haveChildRefSize = (mReferenceChildWidth != -1 && mReferenceChildHeight != -1);
//
//        // We need to deal with the case where our parent hasn't told us how
//        // big we should be. In this case we should
//        float factorY = 1/(1 - PERSPECTIVE_SHIFT_FACTOR_Y);
//        if (heightSpecMode == MeasureSpec.UNSPECIFIED) {
//            heightSpecSize = haveChildRefSize ? Math.round(mReferenceChildHeight * (1 + factorY)) + getPaddingTop() + getPaddingBottom() : 0;
//        } else if (heightSpecMode == MeasureSpec.AT_MOST) {
//            if (haveChildRefSize) {
//                int height = Math.round(mReferenceChildHeight * (1 + factorY)) + getPaddingTop() + getPaddingBottom();
//                if (height <= heightSpecSize) {
//                    heightSpecSize = height;
//                } else {
//                    heightSpecSize |= MEASURED_STATE_TOO_SMALL;
//                }
//            } else {
//                heightSpecSize = 0;
//            }
//        }
//
//        if (widthSpecMode == MeasureSpec.UNSPECIFIED) {
//            widthSpecSize = haveChildRefSize ?Math.round(mReferenceChildWidth * 2) +getPaddingLeft() + getPaddingRight() : 0;
//        } else if (heightSpecMode == MeasureSpec.AT_MOST) {
//            if (haveChildRefSize) {
//                int width = mReferenceChildWidth + getPaddingLeft() + getPaddingRight();
//                if (width <= widthSpecSize) {
//                    widthSpecSize = width;
//                } else {
//                    widthSpecSize |= MEASURED_STATE_TOO_SMALL;
//                }
//            } else {
//                widthSpecSize = 0;
//            }
//        }
//        setMeasuredDimension(widthSpecSize, heightSpecSize);
//        measureChildren();
//    }
//}
