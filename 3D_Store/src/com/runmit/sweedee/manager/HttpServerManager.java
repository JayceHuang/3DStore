package com.runmit.sweedee.manager;

import java.io.Serializable;
import java.security.MessageDigest;
import java.util.Locale;

import org.apache.http.Header;
import org.apache.http.HttpStatus;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.loopj.android.http.RequestHandle;
import com.loopj.android.http.TextHttpResponseHandler;
import com.runmit.sweedee.BuildConfig;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.manager.ObjectCacheManager.DiskCacheOption;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.sdkex.httpclient.SecureSocketFactory;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.EnvironmentTool;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

/**
 * @author Sven.Zhan
 * 与http服务器业务的抽象Manager类
 */
public abstract class HttpServerManager extends AbstractManager {
	
	public static final String CMS_V1_BASEURL = "http://poseidon.d3dstore.com/api/v1/";
	
	public static final String CMS_DEV_BASEURL = "http://cms.runmit-dev.com/api/v1/";
	
	/**对应CMS返回标准json串的rtn字段*/
	protected static final String JSON_RTN_FIELD = "rtn";
	
	/**对应CMS返回标准json串的list字段*/
	protected static final String JSON_LIST_FIELD = "list";
	
	/**对应CMS返回标准json串的groups字段*/
	protected static final String JSON_GROUPS_FIELD = "groups";
	
	/**对应CMS返回标准json串的data字段*/
	protected static final String JSON_DATA_FIELD = "data";
	
	public static final String sha1key = "sweedee3drocks001";
	
	protected Gson gson = null;
	
	private TDRequestClient mRequestClient;
	
	private final String versionName;
	
	private XL_Log log = new XL_Log(HttpServerManager.class);
	
	private boolean isTestMode = false;
	
	private String testInput = "";
	
	HttpServerManager(){
		mRequestClient = new TDRequestClient();
		mRequestClient.setSSLSocketFactory(SecureSocketFactory.getInstance());		
		gson = new GsonBuilder().create();
		versionName = getVersionName(StoreApplication.INSTANCE);
		testInput = SharePrefence.getInstance(StoreApplication.INSTANCE).getString(SharePrefence.TEST_MODE, "");
		isTestMode = !(testInput == null || testInput.isEmpty());
	}
	
	/**
	 * 发送Get请求，先查缓存，再根据ResponseHandler的设置决定是否进行Http请求
	 * @param url
	 * @param rHandler
	 * @param realRequst 是否使用假数据，测试阶段使用
	 */
	protected void sendGetRequst(String url,ResponseHandler rHandler,boolean realRequst){
		mRequestClient.requestGet(url, rHandler,realRequst);
	}
	
	/**
	 * 发送Get请求，先查缓存，再根据ResponseHandler的设置决定是否进行Http请求
	 * @param url
	 * @param rHandler
	 */
	protected void sendGetRequst(String url,ResponseHandler rHandler){
		mRequestClient.requestGet(url, rHandler,true);
	}
	
	/**
	 * 发送Post请求，先查缓存，再根据ResponseHandler的设置决定是否进行Http请求
	 * @param url
	 * @param rHandler
	 */
	protected void sendPostRequest(String url,String jsonContent,ResponseHandler rHandler){
		mRequestClient.requstPost(url, jsonContent, rHandler);
	}
	

	/**
	 * 发送Http Get请求，无缓存逻辑，请求可取消
	 * @param url
	 * @param rHandler
	 */
	protected RequestHandle sendCancableGetRequest(String url,ResponseHandler rHandler){
		return mRequestClient.requstCancableGet(url, rHandler);
	}
	
