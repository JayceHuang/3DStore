package com.runmit.sweedee.view;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.TransitionDrawable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.EnvironmentTool;
import com.runmit.sweedee.util.MyBitmapDrawable;
import com.runmit.sweedee.util.XL_Log;

public class BlurImageView extends ImageView {

    private XL_Log log = new XL_Log(BlurImageView.class);

    private View mBlurView;

    private Bitmap mFilterBitmap;

    /**
     * 用于获取hashcodeKey的对象，本地保存使用
     */
    private Object mTagObject;

    public void setTagObject(Object mTagObject) {
        this.mTagObject = mTagObject;
    }

    public void setBlurView(View mBlurView, Bitmap filterBitmap) {
        this.mBlurView = mBlurView;
        this.mFilterBitmap = filterBitmap;
    }

    public BlurImageView(Context context) {
        super(context);
    }

    public BlurImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public BlurImageView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public void setImageDrawable(final Drawable drawable) {
        super.setImageDrawable(drawable);
        if (mBlurView != null) {
            final Bitmap bitmap = drawableToBitmap(drawable);
            log.debug("setImageDrawable=" + drawable);
            if (bitmap != null && bitmap != mFilterBitmap) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        Bitmap fbBitmap = null;
                        try {
                            fbBitmap = fastblurUsingJava(bitmap, 50);
                        } catch (Throwable e) {
                            // catch可能出现的oom
                        }
                        final Bitmap blurBitmap = fbBitmap;
                        log.debug("setImageDrawable blurBitmap  = " + blurBitmap);
                        if (blurBitmap != null) {
                            mBlurView.post(new Runnable() {
                                @Override
                                public void run() {
                                    try {
                                        Bitmap newBitmap = toConformBitmap(blurBitmap, blurBitmap);
                                        TransitionDrawable td = new TransitionDrawable(new Drawable[]{mBlurView.getBackground(), new MyBitmapDrawable(newBitmap)});
                                        mBlurView.setBackgroundDrawable(td);
                                        td.startTransition(300);
                                    } catch (OutOfMemoryError e) {
                                        e.printStackTrace();
                                    }

                                }
                            });
                        }
                    }
                }).start();
            }
        }
    }

    /**
     * 从一个Drawble中提取出Bitmap,如果该Drawable不能提取，则返回空
     *
     * @param drawable 能提取出bitmap的Drawable对象
     * @return
     */
    public Bitmap drawableToBitmap(Drawable drawable) {
        Drawable des = null;
        if (drawable instanceof LayerDrawable) {
            LayerDrawable td = (LayerDrawable) drawable;
            int num = td.getNumberOfLayers();
            des = td.getDrawable(num - 1);
        } else {
            des = drawable;
            Bitmap bm = null;
            if (des instanceof BitmapDrawable) {
                BitmapDrawable bd = (BitmapDrawable) des;
                bm = bd.getBitmap();
            } else {
//				BitmapDrawable bd = (BitmapDrawable) des;
//				bm = bd.getBitmap();
            }
            return bm;
        }
        /*if(des instanceof MyBitmapDrawable){
            return ((MyBitmapDrawable) des).getBitmap();
		}*/
        return null;
    }

    /**
     * 生成blur特效的bitmap
     *
     * @param sentBitmap 需要生成blur特效的原bitmap
     * @param radius     blur半径
     * @return
     */
    private Bitmap fastblurUsingJava(Bitmap sentBitmap, int radius) {
        //先从本地获取
        Bitmap diskBitmap = getBitmapFormDisk();
        if (diskBitmap != null) {
            return diskBitmap;
        }
        log.debug("blur start");
        Bitmap bitmap = null;
        try {
            bitmap = sentBitmap.copy(sentBitmap.getConfig(), true);
        } catch (Throwable e) {
            return null;
        }
        if (radius < 1) {
            return (null);
        }
        int w = bitmap.getWidth();
        int h = bitmap.getHeight();

        int[] pix = new int[w * h];
        bitmap.getPixels(pix, 0, w, 0, 0, w, h);

        int wm = w - 1;
        int hm = h - 1;
        int wh = w * h;
        int div = radius + radius + 1;

        int r[] = new int[wh];
        int g[] = new int[wh];
        int b[] = new int[wh];
        int rsum, gsum, bsum, x, y, i, p, yp, yi, yw;
        int vmin[] = new int[Math.max(w, h)];

        int divsum = (div + 1) >> 1;
        divsum *= divsum;
        int dv[] = new int[256 * divsum];
        for (i = 0; i < 256 * divsum; i++) {
            dv[i] = (i / divsum);
        }

        yw = yi = 0;

        int[][] stack = new int[div][3];
        int stackpointer;
        int stackstart;
        int[] sir;
        int rbs;
        int r1 = radius + 1;
        int routsum, goutsum, boutsum;
        int rinsum, ginsum, binsum;

        for (y = 0; y < h; y++) {
            rinsum = ginsum = binsum = routsum = goutsum = boutsum = rsum = gsum = bsum = 0;
            for (i = -radius; i <= radius; i++) {
                p = pix[yi + Math.min(wm, Math.max(i, 0))];
                sir = stack[i + radius];
                sir[0] = (p & 0xff0000) >> 16;
                sir[1] = (p & 0x00ff00) >> 8;
                sir[2] = (p & 0x0000ff);
                rbs = r1 - Math.abs(i);
                rsum += sir[0] * rbs;
                gsum += sir[1] * rbs;
                bsum += sir[2] * rbs;
                if (i > 0) {
                    rinsum += sir[0];
                    ginsum += sir[1];
                    binsum += sir[2];
                } else {
                    routsum += sir[0];
                    goutsum += sir[1];
                    boutsum += sir[2];
                }
            }
            stackpointer = radius;

            for (x = 0; x < w; x++) {

                r[yi] = dv[rsum];
                g[yi] = dv[gsum];
                b[yi] = dv[bsum];

                rsum -= routsum;
                gsum -= goutsum;
                bsum -= boutsum;

                stackstart = stackpointer - radius + div;
                sir = stack[stackstart % div];

                routsum -= sir[0];
                goutsum -= sir[1];
                boutsum -= sir[2];

                if (y == 0) {
                    vmin[x] = Math.min(x + radius + 1, wm);
                }
                p = pix[yw + vmin[x]];

                sir[0] = (p & 0xff0000) >> 16;
                sir[1] = (p & 0x00ff00) >> 8;
                sir[2] = (p & 0x0000ff);

                rinsum += sir[0];
                ginsum += sir[1];
                binsum += sir[2];

                rsum += rinsum;
                gsum += ginsum;
                bsum += binsum;

                stackpointer = (stackpointer + 1) % div;
                sir = stack[(stackpointer) % div];

                routsum += sir[0];
                goutsum += sir[1];
                boutsum += sir[2];

                rinsum -= sir[0];
                ginsum -= sir[1];
                binsum -= sir[2];

                yi++;
            }
            yw += w;
        }
        for (x = 0; x < w; x++) {
            rinsum = ginsum = binsum = routsum = goutsum = boutsum = rsum = gsum = bsum = 0;
            yp = -radius * w;
            for (i = -radius; i <= radius; i++) {
                yi = Math.max(0, yp) + x;

                sir = stack[i + radius];

                sir[0] = r[yi];
                sir[1] = g[yi];
                sir[2] = b[yi];

                rbs = r1 - Math.abs(i);

                rsum += r[yi] * rbs;
                gsum += g[yi] * rbs;
                bsum += b[yi] * rbs;

                if (i > 0) {
                    rinsum += sir[0];
                    ginsum += sir[1];
                    binsum += sir[2];
                } else {
                    routsum += sir[0];
                    goutsum += sir[1];
                    boutsum += sir[2];
                }

                if (i < hm) {
                    yp += w;
                }
            }
            yi = x;
            stackpointer = radius;
            for (y = 0; y < h; y++) {
                // Preserve alpha channel: ( 0xff000000 & pix[yi] )
                pix[yi] = (0xff000000 & pix[yi]) | (dv[rsum] << 16) | (dv[gsum] << 8) | dv[bsum];

                rsum -= routsum;
                gsum -= goutsum;
                bsum -= boutsum;

                stackstart = stackpointer - radius + div;
                sir = stack[stackstart % div];

                routsum -= sir[0];
                goutsum -= sir[1];
                boutsum -= sir[2];

                if (x == 0) {
                    vmin[y] = Math.min(y + r1, hm) * w;
                }
                p = x + vmin[y];

                sir[0] = r[p];
                sir[1] = g[p];
                sir[2] = b[p];

                rinsum += sir[0];
                ginsum += sir[1];
                binsum += sir[2];

                rsum += rinsum;
                gsum += ginsum;
                bsum += binsum;

                stackpointer = (stackpointer + 1) % div;
                sir = stack[stackpointer];

                routsum += sir[0];
                goutsum += sir[1];
                boutsum += sir[2];

                rinsum -= sir[0];
                ginsum -= sir[1];
                binsum -= sir[2];

                yi += w;
            }
        }
        bitmap.setPixels(pix, 0, w, 0, 0, w, h);
        log.debug("blur end");
        
        /*Bitmap foreground=BitmapFactory.decodeResource(getResources(), R.drawable.bg_detail_overlying);
		Bitmap newBitmap=toConformBitmap(bitmap  ,foreground );//合成新图
        saveBitmapToDisk(newBitmap);*/
        log.debug("saveBitmapToDisk end");
        return (bitmap);
    }

    private Bitmap getBitmapFormDisk() {
        log.debug("getBitmapFormDisk");
        String path = getFilePath();
        File f = new File(path);
        if (f != null && f.exists()) {
            try {
                return BitmapFactory.decodeFile(path);
            } catch (Throwable e) {
                return null;
            }
        } else {
            return null;
        }
    }

    private void saveBitmapToDisk(Bitmap bitmap) {
        if (bitmap != null) {
            String path = getFilePath();
            File f = new File(path);
            FileOutputStream out = null;
            try {
                if (!f.exists()) {
                    f.createNewFile();
                }
                out = new FileOutputStream(f);
                bitmap.compress(CompressFormat.PNG, 100, out);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (out != null) {
                    try {
                        out.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    private String getFilePath() {
        String path = EnvironmentTool.getExternalDataPath("blur")+ hashCodeKey(mTagObject) + ".blur";
        return path;
    }

    private String hashCodeKey(Object data) {
        if (data instanceof String) {
            return String.valueOf(data.hashCode());
        }
        return String.valueOf(data);
    }

    public void releaseBitmapObject() {
        //先释放本身的Bitmap
        //（本身的Bitmap对象由ImageCache类进行管理，无需手动释放）
//		Drawable self_drawable = getDrawable();
//		if(self_drawable instanceof LayerDrawable){
//			LayerDrawable layer_drawable  = (LayerDrawable) self_drawable;
//			int self_layer_num = layer_drawable.getNumberOfLayers();
//			Drawable self_last = layer_drawable.getDrawable(self_layer_num - 1);
//			if(self_last instanceof BitmapDrawable){
//				Bitmap bitmap = (Bitmap)((BitmapDrawable) self_last).getBitmap();
//				if(bitmap != null && bitmap != mFilterBitmap && !bitmap.isRecycled()){
//					bitmap.recycle();
//				}
//			}
//		}

        //再释放BlurView的bitmap
        Drawable blur_dra = mBlurView.getBackground();
        if (blur_dra instanceof LayerDrawable) {
            LayerDrawable blur_drawable = (TransitionDrawable) blur_dra;
            Drawable blur_last = blur_drawable.getDrawable(blur_drawable.getNumberOfLayers() - 1);
            if (blur_last instanceof MyBitmapDrawable) {
                Bitmap blur_bitmap = ((MyBitmapDrawable) blur_last).getBitmap();
                if (blur_bitmap != null && !blur_bitmap.isRecycled()) {
                    blur_bitmap.recycle();
                    blur_bitmap = null;
                }
            }
        }
    }

    private Bitmap big(Bitmap bitmap, float widthScale, float heightScale) {
        Matrix matrix = new Matrix();
        matrix.postScale(widthScale, heightScale); //长和宽放大缩小的比例
        Bitmap resizeBmp = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
        return resizeBmp;
    }

    private Bitmap toConformBitmap(Bitmap foreground, Bitmap background) {
        if (background == null) {
            return null;
        }

        int bgWidth = background.getWidth();
        int bgHeight = background.getHeight();

        foreground = big(foreground, bgWidth / (foreground.getWidth() + 0.0f), bgHeight / (foreground.getHeight() + 0.0f));
        //int fgWidth = foreground.getWidth();   
        //int fgHeight = foreground.getHeight();   
        //create the new blank bitmap 创建一个新的和SRC长度宽度一样的位图    
        Bitmap newbmp = Bitmap.createBitmap(bgWidth, bgHeight, Config.ARGB_8888);
        Canvas cv = new Canvas(newbmp);

        cv.drawBitmap(foreground, 0, 0, null);//在 0，0坐标开始画入fg ，可以从任意位置画入
        //draw bg into   
        cv.drawBitmap(background, 0, 0, null);//在 0，0坐标开始画入bg   
        //draw fg into   
        //save all clip   
        cv.save(Canvas.ALL_SAVE_FLAG);//保存   
        //store   
        cv.restore();//存储   
        return newbmp;
    }
}
