package com.runmit.sweedee.view.animation;

import java.util.ArrayList;
import java.util.List;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.animation.LinearInterpolator;
import android.widget.Adapter;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.XL_Log;

public class MyStackView extends ViewGroup {

    private XL_Log log = new XL_Log(MyStackView.class);

    private int mCurPosition = 0;

    private Adapter mAdapter;

    private static final int DEFINE_FILL_SIZE = 6;
    /**
     * 缩放大小，给我们的宽高换算出来的，相对于原始大小的的缩放，比如第一个1.0，则不缩放
     */
    private static final float[] mScaleSize = new float[]{1.0f, 0.9125f, 0.7959f, 0.6501f, 0.5218f, 0.4052f};
    private static final float[] mYTranslations = new float[]{0.0f, 16.3f, 34.0f, 54.21f, 71.92f, 87.66f};// 由于设计师给的是dp所以我们到时换算需要乘以密度
    private static final float[] backgroundAlphas = new float[]{1.0f, 0.8f, 0.6f, 0.4f, 0.2f, 0.00f};// 背景框的透明度

    private static final float[] contentAlphas = new float[]{1.0f, 0.8f, 0.6f, 0.4f, 0.2f, 0.00f};

    private static final int DEFAULT_ANIMATION_DURATION = 1000;

    private FrameAnimatior mFrameAnimatior;

    private int mReferenceChildWidth = -1;
    private int mReferenceChildHeight = -1;

    private float mLastY;

    private float mInitX, mInitY;

    private int mActivePointerId;

    private static final int GESTURE_NONE = 0;

    private int mSwipeGestureType = GESTURE_NONE;

    private static final int INVALID_POINTER = -1;

    private boolean isBeginTranslation = false;

    private int mTouchSlop;

    private int mMaximumVelocity;

    private int mMinimumVelocity;

    private VelocityTracker mVelocityTracker;

    private int mCameraDistance;

    private View mSwipView;

    private List<ViewAndMetaData> mViewMaps = new ArrayList<ViewAndMetaData>();

    private float density;

    private ObjectAnimator mObjectAnimator;

    private final static int TOUCH_SCALE = 100;

    private int mDefineTouchScale;

    public MyStackView(Context context) {
        super(context);
        initStackView();
    }

