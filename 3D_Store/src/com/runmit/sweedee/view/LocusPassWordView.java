package com.runmit.sweedee.view;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.XL_Log;

import android.content.Context;
//import android.content.SharedPreferences;
//import android.content.SharedPreferences.Editor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Bitmap.Config;
import android.graphics.PorterDuff.Mode;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;


/**
 * 轨迹密码
 */
public class LocusPassWordView extends View {

    XL_Log log = new XL_Log(LocusPassWordView.class);

    private float w = 0;
    private float h = 0;

    //
    private boolean isCache = false;
    //
    private Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    //
    private Point[][] mPoints = new Point[3][3];
    // 圆的半径
    private float r = 0;
    // 选中的点
    private List<Point> sPoints = new ArrayList<Point>();
    private boolean checking = false;
    private Bitmap locus_round_original;
    private Bitmap locus_round_click;
    private Bitmap locus_round_click_error;
    private Bitmap locus_line;
    private Bitmap locus_line_semicircle;
    private Bitmap locus_line_semicircle_error;
    private Bitmap locus_arrow;
    private Bitmap locus_line_error;
    private long CLEAR_TIME = 800;
    private int passwordMinLength = 4;
    private boolean isTouch = true; // 是否可操作
    private Matrix mMatrix = new Matrix();
    private int lineAlpha = 100;

    public LocusPassWordView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public LocusPassWordView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public LocusPassWordView(Context context) {
        super(context);
    }

    @Override
    public void onDraw(Canvas canvas) {
        if (!isCache) {
            initCache();
        }
        drawToCanvas(canvas);
    }

    private void drawToCanvas(Canvas canvas) {

        // mPaint.setColor(Color.RED);
        // Point p1 = mPoints[1][1];
        // Rect r1 = new Rect(p1.x - r,p1.y - r,p1.x +
        // locus_round_click.getWidth() - r,p1.y+locus_round_click.getHeight()-
        // r);
        // canvas.drawRect(r1, mPaint);
        // 画所有点
        for (int i = 0; i < mPoints.length; i++) {
            for (int j = 0; j < mPoints[i].length; j++) {
                Point p = mPoints[i][j];
                if (p.state == Point.STATE_CHECK) {
                    canvas.drawBitmap(locus_round_click, p.x - r, p.y - r, mPaint);
                } else if (p.state == Point.STATE_CHECK_ERROR) {
                    canvas.drawBitmap(locus_round_click_error, p.x - r, p.y - r, mPaint);
                } else {
                    canvas.drawBitmap(locus_round_original, p.x - r, p.y - r, mPaint);
                }
            }
        }
        // mPaint.setColor(Color.BLUE);
        // canvas.drawLine(r1.left+r1.width()/2, r1.top, r1.left+r1.width()/2,
        // r1.bottom, mPaint);
        // canvas.drawLine(r1.left, r1.top+r1.height()/2, r1.right,
        // r1.bottom-r1.height()/2, mPaint);

        // 画连线
        if (sPoints.size() > 0) {
            int tmpAlpha = mPaint.getAlpha();
            mPaint.setAlpha(lineAlpha);
            Point tp = sPoints.get(0);
            for (int i = 1; i < sPoints.size(); i++) {
                Point p = sPoints.get(i);
                drawLine(canvas, tp, p);
                tp = p;
            }
            if (this.movingNoPoint) {
                drawLine(canvas, tp, new Point((int) moveingX, (int) moveingY));
            }
            mPaint.setAlpha(tmpAlpha);
            lineAlpha = mPaint.getAlpha();
        }

    }

