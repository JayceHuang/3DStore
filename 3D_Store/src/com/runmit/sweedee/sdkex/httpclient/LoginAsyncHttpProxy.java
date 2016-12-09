package com.runmit.sweedee.sdkex.httpclient;

import java.io.InputStream;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import org.apache.http.Header;
import org.apache.http.HttpEntity;
import org.apache.http.conn.ssl.SSLSocketFactory;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;

import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.BinaryHttpResponseHandler;
import com.loopj.android.http.TextHttpResponseHandler;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

/**
 * 这个类主要是用来定时5天向服务器获取备用ip列表。 获取ip列表逻辑：首先通过正常域名获取，成功后
 * 保存获取ip列表；失败后在用PortalSrvList含有的ip列表去获取。 当用正常的域名登陆服务器失败时，可以用备用ip地址重新登陆服务器。
 * <p/>
 * 此类包含了重试逻辑。
 *
 * @author lzl
 */
public class LoginAsyncHttpProxy {

    private XL_Log log = new XL_Log(LoginAsyncHttpProxy.class);

    private final static LoginAsyncHttpProxy mInstance = new LoginAsyncHttpProxy();

    private AsyncHttpClient mClient;

    private Handler mHandler;

    private LoginAsyncHttpProxy() {
        mClient = new AsyncHttpClient();
        mClient.setSSLSocketFactory(SecureSocketFactory.getInstance());
        String lg = Locale.getDefault().getLanguage();		 
		String country = Locale.getDefault().getCountry();
		if(country.toLowerCase().equals("tw") && lg.toLowerCase().equals("zh")){//台湾的语言加上地区号
			lg = "zh_tw";
		}
		
        mClient.addHeader("language", lg);
        mClient.addHeader("superProjectId", Integer.toString(Constant.superProjectId));
        mClient.addHeader("hardwareId", Util.getPeerId());
        mClient.addHeader("clientId", Integer.toString(Constant.PRODUCT_ID));
    }

    private Context mContext = null;

    public void init(Context context) {
        mContext = context;
        mHandler = new Handler(mContext.getMainLooper());
    }
    
    public static LoginAsyncHttpProxy getInstance() {
        return mInstance;
    }

    private class AsyncHttpProxyReq {

        HttpEntity mEntity = null;

        LoginAsyncHttpProxyListener mResponseListener = null;

        private String mUrl = null;

        public AsyncHttpProxyReq(String url) {
            this.mUrl = url;
        }

        public void setHttpEntity(HttpEntity entity) {
            mEntity = entity;
        }

        public void setListener(LoginAsyncHttpProxyListener responseListener) {
            mResponseListener = responseListener;
        }

        public void sendRequest() {
            mClient.post(mContext, mUrl, mEntity, "application/json", new TextHttpResponseHandler() {

                @Override
                public void onFailure(int arg0, Header[] arg1, String arg2, Throwable error) {
                    log.debug("onFailure arg0=" + arg0);
                    if (mResponseListener != null) {
                        mResponseListener.onFailure(error);
                    }
                }

                @Override
                public void onSuccess(int statusCode, Header[] headers, String content) {
                    if (mResponseListener != null) {
                        mResponseListener.onSuccess(statusCode, headers, content);
                    }
                }

            });
        }
    }

    public void onSuccess(int statusCode, Header[] headers, String content) {

    }

    public void post(final String url, final HttpEntity entity, final LoginAsyncHttpProxyListener responseListener) {
        mHandler.post(new Runnable() {

            @Override
            public void run() {
                AsyncHttpProxyReq asyncHttpProxyReq = new AsyncHttpProxyReq(url);
                asyncHttpProxyReq.setHttpEntity(entity);
                asyncHttpProxyReq.setListener(responseListener);
                asyncHttpProxyReq.sendRequest();
            }
        });

    }

    public void get(String url, TextHttpResponseHandler responseHandler) {
        mClient.get(mContext, url, null, null, responseHandler);
    }
}
