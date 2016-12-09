package com.runmit.sweedee.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

import android.R.bool;
import android.content.Context;
import android.os.Environment;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;

public class EnvironmentTool {

	private static Context mContext = StoreApplication.INSTANCE;	
	
	/**获取应用包名*/
	public static String getAppPackageName(){
		return mContext.getPackageName();
	}	

	/**
	 * 获取应用设备内部的data目录
	 * @param folderName 文件夹名字
	 * @return
	 */
	public static String getInternalDataPath(String folderName){
		String path = mContext.getFilesDir().getPath();
		if(path != null && !path.endsWith(File.separator)){
			path += File.separator;
		}
		if(!TextUtils.isEmpty(folderName)){
			path += folderName+File.separator;
		}
		return path;
	}	

	public static String getFileDownloadCacheDir(){
//		List<String> allExterSdcardPaths = getAllExterSdcardPath();
//		Log.d("getFileDownloadCacheDir", "allExterSdcardPaths="+allExterSdcardPaths);
//		if(allExterSdcardPaths != null && allExterSdcardPaths.size() > 0){
//			String extSdcardPath = allExterSdcardPaths.get(0);
//			Log.d("getFileDownloadCacheDir", "extSdcardPath="+extSdcardPath);
//			if(extSdcardPath != null){
//				String fullPath = extSdcardPath+"/Android/data/com.runmit.sweedee/files/download/";
//				File rootDir = new File(fullPath);
//				if(!rootDir.exists()){
//					boolean isSuccess = rootDir.mkdirs();
//					Log.d("getFileDownloadCacheDir", "isSuccess="+isSuccess);
//				}
//				Log.d("getFileDownloadCacheDir", "rootDir="+rootDir+",readable="+rootDir.canRead()+",exit="+rootDir.exists()+",writeable="+rootDir.canWrite());
//				if(rootDir.exists() && rootDir.canWrite()){
//					return fullPath;
//				}
//			}
//		}
		return getExternalDataPath("download");
	}
	
	public static List<String> getAllExterSdcardPath() {
		List<String> SdList = new ArrayList<String>();
		// 得到路径
		try {
			Runtime runtime = Runtime.getRuntime();
			Process proc = runtime.exec("mount");
			InputStream is = proc.getInputStream();
			InputStreamReader isr = new InputStreamReader(is);
			String line;
			BufferedReader br = new BufferedReader(isr);
			while ((line = br.readLine()) != null) {
				// 将常见的linux分区过滤掉
				if (line.contains("secure"))
					continue;
				if (line.contains("asec"))
					continue;
				if (line.contains("media"))
					continue;
				if (line.contains("system") || line.contains("cache") || line.contains("sys") || line.contains("data") || line.contains("tmpfs") || line.contains("shell") || line.contains("root")
						|| line.contains("acct") || line.contains("proc") || line.contains("misc") || line.contains("obb")) {
					continue;
				}
				if (line.contains("fat") || line.contains("fuse") || (line.contains("ntfs"))) {
					String columns[] = line.split(" ");
					if (columns != null && columns.length > 1) {
						String path = columns[1];
						if (path != null && !SdList.contains(path) && path.contains("sd")) {
							Log.d("getFileDownloadCacheDir", "columns=" + columns[1]);
							SdList.add(columns[1]);
						}
					}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return SdList;
	}


	public static String getExternalSDCard() {
		String sdcard_path = null;
		String sd_default = Environment.getExternalStorageDirectory().getAbsolutePath();
		if (sd_default.endsWith("/")) {
			sd_default = sd_default.substring(0, sd_default.length() - 1);
		}
		// 得到路径
		try {
			Runtime runtime = Runtime.getRuntime();
			Process proc = runtime.exec("mount");
			InputStream is = proc.getInputStream();
			InputStreamReader isr = new InputStreamReader(is);
			String line;
			BufferedReader br = new BufferedReader(isr);
			while ((line = br.readLine()) != null) {
				if (line.contains("secure"))
					continue;
				if (line.contains("asec"))
					continue;
				if (line.contains("fat") && line.contains("/mnt/")) {
					String columns[] = line.split(" ");
					if (columns != null && columns.length > 1) {
						if (sd_default.trim().equals(columns[1].trim())) {
							continue;
						}
						sdcard_path = columns[1];
						Log.d("getFileDownloadCacheDir", "sdcard_path=" + sdcard_path);
					}
				} else if (line.contains("fuse") && line.contains("/mnt/")) {
					String columns[] = line.split(" ");
					if (columns != null && columns.length > 1) {
						if (sd_default.trim().equals(columns[1].trim())) {
							continue;
						}
						sdcard_path = columns[1];
						Log.d("getFileDownloadCacheDir", "sdcard_path=" + sdcard_path);
					}
				}
			}
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return sdcard_path;
	}

	
	/**
	 * 获取应用设备外部的data目录
	 * @param folderName 文件夹名字
	 * @return
	 */
	public static String getExternalDataPath(String folderName){
		File pathRoot = mContext.getExternalFilesDir(folderName);
		if(pathRoot == null){
			String dir = getInternalDataPath(folderName);
			if(TextUtils.isEmpty(dir)){
				return null;
			}else{
				pathRoot = new File(dir);
			}
		}
		if(!pathRoot.exists()){
			pathRoot.mkdirs();
		}
		String path = pathRoot.getPath();	
		if(path != null && !path.endsWith(File.separator)){
			path += File.separator;
		}
		Log.d("getFileDownloadCacheDir", "path="+path);
		return path;
	}

	/**
	 * 优先获取外部存储的目录，如果没有则获取内部SD卡的目录，现在的手机基本不会没有外部SD卡
	 * @param folderName 文件夹名字 
	 * @return
	 */
	public static String getDataPath(String folderName){
		boolean hasSdcard = Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState());
		String path = hasSdcard ? getExternalDataPath(folderName) : getInternalDataPath(folderName);		
		return path;
	}
	
	/**获取应用名称*/
	public static String getAppName() {
		String appName = mContext.getResources().getString(R.string.app_name);		
		return appName;
	}
	
	/**获取包的版本名称*/
	public static String getPackageVersion() {
		String verName = "";
		try {
			verName = mContext.getPackageManager().getPackageInfo(getAppPackageName(), 0).versionName;
		}catch (Exception e) {
			e.printStackTrace();
		}
		return verName;
	}
	
	//获取系统简略信息
	public static String getSimpleSystemInfo() {
		String sdk_ver;
		sdk_ver = android.os.Build.VERSION.SDK;
		sdk_ver += "_";
		sdk_ver += android.os.Build.MANUFACTURER;
		sdk_ver += "_";
		sdk_ver += android.os.Build.MODEL;
		return sdk_ver;
	}
	
	//获取IMEI号
	public static String getIMEI() {
		TelephonyManager tm = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
		String imei = tm.getDeviceId();
		if (null == imei) {
			imei = "000000000000000";
		}
		return imei;
	}
	
	public static DisplayMetrics getScreenSize() {
		DisplayMetrics dm = new DisplayMetrics();
		WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
		wm.getDefaultDisplay().getMetrics(dm);
		return dm;
	}
}
