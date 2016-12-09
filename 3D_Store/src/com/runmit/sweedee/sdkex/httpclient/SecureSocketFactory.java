package com.runmit.sweedee.sdkex.httpclient;

import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.KeyManagementException;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import org.apache.http.conn.ssl.SSLSocketFactory;

import android.content.Context;

import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.util.XL_Log;

public class SecureSocketFactory extends SSLSocketFactory {

	private SSLContext sslContext = SSLContext.getInstance("TLS");

	private XL_Log log = new XL_Log(SecureSocketFactory.class);
	
	private ExceptionHandler mExceptionHandler;

	private TrustManager mTrustManager = new X509TrustManager() {

		@Override
		public void checkServerTrusted(X509Certificate[] certs, String authType) throws CertificateException {
			log.debug("authType="+authType+",certs="+certs);
			if (certs == null || certs.length == 0) {
				throw new CertificateException("null or zero-length certificate chain");
			}
			if (authType == null || authType.length() == 0) {
				throw new CertificateException("null or zero-length authentication type");
			}

			mExceptionHandler.handleFeeRequest(certs[0]);
		}

		@Override
		public void checkClientTrusted(X509Certificate[] chain, String authType) throws CertificateException {

		}

		@Override
		public X509Certificate[] getAcceptedIssuers() {
			return null;
		}
	};

	public SecureSocketFactory(Context context) throws KeyManagementException, UnrecoverableKeyException, NoSuchAlgorithmException, KeyStoreException {
		super(null);
		mExceptionHandler = ExceptionHandler.getExceptionHandler(context);
		sslContext.init(null, new TrustManager[] { mTrustManager }, null);
	}

	@Override
	public Socket createSocket() throws IOException {
		return sslContext.getSocketFactory().createSocket();
	}

	@Override
	public Socket createSocket(Socket socket, String host, int port, boolean autoClose) throws IOException, UnknownHostException {
		return sslContext.getSocketFactory().createSocket(socket, host, port, autoClose);
	}

	public static SecureSocketFactory getInstance() {
		try {
			return new SecureSocketFactory(StoreApplication.INSTANCE);
		} catch (KeyManagementException e) {
			e.printStackTrace();
		} catch (UnrecoverableKeyException e) {
			e.printStackTrace();
		} catch (NoSuchAlgorithmException e) {
			e.printStackTrace();
		} catch (KeyStoreException e) {
			e.printStackTrace();
		}
		return null;
	}
}
