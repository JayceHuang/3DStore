package com.runmit.sweedee.view;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.View;

import com.runmit.sweedee.R;

public class TriangleLineView extends View {

	private Paint mPaint;
	
	private final int defalultLineColor = Color.WHITE;
	
	private Path mPath;
	
	private int mTriangleWidth;
	
	private int mTriangleHeight;
	
	private final float line_height;
	
	private static final int DEFINE_SIZE = 4;
	
	private int screenWidth;

	public TriangleLineView(Context context) {
		this(context, null);
	}

	public TriangleLineView(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
	}

	public TriangleLineView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);

		final float density = getResources().getDisplayMetrics().density;
		screenWidth = getResources().getDisplayMetrics().widthPixels;
		TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.HomePageIndicator);
		int indicatorColor = ta.getColor(R.styleable.HomePageIndicator_lineColor, defalultLineColor);
		mTriangleWidth = ta.getDimensionPixelSize(R.styleable.HomePageIndicator_triangleWidth, (int) (16 * density));
		mTriangleHeight = ta.getDimensionPixelSize(R.styleable.HomePageIndicator_triangleHeight, (int) (9 * density));
		line_height = ta.getDimensionPixelSize(R.styleable.HomePageIndicator_lineHeight, (int) (1 * density));
		ta.recycle();

		mPaint = new Paint();
		mPaint.setAntiAlias(true);
		mPaint.setColor(indicatorColor);
		mPaint.setStyle(Paint.Style.STROKE);
		mPaint.setStrokeWidth(line_height);
	}
	
	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		int height = (int) (mTriangleHeight + line_height);
		setMeasuredDimension(screenWidth, height);
	}

	@Override
	protected void onDraw(Canvas canvas) {
		super.onDraw(canvas);
		canvas.save();
		canvas.translate(0, mTriangleHeight);
		canvas.drawPath(mPath, mPaint);
		canvas.restore();
	}

	public void initTriangle(int position) {
		if (mPath == null) {
			mPath = new Path();
		} else {
			mPath.reset();
		}
		mPath.moveTo(0, 0);
		int tabWidth = screenWidth / DEFINE_SIZE;
		int mTranslationX = (int) (tabWidth * position);
		int startX = mTranslationX + tabWidth / 2 - mTriangleWidth / 2;
		mPath.lineTo(startX, 0);
		mPath.lineTo(startX + mTriangleWidth / 2, -mTriangleHeight);
		mPath.lineTo(startX + mTriangleWidth, 0);
		mPath.lineTo(screenWidth, 0);
	}

}
