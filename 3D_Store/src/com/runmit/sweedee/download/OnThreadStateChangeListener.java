package com.runmit.sweedee.download;

public interface OnThreadStateChangeListener {
	
	public static final int STATE_SUCCESS = 0;
	
	public static final int STATE_FAIL = 1;
	
	/**
	 * 一个下载任务衍生五个下载进程，然后每个进程下载状态改变时候会调用这个回调
	 * @param threadid:线程id
	 * @param state:下载状态，0成功，1失败
	 * @param errorcode:失败的错误码
	 */
	void onDownloadThreadChange(int threadid,int state,int errorcode);
}
