package com.runmit.sweedee.downloadinterface;

import java.io.File;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import android.content.Context;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.DisplayMetrics;

import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.download.DownloadCreater;
import com.runmit.sweedee.download.DownloadInfo;
import com.runmit.sweedee.download.DownloadSQLiteHelper;
import com.runmit.sweedee.download.VideoDownloadInfo;
import com.runmit.sweedee.model.TaskInfo;
import com.runmit.sweedee.model.TaskInfo.LOCAL_STATE;
import com.runmit.sweedee.util.EnvironmentTool;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

/**
 * @author DJ 类功能：缓存库接口的Java层封装，综合了缓存库的所有业务
 */
public class DownloadEngine {

	XL_Log log = new XL_Log(DownloadEngine.class);

	static {
		System.loadLibrary("superd_common");
		System.loadLibrary("xml");
		System.loadLibrary("embed_download");
		System.loadLibrary("embed_download_manager");
		System.loadLibrary("superd_download_android");
	}

	public void initEtmAsync() {
		new Thread(new Runnable() {
			@Override
			public void run() {
				Context mContext = StoreApplication.INSTANCE;
				// 获取各项系统参数
				String package_version = EnvironmentTool.getPackageVersion();// 软件版本
				String system_info = EnvironmentTool.getSimpleSystemInfo();// 系统简略信息
				TelephonyManager tm = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
				String imei = tm.getDeviceId();// String
				if (TextUtils.isEmpty(imei)) {
					imei = "empty_imei";
				}
				log.debug("imei=" + imei);
				String etm_system_data_path = EnvironmentTool.getInternalDataPath("rtm");// rtm配置数据路径
				DisplayMetrics screen_info = EnvironmentTool.getScreenSize();// 屏幕信息
				int channel = 0;// 产品渠道id,默认为官方渠道
				try {
					channel = Integer.parseInt(Util.getPartnerID(mContext).replace("0x", ""), 16);// 源字符串是16进制的表示
					log.debug("channel = " + channel);
				} catch (Exception e) {
					log.debug("Exception channel = " + channel);
				}
				// 初始换缓存库
				initEtm(DownloadEngine.this, etm_system_data_path, package_version, channel, 39/*
																								 * Constant
																								 * .
																								 * PRODUCT_ID
																								 */, system_info, imei, screen_info.widthPixels, screen_info.heightPixels);

				getLocalTaskList();
			}
		}).start();
	}

	public void jniCall_switchThread(int callBack, int userData) {

	}

	private native TaskInfo[] getAllLocalTasks();

	private native int deleteTask(int task_id, boolean is_delete_file);// 删除本地任务

	private native int initEtm(DownloadEngine engine, String etm_system_data_path, String package_version, int channel, int product_id, String system_info, String imei, int screen_width,
			int screen_height);

	private void getLocalTaskList() {
		TaskInfo[] mArrayList = getAllLocalTasks();
		log.debug("mArrayList=" + mArrayList);
		if (mArrayList != null && mArrayList.length > 0) {
			for (int i = mArrayList.length - 1; i >= 0; i--) {
				TaskInfo mInfo = mArrayList[i];
				if (mInfo.mTaskState == LOCAL_STATE.SUCCESS) {// 下载完成任务
					String path = mInfo.mfilePath;
					if (!path.endsWith("/")) {
						path += "/";
					}
					path = path + mInfo.mFileName;
					File file = new File(path);

					if (file.exists()) {
						VideoDownloadInfo downloadInfo = new VideoDownloadInfo();
						downloadInfo.downloadUrl = mInfo.mUrl;
						downloadInfo.albumnId = mInfo.albumnId;
						downloadInfo.mode = mInfo.mode;
						downloadInfo.mSubTitleInfos = mInfo.mSubTitleInfos;
						downloadInfo.videoId = mInfo.videoId;
						downloadInfo.mFileSize = mInfo.fileSize;
						downloadInfo.mTitle = mInfo.mFileName;
						downloadInfo.poster = mInfo.poster;
						downloadInfo.downloadType = DownloadInfo.DOWNLOAD_TYPE_VIDEO;
						downloadInfo.starttime = "" + mInfo.startTime;
						downloadInfo.mPath = path;
						downloadInfo.setDownloadState(DownloadInfo.STATE_FINISH);

						DownloadSQLiteHelper downloadSQLiteHelper = new DownloadSQLiteHelper(StoreApplication.INSTANCE);
						downloadSQLiteHelper.insertData(downloadInfo);

						deleteTask(mInfo.mTaskId, false);// 不删除文件
					} else {
						deleteTask(mInfo.mTaskId, true);
					}
				} else {
					VideoDownloadInfo downloadInfo = new VideoDownloadInfo();
					downloadInfo.downloadUrl = mInfo.mUrl;
					downloadInfo.albumnId = mInfo.albumnId;
					downloadInfo.mode = mInfo.mode;
					downloadInfo.mSubTitleInfos = mInfo.mSubTitleInfos;
					downloadInfo.videoId = mInfo.videoId;
					downloadInfo.mTitle = mInfo.mFileName;
					downloadInfo.poster = mInfo.poster;
					downloadInfo.downloadType = DownloadInfo.DOWNLOAD_TYPE_VIDEO;
					downloadInfo.starttime = "" + mInfo.startTime;
					downloadInfo.mPath = EnvironmentTool.getFileDownloadCacheDir() + mInfo.mFileName;

					if (mInfo.fileSize <= 0) {
						try {
							URL url = new URL(mInfo.mUrl);
							HttpURLConnection conn = (HttpURLConnection) url.openConnection();
							conn.setConnectTimeout(5 * 1000);
							conn.setRequestMethod("GET");
							conn.setRequestProperty("Charset", "UTF-8");
							conn.setRequestProperty("Connection", "Keep-Alive");
							conn.setAllowUserInteraction(true);
							long filesize = 0;
							int responseCode = conn.getResponseCode();
							if (responseCode < 300) {
								filesize = conn.getContentLength();
								if (filesize > 0) {
									downloadInfo.mFileSize = filesize;
								} else {
									continue;
								}
							} else {
								continue;
							}
							conn.disconnect();
						} catch (IOException e) {
							e.printStackTrace();
						}
					} else {
						downloadInfo.mFileSize = mInfo.fileSize;
					}

					downloadInfo.setDownloadState(DownloadInfo.STATE_PAUSE);
					DownloadSQLiteHelper downloadSQLiteHelper = new DownloadSQLiteHelper(StoreApplication.INSTANCE);
					downloadSQLiteHelper.insertData(downloadInfo);
					deleteTask(mInfo.mTaskId, true);
				}
			}
		}
	}

}
