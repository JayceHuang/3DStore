package com.runmit.sweedee.download;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.Handler;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.database.dao.AppInstallSQLDao;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.Game3DConfigManager;
import com.runmit.sweedee.manager.data.EventCode;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.util.ApkInstaller;
import com.runmit.sweedee.util.EnvironmentTool;
import com.runmit.sweedee.util.TDString;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

import java.io.File;

public class DownloadCreater {

	private XL_Log log = new XL_Log(DownloadCreater.class);

	private static Object mSyncObj = new Object();

	private static boolean isGetUrl = false;

	private Context mContext;

	private ProgressDialog mProgressDialog;

	private AppItemInfo mAppItemInfo;

	private OnStateChangeListener mOnStateChangeListener;

	public DownloadCreater(Context context) {
		this.mContext = context;
		mProgressDialog = new ProgressDialog(mContext, R.style.ProgressDialogDark);
	}

	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			int ret = msg.arg1;
			switch (msg.what) {
			case EventCode.CmsEventCode.EVENT_GET_APP_GSLB_URL:
				log.debug("ret=" + ret);
				if (ret != 0) {
					Toast.makeText(StoreApplication.INSTANCE, StoreApplication.INSTANCE.getString(R.string.appdetail_download_error), Toast.LENGTH_LONG).show();
					isGetUrl = false;
					synchronized (mSyncObj) {
						mSyncObj.notifyAll();
					}
					Util.dismissDialog(mProgressDialog);
				}
				break;
			case EventCode.CmsEventCode.EVENT_GET_APP_CDN_URL:
				if (ret == 0) {
					String url = (String) msg.obj;
					log.debug("url=" + url);
					AppDownloadInfo downloadInfo = new AppDownloadInfo();
					downloadInfo.downloadUrl = url;
					downloadInfo.mAppkey = mAppItemInfo.appKey;
					downloadInfo.mTitle = mAppItemInfo.title;

					downloadInfo.mAppId = mAppItemInfo.appId;
					downloadInfo.mFileSize = mAppItemInfo.fileSize;
					downloadInfo.downloadType = mAppItemInfo.type;
					downloadInfo.pkgName = mAppItemInfo.packageName;
					downloadInfo.versioncode = mAppItemInfo.versionCode;
					downloadInfo.starttime = Long.toString(System.currentTimeMillis());
					downloadInfo.setDownloadState(AppDownloadInfo.STATE_WAIT);
					downloadInfo.mPath = EnvironmentTool.getFileDownloadCacheDir() + downloadInfo.pkgName+".apk";

					DownloadSQLiteHelper appInstallSQLiteHelper = new DownloadSQLiteHelper(StoreApplication.INSTANCE);
					appInstallSQLiteHelper.insertData(downloadInfo);

					// 首先触发配置文件的下载
					if (mAppItemInfo.type == AppItemInfo.TYPE_GAME) {
						Game3DConfigManager.getInstance().downloadSingleConfigFile(mAppItemInfo.packageName, mAppItemInfo.versionCode);
					}

					AppInstallSQLDao dbHelper = AppInstallSQLDao.getInstance(StoreApplication.INSTANCE);
					if (dbHelper != null) {
						AppInstallSQLDao.AppDataModel dataModel = new AppInstallSQLDao.AppDataModel();
						dataModel.package_name = mAppItemInfo.packageName;
						dataModel.app_type = mAppItemInfo.type;
						dataModel.appId = mAppItemInfo.appId;
						dataModel.appKey = mAppItemInfo.appKey;
						dataModel.startTime = "" + System.currentTimeMillis();
						dbHelper.insertData(dataModel);
					}
					DownloadManager.getInstance().startDownload(downloadInfo, mOnStateChangeListener);
				} else {
					Toast.makeText(StoreApplication.INSTANCE, StoreApplication.INSTANCE.getString(R.string.appdetail_download_error), Toast.LENGTH_LONG).show();
				}
				isGetUrl = false;
				synchronized (mSyncObj) {
					mSyncObj.notifyAll();
				}
				Util.dismissDialog(mProgressDialog);
				break;
			default:
				break;
			}
		}
	};

	public void startDownload(final AppItemInfo appItemInfo, final OnStateChangeListener onStateChangeListener) {
		new Thread(new Runnable() {
			@Override
			public void run() {
				mAppItemInfo = appItemInfo;
				mOnStateChangeListener = onStateChangeListener;
				DownloadSQLiteHelper appInstallSQLiteHelper = new DownloadSQLiteHelper(StoreApplication.INSTANCE);
				AppDownloadInfo downloadInfo;
				if(appItemInfo.mDownloadInfo!=null){
					 downloadInfo =appItemInfo.mDownloadInfo;// 数据库内容	
				}else{
					 downloadInfo = (AppDownloadInfo) appInstallSQLiteHelper.findDownloadInfoByBeanKey(mAppItemInfo.packageName);// 数据库内容
				}
				log.debug("pkgName=" + mAppItemInfo.packageName + ",versionCode=" + mAppItemInfo.versionCode + "mDownloadInfo=" + downloadInfo);
				if (downloadInfo != null && downloadInfo.versioncode < appItemInfo.versionCode) {// 如果本地下载版本小于服务器最新版本
					DownloadManager.getInstance().deleteTask(downloadInfo);// 删除下载任务，同时置空
					downloadInfo = null;
				}
				if (downloadInfo != null && downloadInfo.isFinish()) {// 判断是否下载完成
					File file = new File(downloadInfo.mPath);
					log.debug("The local uri is " + downloadInfo.mPath);
					if (file != null && file.exists()) {
						ApkInstaller.installApk(mContext, downloadInfo);
						return;
					} else {
						DownloadManager.getInstance().deleteTask(downloadInfo);
						downloadInfo = null;
					}
				}
				if (downloadInfo == null) {// 创建一个新的下载任务
					while (isGetUrl) {
						synchronized (mSyncObj) {
							try {
								mSyncObj.wait();
							} catch (InterruptedException e) {
								e.printStackTrace();
							}
						}
					}
					isGetUrl = true;
					mHandler.post(new Runnable() {
						@Override
						public void run() {
							Util.showDialog(mProgressDialog, TDString.getStr(R.string.download_app_url));
						}
					});
					CmsServiceManager.getInstance().getAppGslbUrl(mAppItemInfo, mHandler);
				} else {// 更新下载状态
					mAppItemInfo.mDownloadInfo = downloadInfo;
					DownloadManager.getInstance().startDownload(downloadInfo, onStateChangeListener);
				}
			}
		}).start();
	}
}
