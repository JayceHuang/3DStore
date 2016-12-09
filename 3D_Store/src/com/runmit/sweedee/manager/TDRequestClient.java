/**
 * 
 */
package com.runmit.sweedee.manager;

import java.util.HashSet;
import java.util.Set;

import org.apache.http.conn.ssl.SSLSocketFactory;
import org.apache.http.entity.ByteArrayEntity;

import android.os.Handler;

import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.RequestHandle;
import com.runmit.sweedee.manager.HttpServerManager.CacheParam;
import com.runmit.sweedee.manager.HttpServerManager.CachePost;
import com.runmit.sweedee.manager.HttpServerManager.ResponseHandler;

/**
 * @author Sven.Zhan
 * 请求的封装类，包括HTTP请求 和 缓存请求
 */
public class TDRequestClient {
	
	private Set<String> cacheUpdatedKeys = new HashSet<String>();
	
	private AsyncHttpClient mAsyncHttpClient = new AsyncHttpClient();
	
	private ObjectCacheManager cacheManager = ObjectCacheManager.getInstance();
	
	public void setSSLSocketFactory(SSLSocketFactory lssf) {
		mAsyncHttpClient.setSSLSocketFactory(lssf);
	}
	
	/**缓存->Http Get*/
	public void requestGet(String url,final ResponseHandler rHandler,boolean realRequst){
		Handler asyncHandler = rHandler.getAsyncHandler();
		asyncHandler.post(new RequsetGetRunnable(url, rHandler,realRequst));
	}
	
	/**缓存->Http Post*/
	public void requstPost(String url,String jsonContent,ResponseHandler rHandler){
		Handler asyncHandler = rHandler.getAsyncHandler();
		asyncHandler.post(new RequsetPostRunnable(url, rHandler,jsonContent));
	}
	
	/**直接Http Get*/
	protected RequestHandle requstCancableGet(String url,ResponseHandler rHandler){
		RequestHandle requstHanle = mAsyncHttpClient.get(url, rHandler);
		return requstHanle;
	}
	
	/**直接Http Post*/
	protected RequestHandle requstCancablePost(String url,String jsonContent,ResponseHandler rHandler){
		ByteArrayEntity byteEntity = new ByteArrayEntity(jsonContent.getBytes());
		RequestHandle requstHanle =  mAsyncHttpClient.post(null, url, byteEntity, "application/json", rHandler);
		return requstHanle;
	}
	
	private class RequsetGetRunnable implements Runnable{
		
		protected String url;
		
		protected ResponseHandler responseHandler;
		
		protected boolean realHttpRequst = true;

		public RequsetGetRunnable(String url, ResponseHandler responseHandler,boolean realHttpRequst) {
			this.url = url;
			this.responseHandler = responseHandler;
			this.realHttpRequst = realHttpRequst;
		}
		
		@Override
		public void run() {
			CacheParam cparam = responseHandler.getCacheParam();
			Object cacheObj = null;
			if(cparam.cacheKey != null){				
				cacheObj = cacheManager.pullFromCache(cparam.cacheKey, cparam.cacheObjClass, cparam.diskCacheOption);
				if(cacheObj != null){
					responseHandler.onCacheObtained(cacheObj);
				}
			}
			
			if(cacheObj != null && cacheUpdatedKeys.contains(cparam.cacheKey) && 
					( cparam.postAction == CachePost.Stop ||  cparam.postAction == CachePost.StopButRefresh)){
				//do - nothing 缓存更新之后不在继续请求
			}else{
				continueNetQeq();
			}
		}
		
		protected void continueNetQeq(){
			if(realHttpRequst){
				mAsyncHttpClient.get(url, responseHandler);
			}else{
				String responseString = FakeDataAndTestHelper.getContentString(responseHandler.getEventCode());
				int statusCode = responseString == null ? 404 : 200;
				responseHandler.onSuccess(statusCode, null, responseString);
			}
		}
	}
	
	
	private class RequsetPostRunnable extends RequsetGetRunnable{
		
		protected String jsonContent;
		
		public RequsetPostRunnable(String url, ResponseHandler responseHandler,String jsonContent) {
			super(url, responseHandler,true);
			this.jsonContent = jsonContent;
		}

		@Override
		protected void continueNetQeq() {
			ByteArrayEntity byteEntity = new ByteArrayEntity(jsonContent.getBytes());
			mAsyncHttpClient.post(null,url, byteEntity, "application/json", responseHandler);
		}
	}
	
	public void cacheUpdated(String key){
		cacheUpdatedKeys.add(key);
	}
	
}
