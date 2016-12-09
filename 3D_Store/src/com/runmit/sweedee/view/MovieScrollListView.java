package com.runmit.sweedee.view;

import com.runmit.sweedee.util.XL_Log;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.AbsListView;
import android.widget.ListView;

public class MovieScrollListView extends ListView {

    XL_Log log = new XL_Log(MovieScrollListView.class);
    /**
     * 下滑的状态
     */
    private final int DOWNREFRESH = 1;

    /**
     * 上滑的状态
     */
    private final int UPREFRESH = 0;

    /**
     * 初始状态下第一个 条目
     */
    private int startfirstItemIndex;

    /**
     * 初始状态下最后一个 条目
     */
    private int startlastItemIndex;

    /**
     * 滑动后的第一个条目
     */
    private int endfirstItemIndex;

    /**
     * 滑动后的最后一个条目
     */
    private int endlastItemIndex;

    public MovieScrollListView(Context context) {
        this(context, null);
    }

    public MovieScrollListView(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.listViewStyle);
    }

    public MovieScrollListView(final Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        super.setOnScrollListener(new MyScrollHelper());
        setOverScrollMode(View.OVER_SCROLL_NEVER);
    }

    class MyScrollHelper implements OnScrollListener {

        private boolean scrollFlag = false;// 标记是否滑动

        @Override
        public void onScrollStateChanged(AbsListView view, int scrollState) {
            if (scrollState == OnScrollListener.SCROLL_STATE_TOUCH_SCROLL) {
                scrollFlag = true;
            } else {
                scrollFlag = false;
            }
        }

        @Override
        public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount) {
            View childView;
            startfirstItemIndex = firstVisibleItem;
            startlastItemIndex = firstVisibleItem + visibleItemCount - 1;
            log.debug("firstVisibleItem：：" + firstVisibleItem + ":visibleItemCount:" + visibleItemCount + ":totalItemCount:" + totalItemCount);
            for (int i = 0; i < getChildCount(); i++) {
                childView = getChildAt(i);
                if (null != childView) {
                    // 只对第一个view进行动画
                    if (i == 0) {
                        StartAnim(childView);
                    } else {
                        EndAnim(childView);
                    }
                }
            }

            // 判断向下或者向上滑动了
            if ((endfirstItemIndex > startfirstItemIndex) && (endfirstItemIndex > 0)) {
                log.debug("DOWNREFRESH");
            } else if ((endlastItemIndex < startlastItemIndex) && (endlastItemIndex > 0)) {
                log.debug("UPREFRESH");
            }

            endfirstItemIndex = startfirstItemIndex;
            endlastItemIndex = startlastItemIndex;
        }
    }

    public void StartAnim(View childView) {
        float childTop = childView.getTop();
        float childHeight = childView.getHeight();
        // 让第一个view Y轴偏移,永远保留在顶部
        childView.setTranslationY(-childTop);
        //往上移动的时候给予1-0.5的渐变
        float rotation = Math.abs(((float) childTop) / childHeight);
        childView.setAlpha(1 - 0.5f * Math.abs(rotation));
        childView.setPivotX(childView.getWidth() / 2);
        childView.setPivotY(childView.getHeight());
        // 根据滑动比例进行角度倾斜 最终角度为5度
        childView.setRotationX(rotation * 90f * 0.06f);
        float scale = 1 - 0.1f * (Math.abs(((float) childTop) / childHeight));
        // 缩小的效果
        childView.setScaleX(scale);
        childView.setScaleY(scale);
    }

    public void EndAnim(View childView) {
        if(childView.getAlpha()!=1.0f){
            childView.setAlpha(1.0f);
        }
        if (childView.getRotationX() != 0.0f) {
            childView.setRotationX(0.0f);
        }
        if (childView.getTranslationY() != 0.0f) {
            childView.setTranslationY(0.0f);
        }
        if (childView.getScaleX() != 1.0f || childView.getScaleY() != 1.0f) {
            childView.setScaleX(1.0f);
            childView.setScaleY(1.0f);
        }
    }

}
