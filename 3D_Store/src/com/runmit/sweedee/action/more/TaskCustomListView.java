package com.runmit.sweedee.action.more;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ListView;

public class TaskCustomListView extends ListView {

	public TaskCustomListView(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
	}

	@Override  
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {  
	    int expandSpec = MeasureSpec.makeMeasureSpec(Integer.MAX_VALUE >> 2, MeasureSpec.AT_MOST);  
	    super.onMeasure(widthMeasureSpec, expandSpec);  
	} 
}
