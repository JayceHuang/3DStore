package com.runmit.sweedee.imageloaderex;

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.DisplayImageOptions.Builder;
import com.nostra13.universalimageloader.core.assist.ImageScaleType;
import com.nostra13.universalimageloader.core.display.BitmapDisplayer;
import com.nostra13.universalimageloader.core.display.RoundedFadeInBitmapDisplayer;
import com.nostra13.universalimageloader.core.display.SimpleBitmapDisplayer;

/***
 * 
 * @author Sven.Zhan * 
 * 为客户端的DisplayImageOptions做一些缺省操作，省去界面的繁琐设置
 */
public class TDImagePlayOptionBuilder extends DisplayImageOptions.Builder{

	public static final int FADE_TIME = 800;
	
	public static final int ROUND_RADIUS = 6;
	
	/**
	 * 构造中默认缓存到内存和磁盘，默认使用SimpleBitmapDisplayer
	 */
	public TDImagePlayOptionBuilder(){
		super();
		cacheInMemory(true);
		cacheOnDisk(true);
		considerExifParams(true);
        displayer(new SimpleBitmapDisplayer());
		
	}	

	public TDImagePlayOptionBuilder setDefaultImage(int imageRes) {
		this.showImageOnFail(imageRes)
		.showImageForEmptyUri(imageRes)
		.showImageOnLoading(imageRes)
		.imageScaleType(ImageScaleType.EXACTLY_STRETCHED)
		.bitmapConfig(Bitmap.Config.RGB_565);
		return this;
	}
	
	public TDImagePlayOptionBuilder setDefaultImage(Drawable drawable) {
		this.showImageOnFail(drawable)
		.showImageForEmptyUri(drawable)
		.showImageOnLoading(drawable);
		return this;
	}
		
	public TDImagePlayOptionBuilder setDisplayer(BitmapDisplayer displayer) {
		this.displayer(displayer);		
		return this;
	}
}
