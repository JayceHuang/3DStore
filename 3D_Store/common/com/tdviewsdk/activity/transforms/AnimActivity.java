package com.tdviewsdk.activity.transforms;

import android.support.v4.app.FragmentActivity;
import android.util.Log;
import android.view.MotionEvent;

import com.tdviewsdk.activity.transforms.BaseEffect.ReverseListener;
import com.umeng.analytics.MobclickAgent;

/**
 * @author Sven.Zhan
 * 
 */
public class AnimActivity extends FragmentActivity {

	private float lastClickX = 0;
	
	private float lastClickY = 0;

	@Override
	public boolean dispatchTouchEvent(MotionEvent ev) {
		int action = ev.getAction();
		if (action == MotionEvent.ACTION_UP) {
			lastClickX = ev.getX();
			lastClickY = ev.getY();
			Log.d("AnimActivity", "lastClickX = "+lastClickX+" lastClickY = "+lastClickY);
		}
		return super.dispatchTouchEvent(ev);
	}

	public float getLastClickX() {
		return lastClickX;
	}

	public float getLastClickY() {
		return lastClickY;
	}

	protected boolean isFinishAnimEnable(){
		return false;
	}
	
	@Override
	public void finish() {
		if(isFinishAnimEnable()){
			DetailActivityAnimTool.reverseAnimation(new ReverseListener() {
				@Override
				public void onReversed() {
					realFinish();
				}
			});	
		}else{
			realFinish();
		}
	}

	public void realFinish() {
		this.overridePendingTransition(0, 0);
		super.finish();
	}

    @Override
    protected void onResume() {
        super.onResume();
        MobclickAgent.onResume(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        MobclickAgent.onPause(this);
    }

}
