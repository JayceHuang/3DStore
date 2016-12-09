package com.runmit.sweedee.util;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;

public class MyBitmapDrawable extends BitmapDrawable {

    /**
     * @param mResources
     * @param bitmap
     */
    public MyBitmapDrawable(Resources mResources, Bitmap bitmap) {
        super(mResources, bitmap);
    }

    /**
     * @param blurBitmap
     */
    public MyBitmapDrawable(Bitmap blurBitmap) {
        super(blurBitmap);
    }

    @Override
    public void draw(Canvas canvas) {
        Bitmap bitmap = getBitmap();
        if (bitmap != null && !bitmap.isRecycled()) {
            super.draw(canvas);
        }
    }

}
