package com.runmit.sweedee.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.math.BigDecimal;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.SimpleDateFormat;
import java.util.List;
import java.util.Locale;

import org.apache.http.NameValuePair;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.StatFs;
import android.telephony.TelephonyManager;
import android.text.Html;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.ViewConfiguration;
import android.view.WindowManager;
import android.widget.Toast;

import com.runmit.sweedee.BuildConfig;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.action.more.SettingItemActivity;
import com.superd.sdsdk.SDDevice;

public class Util {
	public static final boolean PRINT_LOG = true;// 控制崩溃时是否打印
	private static Toast mToast = null;

	private static long lastClickTime;

	/**
	 * 是否为短时间多次点击事件
	 * 
	 * @return
	 */
	public static boolean isDoubleClick(long dt) {
		long time = System.currentTimeMillis();
		long timeD = time - lastClickTime;
		if (0 < timeD && timeD < dt) {
			return true;
		}
		lastClickTime = time;
		return false;
	}

	public static boolean isFastDoubleClick() {
		return isDoubleClick(500);
	}

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

	/**
	 * 在数字型字符串千分位加逗号
	 * 
	 * @param str
	 * @return
	 */
	public static String addComma(int downloadCount, Context mContext) {
		if (Locale.getDefault().getLanguage().equals(Locale.ENGLISH.getLanguage())) {// 因为我们的default是中文，所以仅当用户设置为英文时候
			StringBuilder sb = new StringBuilder("" + downloadCount);
			if (sb.length() <= 3) {
				return sb.toString();
			} else {
				sb.reverse();
				for (int i = 3; i < sb.length(); i += 4) {
					sb.insert(i, ',');
				}
				sb.reverse();
				return sb.toString();
			}
		} else {
			if (downloadCount < 10000) {// 一万
				return "" + downloadCount + mContext.getString(R.string.download_app_install);
			} else if (downloadCount < 100000000) {// 一亿
				return "" + downloadCount / 10000 + " 万" + mContext.getString(R.string.download_app_install);
			} else {
				return "" + downloadCount / 100000000 + " 亿+" + mContext.getString(R.string.download_app_install);
			}
		}
	}

	public static String inputStream2String(InputStream is) {
		BufferedReader in = new BufferedReader(new InputStreamReader(is));
		StringBuffer buffer = new StringBuffer();
		String line = "";
		try {
			while ((line = in.readLine()) != null) {
				buffer.append(line);
			}
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		}
		return buffer.toString();
	}

	public static boolean hasKitKat() {
		return Build.VERSION.SDK_INT >= 19;
	}

	@SuppressLint("NewApi")
	public static boolean hasVirtualKey(Context ctx) {
		// 通过标准api判断
		boolean firstCheck = (Build.VERSION.SDK_INT >= 14) && !ViewConfiguration.get(ctx).hasPermanentMenuKey();
		// 特殊机型通过匹配判断
		boolean secondCheck = isSpecialDeviceOfVirtualKey();
		boolean ret = firstCheck || secondCheck;
		return ret;
	}

	public static boolean isSpecialDeviceOfVirtualKey() {
		String model = Build.MODEL;
		boolean ret = model != null && model.contains("K860");// Lenovo K860i
		return ret;
	}

	public static boolean hasIcs() {
		return Build.VERSION.SDK_INT >= 14;
	}

	public static boolean hasHoneycomb() {
		return Build.VERSION.SDK_INT >= 11;
	}

	public static boolean hasHoneycombMR1() {
		return Build.VERSION.SDK_INT >= 12;
	}

	public static boolean hasJellyBean() {
		return Build.VERSION.SDK_INT >= 16;
	}

