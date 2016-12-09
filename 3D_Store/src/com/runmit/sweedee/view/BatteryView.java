package com.runmit.sweedee.view;


import com.runmit.sweedee.R;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.MeasureSpec;


public class BatteryView extends View {


    private Bitmap mBackground;

    private Paint mPaint;
    private float percent = 0;

    private int bitmapWidth;
    private int bitmapHeight;

    // 为了显示中间显示的电量。设以下参数
    private static int PADDING_LEFT = 7;        //隔左边距离
    private static int PADDING_RIGHT = 5;    //隔右边距离
    private static int PADDING_TOP = 4;        //隔上面距离
    private static int PADDING_BOTTOM = 4;    //隔下面距离


    public BatteryView(Context context) {
        super(context);

        initView(context);
    }

    public BatteryView(Context context, AttributeSet attrs) {
        super(context, attrs);

        initView(context);
    }

    public BatteryView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initView(context);


    }


    private void initView(Context context) {
        Drawable drawable1 = this.getContext().getResources().getDrawable(R.drawable.bg_battery);
        mBackground = ((BitmapDrawable) drawable1).getBitmap();
        bitmapWidth = mBackground.getWidth();
        bitmapHeight = mBackground.getHeight();
        mPaint = new Paint();

        Resources resource = getResources();
        PADDING_LEFT = (int) resource.getDimension(R.dimen.battery_padding_left);
        PADDING_RIGHT = (int) resource.getDimension(R.dimen.battery_padding_right);
        PADDING_TOP = (int) resource.getDimension(R.dimen.battery_padding_top);
        PADDING_BOTTOM = (int) resource.getDimension(R.dimen.battery_padding_bottom);

    }

    public void updateBattery(float percent) {
        this.percent = percent;
        this.invalidate();
    }

    protected void onDraw(Canvas canvas) {

        canvas.drawBitmap(mBackground, 0, 0, null);

        mPaint.setColor(Color.WHITE);

        int allWidth = (int) ((1.0 - percent) * (bitmapWidth - PADDING_LEFT - PADDING_RIGHT));

        canvas.drawRect(allWidth + PADDING_LEFT, PADDING_TOP, bitmapWidth - PADDING_RIGHT, bitmapHeight - PADDING_BOTTOM, mPaint);
    }


    /**
     * 比onDraw先执行
     * <p/>
     * 一个MeasureSpec封装了父布局传递给子布局的布局要求，每个MeasureSpec代表了一组宽度和高度的要求。
     * 一个MeasureSpec由大小和模式组成
     * 它有三种模式：UNSPECIFIED(未指定),父元素部队自元素施加任何束缚，子元素可以得到任意想要的大小;
     * EXACTLY(完全)，父元素决定自元素的确切大小，子元素将被限定在给定的边界里而忽略它本身大小；
     * AT_MOST(至多)，子元素至多达到指定大小的值。
     * <p/>
     * 　　它常用的三个函数：
     * 1.static int getMode(int measureSpec):根据提供的测量值(格式)提取模式(上述三个模式之一)
     * 2.static int getSize(int measureSpec):根据提供的测量值(格式)提取大小值(这个大小也就是我们通常所说的大小)
     * 3.static int makeMeasureSpec(int size,int mode):根据提供的大小值和模式创建一个测量值(格式)
     */
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        setMeasuredDimension(measureWidth(widthMeasureSpec), measureHeight(heightMeasureSpec));
    }

    private int measureWidth(int measureSpec) {
        int result = 0;
        int specMode = MeasureSpec.getMode(measureSpec);
        int specSize = MeasureSpec.getSize(measureSpec);

        if (specMode == MeasureSpec.EXACTLY) {
            // We were told how big to be
            result = specSize;
        } else {
            // Measure the text
            result = bitmapWidth;
            if (specMode == MeasureSpec.AT_MOST) {
                // Respect AT_MOST value if that was what is called for by
                // measureSpec
                result = Math.min(result, specSize);// 60,480
            }
        }

        return result;
    }

    private int measureHeight(int measureSpec) {
        int result = 0;
        int specMode = MeasureSpec.getMode(measureSpec);
        int specSize = MeasureSpec.getSize(measureSpec);


        if (specMode == MeasureSpec.EXACTLY) {
            // We were told how big to be
            result = specSize;
        } else {
            // Measure the text (beware: ascent is a negative number)
            result = bitmapHeight;//(int) (-mAscent + mPaint.descent()) + getPaddingTop() + getPaddingBottom();
            if (specMode == MeasureSpec.AT_MOST) {
                // Respect AT_MOST value if that was what is called for by
                // measureSpec
                result = Math.min(result, specSize);
            }
        }
        return result;
    }


}
