package com.runmit.sweedee.action.more;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Camera;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.support.v4.app.FragmentActivity;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;
import android.widget.ImageView;

import com.runmit.sweedee.util.XL_Log;
import com.umeng.analytics.MobclickAgent;

/**
 * 个人中心翻转动画的基类,继承此类可实现翻转效果
 */
public abstract class NewBaseRotateActivity extends FragmentActivity {
    XL_Log log = new XL_Log(NewBaseRotateActivity.class);

    int screenWidth;

    public static final String TOKEN = "skip";

    ImageView mImageView;

    ViewGroup parentView;

    ValueAnimator mValueAnimator;

    public static Bitmap bitmap = null;
    public static int frameTop = 0;

    private Handler mHandler = new Handler();


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0 && (mValueAnimator != null && mValueAnimator.isRunning())) {
            return true;
        } else
            return super.onKeyDown(keyCode, event);
    }


    /**
     * 在setContentView之后开启动画
     */
    @Override
    public void setContentView(int layoutResID) {
        super.setContentView(layoutResID);
        log.debug("bitmap="+bitmap);
        screenWidth = getResources().getDisplayMetrics().widthPixels;
        if (bitmap != null && !bitmap.isRecycled()) {
            mImageView = new ImageView(this) {
                @Override
                protected void onDraw(Canvas canvas) {
                    try {
                        super.onDraw(canvas);
                    } catch (RuntimeException e) {
                        e.printStackTrace();
                    }
                }
            };
            mImageView.setImageBitmap(bitmap);
            FrameLayout.LayoutParams mParams = new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
            mParams.topMargin = frameTop;
            final ViewGroup decordView = (ViewGroup) getWindow().getDecorView();
            decordView.addView(mImageView, mParams);
            parentView = (ViewGroup) getWindow().getDecorView().findViewById(android.R.id.content).getParent();
            mValueAnimator = ValueAnimator.ofFloat(screenWidth, 0);
            mValueAnimator.setDuration(600);
            mValueAnimator.addUpdateListener(new AnimatorUpdateListener() {

                @Override
                public void onAnimationUpdate(ValueAnimator animation) {
                    float value = (Float) animation.getAnimatedValue();
                    final float rotation = 30 * (screenWidth - value) / screenWidth;
                    float offset = value - getOffsetXForRotation(rotation, mImageView.getWidth(), mImageView.getHeight());
                    mImageView.setTranslationX(offset);
                    mImageView.setRotationY(rotation);

                    final float rotationP = -30 * value / screenWidth;
                    parentView.setRotationY(rotationP);
                    offset = value - (screenWidth - getOffsetXForRotation(rotationP, parentView.getWidth(), parentView.getHeight()));
                    parentView.setTranslationX(offset);
                }
            });
            mValueAnimator.addListener(new AnimatorListener() {

                @Override
                public void onAnimationStart(Animator arg0) {
                    // TODO Auto-generated method stub
                    log.debug("anim   start");
                }

                @Override
                public void onAnimationRepeat(Animator arg0) {
                    // TODO Auto-generated method stub
                    log.debug("anim   Repeat");

                }

                @Override
                public void onAnimationEnd(Animator arg0) {
                    onAnimationFinishIn();
                    log.debug("anim   End");
                }

                @Override
                public void onAnimationCancel(Animator arg0) {
                    onAnimationFinishIn();

                    log.debug("anim   Cancel");
                }
            });

            mValueAnimator.start();
        }
    }

    // 进入界面动画结束 
    protected void onAnimationFinishIn() {

    }

    /**
     * 延迟释放，避免activity finish之后ImageView还会绘制，奇怪
     */
    private void recycle() {
        mHandler.postDelayed(new Runnable() {

            @Override
            public void run() {
                if (bitmap != null) {
                    bitmap.recycle();
                    bitmap = null;
                }
            }
        }, 100);
    }


    private void finishWithOutAnimation() {
        super.finish();
        overridePendingTransition(0, 0);
    }

    @Override
    public void finish() {
        if (mImageView == null) {
            super.finish();
            return;
        }
        BitmapDrawable mBitmapDrawable = (BitmapDrawable) mImageView.getDrawable();
        if (mBitmapDrawable != null) {
            Bitmap b = mBitmapDrawable.getBitmap();
            if (b != null) {
                if (b.isRecycled()) {
                    super.finish();
                    return;
                }
            }
        }
        if (mValueAnimator != null) {
            mValueAnimator.removeAllListeners();
            mValueAnimator.addListener(new AnimatorListener() {

                @Override
                public void onAnimationStart(Animator animation) {
                }

                @Override
                public void onAnimationRepeat(Animator animation) {
                }

                @Override
                public void onAnimationEnd(Animator animation) {
                    finishWithOutAnimation();
                    recycle();

                }

                @Override
                public void onAnimationCancel(Animator animation) {
                }
            });
            mValueAnimator.reverse();
        } else {
            super.finish();
        }
    }

    private static final Matrix OFFSET_MATRIX = new Matrix();
    private static final Camera OFFSET_CAMERA = new Camera();
    private static final float[] OFFSET_TEMP_FLOAT = new float[2];

    private static final float getOffsetXForRotation(float degrees, int width, int height) {
        OFFSET_MATRIX.reset();
        OFFSET_CAMERA.save();
        OFFSET_CAMERA.rotateY(Math.abs(degrees));
        OFFSET_CAMERA.getMatrix(OFFSET_MATRIX);
        OFFSET_CAMERA.restore();

        OFFSET_MATRIX.preTranslate(-width * 0.5f, -height * 0.5f);
        OFFSET_MATRIX.postTranslate(width * 0.5f, height * 0.5f);
        OFFSET_TEMP_FLOAT[0] = width;
        OFFSET_TEMP_FLOAT[1] = height;
        OFFSET_MATRIX.mapPoints(OFFSET_TEMP_FLOAT);
        return OFFSET_TEMP_FLOAT[0];
    }

    /**
     * 3D翻转界面的启动类
     *
     * @param target
     */
    public static void rotateStartActivity(Activity mActivity, Class target) {
        Intent intent = new Intent(mActivity, target);
        mActivity.startActivity(intent);
        mActivity.overridePendingTransition(0, 0);
    }

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        super.onResume();
        MobclickAgent.onResume(this);
    }

    @Override
    protected void onPause() {
        // TODO Auto-generated method stub
        super.onPause();
        MobclickAgent.onPause(this);
    }
}
