package com.runmit.sweedee.sdkex.httpclient;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.Enumeration;

import com.loopj.android.http.AsyncHttpClient;
import com.runmit.sweedee.util.XL_Log;

import android.content.Context;

public abstract class ExceptionHandler {
	
	protected Context mContext;
	
	protected static final String ssl_key = "d3dstore.com";
	
	protected XL_Log log = new XL_Log(ExceptionHandler.class);
	
	protected X509Certificate mX509Certificate;
	
	protected ExceptionHandler successor = null;
	
	public ExceptionHandler(Context context){
		this.mContext = context;
	}

	public void setSuccessor(ExceptionHandler successor) {
		this.successor = successor;
	}
	
	protected X509Certificate loadCertificate(InputStream finStream) {
		try {
			KeyStore trusted = KeyStore.getInstance("BKS");
			trusted.load(finStream, ssl_key.toCharArray());

			Enumeration<String> mEnumeration = trusted.aliases();
			String alias = null;
			if (mEnumeration.hasMoreElements()) {
				alias = mEnumeration.nextElement().toString();
			}
			final Certificate rootca = trusted.getCertificate(alias);
			InputStream is = new ByteArrayInputStream(rootca.getEncoded());
			X509Certificate x509ca = (X509Certificate) CertificateFactory.getInstance("X.509").generateCertificate(is);
			AsyncHttpClient.silentCloseInputStream(is);
			if (null == x509ca) {
				throw new CertificateException("Embedded SSL certificate has expired.");
			}
			x509ca.checkValidity();

			return x509ca;
		} catch (IOException e) {
			e.printStackTrace();
		} catch (KeyStoreException e) {
			e.printStackTrace();
		} catch (NoSuchAlgorithmException e) {
			e.printStackTrace();
		} catch (CertificateException e) {
			e.printStackTrace();
		}
		return null;
	}

	public void handleFeeRequest(X509Certificate mServerX509Certificate) throws CertificateException {
		log.debug("currenthandler="+this.getClass().getName()+",successor="+successor+",mX509Certificate isnull="+(mX509Certificate == null));
		if(mX509Certificate != null){//如果当前的mX509Certificate不为空，则验证
			try {
				if (!mServerX509Certificate.equals(mX509Certificate)) {
					mServerX509Certificate.verify(mX509Certificate.getPublicKey());
				}
				mServerX509Certificate.checkValidity();
			} catch (Exception e) {//当前验证失败
				if(successor != null){//否则,它的下一个责任链不为空，则传递给下一个授权链接
					successor.handleFeeRequest(mServerX509Certificate);
				}else{//如果都没有，则抛出异常
					throw new CertificateException("current auth Certificate check fail and the next handler is null");
				}
			}
		}else if(successor != null){//否则,它的下一个责任链不为空，则传递给下一个授权链接
			successor.handleFeeRequest(mServerX509Certificate);
		}else{//如果都没有，则抛出异常
			throw new CertificateException("mX509Certificate is null and the next handler is also null");
		}
	}
	
	public static ExceptionHandler getExceptionHandler(Context context){
		NewKeySSLHandler mNewKeySSLHandler = new NewKeySSLHandler(context);
		OldSSLKeyHandler mOldSSLKeyHandler = new OldSSLKeyHandler(context);
		ServerSSLHandler mServerSSLHandler = new ServerSSLHandler(context);
		
		mNewKeySSLHandler.setSuccessor(mOldSSLKeyHandler);
		mOldSSLKeyHandler.setSuccessor(mServerSSLHandler);
		
		return mNewKeySSLHandler;
	}
}
