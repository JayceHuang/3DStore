/*
 * Copyright (C) 2011 Patrik Akerfeldt
 * Copyright (C) 2011 Jake Wharton
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.runmit.sweedee.view;

import java.util.ArrayList;
import java.util.List;

import android.animation.ArgbEvaluator;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.view.View;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.XL_Log;

/**
 * Draws circles (one for each view). The current view position is filled and
 * others are only stroked.
 */
public class CirclePageIndicator extends View implements PageIndicator {

	XL_Log log = new XL_Log(CirclePageIndicator.class);

	private float mRadius;

	private ViewPager mViewPager;

	private ViewPager.OnPageChangeListener mListener;

	private int mCurrentPage;

	private int mScrollState;

	int defaultFillColor = 0;

	int defaultCommonColor = 0;

	private List<CircleItem> mCircleItemList;

	private ArgbEvaluator mColorEvaluator;

	private int realCount = 0;

	AnimatorUpdateListener mAnimatorUpdateListener = new AnimatorUpdateListener() {

		@Override
		public void onAnimationUpdate(ValueAnimator animation) {
			invalidate();
		}
	};

	public CirclePageIndicator(Context context) {
		this(context, null);
	}

	public CirclePageIndicator(Context context, AttributeSet attrs) {
		this(context, attrs, R.attr.vpiCirclePageIndicatorStyle);
	}

