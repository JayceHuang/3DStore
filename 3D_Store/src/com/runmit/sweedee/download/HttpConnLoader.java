package com.runmit.sweedee.download;

import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;

import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.manager.PlayerEntranceMgr;
import com.runmit.sweedee.manager.data.EventCode;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.util.XL_Log;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;

/**
 * http的urlconnection加载器，并且考虑到多线程访问和url过期失效逻辑
 * 
 * @author Zhi.Zhang
 * 
 */
public class HttpConnLoader {

	private XL_Log log = new XL_Log(HttpConnLoader.class);

	private Object mSyncObj = new Object();

	private int countId = 0;// 循环累计，因为如果一直获取不到下载地址，则可能会走入死循环

	private DownloadInfo mDownloadInfo;

	private DownloadSQLiteHelper mAppInstallSQLiteHelper;

	private boolean hasUpdateUrl = false;

	public HttpConnLoader(Context context, DownloadInfo downloadInfo) {
		this.mDownloadInfo = downloadInfo;
		this.mAppInstallSQLiteHelper = new DownloadSQLiteHelper(context);
	}

	public HttpURLConnection getHttpURLConnection(long startPos, long endPos) {
		log.debug("downloadurl=" + mDownloadInfo.downloadUrl);
		boolean invalideUrl = false;
		try {
			URL url = new URL(mDownloadInfo.downloadUrl);
			HttpURLConnection conn = (HttpURLConnection) url.openConnection();
			conn.setConnectTimeout(5 * 1000);
			conn.setRequestMethod("GET");
			conn.setRequestProperty("Charset", "UTF-8");
			conn.setRequestProperty("Connection", "Keep-Alive");
			conn.setRequestProperty("Range", "bytes=" + startPos + "-" + endPos);
			conn.setAllowUserInteraction(true);

			int responseCode = conn.getResponseCode();
			log.debug("responseCode = " + responseCode + ",startPos=" + startPos + ",endPos=" + endPos);
			if (responseCode < 300) {
				return conn;
			} else if (responseCode >= 400 && responseCode <= 500) {
				invalideUrl = true;
			}
		} catch (IOException e) {
			e.printStackTrace();
			invalideUrl = true;
		}
		if(invalideUrl) {
			if(mDownloadInfo.downloadType == DownloadInfo.DOWNLOAD_TYPE_APP || mDownloadInfo.downloadType == DownloadInfo.DOWNLOAD_TYPE_GAME){
				return getServerData(startPos, endPos);// 换源
			}else {
				(new PlayerEntranceMgr(StoreApplication.INSTANCE)).startRegetUrl((VideoDownloadInfo) mDownloadInfo);
				VideoDownloadInfo downloadInfo = (VideoDownloadInfo) mDownloadInfo;
				ReportActionSender.getInstance().changeVideoCDN("", 3, getFileCrt(downloadInfo.mTitle), downloadInfo.videoId, downloadInfo.mTitle);
			}
		}
		return null;
	}
	
	private int getFileCrt(String fileName) {
		int ret = 0;
		if(fileName.contains("_720P")) 
			ret = 1;
		else if(fileName.contains("_1080P")) 
			ret = 2;
		return ret;
	}

	private synchronized HttpURLConnection getServerData(long startPos, long endPos) {
		log.debug("hasUpdateUrl=" + hasUpdateUrl + ",countId=" + countId);
		if (hasUpdateUrl) {// 如果已经替换了url，则直接返回
			hasUpdateUrl = false;
			return getHttpURLConnection(startPos, endPos);
		}
		if (countId++ < 4) {
			final HandlerThread mHandlerThread = new HandlerThread("FileDownloader_HandlerThread");
			mHandlerThread.start();
			final Handler mHandler = new Handler(mHandlerThread.getLooper()) {
				@Override
				public void handleMessage(Message msg) {
					log.debug("ret=" + msg.arg1 + ",what=" + msg.what);
					switch (msg.what) {
					case EventCode.CmsEventCode.EVENT_GET_APP_GSLB_URL:
					case EventCode.CmsEventCode.EVENT_GET_APP_CDN_URL:
						if (msg.arg1 == 0 && msg.what == EventCode.CmsEventCode.EVENT_GET_APP_CDN_URL) {
							hasUpdateUrl = true;
							String url = (String) msg.obj;
							String oldUrl = mDownloadInfo.downloadUrl;
							mDownloadInfo.downloadUrl = url;
							if(mDownloadInfo.downloadType == DownloadInfo.DOWNLOAD_TYPE_APP || mDownloadInfo.downloadType == DownloadInfo.DOWNLOAD_TYPE_GAME){
								AppDownloadInfo downloadInfo = (AppDownloadInfo) mDownloadInfo;
								ReportActionSender.getInstance().appdownloadChangeCDN(downloadInfo.mAppId, downloadInfo.mAppkey, mDownloadInfo.downloadType, System.currentTimeMillis(), oldUrl, url);	
							}							
							mAppInstallSQLiteHelper.update(mDownloadInfo);
						}
						synchronized (mSyncObj) {
							mSyncObj.notifyAll();
						}
						break;
					default:
						break;
					}
				}
			};
			log.debug("start get app download url and wait for response");
			synchronized (mSyncObj) {// 同步等待gslb返回
				try {
					mSyncObj.wait();
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
			log.debug("run here and release lock and get new url is = " + mDownloadInfo.downloadUrl);
			mHandlerThread.quit();
			return getHttpURLConnection(startPos, endPos);
		}
		return null;
	}
}
