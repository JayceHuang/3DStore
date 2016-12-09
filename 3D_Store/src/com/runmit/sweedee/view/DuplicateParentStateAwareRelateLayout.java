package com.runmit.sweedee.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

public class DuplicateParentStateAwareRelateLayout extends RelativeLayout {

    public DuplicateParentStateAwareRelateLayout(Context context) {
        super(context);
    }

    public DuplicateParentStateAwareRelateLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public DuplicateParentStateAwareRelateLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /*
     * By default ViewGroup call setPressed on each child view, this take into account duplicateparentstate parameter
     */
    @Override
    protected void dispatchSetPressed(boolean pressed) {
        for (int i = 0; i < getChildCount(); i++) {
            View child = getChildAt(i);
            if (child.isDuplicateParentStateEnabled()) {
                getChildAt(i).setPressed(pressed);
            }
        }
    }
}
