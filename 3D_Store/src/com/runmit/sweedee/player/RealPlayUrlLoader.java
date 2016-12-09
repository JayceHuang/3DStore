/**
 * 
 */
package com.runmit.sweedee.player;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.util.EntityUtils;

import android.text.TextUtils;
import android.util.Log;

import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;

/**
 * @author Sven.Zhan
 * 从CMS返回的调度地址中请求各组清晰度地址，以各组清晰度的真实播放地址为输出结果
 */
public class RealPlayUrlLoader{
	
	public static final String P720 = "720P";
	
	public static final String P1080 = "1080P";
	
	//private static final String P252 = "252P";//分辨率的名称服务器暂未改正，已预告片252P当做720P
	
	/**GSLB的清晰度地址信息*/
	List<Redition> gslbReditons;
	
	/**GSLB对应的CND真是地址清晰度信息*/
	ArrayList<Redition> cdnReditons = new ArrayList<Redition>();
		
	/**转换回调*/
	private LoaderListener mLoaderListener;
	
	private Gson gson = new Gson();
	
	/**计数器*/
	private int loadCount = 0;
	
	int sortValue = 0;
	
	public RealPlayUrlLoader(List<Redition> gslbs,LoaderListener listener) {
		this.gslbReditons = gslbs;
		mLoaderListener = listener;
		if(isEmpty(gslbReditons)){
			loadCount = 0;
		}else{
			loadCount = gslbReditons.size();
		}
	}


	/**
	 * 根据GSLB调度地址请求真实地址
	 */
	public void load(){
		if(loadCount <= 0){
			mLoaderListener.onRealPlayUrlLoaded(-100, null);
			return;
		}
		for(final Redition redition:gslbReditons){
			new Thread(new Runnable() {				
				@Override
				public void run() {
					Redition cdnRediton = reqRealPlayUrlList(redition);
					onTreadFinished(cdnRediton);
				}
			}).start();
		}
	}
	
	private Redition reqRealPlayUrlList(Redition rediont){
		if(isEmpty(rediont.url)){
			return null;
		}
		for(String gurl : rediont.url){
			ArrayList<String> urls = reqRealUrlByGurl(gurl);
			if(urls != null){
				Redition ret = new Redition(rediont.title,urls);
				return ret;
			}
		}
		return null;
	}
	
	private ArrayList<String> reqRealUrlByGurl(String url){
		try {
			BasicHttpParams httpParams = new BasicHttpParams();
			HttpConnectionParams.setConnectionTimeout(httpParams, 5 * 1000);
			HttpConnectionParams.setSoTimeout(httpParams, 5 * 1000);
			HttpClient client = new DefaultHttpClient(httpParams);
			HttpGet request = new HttpGet(url);	
			request.addHeader("Content-Type", "application/json");
			HttpResponse httpResponse = client.execute(request);
			if(httpResponse.getStatusLine().getStatusCode()==HttpStatus.SC_OK){
				String strResult = EntityUtils.toString(httpResponse.getEntity(),"UTF-8");
				Log.d("reqRealUrlByGurl", "strResult="+strResult);
				VORealUrl vo = gson.fromJson(strResult, VORealUrl.class);
				ArrayList<String> urls = vo.toUrlList();
				return urls;
			}
		} catch (IOException e) {
			e.printStackTrace();
		}catch (JsonSyntaxException e) {
			e.printStackTrace();
		}
		return null;
	}
	
	private synchronized void onTreadFinished(Redition cdnRediton){
		if(cdnRediton != null){
			cdnReditons.add(cdnRediton);	
		}		
		loadCount --;		
		if(loadCount == 0){				
			List<String> P720RealList = null;
			List<String> P1080RealList = null;
			if(!cdnReditons.isEmpty()){
				for(Redition rd : cdnReditons){
					if(P720.equals(rd.title)){// || P252.equals(rd.title)){
						P720RealList = rd.url;
					}else if(P1080.equals(rd.title)){
						P1080RealList = rd.url;
					}
				}
			}
			RealPlayUrlInfo rpInfo = new RealPlayUrlInfo(P720RealList, P1080RealList);
			int ret = rpInfo.isEmpty() ? -101 : 0;
			mLoaderListener.onRealPlayUrlLoaded(ret, rpInfo);
		}
	}
	
	private boolean isEmpty(List<? extends Object> list){
		return list == null || list.isEmpty();
	}
	
	/**CMS返回的调度地址的Json VO*/
	public class VORealUrl{
		
		public String ip;
		
		public String location;
		
		public String besturl;
		
		public List<RURL> urls;
		
		public ArrayList<String> toUrlList(){
			ArrayList<String> result = new ArrayList<String>();
			if(!TextUtils.isEmpty(besturl)){
				result.add(besturl);
			}			
			if(urls != null){
				for(RURL rurl : urls ){
					String ourl = rurl.url;
					if(!TextUtils.isEmpty(ourl) && !ourl.equals(besturl)){
						result.add(rurl.url);
					}
				}
			}
			if(result.size() > 0){
				return result;
			}else{
				return null;
			}
		}
		
		public class RURL{
			
			public String url;
			
			public int cdnid;
			
			public String cdndesc;
		}
	}

	public interface LoaderListener{		
		public void onRealPlayUrlLoaded(int result,RealPlayUrlInfo rInfo);
	}
	
	/**视频清晰度信息<>*/
	public static class Redition{
		
		public Redition(String title, ArrayList<String> url) {
			super();
			this.title = title;
			this.url = url;
		}

		/**清晰度名字*/
		public String title;
		
		/**清晰度对应的url列表*/
		public ArrayList<String> url;
	}
}
