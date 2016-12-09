//package com.runmit.sweedee.download;
//
//import android.content.Context;
//
//import com.runmit.sweedee.report.sdk.ReportActionSender;
//import com.runmit.sweedee.util.ApkInstaller;
//import com.runmit.sweedee.util.XL_Log;
//
//import java.io.File;
//import java.util.concurrent.atomic.AtomicBoolean;
//import java.util.concurrent.atomic.AtomicInteger;
//
///**
// * 文件下载器
// * 
// */
//public class FileDownloader implements Runnable {
//
//	private XL_Log log = new XL_Log(FileDownloader.class);
//
//	private static final int DOWNLOAD_THREAD_NUM = 5;
//
//	private Context mContext;
//
//	private AtomicBoolean mExit = new AtomicBoolean(false);
//
//	private DownloadInfo mDownloadInfo;
//
//	private OnStateChangeListener mOnStateChangeListener;
//
//	private DownloadSQLiteHelper mAppInstallSQLiteHelper;
//
//	private long mLastDownloadPos;
//
//	private long mLastComputeTime;
//
//	/**
//	 * 是否还存活，相当于线程是否结束
//	 * 默认是true，代表线程连执行都没执行的话，则它还在任务池中等待
//	 */
//	private boolean isAlive = true;
//
//	/**
//	 * 线程等待的锁
//	 */
//	private Object mSyncObj = new Object();
//
//	/**
//	 * 操作锁
//	 */
//	private Object mSyncActionObj = new Object();
//
//	private DownloadFileThread[] mThreads = null;
//	
//	private boolean hasError = false;
//	
//	private int errorcode;
//	
//	/**
//	 * 暂停位置
//	 */
//	private long mPausePos;
//
//	public boolean isAlive() {
//		return isAlive;
//	}
//
//	public DownloadInfo getDownloadInfo() {
//		return mDownloadInfo;
//	}
//
//	private OnThreadStateChangeListener mOnThreadStateChangeListener = new OnThreadStateChangeListener() {
//
//		private AtomicInteger mCondition = new AtomicInteger(DOWNLOAD_THREAD_NUM);
//
//		@Override
//		public synchronized void onDownloadThreadChange(int threadid, int state, int error) {
//			log.debug("threadid=" + threadid + ",state=" + state + ",errorcode=" + error);
//			if (state == OnThreadStateChangeListener.STATE_FAIL) {// 如果某个线程下载失败，则暂停所有任务，并且回调下载失败消息给UI
//				hasError = true;
//				errorcode = error;
//				pauseTaks();
//				synchronized (mSyncObj) {
//					mSyncObj.notifyAll();
//				}
//			} else {
//				int count = mCondition.decrementAndGet();
//				log.debug("count=" + count);
//				if (count == 0) {
//					synchronized (mSyncObj) {
//						mSyncObj.notifyAll();
//					}
//				}
//			}
//		}
//	};
//
//	/**
//	 * 
//	 * @param context
//	 * @param downloadInfo
//	 * @param onStateChangeListener
//	 */
//	public FileDownloader(Context context, DownloadInfo downloadInfo, OnStateChangeListener onStateChangeListener) {
//		this.mContext = context;
//		this.mDownloadInfo = downloadInfo;
//		this.mOnStateChangeListener = onStateChangeListener;
//		this.mAppInstallSQLiteHelper = new DownloadSQLiteHelper(context);
//		mLastDownloadPos = downloadInfo.getDownPos();
//		if (mLastDownloadPos != 0) {
//			mPausePos = mLastDownloadPos;
//		}
//		mLastComputeTime = System.currentTimeMillis();
//		mThreads = new DownloadFileThread[DOWNLOAD_THREAD_NUM];
//	}
//
//	@Override
//	public void run() {
//		long startT = System.currentTimeMillis();
//
//		synchronized (mSyncActionObj) {
//			if (!mExit.get()) {
//				isAlive = true;
//				mDownloadInfo.setDownloadState(DownloadInfo.STATE_RUNNING);
//				mOnStateChangeListener.onDownloadStateChange(mDownloadInfo, DownloadInfo.STATE_RUNNING);
//				mAppInstallSQLiteHelper.update(mDownloadInfo);
//
//				HttpConnLoader mHttpConnLoader = new HttpConnLoader(mContext, mDownloadInfo);
//				long blockSize = (mDownloadInfo.mFileSize + DOWNLOAD_THREAD_NUM - 1) / DOWNLOAD_THREAD_NUM;
//				File file = new File(mDownloadInfo.mPath);
//				
//				for (int i = 0; i < mThreads.length; i++) {
//					mThreads[i] = new DownloadFileThread(mHttpConnLoader, mDownloadInfo, file, blockSize, i, mOnThreadStateChangeListener);
//					mThreads[i].setName("DownloadThread:" + i);
//					mThreads[i].start();
//				}
//			}
//		}
//		log.debug("mExit="+mExit+",isAlive="+isAlive);
//		if (!mExit.get()) {
//			synchronized (mSyncObj) {// 同步等待线程返回消息
//				try {
//					mSyncObj.wait();
//				} catch (InterruptedException e) {
//					e.printStackTrace();
//				}
//			}
//		}
//
//		checkState(false);
//		
//		//////////////////////////上报代码
//		long endT = System.currentTimeMillis();
//		long coastT = endT - startT;
//		long curDownPos = mDownloadInfo.getDownPos();
//		long downloadSize = 0;
//		if (curDownPos < mDownloadInfo.mFileSize) {
//			mPausePos = curDownPos;
//		}
//		downloadSize = curDownPos - mPausePos;
//		if(mDownloadInfo.downloadType == DownloadInfo.DOWNLOAD_TYPE_APP || mDownloadInfo.downloadType == DownloadInfo.DOWNLOAD_TYPE_GAME) {
//			AppDownloadInfo downloadInfo = (AppDownloadInfo) mDownloadInfo;
//			ReportActionSender.getInstance().appdownloadSpeed(downloadInfo.versioncode + "", downloadInfo.mAppId, downloadInfo.mAppkey, mDownloadInfo.downloadType, downloadSize, coastT,
//					System.currentTimeMillis(), mDownloadInfo.downloadUrl);
//		}
//		/////////////////////////////////////
//		isAlive = false;
//	}
//
//	public long computeSpeed() {
//		long curDownPos = mDownloadInfo.getDownPos();
//		long totalDownloadByte = (curDownPos - mLastDownloadPos) / 1024;// byte
//		float time = (System.currentTimeMillis() - mLastComputeTime) / 1000.0f;// 毫秒换算成秒
//		long mSeed = (long) (totalDownloadByte / time);
//		mLastComputeTime = System.currentTimeMillis();
//		mLastDownloadPos = curDownPos;
//		log.debug("mSeed=" + mSeed + ",curDownPos=" + curDownPos + ",mLastDownloadPos=" + mLastDownloadPos + ",mDownloadInfo=" + mDownloadInfo);
//		return mSeed;
//	}
//
//	private void checkState(boolean isUser) {
//		if (mDownloadInfo.getDownPos() >= mDownloadInfo.mFileSize && !isUser) {
//			mDownloadInfo.finishtime = Long.toString(System.currentTimeMillis());
//			mDownloadInfo.setDownloadState(DownloadInfo.STATE_FINISH);
//			ApkInstaller.installApk(mContext, mDownloadInfo);
//		} else {
//			if(hasError){
//				mDownloadInfo.setDownloadState(DownloadInfo.STATE_FAILED);
//				mDownloadInfo.errorCode = errorcode;
//			}else{
//				mDownloadInfo.setDownloadState(DownloadInfo.STATE_PAUSE);
//			}
//		}
//		mAppInstallSQLiteHelper.update(mDownloadInfo);
//		mOnStateChangeListener.onDownloadStateChange(mDownloadInfo, mDownloadInfo.getState());
//		log.debug("mDownloadInfo.mDownPos=" + mDownloadInfo.getDownPos() + ",mDownloadInfo.mFileSize=" + mDownloadInfo.mFileSize + ",exit=" + mExit + ",isAlive=" + isAlive);
//	}
//
//	/**
//	 * 暂停下载
//	 */
//	public void pauseTaks() {
//		log.debug("mDownloadInfo="+mDownloadInfo+",currentState="+Thread.currentThread().getState());
//		synchronized (mSyncActionObj) {
//			this.mExit.set(true);
//			boolean isThreadEmpty = false;
//			for (int i = 0; i < mThreads.length; i++) {
//				DownloadFileThread thread = mThreads[i];
//				log.debug("thread="+thread);
//				if (thread != null) {
//					thread.setStop();
//				}else{
//					isThreadEmpty = true;
//				}
//			}
//			if(isThreadEmpty){//如果线程压根没启动，则这个任务是等待状态
//				checkState(false);
//			}
//		}
//	}
//
//	@Override
//	public String toString() {
//		return "FileDownloader [exit=" + mExit + ", mLastDownloadPos=" + mLastDownloadPos + ", mLastComputeTime=" + mLastComputeTime + ", isAlive=" + isAlive + "]";
//	}
//
//}
