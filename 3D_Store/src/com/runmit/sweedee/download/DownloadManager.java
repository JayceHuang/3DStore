package com.runmit.sweedee.download;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.FileObserver;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.util.ApkInstaller;
import com.runmit.sweedee.util.EnvironmentTool;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

public class DownloadManager {

	private static DownloadManager INSTANCE;

	private static final int THREAD_POOL = StoreApplication.isSnailPhone ? 1 : 3;

	private final FetureThreadPoolExecutor mExecutor;

	private HashMap<String, Set<OnStateChangeListener>> mDownloadListener = new HashMap<String, Set<OnStateChangeListener>>();

	private List<OnDownloadDataChangeListener> mOnProgressUpdateList = new ArrayList<OnDownloadDataChangeListener>();

	private Handler mHandler = new Handler(StoreApplication.INSTANCE.getMainLooper());// 此Handler用于切换到主线程

	private static final int DEFINE_SCROLL_TIME = 1 * 1000;

	private XL_Log log = new XL_Log(DownloadManager.class);

	private DownloadSQLiteHelper mDownloadSQLiteHelper;

	private Handler mThreadHandler;// 此thread是执行数据库操作或者其他一些需要保持同步操作的线程

	private static final int MSG_UPDATE_TASK = 10000;

	private SDCardListener mSDCardListener;
	
	private List<DownloadInfo> mDownloadInfoList;

	public List<DownloadInfo> getDownloadInfoList() {
		return getDownloadInfoList(false);
	}
	
	public List<DownloadInfo> getDownloadInfoList(boolean realTime) {
		if(realTime){
			return mDownloadSQLiteHelper.getAllDownloadLists(); 
		}
		return mDownloadInfoList;
	}

	private DownloadManager() {
		HandlerThread mHandlerThread = new HandlerThread("DownloadEngine_HandlerThread");
		mHandlerThread.start();
		mThreadHandler = new Handler(mHandlerThread.getLooper()) {
			@Override
			public void handleMessage(Message msg) {
				switch (msg.what) {
				case MSG_UPDATE_TASK:
					update();
					mThreadHandler.sendEmptyMessageDelayed(MSG_UPDATE_TASK, DEFINE_SCROLL_TIME);
					break;
				default:
					break;
				}
			}
		};

		mDownloadSQLiteHelper = new DownloadSQLiteHelper(StoreApplication.INSTANCE);
		mExecutor = new FetureThreadPoolExecutor(THREAD_POOL, THREAD_POOL, 0L, TimeUnit.MILLISECONDS, new LinkedBlockingQueue<Runnable>());
		checkDownloadState();
		mThreadHandler.sendEmptyMessageDelayed(MSG_UPDATE_TASK, DEFINE_SCROLL_TIME);
		mSDCardListener = new SDCardListener(EnvironmentTool.getFileDownloadCacheDir());
		mSDCardListener.startWatching();
	}

	public static DownloadManager getInstance() {
		if (INSTANCE == null) {
			INSTANCE = new DownloadManager();
		}
		return INSTANCE;
	}

	/**
	 * 暂停任务 同时移除下载队列
	 * 
	 * @param downloadInfo
	 * @return
	 */
	public void pauseDownloadTask(final DownloadInfo downloadInfo) {
		mThreadHandler.post(new Runnable() {
			@Override
			public void run() {
				FileDownFutureTask fileDownloader = mExecutor.getFileDownloader(downloadInfo.getBeanKey());
				log.debug("mFileDownloader=" + fileDownloader+",downloadInfo="+downloadInfo);
				if (fileDownloader != null) {
					fileDownloader.pauseTaks();
					mExecutor.remove(fileDownloader);
				}
			}
		});
	}

