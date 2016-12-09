package com.runmit.sweedee.action.search;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.view.View;

public class Activity3DEffectTool {

	public static Bitmap bgViewBmp;
	public static boolean isLaunching = false;
	public static void startActivity(Activity currActivity, Intent intent) {
		if(isLaunching)return;
		isLaunching = true;
		currActivity.startActivity(intent);
        currActivity.overridePendingTransition(0, 0);
	}
	
	public static Bitmap takeScreenshot(View view) {
		if (view.getWidth() <= 0 || view.getHeight() <= 0) {
			return null;
		}
		view.setDrawingCacheEnabled(true);
		Bitmap bitmap = Bitmap.createBitmap(view.getDrawingCache()); 
		view.setDrawingCacheEnabled(false);
		return bitmap;
	}

	public static Bitmap getCurrentBmp() {
		return bgViewBmp;
	}
}
