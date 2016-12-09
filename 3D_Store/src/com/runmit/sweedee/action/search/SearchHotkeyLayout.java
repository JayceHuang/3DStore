package com.runmit.sweedee.action.search;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.Constant;

import java.util.ArrayList;

public class SearchHotkeyLayout extends ViewGroup {
    ArrayList<TextView> mTvList;
    private Paint mPaint;
    int padding = (int) (24 * Constant.DENSITY);
    private IOnTextItemClickListener onClickHandler;

    public SearchHotkeyLayout(Context context) {
        super(context);
        init(context);
    }

    public SearchHotkeyLayout(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SearchHotkeyLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        mPaint.setColor(getContext().getResources().getColor(R.color.default_circle_indicator_fill_color));
        canvas.drawCircle(0, 0, 100, mPaint);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        int i=0;
        int preChildWidth = 0;
        for (View child : mTvList) {
            // Log.d("mzh", "left:" + (padding * i + l) + " right:" + r + "  width:" + (r - l) + "  heigh:" + (b - t));
            if(padding * i + l + preChildWidth + child.getMeasuredWidth() > r)break;
            child.layout(padding * i + l + preChildWidth, 0, r - l, b - t);
            preChildWidth += child.getMeasuredWidth();
            i++;
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        for (View child : mTvList)
            child.measure(widthMeasureSpec, heightMeasureSpec);
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
        if ((specMode == MeasureSpec.EXACTLY) || (mTvList == null)) {
            // We were told how big to be
            result = specSize;
        } else {
            // Calculate the width according the views count
            result = getPaddingLeft() + getPaddingRight();
            for (TextView tv : mTvList){
                result += tv.getMeasuredWidth() + padding;
            }
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
            result = getPaddingTop() + getPaddingBottom();
            int height = 0;
            for (TextView tv : mTvList){
                if( height < tv.getMeasuredHeight()){
                    height = tv.getMeasuredHeight();
                }
            }
            result += height;
            // Respect AT_MOST value if that was what is called for by
            // measureSpec
            if (specMode == MeasureSpec.AT_MOST) {
                result = Math.min(result, specSize);
            }
        }
        return result;
    }

    private void init(Context context) {
        mTvList = new ArrayList<TextView>();
        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setStyle(Paint.Style.FILL);
    }

    public void addTextChild(TextView v){
        mTvList.add(v);
        addView(v);
    }

    public void setDataList(Context context, ArrayList<String> dataList){
        for (String data : dataList){
            TextView tv = new TextView(context);
            tv.setText(data);
            tv.setTextSize(14);
            tv.setTextColor(context.getResources().getColor(R.color.textview_deep_black));
            tv.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    TextView tv = (TextView)view;
                    onClickHandler.onTextItemClick(""+tv.getText());
                }
            });
            addTextChild(tv);
            invalidate();
        }
    }

    public void setOnTextItemClickListener(IOnTextItemClickListener handler){
        this.onClickHandler = handler;
    }

    public interface IOnTextItemClickListener {
        public void onTextItemClick(String string);
    }

}
