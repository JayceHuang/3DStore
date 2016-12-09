package com.runmit.mediaserver;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.Timer;
import java.util.TimerTask;

import javax.net.ssl.SSLHandshakeException;

import org.apache.http.HttpEntityEnclosingRequest;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.NoHttpResponseException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.HttpRequestRetryHandler;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.protocol.ExecutionContext;
import org.apache.http.protocol.HttpContext;
import org.apache.http.util.EntityUtils;
import org.json.JSONObject;

import com.runmit.sweedee.util.EnvironmentTool;

import android.content.Context;
import android.net.Uri;
import android.util.Log;

/**
 * 点播库接口
 * @auther  
 * @Version 1.0 2014/4/16
 *  
 **/
public class MediaServer {

	private UtilityHandlerThread mHandlerThread = null;

	
	private final int SHOULEI_APPID = 0;
	private final int MEDIA_SERVER_PORT = 17626;
	private final int DEBAULT_SCREENW = 800;
	private final int DEBAULT_SECTION_TYPE = 0;
	private final int DEBAULT_PLATFORM = 0; // 0(Pc, flv);
											// 1(Ios,ts);2(android,mp4)

	private final String MEDIA_SERVER_URL_PREFIX = "http://127.0.0.1:";
	private final String PLAY_URL_CMD = "playVodUrl";
	private final String PLAY_LOCAL_URL="playLocalUrl";
	
	private final String GET_PLAY_INFO_CMD = "getVodPlayInfo";
	private final String START_DOWNLOAD_CMD = "startDownload";
	private final String STOP_DOWNLOAD_CMD = "stopDownload";
	private final String GET_VOD_ERROR_INFO = "getVodErrorInfo";
	private final String STOP_ALL_VOD_TASK_CMD = "stopAllVodSession";
	private final String POST_STAT_INFO = "postSdkStaticInfo";

	private String mSystemPath = null;
	private static MediaServer mMediaServer;

	private Object mSyncObj = new Object();
	private Timer mGetSpeedTimer;
	private GetSpeedTimerTask mGetSpeedTimerTask;

	private final static String TAG = "MediaServer";
	/**
	 * 得到 MediaServer的单例
	 * @param context   全局应用程序上下文接口
	 * @param appid     产品的ID
	 * @param peerId    P2p客户端的ID
	 * @return       MediaServer 的指针
	 **/
	public static MediaServer getInstance(Context context, int appid, String peerId) {
		if (mMediaServer == null) {
			mMediaServer = new MediaServer(context, appid, peerId);
		}

		return mMediaServer;
	}

	private MediaServer(Context context,  int appid, String peerId) {
		InitMediaServer(context, appid, peerId);
	}
    
	/**
	 * 初始化MediaSwerver库
	 * 使用下载库下载前必须初始化
	 * @see #getInstance(Context context, int appid, String peerId)
	 * @param context 
	 * @param appid 
	 * @param peerId 
	 * @return 0: Success -1: failed
	 */
	public boolean InitMediaServer(Context context, int appid, String peerId) {
		//TODO先忽略peerId，由底层自己生产。 后续删除此peerId参数。
		
		int ret = -1;
		mHandlerThread = new UtilityHandlerThread();
		mHandlerThread.init();
		
		String cachePath = EnvironmentTool.getDataPath("mediatemp");
		Utility.ensureDir(cachePath);
		ret = init(appid, MEDIA_SERVER_PORT, cachePath);
		// DEBAULT_SCREENW, DEBAULT_SECTION_TYPE, mSystemPath, CACHE_PATH);
		Log.i(TAG, "mediaserver init info and appid ="+appid);
		setLocalInfo(context);
		mHandlerThread.ExecuteRunnable(mMediaServerRunnable);
		return (ret == 0);
	}
	
	
	public boolean isInitFinished() {
		return getHttpListenPort() != 0;
	}
	
	public void reInit(Context context, int appid, String peerId) {
		InitMediaServer(context, appid, peerId);
	}

