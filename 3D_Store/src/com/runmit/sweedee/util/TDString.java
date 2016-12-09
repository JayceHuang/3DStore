package com.runmit.sweedee.util;

import com.runmit.sweedee.StoreApplication;

/**
 * 
 * @author Sven.Zhan
 * 统一获取资源字符串，避免各类频繁引用StoreApplication
 */
public class TDString {	
	
	public static String getStr(int resId){
		return StoreApplication.INSTANCE.getResources().getString(resId);
	}
	
}
