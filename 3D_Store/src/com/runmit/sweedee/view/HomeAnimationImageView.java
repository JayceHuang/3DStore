package com.runmit.sweedee.view;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;

import android.animation.ObjectAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.animation.Interpolator;
import android.widget.ImageView;

public class HomeAnimationImageView extends ImageView {

	private static final float Radius = 28;

	private int index;
	private float density;
	private float padding;

	private Paint mPaint;
	private float line_height;
	private int pageState = 0;

	private boolean showCircle;
	private float mRatio;
	private float animatinRatio;

	private int lastPosition;
	private boolean isAnimating;

	private int ID_DEFAULT;
	private int ID_SRC;

	private int resources[];
	private int resources_down[];
	private ImageHomeScrollTabStrip parentStrip;
	private MyThread mAnimateWalker;

	public HomeAnimationImageView(Context context) {
		super(context);
		init();
	}

	public HomeAnimationImageView(Context context, AttributeSet attrs) {
		super(context, attrs);
		init();
	}

	MyAnimatorObject mAnimateObj = new MyAnimatorObject();
	ObjectAnimator mPressAnimator;

	private void init() {
		if(StoreApplication.isSnailPhone){
			resources = new int[]{R.drawable.ic_home_guide, R.drawable.ic_home_video, R.drawable.ic_home_app};
			resources_down = new int[]{R.drawable.ic_home_guide_down, R.drawable.ic_home_video_down, R.drawable.ic_home_app_down};
		}else{
			resources = new int[]{R.drawable.ic_home_guide, R.drawable.ic_home_video, R.drawable.ic_home_game, R.drawable.ic_home_app};
			resources_down = new int[]{R.drawable.ic_home_guide_down, R.drawable.ic_home_video_down, R.drawable.ic_home_game_down, R.drawable.ic_home_app_down};
		}
		density = getResources().getDisplayMetrics().density;
		line_height = 1 * density;
		mPaint = new Paint();
		mPaint.setAntiAlias(true);
		mPaint.setColor(Color.rgb(0, 161, 255));
		mPaint.setStyle(Paint.Style.STROKE);
		mPaint.setStrokeWidth(line_height);
		
		mPressAnimator = ObjectAnimator.ofFloat(mAnimateObj, "currentFrame", 1, .8f);
		//mPressAnimator.setInterpolator(new  MyBounceInterpolator());
		mPressAnimator.setDuration(80);
	}

	public void cancelScale() {
		mPressAnimator.cancel();
	}

	public void commitScaleIn() {
		mPressAnimator.start();
	}

	public void commitScaleOut() {
		mPressAnimator.cancel();
		mPressAnimator.reverse();
	}

	public void draw(Canvas canvas, Paint parentPaint) {
		// canvas.drawCircle( (index + .5f) * padding, -height / 2, radius *
		// density , mPaint);
		// Log.d("mzh", "index:"+index + " padding:" +padding + " hei:"+height);
		if (showCircle) {
			// Log.d("mzh", "drawing ratio:"+ ratio + "  alpha" +
			// mPaint.getAlpha());
			animatinRatio = mRatio;
			canvas.drawCircle((index + .5f) * padding, 0, Radius * mRatio * density, mPaint);
		}
	}

	// foward 向左
	public void update(int position, float offset) {
		boolean pageEnd = offset < 0.00001;
		boolean isFoward = lastPosition != position;
		// Log.d("mzh", "page state:" + pageState + "  page end:"+pageEnd +
		// " offset:"+offset);
		if (pageState > 0 && !pageEnd) {
			if (isFoward) {
				if (position == index) {
					runNext(isFoward, offset);
				} else if (position + 1 == index) {
					runCurrent(isFoward, offset);
				}
			} else {
				if (position == index) {
					runCurrent(isFoward, offset);
				} else if (position + 1 == index) {
					runNext(isFoward, offset);
				}
			}
		}

		if (pageState == 2 && pageEnd) {
			lastPosition = position;
			isLeftIN = false;
			isRightIN = false;
			isLeftOut = false;
			isRightOut = false;
		}
	}

	boolean isLeftIN;
	boolean isRightIN;
	boolean isLeftOut;
	boolean isRightOut;
	private float lastOffsetIn;
	private float lastOffsetOut;

