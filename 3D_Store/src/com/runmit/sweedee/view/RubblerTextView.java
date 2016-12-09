//package com.runmit.sweedee.view;
//
//import java.util.HashSet;
//import java.util.Iterator;
//import java.util.Set;
//
//import android.content.Context;
//import android.graphics.Bitmap;
//import android.graphics.Bitmap.Config;
//import android.graphics.BitmapFactory;
//import android.graphics.Canvas;
//import android.graphics.Color;
//import android.graphics.Paint;
//import android.graphics.Path;
//import android.graphics.PorterDuff;
//import android.graphics.PorterDuff.Mode;
//import android.graphics.PorterDuffXfermode;
//import android.graphics.Rect;
//import android.graphics.RectF;
//import android.media.ThumbnailUtils;
//import android.text.Layout;
//import android.util.AttributeSet;
//import android.view.MotionEvent;
//import android.widget.TextView;
//
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.util.XL_Log;
//
///**
// * 橡皮檫功能，类似刮刮乐效果
// */
//public class RubblerTextView extends TextView {
//
//
//    private XL_Log log = new XL_Log(RubblerTextView.class);
//
//    private float TOUCH_TOLERANCE = 1.0f;        //填充距离，使线条更自然，柔和,值越小，越柔和。
//    private int PaintStrokeWidth = 10;
//
//    private Bitmap mBitmap;
//    private Canvas mCanvas;
//    private Paint mPaint;
//    private Path mPath;
//    private float mX, mY;
//
//    private boolean isDraw = false;
//
//    private OnRubblerFinish listener;
//
//    Rect mRectText = null;//字符串所在的矩阵
//    /**
//     * 将上面的mRectText分割成2*6的小的矩阵，
//     * 如果按下的点落在某一个小的矩阵内，
//     * 则remove掉，当mRectList的size小于某一阈值，则判断可见
//     */
//    private Set<Rect> mRectList = new HashSet<Rect>();
//
//    private boolean isCallback = false;
//
//    public void setOnRubblerFinishListener(OnRubblerFinish listener) {
//        this.listener = listener;
//    }
//
//    public RubblerTextView(Context context) {
//        super(context);
//
//    }
//
//    public RubblerTextView(Context context, AttributeSet attrs, int defStyle) {
//        super(context, attrs, defStyle);
//    }
//
//    public RubblerTextView(Context context, AttributeSet attrs) {
//        super(context, attrs);
//    }
//
//    @Override
//    protected void onDraw(Canvas canvas) {
//        super.onDraw(canvas);
//        if (isDraw) {
//            mCanvas.drawPath(mPath, mPaint);
//            canvas.drawBitmap(mBitmap, 0, 0, null);
//        }
//
//    }
//
//    /**
//     * 开启檫除功能
//     *
//     * @param bgColor          覆盖的背景颜色
//     * @param paintStrokeWidth 触点（橡皮）宽度
//     * @param touchTolerance   填充距离,值越小，越柔和。
//     */
//    public void beginRubbler() {
//        //设置画笔
//        mPaint = new Paint();
//        //画笔划过的痕迹就变成透明色了
//        mPaint.setColor(Color.BLACK);//此处不能为透明色
//        mPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DST_OUT));
//
//        mPaint.setAntiAlias(true);
//        mPaint.setDither(true);
//        mPaint.setStyle(Paint.Style.STROKE);
//        mPaint.setStrokeJoin(Paint.Join.ROUND);        //前圆角
//        mPaint.setStrokeCap(Paint.Cap.ROUND);        //后圆角
//        mPaint.setStrokeWidth(PaintStrokeWidth);    //笔宽
//        //痕迹
//        mPath = new Path();
//        ;
//
//        Bitmap b = BitmapFactory.decodeResource(getResources(), R.drawable.scratch_ui_background);
//        int mesure_width = getMeasuredWidth();
//        b = ThumbnailUtils.extractThumbnail(b, mesure_width, getMeasuredHeight(), ThumbnailUtils.OPTIONS_RECYCLE_INPUT);
//        mBitmap = getRoundedCornerBitmap(b);
//        if (mBitmap != b) {
//            b.recycle();
//            b = null;
//        }
//        mCanvas = new Canvas(mBitmap);
//        isDraw = true;
//        invalidate();
//
//        Rect bounds = new Rect();
//        getLineBounds(0, bounds);
//
//        Layout layout = getLayout();
//        int curLeft = (int) layout.getLineLeft(0);
//        int w = (int) layout.getLineWidth(0);
//        mRectText = new Rect(curLeft, bounds.top, curLeft + w, bounds.bottom);
//        int dy = (mRectText.bottom - mRectText.top) / 2;
//        int dx = (mRectText.right - mRectText.left) / 6;
//        for (int x = 0; x < 6; x++) {
//            for (int y = 0; y < 2; y++) {
//                mRectList.add(new Rect(mRectText.left + dx * x, mRectText.top + dy * y, mRectText.left + dx * (x + 1), mRectText.top + dy * (y + 1)));
//            }
//        }
//    }
//
//    private Bitmap getRoundedCornerBitmap(Bitmap bitmap) {
//        try {
//            final int width = bitmap.getWidth();
//            final int height = bitmap.getHeight();
//
//            Rect rect = new Rect(0, 0, width - 4, height - 4);
//            RectF rectF = new RectF(rect);
//            float round_cornor_px = getResources().getDimensionPixelSize(R.dimen.round_cornor_px);
//
//            Bitmap output = Bitmap.createBitmap(width, height, Config.ARGB_8888);
//            Canvas canvas = new Canvas(output);
//            final Paint paint = new Paint();
//            paint.setAntiAlias(true);
//            canvas.drawARGB(0, 0, 0, 0);
//            paint.setColor(Color.BLACK);
//            canvas.drawRoundRect(rectF, round_cornor_px, round_cornor_px, paint);
//            paint.setXfermode(new PorterDuffXfermode(Mode.SRC_IN));
//            canvas.drawBitmap(bitmap, rect, rect, paint);
//            return output;
//        } catch (Exception e) {
//            return null;
//        }
//    }
//
//    @Override
//    public boolean onTouchEvent(MotionEvent event) {
//        if (!isDraw) {
//            return true;
//        }
//        switch (event.getAction()) {
//            case MotionEvent.ACTION_DOWN:    //触点按下
//                touchDown(event.getX(), event.getY());
//                invalidate();
//                break;
//            case MotionEvent.ACTION_MOVE:    //触点移动
//                touchMove(event.getX(), event.getY());
//                invalidate();
//                break;
//            case MotionEvent.ACTION_UP:        //触点弹起
//                touchUp(event.getX(), event.getY());
//                invalidate();
//                break;
//            default:
//                break;
//        }
//        return true;
//    }
//
//
//    private void touchDown(float x, float y) {
//        mPath.reset();
//        mPath.moveTo(x, y);
//        mX = x;
//        mY = y;
//
//
//    }
//
//    private void touchMove(float x, float y) {
//        float dx = Math.abs(x - mX);
//        float dy = Math.abs(y - mY);
//        int int_x = (int) x;
//        int int_y = (int) y;
//        if (mRectText.contains(int_x, int_y)) {
//            Iterator<Rect> it = mRectList.iterator();
//            while (it.hasNext()) {
//                Rect temp = it.next();
//                if (temp.contains(int_x, int_y)) {
//                    it.remove();
//                    break;
//                }
//            }
//        }
//        if (dx >= TOUCH_TOLERANCE || dy >= TOUCH_TOLERANCE) {
//            mPath.quadTo(mX, mY, (x + mX) / 2, (y + mY) / 2);
//            mX = x;
//            mY = y;
//        }
//
//    }
//
//    private void touchUp(float x, float y) {
//        mPath.lineTo(x, y);
//        mCanvas.drawPath(mPath, mPaint);
//        mPath.reset();
//        if (mRectList.size() < 6) {
//            log.debug("onFinish");
//            if (listener != null && !isCallback) {
//                isCallback = true;
//                listener.onFinish();
//            }
//        }
//    }
//
//    /**
//     * 设置当前刮刮卡是否允许用户操作
//     * <p>方法详述（简单方法可不必详述）</p>
//     *
//     * @param enable
//     */
//    public void setIsEnable(boolean enable) {
//        isDraw = enable;
//    }
//
//    public boolean canExit() {
//        return mRectList.size() > 10;
//    }
//
//    /**
//     * @author zhang_zhi
//     *         刮刮卡回调监听，用来监听呱呱卡用户是否能感知到已经刮出字
//     *         <p/>
//     *         修改记录：修改者，修改日期，修改内容
//     */
//    public interface OnRubblerFinish {
//        void onFinish();
//    }
//
//    public void onDestroy() {
//        if (mBitmap != null && !mBitmap.isRecycled()) {
//            mBitmap.recycle();
//            mBitmap = null;
//        }
//    }
//}