	public void startDownload(final DownloadInfo downloadInfo, final OnStateChangeListener onStateChangeListener) {
		log.debug("downloadInfo="+downloadInfo);
		if (downloadInfo != null && (downloadInfo.isFinish() || downloadInfo.isRunning())) {
			return;//如果任务状态是下载，完成状态则直接返回
		}
		
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				if (onStateChangeListener != null) {
					Set<OnStateChangeListener> mSet = mDownloadListener.get(downloadInfo.getBeanKey());
					log.debug("mSet=" + mSet);
					if (mSet == null) {
						mSet = new HashSet<OnStateChangeListener>();
						mSet.add(onStateChangeListener);
						mDownloadListener.put(downloadInfo.getBeanKey(), mSet);
					} else {
						if (!mSet.contains(onStateChangeListener)) {
							mSet.add(onStateChangeListener);
						}
					}
				}
			}
		});

		mThreadHandler.post(new Runnable() {
			@Override
			public void run() {
				FileDownFutureTask fileDownloader = mExecutor.getFileDownloader(downloadInfo.getBeanKey());
				log.debug("mFileDownloader=" + fileDownloader + ",mExecutor=" + mExecutor.getActiveCount());
				if (fileDownloader == null || !fileDownloader.isAlive()) {// 如果为空或者已经运行完
					if(fileDownloader != null){
						mExecutor.remove(fileDownloader);
					}
					fileDownloader = FileDownFutureTask.createInstance(StoreApplication.INSTANCE, downloadInfo, mFileDownloadStateChangeListener);

					if (mExecutor.getActiveCount() >= THREAD_POOL) {// 下载任务池已经满了
						log.debug("mDownloadInfo=" + downloadInfo);
						downloadInfo.setDownloadState(AppDownloadInfo.STATE_WAIT);// 设置下载状态为等待下载，同时更新到数据库
						mDownloadSQLiteHelper.update(downloadInfo);
						mFileDownloadStateChangeListener.onDownloadStateChange(downloadInfo, downloadInfo.getState());// 此时应该是下载等待，回调一个下载等待状态给界面
					}
					mExecutor.submit(fileDownloader);
				}else{
					
				}
			}
		});
	}

	private OnStateChangeListener mFileDownloadStateChangeListener = new OnStateChangeListener() {

		@Override
		public void onDownloadStateChange(final DownloadInfo downloadInfo, final int new_state) {
			log.debug("onDownloadStateChange new_state=" + new_state + ",downloadInfo=" + downloadInfo);
			mHandler.post(new Runnable() {
				@Override
				public void run() {
					if (new_state == AppDownloadInfo.STATE_FAILED) {
						Util.showToast(StoreApplication.INSTANCE, "下载失败，请重试", Toast.LENGTH_SHORT);
					}

					Set<OnStateChangeListener> mSet = mDownloadListener.get(downloadInfo.getBeanKey());
					log.debug("mSet=" + mSet);
					if (mSet != null) {
						for (OnStateChangeListener onStateChangeListener : mSet) {
							log.debug("onStateChangeListener=" + onStateChangeListener + ",new_state=" + new_state);
							onStateChangeListener.onDownloadStateChange(downloadInfo, new_state);
						}
					}
				}
			});
		}
	};

	private void stopAllDownloadTask() {
		mExecutor.pauseAllTask();
		mExecutor.shutdown();

		mDownloadListener.clear();
		mOnProgressUpdateList.clear();
		INSTANCE = null;
	}

	/**
	 * 传入一个AppItemInfo，绑定一个DownloadInfo 此DownloadInfo里面保存下载信息和安装信息
	 * 
	 * @return
	 */
	public AppDownloadInfo bindDownloadInfo(AppItemInfo mAppItemInfo) {
		Log.e("", "bindDownLoadInfo");
		AppDownloadInfo downloadInfo = (AppDownloadInfo) getDownloadInfo(mAppItemInfo.packageName);// 数据库内容
		if (downloadInfo != null) {// 检查本地文件在数据库是否被删除
			if (downloadInfo.getDownPos() > 0 && !ApkInstaller.fileExist(downloadInfo.mPath)) {
				deleteTask(downloadInfo);
				downloadInfo = null;
			}
		}
		mAppItemInfo.installState = getInstallState(mAppItemInfo);
		return downloadInfo;
	}

	public static int getInstallState(AppItemInfo mAppItemInfo) {
		if (mAppItemInfo == null) {
			return 0;
		}
		try {
			PackageManager manager = StoreApplication.INSTANCE.getPackageManager();
			PackageInfo info = manager.getPackageInfo(mAppItemInfo.packageName, 0);
			Log.d("getInstallState", "current_version=" + info.versionCode + ",mAppItemInfo=" + mAppItemInfo.versionCode);
			if (info.versionCode < mAppItemInfo.versionCode) {
				return AppDownloadInfo.STATE_UPDATE;
			} else {
				return AppDownloadInfo.STATE_INSTALL;
			}
		} catch (NameNotFoundException e) {
		}
		return 0;
	}
	
	public boolean haveTaskRunning(){
		return mExecutor.getActiveCount() > 0;
	}
	
	public static int  getAppInstallState(String pkgName, int v) {
		try {
			PackageManager manager = StoreApplication.INSTANCE.getPackageManager();
			PackageInfo info = manager.getPackageInfo(pkgName, 0);
			Log.d("getInstallState", "current_version=" + info.versionCode + ",mAppItemInfo=" + v);
			if (info.versionCode < v) {
				return AppDownloadInfo.STATE_UPDATE;
			} else {
				return AppDownloadInfo.STATE_INSTALL;
			}
		} catch (NameNotFoundException e) {
		}
		return 0;
	}

	public DownloadInfo getDownloadInfo(String packageName) {
		DownloadInfo mDownloadInfo = mDownloadSQLiteHelper.findDownloadInfoByBeanKey(packageName);// 数据库内容
		return mDownloadInfo;
	}
	
	public DownloadInfo getDownloadInfoById(int downloadId) {
		for(DownloadInfo info : mDownloadInfoList){
			if(info.downloadId == downloadId)
				return info;
		}
		return null;
	}
	
	public void updateInfo(final DownloadInfo mDownloadInfo) {
		mDownloadSQLiteHelper.update(mDownloadInfo);
	}

	public void deleteTask(final DownloadInfo mDownloadInfo) {
		deleteTask(mDownloadInfo, false);
	}

	public void deleteTask(final DownloadInfo downloadInfo, final boolean deleteLocalFile) {
		mThreadHandler.post(new Runnable() {
			@Override
			public void run() {
				pauseDownloadTask(downloadInfo);// 首先暂停下载任务
				mDownloadSQLiteHelper.clearDownloaded(downloadInfo.downloadId);
				if (deleteLocalFile) {
					File file = new File(downloadInfo.mPath);
					if (file.exists()) {
						file.delete();
					}
				}
				update();
			}
		});
	}

	/**
	 * 检查下载状态和判断本地文件是否存在因为可能在应用异常退出时候下载状态不对
	 */
	private void checkDownloadState() {

		mThreadHandler.post(new Runnable() {

			@Override
			public void run() {
				final ArrayList<DownloadInfo> mDownloadList = mDownloadSQLiteHelper.getAllDownloadLists();
				for (DownloadInfo mDownloadInfo : mDownloadList) {
					log.debug("mDownloadInfo.getState="+mDownloadInfo);
					if (mDownloadInfo.getState() == AppDownloadInfo.STATE_RUNNING || mDownloadInfo.getState() == AppDownloadInfo.STATE_WAIT) {
						mDownloadInfo.setDownloadState(DownloadInfo.STATE_WAIT);
						startDownload(mDownloadInfo, null);
					}
					if (mDownloadInfo.isFinish() || mDownloadInfo.getDownPos() > 0) {
						// 已经下载完成，或者已经下载了一些
						File f = new File(mDownloadInfo.mPath);
						if (!f.exists()) {// 如果文件不存在，则重置任务
							deleteTask(mDownloadInfo);
						}
					}
				}
			}
		});
	}

	public void onDestroy() {
		mThreadHandler.removeCallbacksAndMessages(null);
		mThreadHandler.removeMessages(MSG_UPDATE_TASK);
		mSDCardListener.stopWatching();
		stopAllDownloadTask();
	}

	private synchronized void update() {
		mDownloadInfoList = mDownloadSQLiteHelper.getAllDownloadLists();
		for (DownloadInfo downloadInfo : mDownloadInfoList) {
			if (downloadInfo.isRunning()) {
				FileDownFutureTask mFileDownloader = mExecutor.getRunningFileDownloader(downloadInfo.getBeanKey());
				log.debug("synchronized mFileDownloader=" + mFileDownloader+",downloadInfo="+downloadInfo);
				if (mFileDownloader != null) {
					downloadInfo.mHasDownloadPosses = mFileDownloader.getDownloadInfo().mHasDownloadPosses;// 如果是正在下载的任务，则使用FileDownloader里面的downloadInfo，因为正在下载任务进度没有实时保存数据库
					downloadInfo.downloadSpeed = mFileDownloader.computeSpeed();

					log.debug("synchronized downloadInfo=" + downloadInfo.getDownloadProgress() + ",downloadInfo=" + downloadInfo);
				}
			}
		}
		log.debug("update mDownloadInfoList="+mDownloadInfoList);
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				for (OnDownloadDataChangeListener mOnDownloadStateChangeListener : mOnProgressUpdateList) {
					if (mOnDownloadStateChangeListener != null) {
						mOnDownloadStateChangeListener.onDownloadDataChange(mDownloadInfoList);
					}
				}
			}
		});
	}

	public class SDCardListener extends FileObserver {

		public SDCardListener(String path) {
			super(path, FileObserver.DELETE);
		}

		@Override
		public void onEvent(final int event, final String path) {
			switch (event) {
			case FileObserver.DELETE:
				log.debug("onEvent event=" + event + ",path=" + path);
				mThreadHandler.post(new Runnable() {

					@Override
					public void run() {
						if (path != null && path.endsWith(".apk")) {
							String packagename = path.substring(0, path.lastIndexOf(".apk"));
							log.debug("packagename=" + packagename);
							if (!TextUtils.isEmpty(packagename)) {
								final DownloadInfo mDownloadInfo = getDownloadInfo(packagename);
								if (mDownloadInfo != null) {
									deleteTask(mDownloadInfo);
								}
							}
						}
					}
				});
				break;
			}
		}
	}

	void onAppInstallOrUnistall(final String packagename, final boolean isInstall) {
		final DownloadInfo mDownloadInfo = getDownloadInfo(packagename);
		log.debug("mDownloadInfo=" + mDownloadInfo + ",packagename=" + packagename + ",isInstall=" + isInstall);
		if (mDownloadInfo != null) {
			File file = new File(mDownloadInfo.mPath);
			log.debug("onAppInstallOrUnistall = " + mDownloadInfo.mPath);
			if (file != null && file.exists()) {
				file.delete();
			}
			deleteTask(mDownloadInfo);
			try {
				PackageInfo info = StoreApplication.INSTANCE.getPackageManager().getPackageInfo(packagename, 0);
				AppDownloadInfo downloadInfo = null;
				if(mDownloadInfo.downloadType == DownloadInfo.DOWNLOAD_TYPE_APP || mDownloadInfo.downloadType == DownloadInfo.DOWNLOAD_TYPE_GAME){
					downloadInfo = (AppDownloadInfo)mDownloadInfo;
				}
				if (info != null) {
					String versionName = info.versionName;
					ReportActionSender.getInstance().appinstall(downloadInfo.mAppkey, versionName, downloadInfo.mAppId, mDownloadInfo.downloadType, System.currentTimeMillis());
				} else if (info == null && !isInstall) {
					ReportActionSender.getInstance().appUninstall(downloadInfo.versioncode + "", downloadInfo.mAppId, downloadInfo.mAppkey, mDownloadInfo.downloadId, System.currentTimeMillis());
				}
			} catch (NameNotFoundException e) {
				e.printStackTrace();
			}
		} 
		mHandler.post(new Runnable() {

			@Override
			public void run() {
				for (OnDownloadDataChangeListener mOnNetWorkChangeListener : mOnProgressUpdateList) {
					if (mOnNetWorkChangeListener != null) {
						mOnNetWorkChangeListener.onAppInstallChange((AppDownloadInfo) mDownloadInfo, packagename, isInstall);
					}
				}
			}
		});
	}

	public void addOnDownloadDataChangeListener(final OnDownloadDataChangeListener mListener) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mOnProgressUpdateList.add(mListener);
			}
		});
	}

	public void removeOnDownloadDataChangeListener(final OnDownloadDataChangeListener mListener) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mOnProgressUpdateList.remove(mListener);
			}
		});

	}

	public interface OnDownloadDataChangeListener {

		void onDownloadDataChange(List<DownloadInfo> mDownloadInfos);

		void onAppInstallChange(AppDownloadInfo mDownloadInfo, String packagename, boolean isInstall);
	}

}
