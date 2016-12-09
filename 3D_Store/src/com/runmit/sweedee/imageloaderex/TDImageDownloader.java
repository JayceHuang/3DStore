package com.runmit.sweedee.imageloaderex;

import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.Map;

import android.content.Context;
import android.text.TextUtils;

import com.nostra13.universalimageloader.core.download.BaseImageDownloader;
import com.runmit.sweedee.manager.HttpServerManager;

/***
 * @author Sven.Zhan
 * cms请求图片的URL是动态的，gt和ts每一次请求都会变化
 * 此类在请求前拼凑一个完整的url
 */
public class TDImageDownloader extends BaseImageDownloader {

	private final String TAG = TDImageDownloader.class.getSimpleName();
	
	public TDImageDownloader(Context context) {
		super(context);
	}

	@Override
	protected InputStream getStreamFromNetwork(String imageUri, Object extra) throws IOException {
		Map<String,String> map = getParamsMap(imageUri);
		if(!map.isEmpty()){//重新拼接gt,ts参数
			String lg = map.get("lg");
			String ci = map.get("ci");
			String ts = String.valueOf(System.currentTimeMillis());
			String sha1key = HttpServerManager.sha1key;
			String gt = HttpServerManager.computeSHAHash(lg+ci+ts+sha1key);
			imageUri = imageUri+"&gt="+gt+"&ts="+ts;
		}
		return super.getStreamFromNetwork(imageUri, extra);
	}
	
	/**从URL中获取缓冲键值对*/
	private static Map<String,String> getParamsMap(String queryString) {
		Map<String,String> paramsMap = new HashMap<String,String>();
		if(TextUtils.isEmpty(queryString) || !queryString.startsWith(HttpServerManager.CMS_V1_BASEURL)){
			return paramsMap;
		}
		try {
			String[] splitStr = queryString.split("\\?");
			if (splitStr != null && splitStr.length == 2) {
				String tempQueryString = splitStr[1];
				String[] params = tempQueryString.split("&");
				if (params != null) {
					for (String str : params) {
						String[] keyValue = str.split("=");
						if (keyValue != null && keyValue.length == 2) {
							paramsMap.put(keyValue[0], keyValue[1]);
						}
					}
				}
			}
		} catch (Exception e) {
			
		}
		return paramsMap;
	}

}
