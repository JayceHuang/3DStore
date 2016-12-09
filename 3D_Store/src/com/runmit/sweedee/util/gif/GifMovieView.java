package com.runmit.sweedee.util.gif;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Movie;
import android.os.Build;
import android.util.AttributeSet;
import android.view.View;

import com.runmit.sweedee.R;

/**
 * This is a View class that wraps Android {@link Movie} object and displays it.
 * You can set GIF as a Movie object or as a resource id from XML or by calling
 * {@link #setMovie(Movie)} or {@link #setMovieResource(int)}.
 * <p>
 * You can pause and resume GIF animation by calling {@link #setPaused(boolean)}.
 * <p>
 * The animation is drawn in the center inside of the measured view bounds.
 * 
 * @author Sergey Bakhtiarov
 * 
 * modified by Sven.Zhan
 * 
 * 1 修改测量逻辑，需要全屏 
 * 2 增加播放完成逻辑，供外部监听
 */

public class GifMovieView extends View {

	String TAG = GifMovieView.class.getSimpleName();
	
	private Movie mMovie;

	private long mMovieStart;
	
	private int mPreviousAnimationTime = 0;
	
	private int mCurrentAnimationTime = 0;	

	/**
	 * Scaling factor to fit the animation within view bounds.
	 */
	private float mScale;

	/**
	 * Scaled movie frames width and height.
	 */
	private int mMeasuredMovieWidth;
	private int mMeasuredMovieHeight;

	private float mLeft;
	private float mTop;

	private volatile boolean mPaused = false;
	
	private boolean mVisible = true;
	
	private EndListener mEndListener;	
	
	private int duration = 0;

	public GifMovieView(Context context) {
		this(context, null);
	}

	public GifMovieView(Context context, AttributeSet attrs) {
		this(context, attrs, R.styleable.CustomTheme_gifMoviewViewStyle);
	}