	// 退出动画
	private void runCurrent(boolean isFoward, float offset) {
		int alpha = 0;
		if (isFoward) { // 右边退出
			if (offset > 0.6f) {
				float alp = 1.0f - (1.0f - offset) / 0.4f / 2;
				this.setAlpha(alp);
				this.setScaleX(alp);
				this.setScaleY(alp);
			} else if (offset > 0.4) {
				float alp = 0.5f;
				this.setAlpha(alp);
			} else {
				this.setAlpha(1.0f);
				float scale = 1.0f - offset / 0.4f / 2;
				this.setScaleX(scale);
				this.setScaleY(scale);
			}

			// Log.d("mzh", "isRightOut:" + isRightOut +" offset:"+offset +
			// " last:"+lastOffsetOut);
			if (offset < 0.4) {
				if (!isRightOut && lastOffsetOut > offset) {
					isRightOut = true;
					this.setImageResource(ID_DEFAULT);
					// Log.d("mzh", "right exit set down");
				}
			}
			if (offset > 0.6) {
				if (isRightOut && lastOffsetOut < offset) {
					isRightOut = false;
					this.setImageResource(ID_SRC);
					// Log.d("mzh", "right exit set default");
				}
			}
			alpha = (int) (offset * 255);
			if (!isAnimating)
				mRatio = offset;

		} else { // 左边退出

			if (offset > 0.6) {
				float alp = offset;
				this.setAlpha(alp);
				this.setScaleX(alp);
				this.setScaleY(alp);
			} else if (offset > 0.4) {
				float alp = 0.5f;
				this.setAlpha(alp);
			} else {
				float alp = 0.5f + (0.4f - offset) / 0.4f / 2;
				this.setAlpha(alp);
				this.setScaleX(alp);
				this.setScaleY(alp);
			}

			// Log.d("mzh", "isLeftOut:" + isLeftOut +" offset:"+offset +
			// " last:"+lastOffsetOut);
			if (offset > 0.6) {
				if (!isLeftOut && lastOffsetOut < offset) {
					isLeftOut = true;
					this.setImageResource(ID_DEFAULT);
					// Log.d("mzh", "left exit set down");
				}
			}

			if (offset < 0.4) {
				if (isLeftOut && lastOffsetOut > offset) {
					isLeftOut = false;
					this.setImageResource(ID_SRC);
					// Log.d("mzh", "left exit set default");
				}
			}
			alpha = (int) ((1 - offset) * 255);
			if (!isAnimating)
				mRatio = 1 - offset;
		}
		if (alpha < 100)
			alpha = 0;
		mPaint.setAlpha(alpha);
		lastOffsetOut = offset;
	}

	// 进入动画
	private void runNext(boolean isFoward, float offset) {

		if (isFoward) { // 向左进入
			// Log.d("mzh", "off:"+offset);
			if (offset > 0.6f) {
				float alp = 1.0f - (1.0f - offset) / 0.4f / 2;
				this.setAlpha(alp);
				this.setScaleX(alp);
				this.setScaleY(alp);
			} else if (offset > 0.4) {
				float alp = 0.5f;
				this.setAlpha(alp);
			} else {
				float alp = (0.4f - offset) / 0.4f / 2 + 0.5f;
				this.setAlpha(alp);
				this.setScaleX(alp);
				this.setScaleY(alp);
			}

			if (offset < 0.4) {
				if (!isLeftIN && lastOffsetIn > offset) {
					isLeftIN = true;
					this.setImageResource(ID_SRC);
					// Log.d("mzh", "left enter set down");
				}
			}
			if (offset > 0.6) {
				if (isLeftIN && lastOffsetIn < offset) {
					isLeftIN = false;
					this.setImageResource(ID_DEFAULT);
					// Log.d("mzh", "left enter set down");
				}
			}

		} else {// 向右进入

			if (offset > 0.6f) {
				float alp = 1.0f - (1.0f - offset) / 0.4f / 2;
				this.setAlpha(alp);
				this.setScaleX(alp);
				this.setScaleY(alp);
			} else if (offset > 0.4) {
				float alp = 0.5f;
				this.setAlpha(alp);
			} else {
				float alp = 1.0f - offset / 0.4f / 2;
				this.setAlpha(alp);
				this.setScaleX(alp);
				this.setScaleY(alp);
			}

			if (offset > 0.6) {
				if (!isRightIN && lastOffsetIn < offset) {
					isRightIN = true;
					this.setImageResource(ID_SRC);
					// Log.d("mzh", "right enter set down");
				}
			}
			if (offset < 0.4) {
				if (isRightIN && lastOffsetIn > offset) {
					isRightIN = false;
					this.setImageResource(ID_DEFAULT);
					// Log.d("mzh", "right enter set down");
				}
			}

		}

		lastOffsetIn = offset;
	}

	// 初次进入时调用
	public void setIndex(ImageHomeScrollTabStrip imageHomeScrollTabStrip, int i) {
		this.parentStrip = imageHomeScrollTabStrip;
		this.index = i;
		ID_DEFAULT = resources[index];
		ID_SRC = resources_down[index];

		if (i == 0) {
			showCircle = true;
			/*
			 * if(mAnimateWalker != null){ mAnimateWalker.cancel(); }
			 * mAnimateWalker = new MyThread(500); mAnimateWalker.start();
			 */
			mRatio = 1;
			setImageResource(ID_SRC);
		} else {
			setImageResource(ID_DEFAULT);
		}

	}

	public int getIndex() {
		return index;
	}

	public void initWithSize(float width, float height, int DEFINE_SIZE) {
		padding = width / DEFINE_SIZE;
	}

	public void pageChanged(int position) {

		setImageResource(position != index ? ID_DEFAULT : ID_SRC);

		// mObjectAnimator.cancel();
		if (mAnimateWalker != null) {
			mAnimateWalker.cancel();
		}

		resetDefault();

		if (position != index) {
			showCircle = false;
			return;
		}

		// mObjectAnimator.start();

		mAnimateWalker = new MyThread(800);
		mAnimateWalker.start();
		// canimator.cancel();
		// canimator.start();
	}

