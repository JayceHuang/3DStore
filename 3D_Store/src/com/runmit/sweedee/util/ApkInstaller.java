package com.runmit.sweedee.util;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.hardware.Camera;
import android.net.Uri;
import android.text.TextUtils;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.database.dao.AppInstallSQLDao;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadInfo;
import com.runmit.sweedee.manager.Game3DConfigManager;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.provider.GameConfigTable;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.sdk.user.member.task.UserManager;

import java.io.File;

public class ApkInstaller {

	static XL_Log log = new XL_Log(ApkInstaller.class);

	public static final int FLAG_ACTIVITY_WITH_STEREO = 0x00000080;

	public static boolean fileExist(String fileName) {
		File file = new File(fileName);
		if (!file.exists()) {
			return false;
		}
		return true;
	}

	// public static boolean installApk(final Context context, final AppItemInfo
	// mAppItemInfo, final long downloadId) {
	// Intent install = new Intent(Intent.ACTION_VIEW);
	// final Uri downloadFileUri =
	// DownloadTaskManager.getInstance().getLocalUri(downloadId);
	//
	// if (!fileExist(downloadFileUri.getPath())) {
	// return false;
	// }
	//
	// PackageManager pm = context.getPackageManager();
	// PackageInfo pkgInfo = pm.getPackageArchiveInfo(downloadFileUri.getPath(),
	// PackageManager.GET_ACTIVITIES);
	// if(pkgInfo != null){
	// install.setDataAndType(downloadFileUri,
	// "application/vnd.android.package-archive");
	// install.putExtra("DownloadId", downloadId);
	// install.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
	// context.startActivity(install);
	//
	// String versionName = pkgInfo.versionName;
	// String packageName =
	// DownloadTaskManager.getInstance().getPackageNameById(downloadId);
	// AppInstallSQLDao.AppDataModel mAppDataModel =
	// AppInstallSQLDao.getInstance(context).findDownloadInfoByBeanKey(packageName);
	// // 如果不存在该包名,就添加
	// if (mAppDataModel == null) {
	// mAppDataModel = new AppInstallSQLDao.AppDataModel();
	// mAppDataModel.package_name = packageName;
	// log.debug("start install package_name = " + packageName);
	// mAppDataModel.app_type = mAppItemInfo.type;
	// mAppDataModel.appId = mAppItemInfo.appId;
	// mAppDataModel.appKey = mAppItemInfo.appKey;
	// mAppDataModel.startTime = "" + System.currentTimeMillis();
	// AppInstallSQLDao.getInstance(context).insertData(mAppDataModel);
	// }
	// ReportActionSender.getInstance().appdownload(mAppDataModel.appKey,
	// versionName, mAppDataModel.appId, mAppDataModel.app_type,
	// System.currentTimeMillis(),
	// (System.currentTimeMillis() - Long.parseLong(mAppDataModel.startTime)));
	// }else{
	// //TODO 提示解析包失败
	// }
	// return true;
	// }