    /**
     * 初始化Cache信息
     *
     * @param canvas
     */
    private void initCache() {

        w = this.getWidth();
        h = this.getHeight();
        log.debug("initCache w = " + w + " h = " + h);
        float x = 0;
        float y = 0;

        // 以最小的为准
        // 纵屏
        if (w > h) {
//			//云播项目中都按w<=h处理
//			x = (w - h) / 2;
//			w = h;
            y = (h - w) / 2;
            w = h;
            if (y < 0) y = 0;
        }
        // 横屏
        else {
            y = (h - w) / 2;
            h = w;
        }

        log.debug("initCache x = " + x + " y = " + y);
        //原版逻辑
//		locus_round_original = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_round_original);
//		locus_round_click = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_round_click);
//		locus_round_click_error = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_round_click_error);
//		locus_line = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_line);
//		locus_line_semicircle = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_line_semicircle);
//		locus_line_error = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_line_error);
//		locus_line_semicircle_error = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_line_semicircle_error);

        locus_round_original = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_round_original);
        locus_round_click = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_round_click);
        locus_round_click_error = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_round_original);
        locus_line = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_line);
        locus_line_semicircle = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_arrow);
        locus_line_error = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_line);
        locus_line_semicircle_error = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_arrow);


        locus_arrow = BitmapFactory.decodeResource(this.getResources(), R.drawable.locus_arrow);
        // Log.d("Canvas w h :", "w:" + w + " h:" + h);

        // 计算圆圈图片的大小
        float canvasMinW = w;
        if (w > h) {
            canvasMinW = h;
        }
        float roundMinW = canvasMinW / 8.0f * 2;
        float roundW = roundMinW / 2.f;
        //
        float deviation = canvasMinW % (8 * 2) / 2;
        x += deviation;
        x += deviation;
        log.debug("initCache roundMinW = " + roundMinW + " roundW" + roundW);
        if (locus_round_original.getWidth() > roundMinW) {
            float sf = roundMinW * 1.0f / locus_round_original.getWidth(); // 取得缩放比例，将所有的图片进行缩放
            locus_round_original = zoom(locus_round_original, sf);
            locus_round_click = zoom(locus_round_click, sf);
            locus_round_click_error = zoom(locus_round_click_error, sf);

            locus_line = zoom(locus_line, sf);
            locus_line_semicircle = zoom(locus_line_semicircle, sf);

            locus_line_error = zoom(locus_line_error, sf);
            locus_line_semicircle_error = zoom(locus_line_semicircle_error, sf);
            locus_arrow = zoom(locus_arrow, sf);
            roundW = locus_round_original.getWidth() / 2;
        }

        mPoints[0][0] = new Point(x + 0 + roundW, y + 0 + roundW);
        mPoints[0][1] = new Point(x + w / 2, y + 0 + roundW);
        mPoints[0][2] = new Point(x + w - roundW, y + 0 + roundW);
        mPoints[1][0] = new Point(x + 0 + roundW, y + h / 2);
        mPoints[1][1] = new Point(x + w / 2, y + h / 2);
        mPoints[1][2] = new Point(x + w - roundW, y + h / 2);
        mPoints[2][0] = new Point(x + 0 + roundW, y + h - roundW);
        mPoints[2][1] = new Point(x + w / 2, y + h - roundW);
        mPoints[2][2] = new Point(x + w - roundW, y + h - roundW);
        int k = 0;
        for (Point[] ps : mPoints) {
            for (Point p : ps) {
                p.index = k;
                k++;
            }
        }
        r = locus_round_original.getHeight() / 2;// roundW;
        log.debug("initCache r = " + r);
        isCache = true;
    }

    /**
     * 画两点的连接
     *
     * @param canvas
     * @param a
     * @param b
     */
    private void drawLine(Canvas canvas, Point a, Point b) {
        float ah = (float) distance(a.x, a.y, b.x, b.y);
        float degrees = getDegrees(a, b);
        // Log.d("=============x===========", "rotate:" + degrees);
        canvas.rotate(degrees, a.x, a.y);

        if (a.state == Point.STATE_CHECK_ERROR) {
            mMatrix.setScale((ah - locus_line_semicircle_error.getWidth()) / locus_line_error.getWidth(), 1);
            mMatrix.postTranslate(a.x, a.y - locus_line_error.getHeight() / 2.0f);
            canvas.drawBitmap(locus_line_error, mMatrix, mPaint);
            canvas.drawBitmap(locus_line_semicircle_error, a.x + locus_line_error.getWidth(), a.y - locus_line_error.getHeight() / 2.0f,
                    mPaint);
        } else {
            mMatrix.setScale((ah - locus_line_semicircle.getWidth()) / locus_line.getWidth(), 1);
            mMatrix.postTranslate(a.x, a.y - locus_line.getHeight() / 2.0f);
            canvas.drawBitmap(locus_line, mMatrix, mPaint);
            canvas.drawBitmap(locus_line_semicircle, a.x + ah - locus_line_semicircle.getWidth(), a.y - locus_line.getHeight() / 2.0f,
                    mPaint);
        }

        canvas.drawBitmap(locus_arrow, a.x, a.y - locus_arrow.getHeight() / 2.0f, mPaint);

        canvas.rotate(-degrees, a.x, a.y);

    }

    public float getDegrees(Point a, Point b) {
        float ax = a.x;// a.index % 3;
        float ay = a.y;// a.index / 3;
        float bx = b.x;// b.index % 3;
        float by = b.y;// b.index / 3;
        float degrees = 0;
        if (bx == ax) // y轴相等 90度或270
        {
            if (by > ay) // 在y轴的下边 90
            {
                degrees = 90;
            } else if (by < ay) // 在y轴的上边 270
            {
                degrees = 270;
            }
        } else if (by == ay) // y轴相等 0度或180
        {
            if (bx > ax) // 在y轴的下边 90
            {
                degrees = 0;
            } else if (bx < ax) // 在y轴的上边 270
            {
                degrees = 180;
            }
        } else {
            if (bx > ax) // 在y轴的右边 270~90
            {
                if (by > ay) // 在y轴的下边 0 - 90
                {
                    degrees = 0;
                    degrees = degrees + switchDegrees(Math.abs(by - ay), Math.abs(bx - ax));
                } else if (by < ay) // 在y轴的上边 270~0
                {
                    degrees = 360;
                    degrees = degrees - switchDegrees(Math.abs(by - ay), Math.abs(bx - ax));
                }

            } else if (bx < ax) // 在y轴的左边 90~270
            {
                if (by > ay) // 在y轴的下边 180 ~ 270
                {
                    degrees = 90;
                    degrees = degrees + switchDegrees(Math.abs(bx - ax), Math.abs(by - ay));
                } else if (by < ay) // 在y轴的上边 90 ~ 180
                {
                    degrees = 270;
                    degrees = degrees - switchDegrees(Math.abs(bx - ax), Math.abs(by - ay));
                }

            }

        }
        return degrees;
    }

    /**
     * 1=30度 2=45度 4=60度
     *
     * @param tan
     * @return
     */
    private float switchDegrees(float x, float y) {
        return (float) pointTotoDegrees(x, y);
    }

    /**
     * 取得数组下标
     *
     * @param index
     * @return
     */
    public int[] getArrayIndex(int index) {
        int[] ai = new int[2];
        ai[0] = index / 3;
        ai[1] = index % 3;
        return ai;
    }

    /**
     * 检查
     *
     * @param x
     * @param y
     * @return
     */
    private Point checkSelectPoint(float x, float y) {
        for (int i = 0; i < mPoints.length; i++) {
            for (int j = 0; j < mPoints[i].length; j++) {
                Point p = mPoints[i][j];
                if (p != null) {
                    if (checkInRound(p.x, p.y, r, (int) x, (int) y)) {
                        return p;
                    }
                }
            }
        }
        return null;
    }

    /**
     * 重置
     */
    private void reset() {
        for (Point p : sPoints) {
            p.state = Point.STATE_NORMAL;
        }
        sPoints.clear();
        this.enableTouch();
    }

    /**
     * 判断点是否有交叉 返回 0,新点 ,1 与上一点重叠 2,与非最后一点重叠
     *
     * @param p
     * @return
     */
    private int crossPoint(Point p) {
        // 重叠的不最后一个则 reset
        if (sPoints.contains(p)) {
            if (sPoints.size() > 2) {
                // 与非最后一点重叠
                if (sPoints.get(sPoints.size() - 1).index != p.index) {
                    return 2;
                }
            }
            return 1; // 与最后一点重叠
        } else {
            return 0; // 新点
        }
    }

    /**
     * 添加一个点
     *
     * @param point
     */
    private void addPoint(Point point) {
        this.sPoints.add(point);
    }

    /**
     * 转换为String
     *
     * @param points
     * @return
     */
    private String toPointString() {
        if (sPoints.size() >= passwordMinLength) {
            StringBuffer sf = new StringBuffer();
            for (Point p : sPoints) {
                sf.append(",");
                sf.append(p.index);
            }
            return sf.deleteCharAt(0).toString();
        } else {
            return "";
        }
    }

    boolean movingNoPoint = false;
    float moveingX, moveingY;

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // 不可操作
        if (!isTouch) {
            return false;
        }

        movingNoPoint = false;

        float ex = event.getX();
        float ey = event.getY();
        boolean isFinish = false;
        boolean redraw = false;
        Point p = null;
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN: // 点下
                // 如果正在清除密码,则取消
                if (task != null) {
                    task.cancel();
                    task = null;
                    Log.d("task", "touch cancel()");
                }
                // 删除之前的点
                reset();
                p = checkSelectPoint(ex, ey);
                if (p != null) {
                    checking = true;
                }
                break;
            case MotionEvent.ACTION_MOVE: // 移动
                if (checking) {
                    p = checkSelectPoint(ex, ey);
                    if (p == null) {
                        movingNoPoint = true;
                        moveingX = ex;
                        moveingY = ey;
                    }
                }
                break;
            case MotionEvent.ACTION_UP: // 提起
                p = checkSelectPoint(ex, ey);
                checking = false;
                isFinish = true;
                break;
        }
        if (!isFinish && checking && p != null) {

            int rk = crossPoint(p);
            if (rk == 2) // 与非最后一重叠
            {
                // reset();
                // checking = false;

                movingNoPoint = true;
                moveingX = ex;
                moveingY = ey;

                redraw = true;
            } else if (rk == 0) // 一个新点
            {
                p.state = Point.STATE_CHECK;
                addPoint(p);
                redraw = true;
            }
            // rk == 1 不处理

        }

        // 是否重画
        if (redraw) {

        }
        if (isFinish) {
            if (this.sPoints.size() <= 1) {
                this.reset();
            }
            //太短的输入交给外部判断
//			else if (this.sPoints.size() < passwordMinLength && this.sPoints.size() > 0)
//			{
//				// mCompleteListener.onPasswordTooMin(sPoints.size());
//				error();
//				clearPassword();
//				Toast.makeText(this.getContext(), "密码太短,请重新输入!", Toast.LENGTH_SHORT).show();
//			}
            else if (mCompleteListener != null) {
                this.disableTouch();
                mCompleteListener.onComplete(toPointString(), this.sPoints.size());
//				if (this.sPoints.size() >= passwordMinLength)
//				{
//					this.disableTouch();
//					mCompleteListener.onComplete(toPointString());
//				}

            }
        }
        this.postInvalidate();
        return true;
    }

    /**
     * 设置已经选中的为错误
     */
    public void error() {
        for (Point p : sPoints) {
            p.state = Point.STATE_CHECK_ERROR;
        }
    }

    /**
     * 设置为输入错误
     */
    public void markError() {
        markError(CLEAR_TIME);
    }

    /**
     * 设置为输入错误
     */
    public void markError(final long time) {
        for (Point p : sPoints) {
            p.state = Point.STATE_CHECK_ERROR;
        }
        this.clearPassword(time);
    }

    /**
     * 设置为可操作
     */
    public void enableTouch() {
        isTouch = true;
    }

    /**
     * 设置为不可操作
     */
    public void disableTouch() {
        isTouch = false;
    }

    private Timer timer = new Timer();
    private TimerTask task = null;

    /**
     * 清除密码
     */
    public void clearPassword() {
        clearPassword(CLEAR_TIME);
    }

    /**
     * 清除密码
     */
    public void clearPassword(final long time) {
        if (time > 1) {
            if (task != null) {
                task.cancel();
                Log.d("task", "clearPassword cancel()");
            }
            lineAlpha = 130;
            postInvalidate();
            task = new TimerTask() {
                public void run() {
                    reset();
                    postInvalidate();
                }
            };
            Log.d("task", "clearPassword schedule(" + time + ")");
            timer.schedule(task, time);
        } else {
            reset();
            postInvalidate();
        }

    }

    //
    private OnCompleteListener mCompleteListener;

    /**
     * @param mCompleteListener
     */
    public void setOnCompleteListener(OnCompleteListener mCompleteListener) {
        this.mCompleteListener = mCompleteListener;
    }

