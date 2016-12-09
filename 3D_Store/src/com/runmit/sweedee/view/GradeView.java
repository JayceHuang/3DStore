package com.runmit.sweedee.view;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.Constant;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.AttributeSet;
import android.widget.ImageView;
import android.widget.LinearLayout;

public class GradeView extends LinearLayout {

    private Bitmap bitmapDefault;
    private Bitmap bitmapGrade;

    private final int SIZE = 5;
    private int PADDING_LEFT = (int) (5 * Constant.DENSITY);

    public GradeView(Context context) {
        super(context);

        init();

    }

    public GradeView(Context context, AttributeSet attrs) {
        super(context, attrs);

        init();

    }

    public GradeView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        init();
    }


    private void init() {
        if (bitmapDefault == null)
            bitmapDefault = BitmapFactory.decodeResource(getResources(), R.drawable.star_default);

        if (bitmapGrade == null) {
            bitmapGrade = BitmapFactory.decodeResource(getResources(), R.drawable.star_press);

        }
        LayoutParams layoutParams = new LayoutParams(bitmapDefault.getWidth() + PADDING_LEFT, bitmapDefault.getHeight());
        for (int i = 0; i < SIZE; i++) {
            ImageView image = new ImageView(getContext());
            image.setLayoutParams(layoutParams);
            image.setImageBitmap(bitmapDefault);
            this.addView(image);
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    /**
     * 设置星级数量
     *
     * @param grade
     * @param bitmap
     */
    public void setGrade(int grade) {
        if (grade > SIZE) {
            grade = SIZE;
        }

        for (int i = 0; i < grade; i++) {
            ImageView imageView = (ImageView) getChildAt(i);
            imageView.setImageBitmap(bitmapGrade);
        }
    }

    /**
     * 释放无用资源
     */
    public void release() {
        if (bitmapDefault != null && !bitmapDefault.isRecycled()) {
            bitmapDefault.recycle();
            bitmapDefault = null;
        }

        if (bitmapGrade != null && !bitmapGrade.isRecycled()) {
            bitmapGrade.recycle();
            bitmapGrade = null;
        }
    }

}
