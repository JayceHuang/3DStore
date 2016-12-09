//package com.runmit.sweedee.view;
//
//import java.util.Locale;
//
//import android.content.Context;
//import android.content.res.TypedArray;
//import android.graphics.Canvas;
//import android.graphics.Color;
//import android.graphics.Paint;
//import android.graphics.Paint.Style;
//import android.graphics.Typeface;
//import android.os.Parcel;
//import android.os.Parcelable;
//import android.support.v4.view.ViewPager;
//import android.support.v4.view.ViewPager.OnPageChangeListener;
//import android.util.AttributeSet;
//import android.util.TypedValue;
//import android.view.Gravity;
//import android.view.View;
//import android.view.ViewTreeObserver.OnGlobalLayoutListener;
//import android.widget.FrameLayout;
//import android.widget.LinearLayout;
//import android.widget.TextView;
//
//import com.runmit.sweedee.R;
//
///**
// * 为首页而生
// * 
// */
//public class HomeScrollTabStrip extends FrameLayout {
//
//	private LinearLayout.LayoutParams expandedTabLayoutParams;
//
//	private final PageListener pageListener = new PageListener();
//	
//	public OnPageChangeListener delegatePageListener;
//
//	private LinearLayout tabsContainer;
//	private ViewPager pager;
//
//	private int tabCount;
//
//	private int currentPosition = 0;
//	private int selectedPosition = 0;
//	private float currentPositionOffset = 0f;
//
//	private Paint rectPaint;
//
//	private int indicatorColor = 0xFF666666;
//
//	private boolean textAllCaps = false;
//
//	private int tabTextSize = 12;
//	private int tabTextColor = Color.WHITE;
//	private int selectedTabTextColor = 0xFF666666;
//	private Typeface tabTypeface = null;
//	private int tabTypefaceStyle = Typeface.NORMAL;
//
//	private int tabBackgroundResId = R.drawable.background_tab;
//	
//	private OnTabClickListener mOnTabClickListener;
//
//	public void setOnTabClickListener(OnTabClickListener mOnTabClickListener) {
//		this.mOnTabClickListener = mOnTabClickListener;
//	}
//
//	private Locale locale;
//
//	public HomeScrollTabStrip(Context context) {
//		this(context, null);
//	}
//
//	public HomeScrollTabStrip(Context context, AttributeSet attrs) {
//		this(context, attrs, 0);
//	}
//
//	public HomeScrollTabStrip(Context context, AttributeSet attrs, int defStyle) {
//		super(context, attrs, defStyle);
//		setWillNotDraw(false);
//
//		tabsContainer = new LinearLayout(context);
//		tabsContainer.setOrientation(LinearLayout.HORIZONTAL);
//		tabsContainer.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
//		addView(tabsContainer);
//
//		// get custom attrs
//		TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.PagerSlidingTabStrip);
//		indicatorColor = a.getColor(R.styleable.PagerSlidingTabStrip_pstsIndicatorColor, indicatorColor);//背景色
//
//		// tab文字选中时的颜色,默认和滑动指示器的颜色一致
//		selectedTabTextColor = a.getColor(R.styleable.PagerSlidingTabStrip_selectedTabTextColor, indicatorColor);
//		tabTextColor = a.getColor(R.styleable.PagerSlidingTabStrip_textColor, tabTextColor);
//		tabTextSize = a.getDimensionPixelSize(R.styleable.PagerSlidingTabStrip_textSize, tabTextSize);
//
//		tabBackgroundResId = a.getResourceId(R.styleable.PagerSlidingTabStrip_pstsTabBackground, tabBackgroundResId);
//		textAllCaps = a.getBoolean(R.styleable.PagerSlidingTabStrip_pstsTextAllCaps, textAllCaps);
//
//		a.recycle();
//
//		rectPaint = new Paint();
//		rectPaint.setAntiAlias(true);
//		rectPaint.setStyle(Style.FILL);
//		rectPaint.setColor(indicatorColor);
//		expandedTabLayoutParams = new LinearLayout.LayoutParams(0, LayoutParams.MATCH_PARENT, 1.0f);
//		if (locale == null) {
//			locale = getResources().getConfiguration().locale;
//		}
//	}
//
//	public void setViewPager(ViewPager pager) {
//		this.pager = pager;
//
//		if (pager.getAdapter() == null) {
//			throw new IllegalStateException("ViewPager does not have adapter instance.");
//		}
//
//		pager.setOnPageChangeListener(pageListener);
//
//		notifyDataSetChanged();
//	}
//
//	public void setOnPageChangeListener(OnPageChangeListener listener) {
//		this.delegatePageListener = listener;
//	}
//
//	public void notifyDataSetChanged() {
//		tabsContainer.removeAllViews();
//		tabCount = pager.getAdapter().getCount();
//		for (int i = 0; i < tabCount; i++) {
//			addTextTab(i, pager.getAdapter().getPageTitle(i).toString());
//		}
//
//		updateTabStyles();
//
//		getViewTreeObserver().addOnGlobalLayoutListener(new OnGlobalLayoutListener() {
//
//			@Override
//			public void onGlobalLayout() {
//				getViewTreeObserver().removeGlobalOnLayoutListener(this);
//				currentPosition = pager.getCurrentItem();
//			}
//		});
//
//	}
//
//	private void addTextTab(final int position, String title) {
//		TextView tab = new TextView(getContext());
//		tab.setText(title);
//		tab.setGravity(Gravity.CENTER);
//		tab.setSingleLine();
//		addTab(position, tab);
//	}
//
//	private void addTab(final int position, View tab) {
//		tab.setFocusable(true);
//		tab.setOnClickListener(new OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				mOnTabClickListener.onclick(position);
//			}
//		});
//		tabsContainer.addView(tab, position,expandedTabLayoutParams);
//	}
//
//	private void updateTabStyles() {
//		for (int i = 0; i < tabCount; i++) {
//			View v = tabsContainer.getChildAt(i);
//			v.setBackgroundResource(tabBackgroundResId);
//			if (v instanceof TextView) {
//				TextView tab = (TextView) v;
//				tab.setTextSize(TypedValue.COMPLEX_UNIT_PX, tabTextSize);
//				tab.setTypeface(tabTypeface, tabTypefaceStyle);
//				tab.setTextColor(tabTextColor);
//				if (textAllCaps) {
//					tab.setAllCaps(true);
//				}
//				if (i == selectedPosition) {
//					tab.setTextColor(selectedTabTextColor);
//				}else{
//					tab.setTextColor(tabTextColor);
//				}
//			}
//		}
//	}
//
//	@Override
//	protected void onDraw(Canvas canvas) {
//		if (isInEditMode() || tabCount == 0) {
//			return;
//		}
//		final int height = getHeight();
//		View currentTab = tabsContainer.getChildAt(currentPosition);
//		float lineLeft = currentTab.getLeft();
//		float lineRight = currentTab.getRight();
//
//		if (currentPositionOffset > 0f && currentPosition < tabCount - 1) {
//			View nextTab = tabsContainer.getChildAt(currentPosition + 1);
//			final float nextTabLeft = nextTab.getLeft();
//			final float nextTabRight = nextTab.getRight();
//			
//			lineLeft = (currentPositionOffset * nextTabLeft + (1f - currentPositionOffset) * lineLeft);
//			lineRight = (currentPositionOffset * nextTabRight + (1f - currentPositionOffset) * lineRight);
//		}
//		canvas.drawRect(lineLeft, 0, lineRight, height, rectPaint);
//		
//		super.onDraw(canvas);
//	}
//
//	private class PageListener implements OnPageChangeListener {
//
//		@Override
//		public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
//			currentPosition = position;
//			currentPositionOffset = positionOffset;
//			invalidate();
//			if (delegatePageListener != null) {
//				delegatePageListener.onPageScrolled(position, positionOffset, positionOffsetPixels);
//			}
//		}
//
//		@Override
//		public void onPageScrollStateChanged(int state) {
//			if (delegatePageListener != null) {
//				delegatePageListener.onPageScrollStateChanged(state);
//			}
//		}
//
//		@Override
//		public void onPageSelected(int position) {
//			selectedPosition = position;
//			updateTabStyles();
//			if (delegatePageListener != null) {
//				delegatePageListener.onPageSelected(position);
//			}
//		}
//	}
//
//	public void setIndicatorColor(int indicatorColor) {
//		this.indicatorColor = indicatorColor;
//
//		invalidate();
//	}
//
//	/**
//	 * 设置滚动条字体颜色
//	 * 
//	 * @param resId
//	 */
//	public void setIndicatorColorResource(int resId) {
//		this.indicatorColor = getResources().getColor(resId);
//
//		invalidate();
//	}
//
//	public boolean isTextAllCaps() {
//		return textAllCaps;
//	}
//
//	public void setAllCaps(boolean textAllCaps) {
//		this.textAllCaps = textAllCaps;
//	}
//
//	/**
//	 * 设置tab文字的大小
//	 * 
//	 * @param textSizePx
//	 */
//	public void setTextSize(int textSizePx) {
//		this.tabTextSize = textSizePx;
//		updateTabStyles();
//	}
//
//	public int getTextSize() {
//		return tabTextSize;
//	}
//
//	public void setTextColor(int textColor) {
//		this.tabTextColor = textColor;
//		updateTabStyles();
//	}
//
//	public void setTextColorResource(int resId) {
//		this.tabTextColor = getResources().getColor(resId);
//		updateTabStyles();
//	}
//
//	public int getTextColor() {
//		return tabTextColor;
//	}
//
//	public void setSelectedTextColor(int textColor) {
//		this.selectedTabTextColor = textColor;
//		updateTabStyles();
//	}
//
//	/**
//	 * 设置选中的item字体颜色
//	 * 
//	 * @param resId
//	 */
//	public void setSelectedTextColorResource(int resId) {
//		this.selectedTabTextColor = getResources().getColor(resId);
//		updateTabStyles();
//	}
//
//	public int getSelectedTextColor() {
//		return selectedTabTextColor;
//	}
//
//	public void setTypeface(Typeface typeface, int style) {
//		this.tabTypeface = typeface;
//		this.tabTypefaceStyle = style;
//		updateTabStyles();
//	}
//
//	public void setTabBackground(int resId) {
//		this.tabBackgroundResId = resId;
//		updateTabStyles();
//	}
//
//	public int getTabBackground() {
//		return tabBackgroundResId;
//	}
//
//	@Override
//	public void onRestoreInstanceState(Parcelable state) {
//		SavedState savedState = (SavedState) state;
//		super.onRestoreInstanceState(savedState.getSuperState());
//		currentPosition = savedState.currentPosition;
//		requestLayout();
//	}
//
//	@Override
//	public Parcelable onSaveInstanceState() {
//		Parcelable superState = super.onSaveInstanceState();
//		SavedState savedState = new SavedState(superState);
//		savedState.currentPosition = currentPosition;
//		return savedState;
//	}
//
//	static class SavedState extends BaseSavedState {
//		int currentPosition;
//
//		public SavedState(Parcelable superState) {
//			super(superState);
//		}
//
//		private SavedState(Parcel in) {
//			super(in);
//			currentPosition = in.readInt();
//		}
//
//		@Override
//		public void writeToParcel(Parcel dest, int flags) {
//			super.writeToParcel(dest, flags);
//			dest.writeInt(currentPosition);
//		}
//
//		public static final Parcelable.Creator<SavedState> CREATOR = new Parcelable.Creator<SavedState>() {
//			@Override
//			public SavedState createFromParcel(Parcel in) {
//				return new SavedState(in);
//			}
//
//			@Override
//			public SavedState[] newArray(int size) {
//				return new SavedState[size];
//			}
//		};
//	}
//	
//	public interface OnTabClickListener{
//		void onclick(int pisition);
//	}
//}