    public MyStackView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initStackView();
    }

    private void initStackView() {
        setStaticTransformationsEnabled(true);
        setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        final ViewConfiguration configuration = ViewConfiguration.get(getContext());
        //获得能够进行手势滑动的距离
        mTouchSlop = configuration.getScaledTouchSlop();
        //获得允许执行一个fling手势动作的最大速度值
        mMaximumVelocity = configuration.getScaledMaximumFlingVelocity();
        //获得允许执行一个fling手势动作的最小速度值
        mMinimumVelocity = configuration.getScaledMinimumFlingVelocity();
        mActivePointerId = INVALID_POINTER;
        setClipChildren(false);
        setClipToPadding(false);
        density = getContext().getResources().getDisplayMetrics().density;

        mDefineTouchScale = (int) (TOUCH_SCALE * density);

        mCameraDistance = getContext().getResources().getDisplayMetrics().heightPixels * 16;


    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        final int childCount = getChildCount();
        for (int i = 0; i < childCount; i++) {
            final View child = getChildAt(i);
            int childRight = getPaddingLeft() + child.getMeasuredWidth();
            int childTop = getMeasuredHeight() - getPaddingBottom() - child.getMeasuredHeight();
            child.layout(getPaddingLeft(), childTop, childRight, childTop + child.getMeasuredHeight());
        }
        onLayout();
    }

    private void onLayout() {
        updateChildTransforms();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int widthSpecSize = MeasureSpec.getSize(widthMeasureSpec);
        int heightSpecSize = MeasureSpec.getSize(heightMeasureSpec);
        log.debug("widthSpecSize=" + widthSpecSize + ",heightSpecSize=" + heightSpecSize);
        setMeasuredDimension(widthSpecSize, heightSpecSize);
        measureChildren();
    }

    private void measureChildren() {
        final int count = getChildCount();
        final int measuredWidth = getMeasuredWidth();
        final int measuredHeight = getMeasuredHeight();

        final int childWidth = measuredWidth - getPaddingLeft() - getPaddingRight();
        final int childHeight = measuredHeight - getPaddingTop() - getPaddingBottom();

        for (int i = 0; i < count; i++) {
            final View child = getChildAt(i);
            child.measure(MeasureSpec.makeMeasureSpec(childWidth, MeasureSpec.AT_MOST), MeasureSpec.makeMeasureSpec(childHeight, MeasureSpec.UNSPECIFIED));
        }
    }

    private void updateChildTransforms() {
        for (int i = 0, size = mViewMaps.size(); i < size; i++) {
            transformViewAtIndex(i, 0, mViewMaps.get(i), false);
        }
    }

    private void transformViewAtIndex(int index, float offset, final ViewAndMetaData mViewAndMetaData, boolean isAnimation) {
        final float scale;
        final float transY;
        final float backgroundAlpha;
        final float contentAlpha;
        if (index == 0) {
            scale = 1.0f;
            transY = 0.0f;
            backgroundAlpha = 1.0f;
            contentAlpha = 1.0f;
        } else {
            scale = (mScaleSize[index - 1] - mScaleSize[index]) * offset + mScaleSize[index];// offset==0时候则为mScaleSize[index]，offset==1时候mScaleSize[index-1]
            transY = -density * ((mYTranslations[index - 1] - mYTranslations[index]) * offset + mYTranslations[index]);

            backgroundAlpha = (backgroundAlphas[index - 1] - backgroundAlphas[index]) * offset + backgroundAlphas[index];
            contentAlpha = (contentAlphas[index - 1] - contentAlphas[index]) * offset + contentAlphas[index];
        }

        mViewAndMetaData.view.setTranslationY(transY);
        mViewAndMetaData.view.setScaleX(scale);
        mViewAndMetaData.view.setScaleY(scale);
        if (index == 1) {
            log.debug("transformViewAtIndextransY=" + transY + ",scale=" + scale + ",index=" + index);
        }
        mViewAndMetaData.slidBackground.setAlpha(backgroundAlpha);
        mViewAndMetaData.slidContent.setAlpha(contentAlpha);
        if (index == mViewMaps.size() - 1) {// 最后一个要循序渐进的进入
            mViewAndMetaData.view.setAlpha(offset);
        }
    }

    public void setAdapter(Adapter adapter) {
        mAdapter = adapter;
        fillView();
        requestLayout();
        invalidate();
    }

    private void fillView() {
        mViewMaps.clear();
        removeAllViewsInLayout();
        log.debug("fillView=" + mCurPosition);
        if (mAdapter != null) {
            for (int i = 0; i < DEFINE_FILL_SIZE; i++) {
                View mView = mAdapter.getView(mCurPosition + i, null, this);
                mViewMaps.add(new ViewAndMetaData(mView, i, mCurPosition + i, mAdapter.getItemId(mCurPosition + i)));
            }
        }

        for (int i = mViewMaps.size() - 1; i >= 0; i--) {
            addChild(mViewMaps.get(i).view);
        }
        mFrameAnimatior = new FrameAnimatior();
        mSwipView = getCurrentView();
    }

    private void addChild(View child) {
        addView(child);
        if (mReferenceChildWidth == -1 || mReferenceChildHeight == -1) {
            int measureSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
            child.measure(measureSpec, measureSpec);
            mReferenceChildWidth = child.getMeasuredWidth();
            mReferenceChildHeight = child.getMeasuredHeight();
        }
    }

    class ViewAndMetaData {
        View view;
        int relativeIndex;
        int adapterPosition;
        long itemId;

        View slidBackground;
        View slidContent;
        View blackplane;

        ViewAndMetaData(View view, int relativeIndex, int adapterPosition, long itemId) {
            this.view = view;
            this.relativeIndex = relativeIndex;
            this.adapterPosition = adapterPosition;
            this.itemId = itemId;

            slidBackground = view.findViewById(R.id.slid_background);
            slidContent = view.findViewById(R.id.slid_content);
            blackplane = view.findViewById(R.id.blackplane);
            if (slidBackground == null || slidContent == null) {
                throw new RuntimeException();
            }
        }
    }

    public View getCurrentView() {
        return mViewMaps.get(0).view;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        int action = ev.getAction();
        switch (action & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_DOWN: {
                if (mActivePointerId == INVALID_POINTER) {
                    mLastY = ev.getY();

                    mInitX = ev.getX();
                    mInitY = ev.getY();
                    mActivePointerId = ev.getPointerId(0);
                }
                if (mObjectAnimator != null && mObjectAnimator.isStarted()) {
                    mObjectAnimator.cancel();
                }
                break;
            }
            case MotionEvent.ACTION_MOVE: {
                int pointerIndex = ev.findPointerIndex(mActivePointerId);
                if (pointerIndex == INVALID_POINTER) {
                    return false;
                }
                float newY = ev.getY(pointerIndex);
                float newX = ev.getX(pointerIndex);

                float deltaY = newY - mInitY;
                float deltaX = newX - mInitX;

                mLastY = newY;
                log.debug("deltaY=" + deltaY + ",deltaX=" + deltaX +",parent="+getParent());
                View mParent = (View) getParent();
                if (deltaY > deltaX && (int) Math.abs(deltaY) > mTouchSlop && (mParent != null && mParent.getRotationX() == 0)) {
                    isBeginTranslation = true;
                    requestDisallowInterceptTouchEvent(true);
                    return true;
                }
                break;
            }
            case MotionEvent.ACTION_POINTER_UP: {
                break;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL: {
                mActivePointerId = INVALID_POINTER;
                mSwipeGestureType = GESTURE_NONE;
            }
        }
        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        int action = ev.getAction();
        int pointerIndex = ev.findPointerIndex(mActivePointerId);
        if (pointerIndex == INVALID_POINTER) {
            return false;
        }
        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain();
        }
        mVelocityTracker.addMovement(ev);
        switch (action & MotionEvent.ACTION_MASK) {
        	case MotionEvent.ACTION_DOWN:
        		mLastY = ev.getY();
        		break;
            case MotionEvent.ACTION_MOVE: {
                float newY = ev.getY();
                float deltaY = newY - mLastY;
                log.debug("onTouchEventdeltaY="+deltaY+",newY="+newY+",mLastY="+mLastY);
                mLastY = newY;
                
                if (isBeginTranslation) {
                    if (mFrameAnimatior == null) {
                        mFrameAnimatior = new FrameAnimatior();
                    }
                    if (mSwipView != null) {
                        float deltaFrame = FrameAnimatior.FRAME_COUNT * deltaY / mDefineTouchScale;
                        mFrameAnimatior.addFrameOffset(Math.min(3.0f, deltaFrame));
                        cancelHandleClick();
                        cancelLongPress();
                        return true;
                    }
                }
                break;
            }
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL: {
            	handlePointerUp(ev);
                isBeginTranslation = false;
                mActivePointerId = INVALID_POINTER;
                mSwipeGestureType = GESTURE_NONE;
                break;
            }
        }
        return false;
    }

    private void handlePointerUp(MotionEvent ev) {
        int mYVelocity = 0;
        if (mVelocityTracker != null) {
            mVelocityTracker.computeCurrentVelocity(1000, mMaximumVelocity);
            mYVelocity = (int) mVelocityTracker.getYVelocity(mActivePointerId);
        }
        if (mVelocityTracker != null) {
            mVelocityTracker.recycle();
            mVelocityTracker = null;
        }

        if (mSwipView != null && mFrameAnimatior != null) {
            boolean needSwip = mFrameAnimatior.currentFrame != 0 && mFrameAnimatior.currentFrame != FrameAnimatior.FRAME_COUNT;
            log.debug("needSwip=" + needSwip);
            if (needSwip) {// SWIP TO NEXT
                showNextWithAnimation(mYVelocity,mFrameAnimatior.currentFrame > 7 || Math.abs(mYVelocity) > mMinimumVelocity);
            }
        }
        mActivePointerId = INVALID_POINTER;
        mSwipeGestureType = GESTURE_NONE;
    }

    public void showNextWithAnimation(final boolean isToNext) {
    	showNextWithAnimation(0, isToNext);
    }
    
    public void showNextWithAnimation(final long mYVelocity,final boolean isToNext) {
    	 if (mSwipView == null || mFrameAnimatior == null) {
             return;
         }
         if(mObjectAnimator != null && mObjectAnimator.isStarted()){
         	mObjectAnimator.cancel();
         }
         float endPosition = isToNext ? FrameAnimatior.FRAME_COUNT : 0;
         float starPosition = mFrameAnimatior.currentFrame;
         log.debug("endPosition=" + endPosition + ",starPosition=" + starPosition + ",isToNext=" + isToNext);
         mObjectAnimator = ObjectAnimator.ofFloat(mFrameAnimatior, "currentFrame", starPosition, endPosition);
         long duration = mFrameAnimatior.getAnimationDuration(mYVelocity,Math.abs(starPosition - endPosition));
         log.debug("duration="+duration);
         mObjectAnimator.setDuration(duration);
         mObjectAnimator.setInterpolator(new LinearInterpolator());
         mObjectAnimator.addListener(new AnimatorListener() {

             @Override
             public void onAnimationStart(Animator animation) {
             }

             @Override
             public void onAnimationRepeat(Animator animation) {
             }

             @Override
             public void onAnimationEnd(Animator animation) {
                 if (isToNext) {
                     showNext();
                     mFrameAnimatior.reset();
                 }
             }

             @Override
             public void onAnimationCancel(Animator animation) {

             }
         });
         mObjectAnimator.start();
    }

    private void showNext() {
        mCurPosition += 1;
        log.debug("showNext="+mCurPosition);
        fillView();
    }

    private void cancelHandleClick() {
        View v = getCurrentView();
        if (v != null) {
            hideTapFeedback(v);
        }
    }

    private void hideTapFeedback(View v) {
        v.setPressed(false);
    }

    private class FrameAnimatior {

        public float currentFrame;

        private static final int FRAME_COUNT = 30;// 总共30帧

        private static final int DEFINE_TIME_SPACE = 2;// 设计师给的效果图是按帧算的，每个item间隔是15帧，第一个动画从0帧开始，第二个动画从2帧开始，以此类推

        private static final int DEFINE_ANI_DURATION = 15;// 每个item的动画间隔

        private static final float DEFINE_START_ALPHA_ANI = 2.0f / 3;// 第一个界面旋转到60度时候，开始透明度的变化
        
        private static final float MAX_OFFSET = 2.0f;

        private static final int MINIMUM_ANIMATION_DURATION = 500;
        
        public int getAnimationDuration(final float mYVelocity,float dutation) {
        	log.debug("mYVelocity="+mYVelocity+",dutation="+dutation);
        	if(mYVelocity == 0){
        		return (int) (DEFAULT_ANIMATION_DURATION * dutation / FRAME_COUNT);
        	}else{
        		float velocity = Math.min(Math.abs(mYVelocity)/2000, MAX_OFFSET);
        		velocity = Math.max(velocity, 1);
        		log.debug("velocity="+velocity);
        		return (int) (DEFAULT_ANIMATION_DURATION * dutation / (FRAME_COUNT * velocity));
        	}
        }

        public void setCurrentFrame(float frame) {
            currentFrame = frame;
            mSwipView = getCurrentView();
            if (mSwipView == null) {
                return;
            }
            if (frame >= 0 && frame <= DEFINE_ANI_DURATION + 3) {// 第一个item的旋转动画
                mSwipView.setCameraDistance(mCameraDistance);
                float rotation = frame / DEFINE_ANI_DURATION;
                rotation = Math.min(rotation, 1);
                mSwipView.setPivotX(mSwipView.getWidth() / 2);
                mSwipView.setPivotY(mSwipView.getHeight() - mSwipView.getPaddingBottom() - mSwipView.getPaddingTop());
                mSwipView.setRotationX(-rotation * 90);
                if (rotation > DEFINE_START_ALPHA_ANI) {// 当旋转角度大于60度的时候才开始透明度的变化，从1到0
                    float alpha = 3 * (1 - rotation);
                    mSwipView.setAlpha(alpha);
                }
            }
            for (int i = 1, size = mViewMaps.size(); i < size; i++) {
                float rotation = (frame - DEFINE_TIME_SPACE * i) / DEFINE_ANI_DURATION;
                rotation = Math.min(rotation, 1);
                if (rotation >= 0 && rotation <= 1) {
                    transformViewAtIndex(i, rotation, mViewMaps.get(i), true);
                }
            }

        }

        public void reset() {
            currentFrame = 0;
            setCurrentFrame(currentFrame);
        }

        /**
         * 递增函数。考虑滑动越界
         *
         * @param deltaFrame
         */
        public void addFrameOffset(float deltaFrame) {
            currentFrame += deltaFrame;
            if (currentFrame > FRAME_COUNT) {
                showNext();
                currentFrame -= FRAME_COUNT;
            }
            setCurrentFrame(currentFrame);
        }

    }
}
