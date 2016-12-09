package com.runmit.sweedee.sdkex.httpclient.simplehttp;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringWriter;
import java.io.UnsupportedEncodingException;
import java.io.Writer;

import org.apache.http.Header;

import com.google.gson.JsonSyntaxException;
import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.TextHttpResponseHandler;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.util.XL_Log;

public class LoadProxy {

	private static LoadProxy INSTANCE;

	private AsyncHttpClient mClient = new AsyncHttpClient();

	private XL_Log log = new XL_Log(LoadProxy.class);

	public static LoadProxy getInstance() {
		if (INSTANCE == null) {
			INSTANCE = new LoadProxy();
		}
		return INSTANCE;
	}

	public void loadData(final String url, final int rawId, final OnloadCallBackListener mOnloadCallBackListener) {
		log.debug("loadData url=" + url + ",rawId=" + rawId);
		mClient.get(url, new TextHttpResponseHandler() {

			final DiskDataCache mDiskDataCache = new DiskDataCache(StoreApplication.INSTANCE);

			public void onSuccess(int statusCode, Header[] headers, String content) {
				log.debug("content=" + content);
				try {
					mOnloadCallBackListener.onLoadSuccess(content);
					mDiskDataCache.saveCacheData(content, url);
				} catch (JsonSyntaxException e) {
					e.printStackTrace();
					onFailure(0, null, "", e);
				}
			}

			@Override
			public void onFailure(int statusCode, Header[] headers, String responseString, Throwable throwable) {
				String cacheData = mDiskDataCache.loadDataFromDiskImpl(url);
				if ((cacheData == null || cacheData.length() == 0) && rawId != 0) {
					cacheData = openRawResource(rawId);
				}
				log.debug("loadAppData cacheData=" + cacheData);
				if (cacheData != null && cacheData.length() > 0) {
					try {
						mOnloadCallBackListener.onLoadSuccess(cacheData);
					} catch (JsonSyntaxException e) {
						e.printStackTrace();
					}
				}
			}
		});
	}
	
	public String loadOnlyCache(final String url, final int rawId){
		final DiskDataCache mDiskDataCache = new DiskDataCache(StoreApplication.INSTANCE);
		String cacheData = mDiskDataCache.loadDataFromDiskImpl(url);
		if ((cacheData == null || cacheData.length() == 0) && rawId != 0) {
			cacheData = openRawResource(rawId);
		}
		log.debug("loadAppData cacheData=" + cacheData);
		return cacheData;
	}

	public static String openRawResource(int resourceid) {
		InputStream is = StoreApplication.INSTANCE.getResources().openRawResource(resourceid);
		Writer writer = new StringWriter();
		char[] buffer = new char[1024];
		try {
			Reader reader;
			try {
				reader = new BufferedReader(new InputStreamReader(is, "UTF-8"));
				int n;
				while ((n = reader.read(buffer)) != -1) {
					writer.write(buffer, 0, n);
				}
			} catch (UnsupportedEncodingException e) {
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

		} finally {
			try {
				is.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}

		String jsonString = writer.toString();
		return jsonString;
	}

	public interface OnloadCallBackListener {
		public void onLoadSuccess(String data) throws JsonSyntaxException;
	}
}
