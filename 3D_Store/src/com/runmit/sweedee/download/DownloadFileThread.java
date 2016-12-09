package com.runmit.sweedee.download;

import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.util.XL_Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.net.HttpURLConnection;


/**
 * 文件下载类
 * 
 * @author qihui.chen
 * @2015-07-28
 */
public class DownloadFileThread extends Thread {

	private static final String TAG = DownloadFileThread.class.getSimpleName();

	private XL_Log log = new XL_Log(DownloadFileThread.class);

	/** 当前已经下载了的文件长度 */
	private long mHasDownloadLength = 0;
	/** 文件保存路径 */
	private File mFile = null;
	/** 当前下载线程ID */
	private int mThreadId = 0;
	/** 线程下载数据长度 */
	private long mBlockSize = 0;
	/** 线程下载的起始坐标点 */
	private long mDownPos = 0;

	private DownloadInfo mDownloadInfo = null;

	private DownloadSQLiteHelper mHelper = null;

	private static final int BUFFER_SIZE = 24 * 1024;

	private boolean mIsStop = false;

	private OnThreadStateChangeListener mOnThreadStateChangeListener;

	private HttpConnLoader mHttpConnLoader;

	/**
	 * 
	 * @param httpConnLoader
	 *            :文件下载地址
	 * @param file
	 *            :文件本地保存
	 * @param blocksize
	 *            :下载数据长度
	 * @param threadId
	 *            :线程ID
	 */
	public DownloadFileThread(HttpConnLoader httpConnLoader, DownloadInfo downloadInfo, File file, long blocksize, int threadId, OnThreadStateChangeListener onThreadStateChangeListener) {
		this.mHttpConnLoader = httpConnLoader;
		this.mOnThreadStateChangeListener = onThreadStateChangeListener;
		this.mDownloadInfo = downloadInfo;
		this.mFile = file;
		this.mThreadId = threadId;
		this.mBlockSize = blocksize;
		this.mDownPos = downloadInfo.mHasDownloadPosses[threadId];
		this.mHelper = new DownloadSQLiteHelper(StoreApplication.INSTANCE);
		log.debug("blocksize=" + blocksize + ",threadId=" + threadId + ",file=" + file + ",mDownPos=" + mDownPos);
	}

	@Override
	public void run() {
		BufferedInputStream bis = null;
		RandomAccessFile raf = null;
		HttpURLConnection conn = null;
		try {
			long startPos = mBlockSize * mThreadId + mDownPos;// 起始位置等于当前块的位置加上当前线程的已经读写的相对位置
			long endPos = mBlockSize * (mThreadId + 1) - 1;// 结束位置
			startPos = Math.min(startPos, mDownloadInfo.mFileSize);
			endPos = Math.min(endPos, mDownloadInfo.mFileSize);
			if (startPos >= endPos) {// 当前线程已经下载完成，则直接返回
				mOnThreadStateChangeListener.onDownloadThreadChange(mThreadId, OnThreadStateChangeListener.STATE_SUCCESS, 0);
				return;
			}
			conn = mHttpConnLoader.getHttpURLConnection(startPos, endPos);
			
			if (conn == null) {
				throw new RuntimeException("get httpurlconnection is error and url is = " + mDownloadInfo.downloadUrl);
			}
			byte[] buffer = new byte[BUFFER_SIZE];
			bis = new BufferedInputStream(conn.getInputStream());
			raf = new RandomAccessFile(mFile, "rwd");
			raf.seek(startPos);
			int len;
			while ((len = bis.read(buffer, 0, BUFFER_SIZE)) != -1 && !mIsStop) {
				raf.write(buffer, 0, len);
				mDownPos += len;
				mDownloadInfo.mHasDownloadPosses[mThreadId] = mDownPos;
			}
			log.debug("threadId=" + mThreadId + ",startPos=" + startPos + ",endPos=" + endPos+",mIsStop="+mIsStop+",len="+len+",filesize="+mDownloadInfo.mFileSize);
			mOnThreadStateChangeListener.onDownloadThreadChange(mThreadId, OnThreadStateChangeListener.STATE_SUCCESS, 0);
		} catch (Exception e) {
			int errorCode;
			if(e.getMessage().contains("ENOSPC")){
				errorCode = ExceptionCode.NoSpaceException;
			}else if(e instanceof RuntimeException){
				errorCode = ExceptionCode.GetConnectionException;
			}else{
				errorCode = ExceptionCode.getErrorCode(e);
			}
//			ReportActionSender.getInstance().appdownloadFailed(mDownloadInfo.versioncode + "", mDownloadInfo.mAppId, mDownloadInfo.mAppkey, mDownloadInfo.downloadType, System.currentTimeMillis(),
//					errorCode);
			mOnThreadStateChangeListener.onDownloadThreadChange(mThreadId, OnThreadStateChangeListener.STATE_FAIL, errorCode);
			e.printStackTrace();
		} finally {
			if (bis != null) {
				try {
					bis.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
			if (raf != null) {
				try {
					raf.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
			if (conn != null) {
				conn.disconnect();
			}
		}
	}

	/**
	 * 线程下载文件长度
	 */
	public long getDownloadLength() {
		return mHasDownloadLength;
	}

	public boolean getIsStop() {
		return mIsStop;
	}

	public void setStop() {
		log.debug("setStop mDownloadInfo="+mDownloadInfo);
		mIsStop = true;
	}
}
