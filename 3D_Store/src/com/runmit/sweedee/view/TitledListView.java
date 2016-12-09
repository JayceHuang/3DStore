package com.runmit.sweedee.view;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.XL_Log;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ListAdapter;
import android.widget.TextView;

public class TitledListView extends MyListView {

    private XL_Log log = new XL_Log(TitledListView.class);

    private View mTitle;
    //private SmartContextualAdapter adapter;

    public TitledListView(Context context) {
        super(context);
    }

    public TitledListView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public TitledListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        if (!getVisiable()) return;
        if (mTitle != null) {
            measureChild(mTitle, widthMeasureSpec, heightMeasureSpec);
        }
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        //log.error("tlv onLayout  visiable = "+visiable+" isRefresh = "+isRefresh);
        //if (!getVisiable())return;
        if (mTitle != null) {
            if (getVisiable()) {
                mTitle.layout(0, 0, mTitle.getMeasuredWidth(), mTitle.getMeasuredHeight());
            } else {
                mTitle.layout(0, 0, 0, 0);
                //mTitle.setVisibility(View.GONE);
            }

        }
    }

    protected int getChildDrawHeight() {
        if (mTitle != null) {
            return mTitle.getHeight();
        }
        return 0;

    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        super.dispatchDraw(canvas);
        if (!getVisiable()) return;
        //log.error("mTitle = "+mTitle+" this = "+this);
        if (mTitle != null) {
            drawChild(canvas, mTitle, getDrawingTime());
        }
    }

    @Override
    public void setAdapter(ListAdapter adapter) {
        super.setAdapter(adapter);
        LayoutInflater inflater = LayoutInflater.from(getContext());
        mTitle = inflater.inflate(R.layout.vod_lastplay_list_title, this, false);
        //adapter = (SmartContextualAdapter) adapter;
    }

    public void moveTitle() {
        if (!getVisiable()) return;
        View bottomChild = getChildAt(0);
        if (bottomChild != null) {
            //log.error("bottomChild0 = "+bottomChild+" bottomChild1 = "+getChildAt(1));
            //log.error("moveTitle title = "+((TextView) mTitle.findViewById(R.id.tv_date)).getText());
            int bottom = bottomChild.getBottom();
            int height = mTitle.getMeasuredHeight();
            int y = 0;
            if (bottom < height) {
                y = bottom - height;
            }
            mTitle.layout(0, y, mTitle.getMeasuredWidth(), mTitle.getMeasuredHeight() + y);

        }
    }

    public void updateTitle(String title) {
        //log.debug("updateTitle bottomChild0 = "+getChildAt(0)+" bottomChild1 = "+getChildAt(1));
        if (!getVisiable()) return;
        //log.error("updateTitle title = "+((TextView) mTitle.findViewById(R.id.tv_date)).getText());
        if (title != null) {
            TextView title_text = (TextView) mTitle.findViewById(R.id.tv_date);
            title_text.setText(title);
        }
        mTitle.layout(0, 0, mTitle.getMeasuredWidth(), mTitle.getMeasuredHeight());
    }

    public boolean getVisiable() {
        return state == State.PULL_TO_REFRESH;
    }
}
