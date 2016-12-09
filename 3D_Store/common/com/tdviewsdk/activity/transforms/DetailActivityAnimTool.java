package com.tdviewsdk.activity.transforms;

import android.app.Activity;
import android.content.Intent;

import com.tdviewsdk.activity.transforms.BaseEffect.ReverseListener;

/**
 * @author Sven.Zhan
 * 工具类
 */
public class DetailActivityAnimTool {
	
    private static BaseEffect mEffect = new ZoomOutEffect();

    public static void setEffect(BaseEffect effect) {
        mEffect = effect;
    }

    public static void startActivity(AnimActivity currActivity, Intent intent) {
    	float x = currActivity.getLastClickX();
    	float y = currActivity.getLastClickY();
    	mEffect.setClickPoint(x, y);       
        currActivity.startActivity(intent);
        currActivity.overridePendingTransition(0, 0);
    }

    public static void animate(Activity destActivity) {
        mEffect.animate(destActivity);
    }

    public static void prepareAnimation(final Activity destActivity) {
        mEffect.prepareAnimation(destActivity);
    }
    
    public static void reverseAnimation(ReverseListener rel){
    	mEffect.reverse(rel);
    }

}