	// ///new
	public static boolean installApk(final Context context, final DownloadInfo downloadInfo) {
		if(downloadInfo == null || downloadInfo.downloadType == DownloadInfo.DOWNLOAD_TYPE_VIDEO) {
			return  false;
		}
		AppDownloadInfo mDownloadInfo = (AppDownloadInfo) downloadInfo;
		if (!fileExist(mDownloadInfo.mPath)) {
			return false;
		}
		Intent install = new Intent(Intent.ACTION_VIEW);
		final Uri downloadFileUri = Uri.fromFile(new File(mDownloadInfo.mPath));
		install.setDataAndType(downloadFileUri, "application/vnd.android.package-archive");
		install.putExtra("DownloadId", mDownloadInfo.downloadId);
		install.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		context.startActivity(install);
		PackageManager pm = context.getPackageManager();
		PackageInfo pkgInfo = pm.getPackageArchiveInfo(downloadFileUri.getPath(), PackageManager.GET_ACTIVITIES);
		if (pkgInfo != null) {
			String versionName = pkgInfo.versionName;
			String packageName = mDownloadInfo.pkgName;
			AppInstallSQLDao.AppDataModel mAppDataModel = AppInstallSQLDao.getInstance(context).findAppInfoByPackageName(packageName);
			// 如果不存在该包名,就添加
			if (mAppDataModel == null) {
				mAppDataModel = new AppInstallSQLDao.AppDataModel();
				mAppDataModel.package_name = packageName;
				log.debug("start install package_name = " + packageName);
				mAppDataModel.app_type = mDownloadInfo.downloadType;
				mAppDataModel.appId = mDownloadInfo.mAppId;
				mAppDataModel.appKey = mDownloadInfo.mAppkey;
				mAppDataModel.startTime = "" + System.currentTimeMillis();
				AppInstallSQLDao.getInstance(context).insertData(mAppDataModel);
			}
			ReportActionSender.getInstance().appdownload(mAppDataModel.appKey, versionName, mAppDataModel.appId, mAppDataModel.app_type, System.currentTimeMillis(),
					(System.currentTimeMillis() - Long.parseLong(mAppDataModel.startTime)), ""+UserManager.getInstance().getUserId());
		}
		return true;
	}

	/**
	 * 检测应用是已经从sweedee安装
	 * 
	 * @param context
	 * @param packageName
	 * @return
	 */
	public static boolean checkApkInstallByMyApp(Context context, String packageName) {
		AppInstallSQLDao.AppDataModel model = AppInstallSQLDao.getInstance(context).findAppInfoByPackageName(packageName);
		if (TextUtils.isEmpty(packageName) && model == null) {
			return false;
		}
		PackageInfo packageInfo;
		try {
			packageInfo = context.getPackageManager().getPackageInfo(packageName, 0);
		} catch (Exception e) {
			packageInfo = null;
			e.printStackTrace();
		}
		if (packageInfo == null) {
			return false;
		} else {
			return true;
		}
	}

