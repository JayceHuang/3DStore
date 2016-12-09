package com.runmit.sweedee.download;

/**
 * 状态监听改变回调
 * 将状态监听回调和下载进度回调分开处理是因为下载状态需要及时更新界面
 * 而下载进度是定时更新界面
 * @author Administrator
 *
 */
public interface OnStateChangeListener {
	/**
	 * 
	 * @param mDownloadInfo
	 * @param new_state :最新状态，@see DownloadInfo.state
	 */
	public void onDownloadStateChange(DownloadInfo mDownloadInfo,int new_state);
}
