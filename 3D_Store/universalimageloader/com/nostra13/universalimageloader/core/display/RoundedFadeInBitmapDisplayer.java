package com.nostra13.universalimageloader.core.display;

import com.nostra13.universalimageloader.core.assist.LoadedFrom;
import com.nostra13.universalimageloader.core.imageaware.ImageAware;

import android.graphics.Bitmap;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.DecelerateInterpolator;
import android.widget.ImageView;

/***
 * @author Sven.Zhan
 * 兼具圆角和淡入显示两种特性
 */
public class RoundedFadeInBitmapDisplayer extends RoundedBitmapDisplayer {

	private final int durationMillis;
	
	private final boolean animateFromNetwork = true;
	private final boolean animateFromDisk = true;
	private final boolean animateFromMemory = false;
	
	public RoundedFadeInBitmapDisplayer(int cornerRadiusPixels ,int duration) {
		super(cornerRadiusPixels);
		durationMillis = duration;
	}
	
	@Override
	public void display(Bitmap bitmap, ImageAware imageAware, LoadedFrom loadedFrom) {
		super.display(bitmap, imageAware, loadedFrom);
		if ((animateFromNetwork && loadedFrom == LoadedFrom.NETWORK) ||(animateFromDisk && loadedFrom == LoadedFrom.DISC_CACHE) 
				||(animateFromMemory && loadedFrom == LoadedFrom.MEMORY_CACHE)) {
			animate(imageAware.getWrappedView(), durationMillis);
		}
	}	

	/**
	 * Animates {@link ImageView} with "fade-in" effect
	 *
	 * @param imageView      {@link ImageView} which display image in
	 * @param durationMillis The length of the animation in milliseconds
	 */
	public static void animate(View imageView, int durationMillis) {
		if (imageView != null) {
			AlphaAnimation fadeImage = new AlphaAnimation(0, 1);
			fadeImage.setDuration(durationMillis);
			fadeImage.setInterpolator(new DecelerateInterpolator());
			imageView.startAnimation(fadeImage);
		}
	}

}