	public GifMovieView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);

		setViewAttributes(context, attrs, defStyle);
	}

	@SuppressLint("NewApi")
	private void setViewAttributes(Context context, AttributeSet attrs, int defStyle) {
		
		/**
		 * Starting from HONEYCOMB have to turn off HW acceleration to draw Movie on Canvas.
		 */
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
			setLayerType(View.LAYER_TYPE_SOFTWARE, null);
		}

		final TypedArray array = context.obtainStyledAttributes(attrs, R.styleable.GifMoviewView, defStyle,R.style.Widget_GifMoviewView);

		int resourceId = array.getResourceId(R.styleable.GifMoviewView_gif, -1);
		mPaused = array.getBoolean(R.styleable.GifMoviewView_paused, false);

		array.recycle();

		if (resourceId != -1) {
			mMovie = Movie.decodeStream(getResources().openRawResource(resourceId));
			duration = mMovie.duration();
		}
	}

	/**返回值为0 则说明该设备系统不支持*/
	public int setMovieResource(int movieResId) {
		mMovie = Movie.decodeStream(getResources().openRawResource(movieResId));
		duration = mMovie.duration();	
		if(duration > 0){
			requestLayout();
		}		
		return duration;
	}

	public void setPaused(boolean paused) {
		this.mPaused = paused;

		/**
		 * Calculate new movie start time, so that it resumes from the same
		 * frame.
		 */
		if (!paused) {
			mMovieStart = android.os.SystemClock.uptimeMillis() - mCurrentAnimationTime;
		}

		invalidate();
	}

	public void setOnEndListener(EndListener listener){
		mEndListener = listener;
	}
	
	public boolean isPaused() {
		return this.mPaused;
	}

	
	
	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {

		if (mMovie != null) {
			int movieWidth = mMovie.width();
			int movieHeight = mMovie.height();

			float scaleW = 1f;			
			mMeasuredMovieWidth = MeasureSpec.getSize(widthMeasureSpec);
			scaleW = (float) mMeasuredMovieWidth / (float) movieWidth;

			float scaleH = 1f;
			mMeasuredMovieHeight = MeasureSpec.getSize(heightMeasureSpec);
			scaleH = (float) mMeasuredMovieHeight / (float) movieHeight ;			
			
			mScale = Math.max(scaleH, scaleW);

			setMeasuredDimension(mMeasuredMovieWidth, mMeasuredMovieHeight);
			
//			Log.d(TAG, "onMeasure movieWidth = "+movieWidth+" movieHeight = "+movieHeight+" mMeasuredMovieWidth = "+mMeasuredMovieWidth
//					+" mMeasuredMovieHeight = "+mMeasuredMovieHeight+" scaleW = "+scaleW+" scaleH = "+scaleH);
		} else {
			/*
			 * No movie set, just set minimum available size.
			 */
			setMeasuredDimension(getSuggestedMinimumWidth(), getSuggestedMinimumHeight());
		}
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b) {
		super.onLayout(changed, l, t, r, b);
		
		mLeft = (getWidth() - mMovie.width()*mScale) / 2f;
		mTop = (getHeight() - mMovie.height()*mScale) / 2f;
		
//		Log.d(TAG, "onLayout mLeft = "+mLeft+" mTop = "+mTop);
		
		mVisible = getVisibility() == View.VISIBLE;
	}

	@Override
	protected void onDraw(Canvas canvas) {
		if (mMovie != null) {
			if (!mPaused) {
				updateAnimationTime();				
				drawMovieFrame(canvas);
				invalidateView();
			} else {
				mCurrentAnimationTime = mPreviousAnimationTime;
				drawMovieFrame(canvas);
			}
		}
	}
	
	/**
	 * Invalidates view only if it is visible.
	 * <br>
	 * {@link #postInvalidateOnAnimation()} is used for Jelly Bean and higher.
	 * 
	 */
	@SuppressLint("NewApi")
	private void invalidateView() {
		if(mVisible) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
				postInvalidateOnAnimation();
			} else {
				invalidate();
			}
		}
	}

	private int chavalue  = 0;
	/**
	 * Calculate current animation time
	 */
	private void updateAnimationTime() {
		mPreviousAnimationTime = mCurrentAnimationTime;
		long now = android.os.SystemClock.uptimeMillis();

		if (mMovieStart == 0) {
			mMovieStart = now;
		}
		
		mCurrentAnimationTime = (int) ((now - mMovieStart) % duration);
		chavalue = mCurrentAnimationTime - mPreviousAnimationTime;
		if(mCurrentAnimationTime + 2 * chavalue > duration){//通过预留两帧的时间来判断
			this.mPaused = true;
			if(mEndListener != null){
				mEndListener.onEnd();
			}
		}
		//Log.d(TAG, "updateAnimationTime mCurrentAnimationTime = "+mCurrentAnimationTime+" chavalue = "+chavalue);
	}

	/**
	 * Draw current GIF frame
	 */
	private void drawMovieFrame(Canvas canvas) {

		mMovie.setTime(mCurrentAnimationTime);

		canvas.save(Canvas.MATRIX_SAVE_FLAG);
		canvas.scale(mScale, mScale);
		mMovie.draw(canvas, mLeft / mScale, mTop / mScale);
		//mMovie.draw(canvas, 0, 0);
		canvas.restore();
	}
	
	@SuppressLint("NewApi")
	@Override
	public void onScreenStateChanged(int screenState) {
		super.onScreenStateChanged(screenState);
		mVisible = screenState == SCREEN_STATE_ON;
		invalidateView();
	}
	
	@SuppressLint("NewApi")
	@Override
	protected void onVisibilityChanged(View changedView, int visibility) {
		super.onVisibilityChanged(changedView, visibility);
		mVisible = visibility == View.VISIBLE;
		invalidateView();
	}
	
	@Override
	protected void onWindowVisibilityChanged(int visibility) {
		super.onWindowVisibilityChanged(visibility);
		mVisible = visibility == View.VISIBLE;
		invalidateView();
	}
	
	public interface EndListener{
		
		public void onEnd();
	}
}