	/**
	 * 发送Http Post请求，无缓存逻辑，请求可取消
	 * @param url
	 * @param rHandler
	 */
	protected void sendCancablePostRequest(String url,String jsonContent,ResponseHandler rHandler){
		mRequestClient.requstCancablePost(url, jsonContent, rHandler);
	}
	
	
	/**客户端的基本信息
	 * country:国家
	 *language:语言
	 *phone_models:手机型号
	 *device_id:设备id，
	 *sys_ver:手机系统版本
	 *app_name:客户端应用名称
	 *app_ver:客户端版本
	 *uid：用户id
	 *简写为：ct=CN&lg=zh&pm=xiaomi&di=1234567890ABCDEF&sv=android_4.4&an=3d_store_for_android&av=1.0&uid=123456
	 *额外字段 sessionid
	 **/
	@SuppressLint("DefaultLocale") 
	public String getClientInfo(boolean needUserInfo){
		final char split = '&';		
		
		String lg = Locale.getDefault().getLanguage();		 
		String country = Locale.getDefault().getCountry();
		if(country.toLowerCase().equals("tw") && lg.toLowerCase().equals("zh")){//台湾的语言加上地区号
			lg = "zh_tw";
		}
		
		
		final String ci = String.valueOf(Constant.PRODUCT_ID);//clientid，客户端id
		final String ts = String.valueOf(System.currentTimeMillis());
		
		//基本参数
		StringBuilder sb = new StringBuilder();
		sb.append("lg=").append(lg).append(split);
		sb.append("superProjectId=").append(String.valueOf(Constant.superProjectId)).append(split);//传递superProjectId
		sb.append("pm=").append(Build.MODEL.replace(" ", "_")).append(split);
		sb.append("di=").append(Util.getPeerId()).append(split);
		sb.append("sv=").append(Build.VERSION.RELEASE).append(split);
		sb.append("an=").append(Uri.encode("Android_"+EnvironmentTool.getAppName())).append(split);
		sb.append("ci=").append(ci).append(split);
		sb.append("av=").append(versionName).append(split);
		if(isTestMode){
			sb.append("mc=").append(Uri.encode(testInput)).append(split);
		}		
		//用户信息
		long uid = UserManager.getInstance().getUserId();
		if(needUserInfo){
			sb.append("uid=").append(uid).append(split);
			sb.append("token=").append(UserManager.getInstance().getUserSessionId()).append(split);
		}
		
		//带上gt和ts参数,必须放在最后，因为后面涉及到截取
		String gt = computeSHAHash(lg+ci+ts+sha1key);
		sb.append("gt=").append(gt).append(split);
		sb.append("ts=").append(ts);
		
		//转成String
		String baseArg = sb.toString();
		return baseArg;
	}
		
	//获取版本号(内部识别号)
	private String getVersionName(Context context)
	{
		try {
			PackageInfo pi=context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
			String versionName = pi.versionName;
			if(versionName.length() >= 5){
				versionName = versionName.substring(0, 5);
			}
			return versionName;
		} catch (NameNotFoundException e) {
			e.printStackTrace();
			return "empty";
		}
	}
	
	protected abstract void dispatchSuccessResponse(ResponseHandler responseHandler,String responseString);
	
	/***
	 * SHA1加密
	 * @param password
	 * @return
	 */
	public static String computeSHAHash(String password) {
		MessageDigest mdSha1 = null;
		try {
			mdSha1 = MessageDigest.getInstance("SHA-1");
			mdSha1.update(password.getBytes());
		} catch (Exception e1) {
			//Log.e(TAG,"computeSHAHash exception = "+e1.getCause());			
		}
		byte[] data = mdSha1.digest();
		StringBuffer SHA1Hash = new StringBuffer();
		for (int i = 0; i < data.length; i++) {
			String h = Integer.toHexString(0xFF & data[i]);
			while (h.length() < 2) h = "0" + h;
			SHA1Hash.append(h);
		}
		return SHA1Hash.toString();	
	}
	
	/**
	 * url作为缓存key时不能有动态参数gt和ts
	 * @param urlWithClientInfo
	 * @return
	 */
	protected String asKey(String urlWithClientInfo){
		String result = urlWithClientInfo;
		if(!TextUtils.isEmpty(result)){
			int index = result.indexOf("&gt=");
			if(index != -1){
				result = result.substring(0, index);
			}
		}
		//Log.v(TAG, "asKey result = "+result);
		return result;
	}
	
	/**请求响应类，响应的类型包括1：HTTP响应, 2：缓存命中响应*/
	class ResponseHandler extends TextHttpResponseHandler{
		
		/**关联的事件类型*/
		private int mEventCode;
		
		/**UI层的回调Handler*/
		private Handler mCallBackHanlder;
		
		/**可序列化的tag对象,用于异步接口对象的传递，相当于<缓存库的userdata>*/
		private Serializable mTag;
		
		/**缓存参数*/
		private CacheParam mCacheParam;
		
		/**是否已给界面回调缓存的内容，如果已回调过了，http请求的解析则无需再次回调，直接更新缓存*/
		private boolean hasReturnCacheValue = false;
		
		public ResponseHandler(int event,Handler callBack){
			super(mAsyncHandler.getLooper());
			mEventCode = event;
			mCallBackHanlder = callBack;
			mCacheParam = new CacheParam(null, null, null,CachePost.Continue);
		}

		public int getEventCode(){
			return mEventCode;
		}
		
		public void setTag(Serializable obj){
			mTag = obj;
		}
		