//	/**
//	 * 取得密码
//	 * 
//	 * @return
//	 */
//	private String getPassword()
//	{
//		SharedPreferences settings = this.getContext().getSharedPreferences(this.getClass().getName(), 0);
//		return settings.getString("password",""); //, "0,1,2,3,4,5,6,7,8"
//	}
//
//	/**
//	 * 密码是否为空
//	 * @return
//	 */
//	public boolean isPasswordEmpty()
//	{
//		return isEmpty(getPassword());
//	}
//	
//	public boolean verifyPassword(String password)
//	{
//		boolean verify = false;
//		if (isNotEmpty(password))
//		{
//			// 或者是超级密码
//			if (password.equals(getPassword()) || password.equals("0,2,8,6,3,1,5,7,4"))
//			{
//				verify = true;
//			}
//		}
//		return verify;
//	}
//
//	/**
//	 * 设置密码
//	 * 
//	 * @param password
//	 */
//	public void resetPassWord(String password)
//	{
//		SharedPreferences settings = this.getContext().getSharedPreferences(this.getClass().getName(), 0);
//		Editor editor = settings.edit();
//		editor.putString("password", password);
//		editor.commit();
//	}

    public int getPasswordMinLength() {
        return passwordMinLength;
    }

    public void setPasswordMinLength(int passwordMinLength) {
        this.passwordMinLength = passwordMinLength;
    }

    /**
     * 轨迹球画完成事件
     */
    public interface OnCompleteListener {
        /**
         * 画完了
         *
         * @param str
         */
        public void onComplete(String password, int points_num);
    }

    //----------------图片处理----------------------

    /**
     * 缩放图片
     *
     * @param bitmap
     * @param f
     * @return
     */
    private Bitmap zoom(Bitmap bitmap, float zf) {
        Matrix matrix = new Matrix();
        matrix.postScale(zf, zf);
        return Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
    }

    /**
     * 缩放图片
     *
     * @param bitmap
     * @param f
     * @return
     */
    private Bitmap zoom(Bitmap bitmap, float wf, float hf) {
        Matrix matrix = new Matrix();
        matrix.postScale(wf, hf);
        return Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
    }

    /**
     * 图片圆角处理
     *
     * @param bitmap
     * @param roundPX
     * @return
     */
    private Bitmap getRCB(Bitmap bitmap, float roundPX) {
        // RCB means
        // Rounded
        // Corner Bitmap
        Bitmap dstbmp = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Config.ARGB_8888);
        Canvas canvas = new Canvas(dstbmp);

        final int color = 0xff424242;
        final Paint paint = new Paint();
        final Rect rect = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());
        final RectF rectF = new RectF(rect);
        paint.setAntiAlias(true);
        canvas.drawARGB(0, 0, 0, 0);
        paint.setColor(color);
        canvas.drawRoundRect(rectF, roundPX, roundPX, paint);
        paint.setXfermode(new PorterDuffXfermode(Mode.SRC_IN));
        canvas.drawBitmap(bitmap, rect, rect, paint);
        return dstbmp;
    }
    //----------------图片处理----------------------

    //----------------数学计算处理----------------------

    /**
     * 两点间的距离
     *
     * @param x1
     * @param y1
     * @param x2
     * @param y2
     * @return
     */
    private double distance(double x1, double y1, double x2, double y2) {
        return Math.sqrt(Math.abs(x1 - x2) * Math.abs(x1 - x2) + Math.abs(y1 - y2) * Math.abs(y1 - y2));
    }

    /**
     * 计算点a(x,y)的角度
     *
     * @param x
     * @param y
     * @return
     */
    private double pointTotoDegrees(double x, double y) {
        return Math.toDegrees(Math.atan2(x, y));
    }
    //----------------数学计算处理----------------------

    /**
     * 点在圆肉
     *
     * @param sx
     * @param sy
     * @param r
     * @param x
     * @param y
     * @return
     */
    private boolean checkInRound(float sx, float sy, float r, float x, float y) {
        return Math.sqrt((sx - x) * (sx - x) + (sy - y) * (sy - y)) < r;
    }

    /**
     * 是否不为空
     *
     * @param s
     * @return
     */
    public static boolean isNotEmpty(String s) {
        return s != null && !"".equals(s.trim());
    }

    /**
     * 是否为空
     *
     * @param s
     * @return
     */
    public static boolean isEmpty(String s) {
        return s == null || "".equals(s.trim());
    }

    /**
     * 通过{n},格式化.
     *
     * @param src
     * @param objects
     * @return
     */
    private String format(String src, Object... objects) {
        int k = 0;
        for (Object obj : objects) {
            src = src.replace("{" + k + "}", obj.toString());
            k++;
        }
        return src;
    }

    static class Point {
        public static int STATE_NORMAL = 0; // 未选中
        public static int STATE_CHECK = 1; // 选中 e
        public static int STATE_CHECK_ERROR = 2; // 已经选中,但是输错误

        public float x;
        public float y;
        public int state = 0;
        public int index = 0;// 下标

        public Point() {

        }

        public Point(float x, float y) {
            this.x = x;
            this.y = y;
        }

    }

}
