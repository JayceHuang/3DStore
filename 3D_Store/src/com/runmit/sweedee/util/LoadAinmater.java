package com.runmit.sweedee.util;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.TimeInterpolator;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.DecelerateInterpolator;
import android.widget.ImageView;

/**
 * 
 * @author Sven.Zhan
 *
 */
public class LoadAinmater {
	
	public static String TAG = "ActivitySplitAnimationUtil";

    public static Bitmap mBitmap = null;
    private static int[] mLoc1;
    private static int[] mLoc2;
    private static ImageView mTopImage;
    private static ImageView mBottomImage;
    private static AnimatorSet mSetAnim;

  
    public static void startActivity(Activity currActivity, Intent intent) {
    	 prepare(currActivity);
         currActivity.startActivity(intent);
         currActivity.overridePendingTransition(0, 0);
    }
  
    public static void prepareAnimation(final Activity destActivity) {
        mTopImage = createImageView(destActivity, mBitmap, mLoc1);
        mBottomImage = createImageView(destActivity, mBitmap, mLoc2);
    }
   
    public static void animate(final Activity destActivity, final int duration, final TimeInterpolator interpolator) {
    	
        new Handler().post(new Runnable() {
            @Override
            public void run() {
                mSetAnim = new AnimatorSet();
                mTopImage.setLayerType(View.LAYER_TYPE_HARDWARE, null);
                mBottomImage.setLayerType(View.LAYER_TYPE_HARDWARE, null);
                final float mTopImageHeight = mTopImage.getHeight();               
                final ObjectAnimator anim1 = ObjectAnimator.ofFloat(mTopImage, "translationY", mTopImageHeight * -1);
                final ObjectAnimator anim2 = ObjectAnimator.ofFloat(mBottomImage, "translationY", mBottomImage.getHeight());
                anim1.addUpdateListener(new AnimatorUpdateListener() {					
					@Override
					public void onAnimationUpdate(ValueAnimator animation) {
						float chaValue = (Float)anim1.getAnimatedValue() + mTopImageHeight;
						if(chaValue < 150){
							destActivity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN, WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
						}
					}
				});
                mSetAnim.addListener(new Animator.AnimatorListener() {
                    @Override
                    public void onAnimationStart(Animator animation) {
                    	
                    }

                    @Override
                    public void onAnimationEnd(Animator animation) {                    	
                        clean(destActivity);
                    }

                    @Override
                    public void onAnimationCancel(Animator animation) {
                        clean(destActivity);
                    }

                    @Override
                    public void onAnimationRepeat(Animator animation) {
                    	
                    }
                });
                
                if (interpolator != null) {
                    anim1.setInterpolator(interpolator);
                    anim2.setInterpolator(interpolator);
                }
                mSetAnim.setStartDelay(20);
                mSetAnim.setDuration(duration);
                mSetAnim.playTogether(anim1, anim2);
                mSetAnim.start();
            }
        });
    }
    
    public static void animate(final Activity destActivity, final int duration) {
        animate(destActivity, duration, new DecelerateInterpolator());
    }

    public static void cancel() {
        if (mSetAnim != null)
            mSetAnim.cancel();
    }
    
    
    private static void clean(Activity activity) {
        if (mTopImage != null) {
            mTopImage.setLayerType(View.LAYER_TYPE_NONE, null);
            try {               
                activity.getWindowManager().removeViewImmediate(mBottomImage);
            } catch (Exception ignored) {
            }
        }
        if (mBottomImage != null) {
            mBottomImage.setLayerType(View.LAYER_TYPE_NONE, null);
            try {
                activity.getWindowManager().removeViewImmediate(mTopImage);
            } catch (Exception ignored) {
            }
        }
        mBitmap = null;
    }
 
    private static void prepare(Activity currActivity) {
        View root = currActivity.getWindow().getDecorView().findViewById(android.R.id.content);
        root.setDrawingCacheEnabled(true);      
        mBitmap = root.getDrawingCache();        
        int  splitYCoord = mBitmap.getHeight() * 596 / 1280;//启动图的比例上下：596:(1280-596)      
        mLoc1 = new int[]{0, splitYCoord, root.getTop()};
        mLoc2 = new int[]{splitYCoord, mBitmap.getHeight(), root.getTop()};
        //Log.d(TAG, "splitYCoord = "+splitYCoord+" mLoc1 = "+Arrays.toString(mLoc1)+" mLoc2 = "+Arrays.toString(mLoc2));
    }

	private static ImageView createImageView(Activity destActivity, Bitmap bmp, int loc[]) {
    	MyImageView imageView = new MyImageView(destActivity);
        imageView.setImageBitmap(bmp);
        imageView.setImageOffsets(bmp.getWidth(), loc[0], loc[1]); 
        WindowManager.LayoutParams windowParams = new WindowManager.LayoutParams();
        windowParams.gravity = Gravity.TOP;
        windowParams.x = 0;
        windowParams.y = loc[2] + loc[0];
        windowParams.height = loc[1] - loc[0];
        windowParams.width = bmp.getWidth();
        windowParams.flags = WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
        windowParams.format = PixelFormat.TRANSLUCENT;
        windowParams.systemUiVisibility = View.SYSTEM_UI_FLAG_FULLSCREEN;
        windowParams.windowAnimations = 0;
        //Log.d(TAG, "createImageView x = "+windowParams.x+" y = "+windowParams.y+" width  = "+windowParams.width+" height = "+ windowParams.height);
        destActivity.getWindowManager().addView(imageView, windowParams);
        return imageView;
    }
 
    private static class MyImageView extends ImageView
    {
    	private Rect mSrcRect;
    	private Rect mDstRect;
    	private Paint mPaint;    	
    	
		public MyImageView(Context context) 
		{
			super(context);
			mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
		}
		
		public void setImageOffsets(int width, int startY, int endY)
		{
			mSrcRect = new Rect(0, startY, width, endY);
			mDstRect = new Rect(0, 0, width, endY - startY);
		}
				
		@Override
		protected void onDraw(Canvas canvas)
		{
			Bitmap bm = null;
			Drawable drawable = getDrawable();
			if (null != drawable && drawable instanceof BitmapDrawable)
			{
				bm = ((BitmapDrawable)drawable).getBitmap();
			}
			
			if (null == bm)
			{
				super.onDraw(canvas);
			}
			else
			{
				canvas.drawBitmap(bm, mSrcRect, mDstRect, mPaint);
			}
		}    	
    }
}
