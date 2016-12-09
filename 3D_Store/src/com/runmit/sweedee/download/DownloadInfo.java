package com.runmit.sweedee.download;

import java.io.Serializable;

/**
 * 下载基础信息
 * @author Zhi.Zhang
 *
 */
public abstract class DownloadInfo implements Serializable {

	public static final int STATE_PAUSE = 0;
	public static final int STATE_RUNNING = STATE_PAUSE + 1;
	public static final int STATE_WAIT = STATE_RUNNING + 1;
	public static final int STATE_FINISH = STATE_WAIT + 1;
	public static final int STATE_FAILED = STATE_FINISH + 1;
	public static final int STATE_INSTALL = STATE_FAILED + 1;// 已经安装，虚拟下载状态，下载任务中不存在此状态，但是AppInfo的downloadState就有这个状态
	public static final int STATE_UPDATE = STATE_INSTALL + 1;// 可升级，虚拟下载状态，下载任务中不存在此状态，但是AppInfo的downloadState就有这个状态
	
	public static final int DOWNLOAD_TYPE_APP = 1;
	public static final int DOWNLOAD_TYPE_GAME = 2;
	public static final int DOWNLOAD_TYPE_VIDEO = 3;

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	/**
	 * 任务id
	 */
	public int downloadId;

	/**
	 * 用户额外数据，可用json封装起来
	 */
	public String mUserData;
	
	/**
	 * 任务展示标题
	 */
	public String mTitle;

	/**
	 * 任务开始时间
	 */
	public String starttime;
	
	/**
	 * 任务结束时间
	 */
	public String finishtime;
	
	/**
	 * 任务路径
	 */
	public String mPath;
	
	/**
	 * 文件总大小
	 */
	public long mFileSize;

	/*
	 * 个现在线程的当前下载下标
	 */
	public long[] mHasDownloadPosses = new long[5];

	/**
	 * 下载地址
	 */
	public String downloadUrl;
	
	/**
	 * 大类型：1:APP, 2:GAME,3:视频
	 */
	public int downloadType;
	
	/**
	 * 下载任务状态
	 */
	protected int state = STATE_PAUSE;
	
	/**
	 * 任务下载速度
	 */
	public long downloadSpeed;
	
	/**
	 * 任务错误码，仅针对STATE_FAILED状态有效，其他情况则为0
	 */
	public int errorCode;
	
	public boolean isSelected;
	
	/**
	 * javabean对应的key，比如AppInfo则是packageName,Video信息里面则是videoId
	 */
	public abstract void setBeanKey(String key);
	
	public  abstract String getBeanKey();
	
	public  abstract String getUserData();
	
	public  abstract void decodeUserData(String userdata);

	public boolean isFinish() {
		return state == STATE_FINISH;
	}

	public boolean isRunning() {
		return state == STATE_RUNNING;
	}

	public boolean isWaiting() {
		return state == STATE_WAIT;
	}

	public void setDownloadState(int state) {
		this.state = state;
	}

	public int getState() {
		return state;
	}

	public int getDownloadProgress() {
		if(mFileSize == 0) return 0;
		long downPos = getDownPos();
		long progress = downPos * 100 / mFileSize; 
		return (int) progress;
	}

	public long getDownPos(){
		long mHasDownloadBytesCount = 0;
		for(long size : mHasDownloadPosses){
			mHasDownloadBytesCount += size;
		}
		return mHasDownloadBytesCount;
	}

	public long[] getHasDownloadPosses() {
		return mHasDownloadPosses;
	}

	@Override
	public String toString() {
		return "DownloadInfo [downloadId=" + downloadId + ", mTitle=" + mTitle + ", state=" + state + ", getBeanKey()=" + getBeanKey() + "]";
	}
}
