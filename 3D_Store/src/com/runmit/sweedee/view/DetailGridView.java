package com.runmit.sweedee.view;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.GridView;

public class DetailGridView extends GridView {
	
	private boolean hasMove = false;
	
    public DetailGridView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public DetailGridView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public DetailGridView(Context context) {
        super(context);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        switch (ev.getAction()) {
		case MotionEvent.ACTION_DOWN:
			hasMove = false;
			break;
		case MotionEvent.ACTION_MOVE:
			hasMove = true;
			break;
		default:
			break;
		}
    	return super.onTouchEvent(ev);
    }
    
    @Override
    public boolean performItemClick(View view, int position, long id) {
    	if(!hasMove){
    		return super.performItemClick(view, position, id);
    	}else{
    		return true;
    	}
    }
    
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int expandSpec = MeasureSpec.makeMeasureSpec(Integer.MAX_VALUE >> 2, MeasureSpec.AT_MOST);
        super.onMeasure(widthMeasureSpec, expandSpec);
    }
}
