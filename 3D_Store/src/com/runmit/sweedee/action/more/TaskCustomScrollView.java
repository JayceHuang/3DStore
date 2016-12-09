package com.runmit.sweedee.action.more;

import android.content.Context;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.widget.ScrollView;

public class TaskCustomScrollView extends ScrollView {

	public TaskCustomScrollView(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
	}
	
	@Override
	 protected int computeScrollDeltaToGetChildRectOnScreen(Rect rect) {
		return 0;
	 }
}