	public static String getSDCardDir() {
		final String cachePath = Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState()) ? Environment.getExternalStorageDirectory().getPath() : StoreApplication.INSTANCE
				.getCacheDir().getPath();
		String path = cachePath + File.separator;
		if (BuildConfig.DEBUG) {
			Log.d("getSDCardDir", "getSDCardDir=" + path);
		}
		return path;
	}

	public static boolean isSDCardExist() {
		return Environment.MEDIA_MOUNTED.equalsIgnoreCase(Environment.getExternalStorageState());
	}

	// 获取可用空间
	public static long getAvailableSizeforDownloadPath(String path) {
		try {
			StatFs stat = new StatFs(path);
			long blockSize = stat.getBlockSize();
			long availableBlocks = stat.getAvailableBlocks();
			return availableBlocks * blockSize;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return 0;// 如果第三方路径引发崩溃，直接返回0表示空间大小
	}

	// 获取总的空间
	public static long getTotalSizeforDownloadPath(String path) {
		try {
			StatFs stat = new StatFs(path);
			long blockSize = stat.getBlockSize();
			long totalBlocks = stat.getBlockCount();
			return totalBlocks * blockSize;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return 0;
	}

	public static void showToast(Context ctx, final String showMsg, final int duration) {
		if (ctx == null) {
			ctx = StoreApplication.INSTANCE;
		}
		final Context context = ctx;
		if (Thread.currentThread() == Looper.getMainLooper().getThread()) {// 是主线程
			showToastImpl(context, showMsg, duration);
		} else {
			new Handler(context.getMainLooper()).post(new Runnable() {
				@Override
				public void run() {
					showToastImpl(context, showMsg, duration);
				}
			});
		}
	}

	private static final void showToastImpl(Context context, String showMsg, int duration) {
		if (mToast == null) {
			mToast = Toast.makeText(context, showMsg, duration);
		}
		mToast.setText(showMsg);
		mToast.setDuration(duration);
		mToast.show();
	}

	public static String getFormatTime(int mCurSec) {
		String formattedTime = "00:00:00";
		if (mCurSec == 0) {
			return formattedTime;

		}
		int hour = mCurSec / 60 / 60;
		int leftSec = mCurSec % (60 * 60);
		int min = leftSec / 60;
		int sec = leftSec % 60;
		String strHour = String.format("%d", hour);
		String strMin = String.format("%d", min);
		String strSec = String.format("%d", sec);

		if (hour < 10) {
			strHour = "0" + hour;
		}
		if (min < 10) {
			strMin = "0" + min;
		}
		if (sec < 10) {
			strSec = "0" + sec;
		}
		formattedTime = strHour + ":" + strMin + ":" + strSec;
		return formattedTime;
	}

	public static String getFormatTimeProgressChange(int second) {
		String formattedTime = "[+00:00]";
		if (second == 0) {
			return formattedTime;
		}
		int ch = Math.abs(second);
		int hour = ch / 60 / 60;
		int leftSec = ch % (60 * 60);
		int min = leftSec / 60;
		int sec = leftSec % 60;
		String strHour = String.format("%d", hour);
		String strMin = String.format("%d", min);
		String strSec = String.format("%d", sec);

		if (min < 10) {
			strMin = "0" + min;
		}
		if (sec < 10) {
			strSec = "0" + sec;
		}

		if (hour <= 0) {
			if (second < 0) {
				formattedTime = "[-" + strMin + ":" + strSec + "]";
			} else {
				formattedTime = "[+" + strMin + ":" + strSec + "]";
			}
		} else {
			if (hour < 10) {
				strHour = "0" + hour;
			}
			if (second < 0) {
				formattedTime = "[-" + strHour + ":" + strMin + ":" + strSec + "]";
			} else {
				formattedTime = "[+" + strHour + ":" + strMin + ":" + strSec + "]";
			}
		}
		return formattedTime;
	}

	private static final long BASE_B = 1L;
	private static final long BASE_KB = 1024L;
	private static final long BASE_MB = 1024L * 1024L;
	private static final long BASE_GB = 1024L * 1024L * 1024L;
	private static final long BASE_TB = 1024L * 1024L * 1024L * 1024L;
	// private static final long BASE_PB = 1024L * 1024L * 1024L * 1024L* 1024L;
	private static final String UNIT_BIT = "B";
	private static final String UNIT_KB = "KB";
	private static final String UNIT_MB = "MB";
	private static final String UNIT_GB = "GB";
	private static final String UNIT_TB = "TB";
	private static final String UNIT_PB = "PB";

	public static String convertFileSize(long file_size, int precision, boolean isForceInt) {
		long int_part = 0;
		double fileSize = file_size;
		double floatSize = 0L;
		long temp = file_size;
		int i = 0;
		double base = 1;
		String baseUnit = "M";
		String fileSizeStr = null;
		int indexMid = 0;

		while (temp / 1024 > 0) {
			int_part = temp / 1024;
			temp = int_part;
			i++;
		}
		switch (i) {
		case 0:
			// B
			base = BASE_B;
			floatSize = fileSize / base;
			baseUnit = UNIT_BIT;
			break;

		case 1:
			// KB
			base = BASE_KB;
			floatSize = fileSize / base;
			baseUnit = UNIT_KB;
			break;

		case 2:
			// MB
			base = BASE_MB;
			floatSize = fileSize / base;
			baseUnit = UNIT_MB;
			break;

		case 3:
			// GB
			base = BASE_GB;
			floatSize = fileSize / base;
			baseUnit = UNIT_GB;
			break;

		case 4:
			// TB
			// base = BASE_TB;
			floatSize = (fileSize / BASE_GB) / BASE_KB;
			baseUnit = UNIT_TB;
			break;
		case 5:
			// PB
			// base = BASE_PB;
			floatSize = (fileSize / BASE_GB) / BASE_MB;
			baseUnit = UNIT_PB;
			break;
		default:
			break;
		}

		// Log.d("filesize", "fileSize="+fileSize+",base="+base);

		fileSizeStr = Double.toString(floatSize);
		if (precision == 0 || (isForceInt && i < 3)) {
			indexMid = fileSizeStr.indexOf('.');
			if (-1 == indexMid) {
				return fileSizeStr + baseUnit;
			}
			return fileSizeStr.substring(0, indexMid) + baseUnit;
		}

		try {
			BigDecimal b = new BigDecimal(Double.toString(floatSize));
			BigDecimal one = new BigDecimal("1");
			double dou = b.divide(one, precision, BigDecimal.ROUND_HALF_UP).doubleValue();
			return dou + baseUnit;
		} catch (Exception e) {
			indexMid = fileSizeStr.indexOf('.');
			if (-1 == indexMid) {
				return fileSizeStr + baseUnit;
			}
			if (fileSizeStr.length() <= indexMid + precision + 1) {
				return fileSizeStr + baseUnit;
			}
			if (indexMid < 3) {
				indexMid += 1;
			}
			if (indexMid + precision < 6) {
				indexMid = indexMid + precision;
			}
			return fileSizeStr.substring(0, indexMid) + baseUnit;
		}
	}

	public static boolean isWifiNet(final Context context) {
		boolean bRet = false;
		ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo wifiInfo = cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
		if (wifiInfo != null && wifiInfo.isConnectedOrConnecting()) {
			bRet = true;
		}
		return bRet;
	}

	public static boolean isMobileNet(final Context context) {
		boolean bRet = false;
		ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo wifiInfo = cm.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
		if (wifiInfo != null && wifiInfo.isConnectedOrConnecting()) {
			bRet = true;
		}
		return bRet;
	}

	/**
	 * 获取具体的网络类型,返回值:none,wifi,2g_wap,2g_net,3g,4g,other *
	 */
	public static String net_type = Constant.NETWORK_OTHER;
	public static boolean net_type_changed = true;

	public static String getNetworkType(final Context context) {
		if (!net_type_changed) {
			return net_type;
		}
		String res = Constant.NETWORK_NONE;
		ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo info = cm.getActiveNetworkInfo();
		if (info != null && info.isAvailable()) {
			// Log.e("NetworkInfo ",info.toString());
			// Log.e("NetWork Type Info : ",
			// "Type = "+info.getType()+" TypeName = "+info.getTypeName()+" SubType = "+info.getSubtype()+" SubtypeName = "+info.getSubtypeName()+" Extra = "+info.getExtraInfo());
			int type = info.getSubtype();
			int classType = getNetworkClass(type);
			// 判断Wifi网络
			if (info.getType() == 1) {
				return Constant.NETWORK_WIFI;
			} else {
				// if(info.getSubtypeName() != null &&
				// info.getSubtypeName().equalsIgnoreCase("GPRS")){
				// 判断2G网络
				if (classType == Constant.NETWORK_CLASS_2G) {
					try {
						String netMode = info.getExtraInfo();
						if (netMode.equals("cmwap") || netMode.equals("3gwap") || netMode.equals("uniwap")) {
							return Constant.NETWORK_2G_WAP;
						}
						final Cursor c = context.getContentResolver().query(Uri.parse("content://telephony/carriers/preferapn"), null, null, null, null);
						String net_type = null;
						if (c != null) {
							c.moveToFirst();
							net_type = c.getString(c.getColumnIndex("user"));
							c.close();
						}
						if (!TextUtils.isEmpty(net_type)) {
							if (net_type.startsWith("ctwap")) {
								return Constant.NETWORK_2G_WAP;
							}
						}
					} catch (Exception e) {
						e.printStackTrace();
					}
					return Constant.NETWORK_2G_NET;
					// 判断3G网络
				} else if (classType == Constant.NETWORK_CLASS_3G) {
					return Constant.NETWORK_3G;
					// 判断4G网络
				} else if (classType == Constant.NETWORK_CLASS_4G) {
					return Constant.NETWORK_4G;
				} else {
					return Constant.NETWORK_OTHER;
				}
			}
		}
		net_type_changed = false;
		net_type = res;
		return net_type;
	}

	public static int getNetworkTypeForInt(final Context context) {
		String type = getNetworkType(context);
		int res = 0x00000000;
		if (type.equalsIgnoreCase(Constant.NETWORK_NONE)) {
			res = 0x00000000;// UNKNOWN_NET 0x00000000
		} else if (type.equalsIgnoreCase(Constant.NETWORK_WIFI)) {
			res = 0x00080000;// NT_WLAN 0x00080000 wifi and lan ...
		} else if (type.equalsIgnoreCase(Constant.NETWORK_2G_NET)) {
			res = 0x00020000;// NT_GPRS_NET 0x00020000
		} else if (type.equalsIgnoreCase(Constant.NETWORK_2G_WAP)) {
			res = 0x00010000;// NT_GPRS_WAP 0x00010000
		} else if (type.equalsIgnoreCase(Constant.NETWORK_3G)) {
			res = 0x00040000;// NT_3G 0x00040000
		} else if (type.equalsIgnoreCase(Constant.NETWORK_OTHER) || type.equalsIgnoreCase(Constant.NETWORK_4G)) {
			res = 0x00008000;// OTHER_NET 0x00008000
		}
		return res;
	}

	private static int getNetworkClass(int networkType) {
		switch (networkType) {
		case TelephonyManager.NETWORK_TYPE_GPRS:
		case TelephonyManager.NETWORK_TYPE_EDGE:
		case TelephonyManager.NETWORK_TYPE_CDMA:
		case TelephonyManager.NETWORK_TYPE_1xRTT:
		case TelephonyManager.NETWORK_TYPE_IDEN:
			return Constant.NETWORK_CLASS_2G;
		case TelephonyManager.NETWORK_TYPE_UMTS:
		case TelephonyManager.NETWORK_TYPE_EVDO_0:
		case TelephonyManager.NETWORK_TYPE_EVDO_A:
		case TelephonyManager.NETWORK_TYPE_HSDPA:
		case TelephonyManager.NETWORK_TYPE_HSUPA:
		case TelephonyManager.NETWORK_TYPE_HSPA:
		case TelephonyManager.NETWORK_TYPE_EVDO_B:
		case TelephonyManager.NETWORK_TYPE_EHRPD:
		case TelephonyManager.NETWORK_TYPE_HSPAP:
			return Constant.NETWORK_CLASS_3G;
		case TelephonyManager.NETWORK_TYPE_LTE:
			return Constant.NETWORK_CLASS_4G;
		default:
			return Constant.NETWORK_CLASS_UNKNOWN;
		}
	}

	public static boolean isNetworkAvailable() {
		return StoreApplication.isNetAvilible;
	}

	public static boolean isNetworkAvailable(Context context) {
		Context ct = context.getApplicationContext();
		ConnectivityManager cm = (ConnectivityManager) ct.getSystemService(Context.CONNECTIVITY_SERVICE);
		if (null == cm) {
			return false;
		} else {
			NetworkInfo[] info = cm.getAllNetworkInfo();
			if (null != info) {
				for (int i = 0; i < info.length; i++) {
					if (info[i].getState() == NetworkInfo.State.CONNECTED) {
						return true;
					}
				}
			}
		}
		return false;
	}

	public static boolean showDialog(ProgressDialog dialog, String msg) {
		return showDialog(dialog, msg, false);
	}

	public static boolean showDialog(ProgressDialog dialog, String msg, boolean flag) {
		if (dialog == null) {
			return false;
		}
		dismissDialog(dialog);

		dialog.setMessage(msg);
		dialog.setCanceledOnTouchOutside(false);
		dialog.setCancelable(true);
		if (flag) {
			dialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
				@Override
				public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
					return true;
				}
			});
		}
		dialog.getWindow().setBackgroundDrawableResource(android.R.color.transparent);
		try {
			dialog.show();
		} catch (Exception e) {
			e.printStackTrace();
		}

		return true;
	}

	public static void dismissDialog(ProgressDialog dialog) {
		if (null != dialog) {
			try {
				dialog.dismiss();
			} catch (IllegalArgumentException e) {
				e.printStackTrace();
			}
		}
	}

	public static void showDialogIn3GDownloadMode(final Context ctx) {
		AlertDialog.Builder builder = new AlertDialog.Builder(ctx, R.style.AlertDialog);
		builder.setMessage(Util.setColourText(ctx, R.string.no_wifi_stop_download, Util.DialogTextColor.BLUE));
		builder.setNegativeButton(Util.setColourText(ctx, R.string.yes, Util.DialogTextColor.BLUE), new OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				ctx.startActivity(new Intent(ctx, SettingItemActivity.class));
			}
		});
		builder.setPositiveButton(R.string.no, new OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
			}
		});
		builder.create().show();
	}

	// 创建，继续任务时根据设置项判断是否可以在3G模式下缓存
	public static boolean checkDownloadableIn3G(Context ctx) {
		boolean isWifi = isWifiNet(ctx);
		boolean isDirectDownload = isWifi || (!isWifi && SharePrefence.getInstance(ctx).getBoolean(SettingItemActivity.MOBILE_DOWNLOAD, false));
		return isDirectDownload;
	}

	public static boolean checkDownloadableIn3GWithoutDialog(Context ctx) {
		return true;
	}

	public static boolean checkOperateNetWorkAvail() {
		if (!StoreApplication.isNetAvilible) {
			Util.showToast(StoreApplication.INSTANCE, StoreApplication.INSTANCE.getString(R.string.has_no_using_network), Toast.LENGTH_SHORT);
			return false;
		} else {
			return true;
		}
	}

	public static String getPartnerID(Context c) {
		try {
			return c.getString(R.string.partner_id);
		} catch (NullPointerException e) {
			e.printStackTrace();
		}
		return "0x11300001";

	}

	public static int getScreenWidth(Context ctx) {
		DisplayMetrics dm = new DisplayMetrics();
		WindowManager wm = (WindowManager) ctx.getSystemService(Context.WINDOW_SERVICE);
		wm.getDefaultDisplay().getMetrics(dm);
		return (int) (dm.widthPixels);
	}

	public static int getScreenHeight(Context ctx) {
		DisplayMetrics dm = new DisplayMetrics();
		WindowManager wm = (WindowManager) ctx.getSystemService(Context.WINDOW_SERVICE);
		wm.getDefaultDisplay().getMetrics(dm);
		return (int) (dm.heightPixels);
	}

	// 获取md5值
	public static byte[] getMD5Byte(byte[] source) throws NoSuchAlgorithmException {
		MessageDigest md5 = null;
		md5 = MessageDigest.getInstance("MD5");
		md5.update(source);
		return md5.digest();
	}

	public static String corverTimeWithYMD(long time) {
		SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd", Locale.CHINESE);
		return df.format(new java.sql.Date(time));
	}

	public static String getMBSize(long fileSize) {
		double floatSize = 0L;
		String result = null;
		floatSize = (double) fileSize / BASE_MB;
		if (floatSize - 1024 > 0) {
			floatSize = floatSize / 1024l;
			result = new java.text.DecimalFormat("0.00").format(floatSize) + "G";
		} else {
			result = new java.text.DecimalFormat("0.0").format(floatSize) + "MB";
		}

		return result/* Double.toString(floatSize) */;
	}

	/**
	 * A hashing method that changes a string (like a URL) into a hash suitable
	 * for using as a disk filename.
	 */
	public static String hashKeyForDisk(String key) {
		String cacheKey;
		try {
			final MessageDigest mDigest = MessageDigest.getInstance("MD5");
			mDigest.update(key.getBytes());
			cacheKey = bytesToHexString(mDigest.digest());
		} catch (NoSuchAlgorithmException e) {
			cacheKey = String.valueOf(key.hashCode());
		}
		return cacheKey;
	}

	public static String bytesToHexString(byte[] bytes) {
		// http://stackoverflow.com/questions/332079
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < bytes.length; i++) {
			String hex = Integer.toHexString(0xFF & bytes[i]);
			if (hex.length() == 1) {
				sb.append('0');
			}
			sb.append(hex);
		}
		return sb.toString();
	}

	private static String gPeerId = null;

	public static String getPeerId() {
		if (gPeerId == null) {
			try { // try的理由是目前的SDK库文件只有放到了170版的ROM里面才能OK
				long time = System.currentTimeMillis();
				String icfwId = SDDevice.getICFWID();
				Log.d("getPeerId", "icfwId=" + icfwId + " timecoast = " + (System.currentTimeMillis() - time));
				if (!TextUtils.isEmpty(icfwId)) {
					gPeerId = icfwId;
				} else {
					gPeerId = "f_f_f_f_ffff_f_f_f_f_f_f";
				}
			} catch (Throwable e) {
				e.printStackTrace();
				gPeerId = "f_f_f_f_ffff_f_f_f_f_f_f";
			}
		}
		return gPeerId;
	}

	public static String getLocalWifiMacAddress(Context mContext) {
		WifiManager wifi = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
		WifiInfo info = wifi.getConnectionInfo();
		return info.getMacAddress();
	}

	public enum DialogTextColor {
		BLUE, WHITE,
	}

	/**
	 * 利用html代码让dialog字体变蓝
	 */
	public static android.text.Spanned setColourText(Context context, int resId, DialogTextColor color) {
		return setColourText(context.getString(resId), color);
	}

	public static android.text.Spanned setColourText(String str, DialogTextColor color) {
		try {
			switch (color) {
			case BLUE:
				return Html.fromHtml("<font color='#1e91eb'>" + str + "</font>");
			case WHITE:
				return Html.fromHtml("<font color='#FFFFFF'>" + str + "</font>");
			default:
				return Html.fromHtml("<font color='#FFFFFF'>" + str + "</font>");
			}
		} catch (Exception e) {
			return null;
		}
	}
	
    private static final String PARAMETER_SEPARATOR = "&";
    private static final String NAME_VALUE_SEPARATOR = "=";
    public static String format (final List <? extends NameValuePair> parameters) {
        final StringBuilder result = new StringBuilder();
        for (final NameValuePair parameter : parameters) {
            final String encodedName = parameter.getName();
            final String value = parameter.getValue();
            final String encodedValue = value;
            if (result.length() > 0)
                result.append(PARAMETER_SEPARATOR);
            result.append(encodedName);
            result.append(NAME_VALUE_SEPARATOR);
            result.append(encodedValue);
        }
        return result.toString();
    }
}
