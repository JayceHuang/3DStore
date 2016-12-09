package com.runmit.sweedee.sdkex.httpclient;

import com.runmit.sweedee.R;
import android.content.Context;

/**
 * 老版本的bks key授权验证，对应资源文件为R.raw.client
 * @author Zhi.Zhang
 *
 */
public class OldSSLKeyHandler extends NewKeySSLHandler{

	public OldSSLKeyHandler(Context context) {
		super(context);
	}

	protected void initCertificate(){
		mX509Certificate = loadCertificate(R.raw.client);
	}

}