	private void resetDefault() {
		float scale = 1;
		setScaleX(scale);
		setScaleY(scale);
		setAlpha(1f);
	}

	public void stateChanged(int state) {
		pageState = state;
	}

	/**
	 * 弹性插值
	 */
	public static class MyBounceInterpolator implements Interpolator {
		public MyBounceInterpolator() {
		}

		public MyBounceInterpolator(Context context, AttributeSet attrs) {
		}

		private static float bounce(float t) {
			return t * t * 8.0f;
		}

		public float getInterpolation(float t) {
			// _b(t) = t * t * 8
			// bs(t) = _b(t) for t < 0.3535
			// bs(t) = _b(t - 0.54719) + 0.7 for t < 0.7408
			// bs(t) = _b(t - 0.8526) + 0.9 for t < 0.9644
			// bs(t) = _b(t - 1.0435) + 0.95 for t <= 1.0
			// b(t) = bs(t * 1.1226)
			t *= 1.1226f;
			if (t < 0.3535f)
				return bounce(t);
			else if (t < 0.7408f)
				return bounce(t - 0.54719f) + 0.7f;
			else if (t < 0.9644f)
				return bounce(t - 0.8526f) + 0.9f;
			else
				return bounce(t - 1.0435f) + 0.95f;
		}
	}

	/**
	 * ObjectAnimator 用于设置属性的对象
	 *
	 */
	private class MyAnimatorObject {
		@SuppressWarnings("unused")
		public void setCurrentFrame(float dt) {
			float scale = dt;
			HomeAnimationImageView.this.setScaleX(scale);
			HomeAnimationImageView.this.setScaleY(scale);
			HomeAnimationImageView.this.setAlpha(scale);
		}

	}

	/**
	 * 播放动画的线程
	 *
	 */
	private class MyThread extends Thread {

		private int fps = 50;
		// private long lastTime;
		private int frame;
		private int duration;
		private float totalFrames;
		private boolean exit;
		private float scale;

		public MyThread(int du) {
			duration = du;
			frame = 0;
			totalFrames = fps * duration / 1000f;

			exit = false;
		}

		@Override
		public void run() {
			if (duration <= 0 || frame > 0) {
				return;
			}
			isAnimating = true;
			animatinRatio = mRatio;
			while (!exit) {
				long t = System.currentTimeMillis();
				// Log.d("mzh", "du:"+duration + " f:" + frame+ " dt:"+(t -
				// lastTime));

				if (totalFrames < frame) {
					break;
				}

				float vt = frame / totalFrames;
				// Log.d("mzh", "vt:"+vt);
				boolean isupdate = computeVt(getInterpolation(vt));

				try {
					Thread.sleep(1000 / fps);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}

				if (!isupdate) {
					continue;
				}

				frame++;
				// lastTime = t;
			}

			post(new Runnable() {

				@Override
				public void run() {
					HomeAnimationImageView.this.setScaleX(1);
					HomeAnimationImageView.this.setScaleY(1);
					HomeAnimationImageView.this.setAlpha(1f);
				}
			});
			isAnimating = false;
		}

		// 弹性算法
		private float bounce(float t) {
			return t * t * 8;
		}

		// 弹性插值
		/*
		 * private float getInterpolation(float t) { t *= 1.1226f; if (t <
		 * 0.3535f) return bounce(t); else if (t < 0.7408f) return bounce(t -
		 * 0.54719f) + 0.7f; else if (t < 0.9644f) return bounce(t - 0.8526f) +
		 * 0.9f; else return bounce(t - 1.0435f) + 0.95f; }
		 */
		// 周期
		private final int mCycles = 2;

		public float getInterpolation(float input) {
			float ra = input * 4 * mCycles < 1 ? 1 : (1 - input) / 6f;
			return (float) (Math.sin(2 * mCycles * Math.PI * input - Math.PI / 2)) * ra + 1;
		}

		// 跟新ui
		private Runnable updateRunnable = new Runnable() {
			@Override
			public void run() {
				HomeAnimationImageView.this.setAlpha(mRatio);
				HomeAnimationImageView.this.setScaleX(scale);
				HomeAnimationImageView.this.setScaleY(scale);
				parentStrip.invalidate();
			}
		};

		// 按照vt更新变量
		private boolean computeVt(float vt) {
			if (animatinRatio != mRatio)
				return false;
			long t = System.currentTimeMillis();
			// Log.d("mzh", " dt:"+(t - lastTime));
			scale = (1 - vt) * 0.4f + 1;
			// scale = (scale + 0.5f) /1.5f;

			mRatio = vt;
			showCircle = true;
			int alpha = Math.min((int) (mRatio * 255), 255);
			// Log.d("mzh", "animating ratio:"+ vt + "  alpha:" + alpha);
			HomeAnimationImageView.this.post(updateRunnable);
			mPaint.setAlpha(alpha);

			// lastTime = t;
			return true;
		}

		private void cancel() {
			exit = true;
		}
	}

}
