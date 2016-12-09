package com.runmit.mediaserver;

import java.io.File;

import android.content.Context;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Environment;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

public class Utility {
	
    //网络类型相关
	public static final String NETWORK_NONE ="none";
	public static final String NETWORK_WIFI ="wifi";
	public static final String NETWORK_2G_WAP ="2g_wap";
	public static final String NETWORK_2G_NET ="2g_net";
	public static final String NETWORK_3G ="3g";
	public static final String NETWORK_4G ="4g";
	public static final String NETWORK_OTHER ="other";
	public static final int NETWORK_CLASS_UNKNOWN = 0;
	public static final int NETWORK_CLASS_2G = 1;
	public static final int NETWORK_CLASS_3G = 2;
	public static final int NETWORK_CLASS_4G = 3;
	
	public static boolean ensureDir(String path) {
		if (null == path) {
			return false;
		}
		boolean ret = false;

		File file = new File(path);
		if (!file.exists() || !file.isDirectory()) {
			try {
				ret = file.mkdirs();
			} catch (SecurityException se) {
				se.printStackTrace();
			}
		}
		return ret;
	}
	
	//获取IMEI号
	public static String getIMEI(Context context) {
		TelephonyManager tm = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
		String imei = tm.getDeviceId();
		if (null == imei) {
			imei = "000000000000000";
		}
		return imei;
	}
	
	/**
	 * 获取网络类型
	 * @param context
	 * @return
	 */
	public static String getNetworkType(final Context context) {
		ConnectivityManager cm = (ConnectivityManager) context
				.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo info = cm.getActiveNetworkInfo();
		if (info != null && info.isAvailable()) {
			int type = info.getSubtype();
			int classType = getNetworkClass(type);
			// 判断Wifi网络
			if (info.getType() == 1) {
				return NETWORK_WIFI;
			} else {
				if (classType == NETWORK_CLASS_2G) {
					try {
						String netMode = info.getExtraInfo();
						if (netMode.equals("cmwap") || netMode.equals("3gwap")
								|| netMode.equals("uniwap")) {
							return NETWORK_2G_WAP;
						}
						final Cursor c = context
								.getContentResolver()
								.query(Uri
										.parse("content://telephony/carriers/preferapn"),
										null, null, null, null);
						if (c != null) {
							c.moveToFirst();
							final String user = c.getString(c
									.getColumnIndex("user"));
							if (!TextUtils.isEmpty(user)) {
								if (user.startsWith("ctwap")) {
									c.close();
									return NETWORK_2G_WAP;
								}
							}
						}
					} catch (Exception e) {
						e.printStackTrace();
					}
					return NETWORK_2G_NET;
					// 判断3G网络
				} else if (classType == NETWORK_CLASS_3G) {
					return NETWORK_3G;
					// 判断4G网络
				} else if (classType == NETWORK_CLASS_4G) {
					return NETWORK_4G;
				} else {
					return NETWORK_OTHER;
				}
			}
		}
		return NETWORK_NONE;
	}
	
	private static int getNetworkClass(int networkType) {
		switch (networkType) {
		case TelephonyManager.NETWORK_TYPE_GPRS:
		case TelephonyManager.NETWORK_TYPE_EDGE:
		case TelephonyManager.NETWORK_TYPE_CDMA:
		case TelephonyManager.NETWORK_TYPE_1xRTT:
		case TelephonyManager.NETWORK_TYPE_IDEN:
			return NETWORK_CLASS_2G;
		case TelephonyManager.NETWORK_TYPE_UMTS:
		case TelephonyManager.NETWORK_TYPE_EVDO_0:
		case TelephonyManager.NETWORK_TYPE_EVDO_A:
		case TelephonyManager.NETWORK_TYPE_HSDPA:
		case TelephonyManager.NETWORK_TYPE_HSUPA:
		case TelephonyManager.NETWORK_TYPE_HSPA:
		case TelephonyManager.NETWORK_TYPE_EVDO_B:
		case TelephonyManager.NETWORK_TYPE_EHRPD:
		case 15:// TelephonyManager.NETWORK_TYPE_HSPAP = 15:
			return NETWORK_CLASS_3G;
		case TelephonyManager.NETWORK_TYPE_LTE:
			return NETWORK_CLASS_4G;
		default:
			return NETWORK_CLASS_UNKNOWN;
		}
	}
	
}
