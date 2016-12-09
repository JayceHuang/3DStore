package com.runmit.sweedee.sdkex.httpclient;

import java.io.IOException;
import java.io.InputStream;
import java.security.cert.X509Certificate;

import android.content.Context;

import com.runmit.sweedee.R;

public class NewKeySSLHandler extends ExceptionHandler{
	
	protected NewKeySSLHandler(Context context) {
		super(context);
		initCertificate();
	}

	protected void initCertificate(){
		mX509Certificate = loadCertificate(R.raw.ssl_key);
	}
	
	/**
	 * 
	 * @param resId
	 * @return
	 */
	protected X509Certificate loadCertificate(int resId) {
		InputStream finStream = mContext.getResources().openRawResource(resId);
		try {
			return loadCertificate(finStream);
		} finally{
			if(finStream != null){
				try {
					finStream.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
	}

}