		public void setCacheParam(CacheParam param){
			mCacheParam = param;
		}
		
		public CacheParam getCacheParam() {
			return mCacheParam;
		}

		public Handler getAsyncHandler(){
			return mAsyncHandler;
		}
		
		/**缓存命中*/
		public void onCacheObtained(Object cacheObj){	
			hasReturnCacheValue = true;
			sendUiMessage(mEventCode, true, 0, cacheObj);
		}
		
		@Override
		public void onFailure(int statusCode, Header[] headers, String responseString, Throwable throwable) {
			throwable.printStackTrace();
			int errorCode = ExceptionCode.getErrorCode(throwable);
			handleResponse(errorCode , null);
		}

		@Override
		public void onSuccess(int statusCode, Header[] headers, String responseString) {
			handleResponse(statusCode,responseString);
		}
		
		private void handleResponse(int stateCode,String responseString){
			if(BuildConfig.DEBUG){//打包后就不打印日志，关键信息
				log.debug("stateCode="+stateCode+",responseString="+responseString);
			}
			//统计上报，上报错误码
			ReportActionSender.getInstance().error(mEventCode, stateCode);
			
			if(stateCode != HttpStatus.SC_OK){
				sendCallBackMsg(mEventCode, stateCode,null);
				return;
			}
			/**由各业务子类处理 */
			dispatchSuccessResponse(this, responseString);
		}
				
		void sendCallBackMsg(int what,int ret,Object obj){	
			
			//更新缓存
			if(mCacheParam.cacheKey != null && ret == 0 && obj != null){				
				ObjectCacheManager.getInstance().pushIntoCache(mCacheParam.cacheKey,obj , mCacheParam.diskCacheOption);
				mRequestClient.cacheUpdated(mCacheParam.cacheKey);//标志缓存已更新
			}
			
			//没有返回过缓存数据
			boolean rule1 = !hasReturnCacheValue;
			
			//返回过缓存数据，下次的网络数据也成功，且配置项要求在此更新缓存
			boolean rule2 = (ret == 0) && (mCacheParam.postAction == CachePost.Continue || mCacheParam.postAction == CachePost.StopButRefresh);
			log.debug("rule1="+rule1+",rule2="+rule2);
			if(rule1 || rule2){
				sendUiMessage(what, false,ret, obj);
			}
		}
		
		private void sendUiMessage(int what,boolean isCache,int ret,Object obj){
			log.debug("sendUiMessage="+what+",isCache="+isCache+",ret="+ret+",mCallBackHanlder="+mCallBackHanlder);
			if(mCallBackHanlder != null){
				Message msg = mCallBackHanlder.obtainMessage(what, ret,isCache ? 1 : 0 , obj);
				Bundle bundle = new Bundle();
				bundle.putSerializable("tag", mTag);
				msg.setData(bundle);
				msg.sendToTarget();
			}
		}
		
		void handleException(Exception e){
			sendCallBackMsg(mEventCode, ExceptionCode.getErrorCode(e),null);
		}
	}
	
	class CacheParam{
		
		/**缓存的键*/
		public String cacheKey;		
		
		/**缓存对象的Class类型*/
		public Class<?> cacheObjClass;
		
		/**磁盘缓存选项*/
		public DiskCacheOption diskCacheOption;
		
		/**缓存更新后的行为*/
		public CachePost postAction = CachePost.Continue;

		/***
		 * @param cacheKey 缓存的键
		 * @param cacheObjClass 缓存对象的Class类型
		 * @param diskCacheOption 磁盘缓存选项  ，null表示不支持磁盘缓存
		 * @param continueNetRequst 缓存命中后是否继续网络请求
		 */
		public CacheParam(String cacheKey, Class<?> cacheObjClass, DiskCacheOption diskCacheOption, CachePost pAction) {		
			this.cacheKey = asKey(cacheKey);
			this.cacheObjClass = cacheObjClass;
			this.diskCacheOption = diskCacheOption;
			this.postAction = pAction;
		}
	}	

	/**程序一次生命周期中缓存更新后的行为*/
	enum CachePost{
		
		/**缓存更新后，之后缓存命中后，依然进行网络请求，更新缓存，适应于实时类事件*/
		Continue,
		
		/**缓存更新后， 之后不在进行网络请求了，UI用上次缓存数据，此次更新的缓存不再回调UI(避免界面连续收到两次回调低概率闪动)，适应于配置类事件*/
		Stop,
		
		/**缓存更新后， 之后不在进行网络请求了，先回调UI上次缓存数据，缓存更新后再次回调UI,适应于配置类事件*/
		StopButRefresh
	}
}
