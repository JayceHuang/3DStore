package com.runmit.sweedee.report.sdk;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.telephony.TelephonyManager;
import android.util.DisplayMetrics;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.update.Config;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

public class Reporter {

	private XL_Log log = new XL_Log(Reporter.class);

	private PendingIntent pendingKeepLiveIntent = null;

	private final String KEEP_LIVE_ACTION = "com.runmit.sweedee.report.sdk.keepLiveTimerAlarm";

	private static final String SV = "1.0";// 接口规范的版本，当前为 1.0。
	
	private static final String SDV = "1.0";// SDK版本，当前为 1.0。

	private String UDID;

	private Context mContext;

	private Gson gson;

	private final ExecutorService executorService = Executors.newFixedThreadPool(3);

	private HttpClient mHttpClient = new DefaultHttpClient();

	private static final String REPORT_URL = "http://metis.d3dstore.com/bdreport/sdk/batch/";
	
	private static final String REPORT_CONFIG_URL = "http://metis.d3dstore.com/bdreport/sdk/config";
	
	private ReportConfig mReportConfig;//上报配置文件
	
	private ReportDataBase mReportDataBase;

	private BroadcastReceiver timerRecever = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			log.debug("action="+action);
			if (action.equals(KEEP_LIVE_ACTION)) {
				addData();
			}
		}
	};
	
	/*public*/ int getPlayTimeDuration(){
		return mReportConfig.phb;
	}

	/**
	 * 初始化通用参数
	 */
	private JsonObject initGeneralParameters() {
		JsonObject mGeneralParameters = new JsonObject();
		mGeneralParameters.addProperty("sv", SV);
		mGeneralParameters.addProperty("sdv", SDV);
		mGeneralParameters.addProperty("hwid", Util.getPeerId());
		mGeneralParameters.addProperty("udid", UDID);
		mGeneralParameters.addProperty("wifimac", Util.getLocalWifiMacAddress(mContext));
		mGeneralParameters.addProperty("wirelesssmac", "");// TODO 获取无线mac
		mGeneralParameters.addProperty("wiremac", "");// TODO 获取有线mac
		mGeneralParameters.addProperty("os", 1);// 系统类型：1-Android；2-Ios
		mGeneralParameters.addProperty("osver", Build.VERSION.RELEASE);// 系统版本
		mGeneralParameters.addProperty("device", 1);// 设备类型：1-phone；2-pad；3-phone
													// on box
		mGeneralParameters.addProperty("area", Locale.getDefault().getCountry());//
		mGeneralParameters.addProperty("language", Locale.getDefault().getLanguage());
		String imei = ((TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE)).getDeviceId();
		mGeneralParameters.addProperty("imei", imei != null ? imei : "unknow");
		
		mGeneralParameters.addProperty("idfv", "");// IOS上报该字段，IOS中的供应商标识，Android上报为空字符串
		mGeneralParameters.addProperty("appkey", mContext.getPackageName());// 应用唯一标识，IOS为bundleID；Android为包名
		mGeneralParameters.addProperty("appver", Config.getVerName(mContext));// app
																				// version
																				// 应用版本，例如：1.1.1.2
		mGeneralParameters.addProperty("uid", UserManager.getInstance().getUserId());// 登录用户唯一标识，如果是匿名用户不用上传
		mGeneralParameters.addProperty("devicebrand", Build.BRAND);
		mGeneralParameters.addProperty("devicedevice", Build.DEVICE);// Android设备DeviceInfo中的DEVICE
		mGeneralParameters.addProperty("devicemodel", Build.MODEL);
		mGeneralParameters.addProperty("devicehardware", Build.HARDWARE);
		mGeneralParameters.addProperty("deviceid", Build.ID);
		mGeneralParameters.addProperty("deviceserial", Build.SERIAL);
		DisplayMetrics mDisplayMetrics = mContext.getResources().getDisplayMetrics();
		mGeneralParameters.addProperty("ro", mDisplayMetrics.widthPixels + "_" + mDisplayMetrics.heightPixels);// 设备分辨率，例如：1024_768
		mGeneralParameters.addProperty("channel", Constant.PRODUCT_ID);// 分发渠道标识，由客户端维护和提供完整的信息
		mGeneralParameters.addProperty("dts", System.currentTimeMillis());// 设备本地时间（毫秒）
		
		return mGeneralParameters;
	}

	/*public*/ void init(Context context) {
		this.mContext = context;
		this.mReportDataBase = new ReportDataBase(context);
		
		this.UDID = new DeviceUuidFactory(mContext).getDeviceUuid();
		gson = new Gson();
		
		getReportConfig();
		log.debug("mReportConfig="+mReportConfig);
		IntentFilter alarmFilter = new IntentFilter();
		Intent intent = new Intent(KEEP_LIVE_ACTION);
		pendingKeepLiveIntent = PendingIntent.getBroadcast(mContext, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		alarmFilter.addAction(KEEP_LIVE_ACTION);
		alarmFilter.setPriority(Integer.MAX_VALUE);
		mContext.registerReceiver(timerRecever, alarmFilter);
		setKeepAlive(mContext, true);
	}

	/**
	 * 设置心跳 start ： 表示是否启动心跳 period : 表示心跳间隔
	 */
	/*public*/ void setKeepAlive(Context mContext, boolean start) {
		AlarmManager am = (AlarmManager) mContext.getSystemService(Context.ALARM_SERVICE);
		if (start) {
			am.setRepeating(AlarmManager.RTC, System.currentTimeMillis() + mReportConfig.hb*1000, mReportConfig.hb*1000, pendingKeepLiveIntent);
		} else {
			am.cancel(pendingKeepLiveIntent);
		}
	}

	/*public*/ void addReport(final ReportItem mItem) {
		log.debug("mItem action="+mItem.action + ",data=" +mItem.toJsonString());
		executorService.submit(new Runnable() {
			
			@Override
			public void run() {
				mReportDataBase.addReporter(mItem);
			}
		});
	}

	/**
	 * 反初始化
	 */
	/*public*/ boolean Uninit() {
		mContext.unregisterReceiver(timerRecever);
		pendingKeepLiveIntent.cancel();
		this.setKeepAlive(mContext, false);
		return true;
	}

	/**
	 * 
	 * @param data
	 *            :上报到服务器的数据
	 * @param mList
	 *            :数据对应的列表 此处有个问题，如果此次上报失败，是缓存数据库还是添加到内存
	 * @return
	 */
	/*private*/ boolean addData() {
		log.debug("addData");
		executorService.submit(new Runnable() {

			@Override
			public void run() {// 上报机制，首先请求服务器，如果没有返回，保存本地数据库
				List<ReportItem> mReportList = mReportDataBase.findTopReportItem(mReportConfig.limit);
				log.debug("mReportList="+mReportList.size());
				
				if (mReportList != null && mReportList.size() > 0) {
					HashMap<String, List<JsonElement>> mhashMap = new HashMap<String, List<JsonElement>>();
					JsonObject mGeneralParameters = initGeneralParameters();
					JsonObject mReportJsonObject = gson.toJsonTree(mGeneralParameters).getAsJsonObject();

					// 数据分拣，根据ReportItem.action
					for (int i = 0, size = mReportList.size(); i < size; i++) {
						ReportItem mItem = mReportList.get(i);
						if (mhashMap.containsKey(mItem.action)) {
							List<JsonElement> mActionList = mhashMap.get(mItem.action);
							mActionList.add(mItem.mReportItemMap);
						} else {
							List<JsonElement> mActionList = new ArrayList<JsonElement>();
							mActionList.add(mItem.mReportItemMap);
							mhashMap.put(mItem.action, mActionList);
						}
					}

					// 循环遍历hashmap添加到jsonobject
					Iterator<Entry<String, List<JsonElement>>> iter = mhashMap.entrySet().iterator();
					while (iter.hasNext()) {
						Map.Entry<String, List<JsonElement>> entry = (Map.Entry<String, List<JsonElement>>) iter.next();
						String key = entry.getKey();
						List<JsonElement> mList = entry.getValue();
						mReportJsonObject.add(key, gson.toJsonTree(mList));
					}
					log.debug("beginReport data=" + mReportJsonObject.toString());

					boolean isSend = postHttp(REPORT_URL,mReportJsonObject.toString());
					log.debug("isSend=" + isSend);
					if (isSend) {// 发送成功，则删除数据库
						mReportDataBase.deleteReporterList(mReportList);
					}
				}
			}
		});
		return true;
	}
	
	private void getReportConfig(){
		final SharePrefence mSharePrefence = SharePrefence.getInstance(mContext);
		mReportConfig = new ReportConfig();//先赋值上一次的
		mReportConfig.hb=mSharePrefence.getInt("hb", 180);
		mReportConfig.phb=mSharePrefence.getInt("phb", 180);
		mReportConfig.limit=mSharePrefence.getInt("limit", 200);
		
		executorService.submit(new Runnable() {
			
			@Override
			public void run() {
				JsonObject mGeneralParameters = initGeneralParameters();
				ByteArrayEntity byteEntity = new ByteArrayEntity(mGeneralParameters.toString().getBytes());
				HttpPost mHttpPost = new HttpPost(REPORT_CONFIG_URL);
				mHttpPost.setEntity(byteEntity);
				mHttpPost.setHeader("Content-Type", "application/json");
				mHttpPost.setHeader("Accept-Encoding", "gzip,deflate");  
				mHttpPost.setHeader("Content-Encoding", "gzip,deflate");  
				try {
					HttpResponse response;
					response = mHttpClient.execute(mHttpPost);
					int ret_code = response.getStatusLine().getStatusCode();
					log.debug("ret_code="+ret_code);
					if (ret_code == 200) {
						String strResult = EntityUtils.toString(response.getEntity());
						log.debug("strResult="+strResult);
						mReportConfig = gson.fromJson(strResult, ReportConfig.class);
						mSharePrefence.putInt("hb", mReportConfig.hb);
						mSharePrefence.putInt("phb", mReportConfig.phb);
						mSharePrefence.putInt("limit", mReportConfig.limit);
					}
				} catch (Exception e) {
					e.printStackTrace();
				}
				
			}
		});
	}

	private boolean postHttp(String url,final String data) {
		boolean return_value = false;
		ByteArrayEntity byteEntity = new ByteArrayEntity(data.getBytes());
		HttpPost mHttpPost = new HttpPost(REPORT_URL);
		mHttpPost.setEntity(byteEntity);
		mHttpPost.setHeader("Content-Type", "application/json");
		mHttpPost.setHeader("Accept-Encoding", "gzip,deflate");  
		mHttpPost.setHeader("Content-Encoding", "gzip,deflate");  
		try {
			HttpResponse response;
			response = mHttpClient.execute(mHttpPost);
			int ret_code = response.getStatusLine().getStatusCode();
			log.debug("ret_code=" + ret_code + ",data=" + data);
			if (ret_code == 200) {
				return_value = true;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return return_value;
	}
}