	/**
	 * 传递本地信息(like peerid, etc) 到 MediaSwerver 库
	 * 运行下载库后，启动点播任务前，调用此函数传递本地信息
	 * @see #getInstance(Context context, int appid, String peerId)
	 * @param context  
	 * @param peerId 
	 * @param imei  国际移动设备标识符
	 * @param netType  网络类型null/2g/3g/4g/wifi/other
	 * @return 0:set Success -1: set failed
	 */
	private int setLocalInfo(Context context) {

		String imei = Utility.getIMEI(context);
		String netType = Utility.getNetworkType(context);
		if (imei == null) {
			imei = "imei_not_know";
		}
		if (netType == null) {
			netType = "net_not_know";
		}
		
		String deviceType = android.os.Build.MODEL.replace(" ", "-");
		String osVersion = android.os.Build.VERSION.RELEASE;
		Log.i(TAG, "mediaserver init setLocalInfo imei=" + imei
				+ ", deviceType=" + deviceType
				+ ", osVersion=" + osVersion
				+ ", nettype=" + netType);
		return setLocalInfo(imei, deviceType, osVersion, netType);
	}
	
	private int TIME_OUT = 1000; // 1s
	/**
	 * 当初始化 MediaSwerver库必须调用此函数，退出时调用
	 * 反初始化 MediaSwerver库
	 * @param 无
	 * @return 无
	 */
	public void UninitMediaServer() {
		Log.i(TAG, "UninitMediaServer");
		uninit();
		synchronized (mMediaServerRunnable) {
			try {
				mMediaServerRunnable.wait(TIME_OUT);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
		
	}

	private String getEncodeUrl(String url) {
		String finalUrl = null;

		if (url == null || url.length() < 5) {
			Log.e(TAG, "url格式错误，url长度小于5");
			return null;
		}
		finalUrl = Uri.encode(url);
		return finalUrl;
	}

	/**
	 * 运行下载库，需要下载数据时，调用
	 * 通知MediaServer层开启播放
	 * @param url 播放URL
	 * @return 无
	 */
	public void startPlayTask(String url) {
		new MediaServerCommunicateThread(url, PLAY_URL_CMD).start();
	}
	
	/**
	 * 获取播放器所要设置的URL，
	 *
	 * @param url    点播 URL
	 * @param cookie  客户端传给服务器的特殊标示
	 * @return 带filesize的Url
	 */
	public String getVodPlayURL(boolean isLocal,String url, String cookie,long fileSize) {
		String finalUrl = getEncodeUrl(url);
		if (finalUrl == null)
			return null;

		StringBuffer sb = new StringBuffer();
		sb.append(GetMediaServerURLPrefix()).append("/").append(isLocal?PLAY_LOCAL_URL:PLAY_URL_CMD).append("?").append("src_url=").append(finalUrl).append("&platform=").append(DEBAULT_PLATFORM).append("&file_size=").append(fileSize);
		if (cookie != null) {
			String finalCookie = getEncodeUrl(cookie);
			sb.append("&cookie=").append(finalCookie);
		}
		Log.i(TAG, "getVodPlayURL url=" + url + ", finalUrl=" + sb.toString()+ " and cookie=" + cookie);

		return sb.toString();
	}

	/**
	 * 获取点播任务的信息
	 * 
	 * @param mHandler OnVodTaskInfoListener回调
	 * @param url 点播URL
	 * @return void
	 */
	public void startGetVodTaskInfo(
			final OnVodTaskInfoListener onVodTaskInfoListener, final String url) {
		if (mGetSpeedTimer != null) {
			mGetSpeedTimer.cancel();
			if (mGetSpeedTimerTask != null) {
				mGetSpeedTimerTask.release();
				mGetSpeedTimerTask = null;
			}
		}
		mGetSpeedTimer = new Timer();
		mGetSpeedTimerTask = new GetSpeedTimerTask(onVodTaskInfoListener, url);
		mGetSpeedTimer.schedule(mGetSpeedTimerTask, 0, 1000);
	}

	private class GetSpeedTimerTask extends TimerTask {
		private OnVodTaskInfoListener mOnVodTaskInfoListener;
		private String mPlayUrl;
		private VodTaskInfoService mVodTaskInfoService;

		public GetSpeedTimerTask(
				final OnVodTaskInfoListener onVodTaskInfoListener,
				final String url) {
			this.mPlayUrl = url;
			this.mOnVodTaskInfoListener = onVodTaskInfoListener;
			mVodTaskInfoService = new VodTaskInfoService(onVodTaskInfoListener,
					"127.0.0.1", getHttpListenPort());
		}

		@Override
		public void run() {
			synchronized (mSyncObj) {
				if (mVodTaskInfoService == null) {
					mVodTaskInfoService = new VodTaskInfoService(mOnVodTaskInfoListener, "127.0.0.1",getHttpListenPort());
				}
				mVodTaskInfoService.sendCommand(generateGetVodPlayInfoURL(mPlayUrl));

			}
		}

		public void release() {
			if (mVodTaskInfoService != null) {
				mVodTaskInfoService.uninit();
				mVodTaskInfoService = null;
			}
		}
	}

	/**
	 * 停止获取点播处要的信息
	 * @param   无
	 * @return  void
	 */
	public void stopGetVodTaskInfo() {
		if (mGetSpeedTimer != null) {
			mGetSpeedTimer.cancel();
			mGetSpeedTimer = null;
		}

		if (mGetSpeedTimerTask != null) {
			mGetSpeedTimerTask.release();
			mGetSpeedTimerTask = null;
		}
	}

	private String generateGetVodPlayInfoURL(String url) {
		String finalUrl = getEncodeUrl(url);
		if (finalUrl == null)
			return null;
		finalUrl = "/" + GET_PLAY_INFO_CMD + "?" + "src_url=" + finalUrl;
//		Log.i(TAG, "generateGetVodPlayInfoURL url=" + url + ", finalUrl="
//				+ finalUrl);
		return finalUrl;
	}

	private String GetMediaServerURLPrefix() {
		Log.i(TAG, "GetMediaServerURLPrefix=" + MEDIA_SERVER_URL_PREFIX
				+ getHttpListenPort());
		return MEDIA_SERVER_URL_PREFIX + getHttpListenPort();
	}

	Runnable mMediaServerRunnable = new Runnable() {

		@Override
		public void run() {
			MediaServer.this.run();
			synchronized (mMediaServerRunnable) {
				mMediaServerRunnable.notify();
			}
		}
	};

	private String generateStopVodTaskDownloadURL(String url) {
		String finalUrl = getEncodeUrl(url);
		if (finalUrl == null)
			return null;
		finalUrl = GetMediaServerURLPrefix() + "/" + STOP_DOWNLOAD_CMD + "?" + "src_url=" + finalUrl;
		Log.i(TAG, "generateStopVodTaskDownloadURL url=" + url + ", finalUrl="
				+ finalUrl);
		return finalUrl;
	}

	private String generateStartVodTaskDownloadURL(String url) {
		String finalUrl = getEncodeUrl(url);
		if (finalUrl == null)
			return null;
		finalUrl = GetMediaServerURLPrefix() + "/" + START_DOWNLOAD_CMD + "?" + "src_url=" + finalUrl;
		Log.i(TAG, "generateStartVodTaskDownloadURL url=" + url + ", finalUrl="
				+ finalUrl);
		return finalUrl;
	}
	
	private String generateStopAllVodTaskURL() {
		String finalUrl = GetMediaServerURLPrefix() + "/" + STOP_ALL_VOD_TASK_CMD + "?src_url=";
		Log.i(TAG, "generateStopAllVodTaskURL finalUrl="
				+ finalUrl);
		return finalUrl;
	}

	private String generateStartVodTaskErrorInfoURL(String url) {
		String finalUrl = getEncodeUrl(url);
		if (finalUrl == null)
			return null;
		finalUrl = GetMediaServerURLPrefix() + "/" + GET_VOD_ERROR_INFO + "?" + "src_url=" + finalUrl;
		Log.i(TAG, "generateStartVodTaskErroInfoURL url=" + url + ", finalUrl="
				+ finalUrl);
		
		return finalUrl;
	}
	/**当用户播放时，估计短暂的处于非WIFI环境时，可以停止MediaServer下载, 防止消耗用户移动数据流量
	 * 预计过一会儿会重新回到WIFI，再使用startVodTaskDownload
	 * 去启动MediaServer下载 
	 * @param url     
	 * @return   0: Success 
	 *          -1:  failed
	 */
	public boolean stopVodTaskDownload(final String url) {
		String communicateUrl = generateStopVodTaskDownloadURL(url);
		new MediaServerCommunicateThread(communicateUrl, STOP_DOWNLOAD_CMD).start();
		return true;
	}
	/**
	 * 当用户回到WIFI环境中，且之前被停止下载了
	 * 调用startVodTaskDownload()可让MediaServer库重新下载 
	 * @see  #stopVodTaskDownload(final String url)
	 * @param url     
	 * @return   0: Success 
	 *           -1:  failed
	 */
	public boolean startVodTaskDownload(final String url) {
		String communicateUrl = generateStartVodTaskDownloadURL(url);
		new MediaServerCommunicateThread(communicateUrl, START_DOWNLOAD_CMD).start();
		return true;
	}
	/**
	 * 停止MediaServer库中的所有下载任务
	 * @param   无
	 * @return   0: Success 
	 *          -1:  failed
	 */
	public boolean stopAllVodTask() {
		//stopAllVodSession
		String communicateUrl = generateStopAllVodTaskURL();
		//new MediaServerCommunicateThread(communicateUrl, STOP_ALL_VOD_TASK_CMD).start();
		
		synSendStop(communicateUrl);
		return true;
		
		
	}

	
	private void synSendStop(String finalUrl) {
		// HttpGet连接对象
					HttpGet httpRequest = new HttpGet(finalUrl);
					
					// 取得HttpClient对象
					HttpClient httpclient = new DefaultHttpClient();
					// 请求HttpClient，取得HttpResponse					
					HttpResponse httpResponse = null;
					try {
						httpResponse = httpclient.execute(httpRequest);
						  if (httpResponse.getStatusLine().getStatusCode() == 200)  {  
							 
						  }
					} catch (Exception e) {
						e.printStackTrace();
					} 
	}
	

	/**
	 * 获取点播任务的错误信息
	 * @param   url  点播url
	 * @param   OnVodTaskErrorListener 错误信息回调
	 * @return   无 
	 *         
	 */
	public void getVodTaskErrorInfo(final String url, OnVodTaskErrorListener listener) {
		String communicateUrl = generateStartVodTaskErrorInfoURL(url);
		new MediaServerCommunicateThread(communicateUrl, listener).start();
	}
	
//	/**
//	 * 点播完成后调用
//	 *  UI传递SDK性能数据给MediaServer库,
//	 *     由MediaServer库去上报统计数据
//	 *	@param src_url	 视频url （需encode）
//	 *	@param gcid	文件gcid
//	 *	@param launch_time  启动时间
//	 *	@param begin_play_time  开始播放时间
//	 *	@param interrupt_times  中断次数
//	 *	@param total_interrupt_duration  中断总时长
//	 *	@param total_play_duration  播放总时长 mPlayingTime
//	 *	@param drag_times   拖动次数
//	 *	@param cache_duration_after_drag   拖动后缓冲总时长
//	 *  @return  void
//     *
//	 */
//	public void postStatInfo(String src_url, String gcid, long launch_time, long begin_play_time, int interrupt_times, 
//			int total_interrupt_duration, int total_play_duration,  int drag_times, int cache_duration_after_drag) {
//		String finalUrl = getEncodeUrl(src_url);
//		StringBuffer sb = new StringBuffer(GetMediaServerURLPrefix() + "/" + POST_STAT_INFO + "?");
//		sb.append("src_url=").append(finalUrl).append("&gcid=").append(gcid).append("&launch_time=").append(launch_time)
//		.append("&begin_play_time=").append(begin_play_time).append("&interrupt_times=").append(interrupt_times)
//		.append("&total_interrupt_duration=").append(total_interrupt_duration).append("&total_play_duration=").append(total_play_duration)
//		.append("&drag_times=").append(drag_times).append("&cache_duration_after_drag=").append(cache_duration_after_drag);
//	
//		new MediaServerCommunicateThread(sb.toString(), POST_STAT_INFO).start();
//	}
	
	private class MediaServerCommunicateThread extends Thread{
		String finalUrl = null;
		OnVodTaskErrorListener onVodTaskErrorListener;
		private String type;
		
		public MediaServerCommunicateThread(String url, String type){
			finalUrl = url;
			this.type = type;
		}
		
		public MediaServerCommunicateThread(String url, OnVodTaskErrorListener listener) {
			finalUrl = url;
			onVodTaskErrorListener = listener;
		}
		
		
		public void run() {
			if (finalUrl == null) {
				return ;
			}
			
			HttpRequestRetryHandler myRetryHandler = new HttpRequestRetryHandler() {
				
				@Override
				public boolean retryRequest(IOException exception, int executionCount,
						HttpContext context) {
					
					if (executionCount >= 0) {

			            // Do not retry if over max retry count

			            return false;

			        }

			        if (exception instanceof NoHttpResponseException) {

			            // Retry if the server dropped connection on us

			            return false;

			        }

			        if (exception instanceof SSLHandshakeException) {

			            // Do not retry on SSL handshake exception

			            return false;

			        }

			        HttpRequest request = (HttpRequest) context.getAttribute(

			                ExecutionContext.HTTP_REQUEST);

			        boolean idempotent = !(request instanceof HttpEntityEnclosingRequest);

			        if (idempotent) {

			            // Retry if the request is considered idempotent

			            return false;

			        }

			        return false;
				}
			};
			
			// HttpGet连接对象
			HttpGet httpRequest = new HttpGet(finalUrl);
			
			// 取得HttpClient对象
			HttpClient httpclient = new DefaultHttpClient();
			// 请求HttpClient，取得HttpResponse
			((DefaultHttpClient) httpclient).setHttpRequestRetryHandler(myRetryHandler);
			
			HttpResponse httpResponse = null;
			try {
				httpResponse = httpclient.execute(httpRequest);
				  if (httpResponse.getStatusLine().getStatusCode() == 200)  {  
					  	//获取错误码信息
					  	if (type != null && PLAY_URL_CMD.equals(type)) {
					  		return;
					  	}
					      String resultJson = EntityUtils.toString(httpResponse.getEntity());
					      if (resultJson == null || !resultJson.contains("ret"))
								return ;
							
							JSONObject jobj = null;
							jobj = new JSONObject(resultJson);
							if (jobj.getInt("ret") == 0) { 
								JSONObject jobjResp = jobj.getJSONObject("resp");
								int errorCode = jobjResp.getInt("vod_resp_code");
								if (this.onVodTaskErrorListener != null) {
									onVodTaskErrorListener.onGetVodTaskErrorCode(errorCode);
								}
							}
				  }
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} 
		}
	};

	
	/**
	 * 获取错误码回调的接口
	 * @author admin
	 * @param 
	 * @return 回调的接口
	 */
	public interface OnVodTaskErrorListener {
		public void onGetVodTaskErrorCode(int errorCode);
	}
	/**
	 * 获取信息回调的接口
	 * @author admin
	 * @param
	 * @return 回调的接口
	 */
	public interface OnVodTaskInfoListener {
		public void onGetVodTask(VodTaskInfo vodTaskInfo);
	}

	static {
		System.loadLibrary("mediaserver");
		System.loadLibrary("mediaserver_jni");
	}


	private static native int init(int appid, int port, String rootPath);
	
	private static native int setLocalInfo(String imei, String deviceType, String osVersion, String netType);
		
	private static native void uninit();

	private static native void run();

	private static native int getHttpListenPort();

	private native int getVodTaskInfo();
	
	

}
