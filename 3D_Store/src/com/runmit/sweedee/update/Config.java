package com.runmit.sweedee.update;

import com.runmit.sweedee.R;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.util.Log;

public class Config {
	private static final String TAG = "Config";
	// public static final String UPDATE_SERVER = "http://121.10.137.216/";
	public static final String UPDATE_SAVENAME = "SweeDee.apk";

	public static int getVerCode(Context context) {
		int verCode = -1;
		try {
			verCode = context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionCode;
		} catch (NameNotFoundException e) {
			Log.e(TAG, e.getMessage());
		} catch (NullPointerException e) {
			e.printStackTrace();
		}
		return verCode;
	}

	public static String getVerName(Context context) {
		String verName = "";
		try {
			verName = context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionName;
		} catch (NameNotFoundException e) {
			Log.e(TAG, e.getMessage());
		} catch (NullPointerException e) {
			e.printStackTrace();
		}
		return verName;

	}

	public static String getMainVerName(Context context) {
		return getVerName(context).substring(0, 3);
	}

	public static String getAppName(Context context) {
		String AppName = context.getResources().getText(R.string.app_name).toString();
		return AppName;
	}
}
