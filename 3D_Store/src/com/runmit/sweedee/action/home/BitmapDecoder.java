///**
// * BitmapDecoder.java 
// * com.xunlei.cloud.action.resource.BitmapDecoder
// * @author: zhang_zhi
// * @date: 2013-7-10 下午2:23:09
// */
//package com.runmit.sweedee.action.home;
//
//import java.io.IOException;
//import java.io.InputStream;
//import java.net.HttpURLConnection;
//import java.net.MalformedURLException;
//import java.net.URL;
//
//import android.content.Context;
//import android.content.res.Resources;
//import android.graphics.Bitmap;
//import android.graphics.Bitmap.Config;
//import android.graphics.BitmapFactory;
//import android.graphics.Canvas;
//import android.graphics.Color;
//import android.graphics.Matrix;
//import android.graphics.Paint;
//import android.graphics.PorterDuff.Mode;
//import android.graphics.PorterDuffXfermode;
//import android.graphics.Rect;
//import android.graphics.RectF;
//
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.model.MoiveCarousel;
//import com.runmit.sweedee.util.Constant;
//import com.runmit.sweedee.util.XL_Log;
//
///**
// * @author zhang_zhi
// *         实现的主要功能。
// *         <p/>
// *         修改记录：修改者，修改日期，修改内容
// */
//public class BitmapDecoder {
//
//    XL_Log log = new XL_Log(BitmapDecoder.class);
//
//    public static int text_background_height = 12;
//
//    private int round_cornor_px;//圆角图半径
//
//    private final Rect rect;
//
//    private final RectF rectF;
//
//    private final int small_img_width;
//
//    private final int small_img_height;
//
//    public BitmapDecoder(Context c) {
//
//        Resources mResources = c.getResources();
//        round_cornor_px = c.getResources().getDimensionPixelSize(R.dimen.round_cornor_px);
//        small_img_width = mResources.getDimensionPixelSize(R.dimen.resource_small_width);
//        small_img_height = mResources.getDimensionPixelSize(R.dimen.resource_small_height);
//        int px = (int) (2 * Constant.DENSITY);
//        if (px < 4) {
//            px = 4;
//        }
//        rect = new Rect(0, 0, small_img_width - px, small_img_height - px);
//        rectF = new RectF(rect);
//    }
//
//    public Bitmap decodeBitmap(MoiveCarousel mResourceInfo, int w, int h) {
//        return decodeBitmap(mResourceInfo.getPoster(), w, h);
//    }
//
//
//    public Bitmap decodeBitmap(String url, int w, int h) {
//        Bitmap mNativeBitmap = getInSizeBitmapByUrl(url, w - 4, h - 4);
//        if (mNativeBitmap != null) {
//            Bitmap decodebitmap = getRoundedCornerBitmap(mNativeBitmap);
//            if (decodebitmap != mNativeBitmap) {
//                mNativeBitmap.recycle();
//                mNativeBitmap = null;
//            } else {
//                log.debug("mNativeBitmap=" + mNativeBitmap + ",decodebitmap=" + decodebitmap);
//            }
//
//            return decodebitmap;
//        }
//
//
//        return null;
//    }
//
//    // 全网搜索专用
//    public Bitmap decodeBitmap4Search(String url, int w, int h) {
//        isFromDetail = false;
//        Bitmap mNativeBitmap = extractThumbnail(downLoadBitmap4Search(url), w - 4,
//                h - 4, OPTIONS_RECYCLE_INPUT);
//        if (mNativeBitmap != null) {
//            Bitmap decodebitmap = getRoundedCornerBitmap(mNativeBitmap);
//            if (decodebitmap != mNativeBitmap) {
//                mNativeBitmap.recycle();
//                mNativeBitmap = null;
//            } else {
//                log.debug("mNativeBitmap=" + mNativeBitmap + ",decodebitmap="
//                        + decodebitmap);
//            }
//
//            return decodebitmap;
//        }
//
//        return null;
//    }
//
//    // 全网搜索专用
//    public static Bitmap downLoadBitmap4Search(String url) {
//        URL fileUrl = null;
//        Bitmap bitmap = null;
//        HttpURLConnection conn = null;
//        InputStream is = null;
//
//        try {
//            fileUrl = new URL(url);
//        } catch (MalformedURLException e) {
//            e.printStackTrace();
//            return null;
//        }
//        try {
//            conn = (HttpURLConnection) fileUrl.openConnection();
//            conn.setConnectTimeout(4 * 1000);
//            conn.setDoInput(true);
//            conn.connect();
//            is = conn.getInputStream();
//            bitmap = BitmapFactory.decodeStream(is);
//            return bitmap;
//        } catch (IOException e) {
//            e.printStackTrace();
//        } catch (OutOfMemoryError e) {
//            e.printStackTrace();
//        } finally {
//            try {
//                if (is != null) {
//                    is.close();
//                }
//            } catch (IOException e) {
//                e.printStackTrace();
//            }
//            if (conn != null) {
//                conn.disconnect();
//            }
//        }
//        return null;
//    }
//
//
//    //生成圆角图片
//    public Bitmap getRoundedCornerBitmap(Bitmap bitmap) {
//        try {
//            final int width = bitmap.getWidth();
//            final int height = bitmap.getHeight();
//            Bitmap output = Bitmap.createBitmap(width, height, Config.ARGB_8888);
//            Canvas canvas = new Canvas(output);
//            final Paint paint = new Paint();
//
//            paint.setAntiAlias(true);
//            canvas.drawARGB(0, 0, 0, 0);
//            paint.setColor(Color.BLACK);
//            canvas.drawRoundRect(rectF, round_cornor_px, round_cornor_px, paint);
//            paint.setXfermode(new PorterDuffXfermode(Mode.SRC_IN));
//            canvas.drawBitmap(bitmap, rect, rect, paint);
//
////	        //添加头像边框阴影
////	        paint.setColor(Color.BLACK);
////	        paint.setAlpha(DEFINE_BLACK_MASKING_ALPHA);
////	        paint.setStyle(Paint.Style.STROKE);
////	        paint.setStrokeWidth(4f);
////	        canvas.drawRoundRect(rectF, round_cornor_px, round_cornor_px, paint);
//
////		    //增加白边
////	        paint.setColor(Color.WHITE);
////	        paint.setAlpha(255);
////	        paint.setStyle(Paint.Style.STROKE);
////	        paint.setStrokeWidth(2);
////	        canvas.drawRoundRect(rectF, round_cornor_px, round_cornor_px, paint);
//
////	        bitmap.recycle();
////	        bitmap=null;
//            return output;
//        } catch (Exception e) {
//            return null;
//        }
//    }
//
////	public Bitmap gettextBitmap(Bitmap bitmap,String text,final int width,final int height) { 
////		int index=text.lastIndexOf(".");
////		if(index>0 && index<text.length()){
////			text=text.substring(0, index);
////		}
////
////        Canvas canvas = new Canvas(bitmap);                   
////        final Paint paint = new Paint();  
////        paint.setAntiAlias(true);  
////        
////        paint.setStyle(Paint.Style.FILL);
////        paint.setXfermode(new PorterDuffXfermode(Mode.DARKEN));  
////        paint.setARGB(DEFINE_BLACK_MASKING_ALPHA, 0, 0, 0);
////        
////        canvas.drawRect(rectangle, paint);
////        int sc = canvas.saveLayer(0, 0, width,height, null,
////                Canvas.MATRIX_SAVE_FLAG |
////                Canvas.CLIP_SAVE_FLAG |
////                Canvas.HAS_ALPHA_LAYER_SAVE_FLAG |
////                Canvas.FULL_COLOR_LAYER_SAVE_FLAG |
////                Canvas.CLIP_TO_LAYER_SAVE_FLAG);
////        
////        paint.setXfermode(null);  
////        paint.setColor(Color.WHITE);
////        paint.setTextSize(black_masking_text_size);
////        paint.setTextAlign(Paint.Align.LEFT);
////        canvas.drawTextOnPath(text, paths, 0, 0, paint);
////        canvas.restoreToCount(sc);
////        return bitmap;  
////     
////	} 
//
//    public static Bitmap downLoadBitmap(String url) {
//        URL fileUrl = null;
//        Bitmap bitmap = null;
//        HttpURLConnection conn = null;
//        InputStream is = null;
//
//        try {
//            fileUrl = new URL(url);
//        } catch (MalformedURLException e) {
//            e.printStackTrace();
//            return null;
//        }
//        try {
//            conn = (HttpURLConnection) fileUrl.openConnection();
//            if (url.contains("xlpan.kanimg.com")) {
//                conn.setRequestProperty("Referer", "http://pad.i.vod.xunlei.com");
//            }
//            conn.setConnectTimeout(4 * 1000);
//            conn.setReadTimeout(4 * 1000);
//            conn.setDoInput(true);
//            conn.connect();
//            is = conn.getInputStream();
//            bitmap = BitmapFactory.decodeStream(is);
//            return bitmap;
//        } catch (IOException e) {
//            e.printStackTrace();
//        } catch (OutOfMemoryError e) {
//            e.printStackTrace();
//        } finally {
//            try {
//                if (is != null) {
//                    is.close();
//                }
//            } catch (IOException e) {
//                e.printStackTrace();
//            }
//            if (conn != null) {
//                conn.disconnect();
//            }
//        }
//        return null;
//    }
//
//    /**
//     * 通过url获取固定大小的图片
//     * url开http下载
//     *
//     * @param url
//     * @param width
//     * @param height
//     * @return
//     */
//    public static Bitmap getInSizeBitmapByUrl(String url, int width, int height) {
//        isFromDetail = false;
//        Bitmap bitmap = null;
//        bitmap = extractThumbnail(downLoadBitmap(url), width, height, OPTIONS_RECYCLE_INPUT);
//        return bitmap;
//    }
//
//    private static boolean isFromDetail = false;
//
//    /**
//     * 詳情頁
//     *
//     * @param url
//     * @param width
//     * @param height
//     * @return
//     */
//    public static Bitmap getMoiveDetailInSizeBitmapByUrl(String url, int width, int height) {
//        isFromDetail = true;
//        Bitmap bitmap = null;
//        bitmap = extractThumbnail(downLoadBitmap(url), width, height, OPTIONS_RECYCLE_INPUT);
//        return bitmap;
//    }
//
//    public static final int OPTIONS_RECYCLE_INPUT = 0x2;
//
//    /**
//     * Creates a centered bitmap of the desired size.
//     *
//     * @param source  original bitmap source
//     * @param width   targeted width
//     * @param height  targeted height
//     * @param options options used during thusbnail extraction
//     */
//    public static Bitmap extractThumbnail(Bitmap source, int width, int height, int options) {
//        if (source == null) {
//            return null;
//        }
//        Bitmap thumbnail = transform(source, width, height, options);
//        return thumbnail;
//    }
//
//    private static Bitmap transform(Bitmap source, int targetWidth, int targetHeight, int options) {
//        boolean recycle = (options & OPTIONS_RECYCLE_INPUT) != 0;
//
//        float bitmapWidthF = source.getWidth();
//        float bitmapHeightF = source.getHeight();
//
//        float bitmapAspect = bitmapWidthF / bitmapHeightF;
//        float viewAspect = (float) targetWidth / targetHeight;
//        Matrix scaler = new Matrix();
//        if (bitmapAspect > viewAspect) {
//            float scale = targetHeight / bitmapHeightF;
//            if (scale < .9F || scale > 1F) {
//                scaler.setScale(scale, scale);
//            } else {
//                scaler = null;
//            }
//        } else {
//            float scale = targetWidth / bitmapWidthF;
//            if (scale < .9F || scale > 1F) {
//                scaler.setScale(scale, scale);
//            } else {
//                scaler = null;
//            }
//        }
//
//        Bitmap b1;
//        if (scaler != null) {
//            b1 = Bitmap.createBitmap(source, 0, 0,
//                    source.getWidth(), source.getHeight(), scaler, true);
//        } else {
//            b1 = source;
//        }
//
//        if (recycle && b1 != source) {
//            source.recycle();
//        }
//
//        int dx1 = Math.max(0, b1.getWidth() - targetWidth);
//        int dy1 = Math.max(0, b1.getHeight() - targetHeight);
//        Bitmap b2 = null;
//        if (!isFromDetail) {
//            b2 = Bitmap.createBitmap(b1, dx1 / 2, dy1 / 2, targetWidth, targetHeight);
//        } else {
//            b2 = Bitmap.createBitmap(b1, 0, 0, targetWidth, targetHeight);
//        }
//        if (b2 != b1) {
//            if (recycle || b1 != source) {
//                b1.recycle();
//            }
//        }
//
//        return b2;
//    }
//}
