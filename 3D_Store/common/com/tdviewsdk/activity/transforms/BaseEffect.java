package com.tdviewsdk.activity.transforms;

import android.app.Activity;

/*** 
 * @author Sven.Zhan
 * 动画效果的抽象类，需要扩展的动画实现该类
 */
public abstract class BaseEffect {
	
	/**动画持续时长*/
	protected int duration = 500;
	
    public abstract void prepareAnimation(Activity destActivity);

    public abstract void animate(Activity destActivity);
    
    public abstract void reverse(ReverseListener rel);
    
    public abstract void setClickPoint(float x,float y);
    
    public interface ReverseListener{
    	public void onReversed();
    }
}
