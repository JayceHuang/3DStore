package com.runmit.libsdk.util;

import android.util.Log;

public class RmLog {
	private static final boolean isDebug = true;
	
	public static void debug(String tag,String message){
		if(isDebug){
			Log.d(tag, message);
		}
	}
}