	public static void lanchAppByPackageName(Activity context, String packagename, int appType, int startFrom) {
		log.debug("要开启的包名是 = " + packagename);
		// 通过包名获取此APP详细信息，包括Activities、services、versioncode、name等等
		PackageInfo packageinfo = null;
		try {
			packageinfo = context.getPackageManager().getPackageInfo(packagename, 0);
			AppInstallSQLDao dbHelper = AppInstallSQLDao.getInstance(context);
			if (dbHelper != null) {
				AppInstallSQLDao.AppDataModel mAppDataModel = dbHelper.findAppInfoByPackageName(packagename);
				ReportActionSender.getInstance().appstart(mAppDataModel.appKey, packageinfo.versionName, mAppDataModel.appId, appType, System.currentTimeMillis(), startFrom);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		if (packageinfo == null) {
			return;
		}

		// 上报
		if (appType == AppItemInfo.TYPE_APP) {
			launchApp(context, packagename);
		} else {
			launchGame(context, packagename);
		}
	}

	/*
	 * startFrom 0 –未知;1– 我的游戏;2 – 我的应用，3.详情页，4.首页TAB推荐
	 */
	public static void lanchAppByPackageName(Activity context, String appKey, String appId, String packagename, int appType, int startFrom) {
		log.debug("appKey=" + appKey + ",appId=" + appId + ",packagename=" + packagename);
		PackageInfo packageinfo = null;
		try {
			packageinfo = context.getPackageManager().getPackageInfo(packagename, 0);
			ReportActionSender.getInstance().appstart(appKey, packageinfo.versionName, appId, appType, System.currentTimeMillis(), startFrom);
		} catch (Exception e) {
			e.printStackTrace();
			return;
		}

		if (appType == AppItemInfo.TYPE_APP) {
			launchApp(context, packagename);
		} else {
			launchGame(context, packagename);
		}
	}

	private static void launchApp(Activity context, String packagename) {
		PackageManager pkgMgr = context.getPackageManager();
		Intent intent = pkgMgr.getLaunchIntentForPackage(packagename);
		intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		context.startActivity(intent);
	}

	private static void launchGame(Activity context, String packagename) {

		ActivityManager mActivityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
		for (ActivityManager.RunningAppProcessInfo appProcess : mActivityManager.getRunningAppProcesses()) {
			if (appProcess.processName.compareTo(packagename) == 0) {
				ActivityManager manager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
				manager.killBackgroundProcesses(appProcess.processName);
			}
		}

		if (Game3DConfigManager.getInstance().isGame3DOn()) {
			if (!isCameraCanUse()) {
				Util.showToast(context, TDString.getStr(R.string.gaimeopen_fail_the_flashlight_isopen), Toast.LENGTH_SHORT);
				return;
			}
			
			Intent intent = new Intent("android.intent.action.MAIN");
			boolean hasArgs = false;
			try {
				Cursor cursor = context.getContentResolver().query(GameConfigTable.CONTENT_URI, null, GameConfigTable.PACKAGE_NAME + "=?", new String[] { String.valueOf(packagename) }, null);
				if (cursor != null && cursor.moveToFirst()) {
					String x = cursor.getString(cursor.getColumnIndex(GameConfigTable.X));
					String y = cursor.getString(cursor.getColumnIndex(GameConfigTable.Y));
					String z = cursor.getString(cursor.getColumnIndex(GameConfigTable.Z));
					String w = cursor.getString(cursor.getColumnIndex(GameConfigTable.W));
					intent.putExtra("x", Float.valueOf(x));
					intent.putExtra("y", Float.valueOf(y));
					intent.putExtra("z", Float.valueOf(z));
					intent.putExtra("w", Float.valueOf(w));
					log.debug("GameConfig packageName = " + packagename + " x = " + x + " y = " + y + " z = " + z + " w = " + w);
					hasArgs = true;
				}
			} catch (Exception e) {
				e.printStackTrace();
				log.error("GameConfig Exception = " + e.getCause());
			}

			if (hasArgs) {
				intent.putExtra("hookedProcessName:", packagename + ".stereo_profile");
				intent.addCategory("android.intent.category.LAUNCHER");
				intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED | FLAG_ACTIVITY_WITH_STEREO);
				intent.setComponent(ComponentName.unflattenFromString(context.getPackageManager().getLaunchIntentForPackage(packagename).getComponent().flattenToString()));
				context.startActivity(intent);
			} else {
				launchWithDialogTip(context, packagename);
			}
		} else {
			launchApp(context, packagename);
		}

	}

	private static void launchWithDialogTip(final Activity context, final String packagename) {
		AlertDialog.Builder builder = new AlertDialog.Builder(context, R.style.AlertDialog);
		builder.setTitle(Util.setColourText(context, R.string.confirm_title, Util.DialogTextColor.BLUE));
		builder.setMessage(R.string.launch_game_without_3d_tip);
		builder.setNeutralButton(R.string.known, new OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				launchApp(context, packagename);
				dialog.dismiss();
			}
		});
		AlertDialog alertDialog = builder.create();
		// alertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
		alertDialog.show();
	}

	public static boolean isCameraCanUse() {
		boolean canUse = true;
		String flashMode = null;
		Camera mCamera = null;
		try {
			mCamera = Camera.open();
			Camera.Parameters p = mCamera.getParameters();
			flashMode = p.getFlashMode();// 获取闪光灯的状态
		} catch (Exception e) {
			e.printStackTrace();
			canUse = false;
		}

		if (canUse) {
			mCamera.release();
			mCamera = null;
		}
		if (canUse && flashMode != null && flashMode.equals(Camera.Parameters.FLASH_MODE_ON)) {
			canUse = false;
		}
		return canUse;
	}

}