	public CirclePageIndicator(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		if (isInEditMode())
			return;

		final Resources res = getResources();
		defaultFillColor = res.getColor(R.color.default_circle_indicator_fill_color);
		defaultCommonColor = res.getColor(R.color.default_circle_indicator_stroke_color);
		final float defaultRadius = res.getDimension(R.dimen.default_circle_indicator_radius);
		TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.CirclePageIndicator, defStyle, 0);
		mRadius = a.getDimension(R.styleable.CirclePageIndicator_radius, defaultRadius);
		a.recycle();
		mColorEvaluator = new ArgbEvaluator();
	}

	public void setRadius(float radius) {
		mRadius = radius;
		invalidate();
	}

	public float getRadius() {
		return mRadius;
	}

	@Override
	protected void onDraw(Canvas canvas) {
		super.onDraw(canvas);
		if (mCircleItemList == null || mCircleItemList.size() == 0) {
			return;
		}
		for (int i = 0, size = mCircleItemList.size(); i < size; i++) {
			mCircleItemList.get(i).draw(canvas, mRadius);
		}
	}

	public void setViewPager(ViewPager view) {
		if (mViewPager == view) {
			return;
		}
		if (mViewPager != null) {
			mViewPager.setOnPageChangeListener(null);
		}
		if (view.getAdapter() == null) {
			throw new IllegalStateException("ViewPager does not have adapter instance.");
		}
		mViewPager = view;
		mViewPager.setOnPageChangeListener(this);

		mCircleItemList = new ArrayList<CircleItem>();
		float dX;
		float dY;

		int longPaddingBefore = getPaddingLeft();
		int shortPaddingBefore = getPaddingTop();

		final float threeRadius = mRadius * 3;
		final float shortOffset = shortPaddingBefore + mRadius;
		
		float longOffset = longPaddingBefore + mRadius;
		realCount = ((RecyclingPagerAdapter)mViewPager.getAdapter()).getRealCount();

		for (int iLoop = 0, size = realCount; iLoop < size; iLoop++) {
			float drawLong = longOffset + (iLoop * threeRadius);
			dX = drawLong;
			dY = shortOffset;
			
			CircleItem mItem = new CircleItem(iLoop, dX, dY);
			mItem.setBackgroundColor(iLoop == 0 ? defaultFillColor : defaultCommonColor);
			mCircleItemList.add(mItem);
		}
		invalidate();
	}

	@Override
	public void setViewPager(ViewPager view, int initialPosition) {
		setViewPager(view);
		setCurrentItem(initialPosition);
	}

	@Override
	public void setCurrentItem(int item) {
		if (mViewPager == null) {
			throw new IllegalStateException("ViewPager has not been bound.");
		}
		mViewPager.setCurrentItem(item);
		mCurrentPage = item;
		invalidate();
	}

	@Override
	public void notifyDataSetChanged() {
		invalidate();
	}

	@Override
	public void onPageScrollStateChanged(int state) {
		mScrollState = state;
		switch (state) {
		case ViewPager.SCROLL_STATE_IDLE:// 停止
			break;
		case ViewPager.SCROLL_STATE_DRAGGING:// 滑动
			break;
		case ViewPager.SCROLL_STATE_SETTLING://
			break;
		default:
			break;
		}
		if (mListener != null) {
			mListener.onPageScrollStateChanged(state);
		}
	}

	@Override
	public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
		Integer color = (Integer) mColorEvaluator.evaluate(positionOffset, defaultFillColor, defaultCommonColor);
		mCircleItemList.get(position % realCount).setBackgroundColor(color);
		color = (Integer) mColorEvaluator.evaluate(positionOffset, defaultCommonColor, defaultFillColor);
		mCircleItemList.get((position + 1) % realCount).setBackgroundColor(color);

		invalidate();
		mCurrentPage = position;
		if (mListener != null) {
			mListener.onPageScrolled(position, positionOffset, positionOffsetPixels);
		}
	}

	@Override
	public void onPageSelected(int position) {
		if (mScrollState == ViewPager.SCROLL_STATE_IDLE) {
			mCurrentPage = position;
			invalidate();
		}
		if (mListener != null) {
			mListener.onPageSelected(position);
		}
	}

	@Override
	public void setOnPageChangeListener(ViewPager.OnPageChangeListener listener) {
		mListener = listener;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.view.View#onMeasure(int, int)
	 */
	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		setMeasuredDimension(measureLong(widthMeasureSpec), measureShort(heightMeasureSpec));
	}

	/**
	 * Determines the width of this view
	 * 
	 * @param measureSpec
	 *            A measureSpec packed into an int
	 * @return The width of the view, honoring constraints from measureSpec
	 */
	private int measureLong(int measureSpec) {
		int result;
		int specMode = MeasureSpec.getMode(measureSpec);
		int specSize = MeasureSpec.getSize(measureSpec);
		log.debug("specMode="+specMode+",specSize="+specSize+",mViewPager="+mViewPager);
		if ((specMode == MeasureSpec.EXACTLY) || (mViewPager == null)) {
			// We were told how big to be
			result = specSize;
		} else {
			// Calculate the width according the views count
			final int count = realCount;
			result = (int) (getPaddingLeft() + getPaddingRight() + (count * 2 * mRadius) + (count - 1) * mRadius + 1);
			// Respect AT_MOST value if that was what is called for by
			// measureSpec
			if (specMode == MeasureSpec.AT_MOST) {
				result = Math.min(result, specSize);
			}
		}
		return result;
	}

	/**
	 * Determines the height of this view
	 * 
	 * @param measureSpec
	 *            A measureSpec packed into an int
	 * @return The height of the view, honoring constraints from measureSpec
	 */
	private int measureShort(int measureSpec) {
		int result;
		int specMode = MeasureSpec.getMode(measureSpec);
		int specSize = MeasureSpec.getSize(measureSpec);

		if (specMode == MeasureSpec.EXACTLY) {
			// We were told how big to be
			result = specSize;
		} else {
			// Measure the height
			result = (int) (2 * mRadius + getPaddingTop() + getPaddingBottom() + 1);
			// Respect AT_MOST value if that was what is called for by
			// measureSpec
			if (specMode == MeasureSpec.AT_MOST) {
				result = Math.min(result, specSize);
			}
		}
		return result;
	}

	@Override
	public void onRestoreInstanceState(Parcelable state) {
		SavedState savedState = (SavedState) state;
		super.onRestoreInstanceState(savedState.getSuperState());
		mCurrentPage = savedState.currentPage;
		requestLayout();
	}

	@Override
	public Parcelable onSaveInstanceState() {
		Parcelable superState = super.onSaveInstanceState();
		SavedState savedState = new SavedState(superState);
		savedState.currentPage = mCurrentPage;
		return savedState;
	}

	static class SavedState extends BaseSavedState {
		int currentPage;

		public SavedState(Parcelable superState) {
			super(superState);
		}

		private SavedState(Parcel in) {
			super(in);
			currentPage = in.readInt();
		}

		@Override
		public void writeToParcel(Parcel dest, int flags) {
			super.writeToParcel(dest, flags);
			dest.writeInt(currentPage);
		}

		public static final Parcelable.Creator<SavedState> CREATOR = new Parcelable.Creator<SavedState>() {
			@Override
			public SavedState createFromParcel(Parcel in) {
				return new SavedState(in);
			}

			@Override
			public SavedState[] newArray(int size) {
				return new SavedState[size];
			}
		};
	}

	class CircleItem {
		float dx;
		float dy;
		int backgroundColor;
		Paint mPaint;
		int id;

		CircleItem(int id, float x, float y) {
			this.dx = x;
			this.dy = y;
			this.id = id;

			mPaint = new Paint();
			mPaint.setAntiAlias(true);
			mPaint.setStyle(Style.FILL);
		}

		void draw(Canvas canvas, float mRadius) {
			mPaint.setColor(backgroundColor);
			canvas.drawCircle(dx, dy, mRadius, mPaint);
		}

		public int getBackgroundColor() {
			return backgroundColor;
		}

		public void setBackgroundColor(int color) {
			this.backgroundColor = color;
		}
	}
}
