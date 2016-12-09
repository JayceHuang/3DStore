//package com.runmit.sweedee.sdkex.httpclient;
//
//import java.io.IOException;
//import java.io.InputStream;
//import java.net.Socket;
//import java.net.UnknownHostException;
//import java.security.KeyManagementException;
//import java.security.KeyStoreException;
//import java.security.NoSuchAlgorithmException;
//import java.security.UnrecoverableKeyException;
//import java.security.cert.CertificateException;
//import java.security.cert.CertificateFactory;
//import java.security.cert.X509Certificate;
//
//import javax.net.ssl.SSLContext;
//import javax.net.ssl.TrustManager;
//import javax.net.ssl.X509TrustManager;
//
//import org.apache.http.conn.ssl.SSLSocketFactory;
//
//import android.content.Context;
//import android.util.Log;
//
//import com.runmit.sweedee.R;
//
///**
// * @author Sven.Zhan https登录所用的到SSL加密工厂类
// */
//public class LoginSSLSocketFactory extends SSLSocketFactory {
//
//	private static final String LOG_TAG = "SecureSocketFactory";
//	
//	private SSLContext sslContext = SSLContext.getInstance("TLS");
//
//	public LoginSSLSocketFactory(final X509Certificate caCertificate) throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException {
//		super(null);
//
//		TrustManager tm = new X509TrustManager() {
//			@Override
//			public X509Certificate[] getAcceptedIssuers() {
//				return null;
//			}
//
//			@Override
//			public void checkServerTrusted(X509Certificate[] certs, String authType) throws CertificateException {
//				if (certs == null || certs.length == 0) {
//					throw new CertificateException("null or zero-length certificate chain");
//				}
//				if (authType == null || authType.length() == 0) {
//					throw new CertificateException("null or zero-length authentication type");
//				}
//				for (X509Certificate cert : certs) {
//					Log.i(LOG_TAG, "Server Certificate Details:");
//                    Log.i(LOG_TAG, "---------------------------");
//                    Log.i(LOG_TAG, "IssuerDN: " + cert.getIssuerDN().toString());
//                    Log.i(LOG_TAG, "SubjectDN: " + cert.getSubjectDN().toString());
//                    Log.i(LOG_TAG, "Serial Number: " + cert.getSerialNumber());
//                    Log.i(LOG_TAG, "Version: " + cert.getVersion());
//                    Log.i(LOG_TAG, "Not before: " + cert.getNotBefore().toString());
//                    Log.i(LOG_TAG, "Not after: " + cert.getNotAfter().toString());
//                    Log.i(LOG_TAG, "---------------------------");
//				}
//				
//				if (!certs[0].equals(caCertificate)) {
//					try {
//						certs[0].verify(caCertificate.getPublicKey());
//					} catch (Exception e) {
//						throw new CertificateException("Certificate not trusted", e);
//					}
//				}
//				try {
//					certs[0].checkValidity();
//				} catch (Exception e) {
//					throw new CertificateException("Certificate not trusted. It has expired", e);
//				}
//			}
//
//			@Override
//			public void checkClientTrusted(X509Certificate[] chain, String authType) throws CertificateException {
//			}
//		};
//		sslContext.init(null, new TrustManager[] { tm }, null);
//	}
//
//	@Override
//	public Socket createSocket() throws IOException {
//		return sslContext.getSocketFactory().createSocket();
//	}
//
//	@Override
//	public Socket createSocket(Socket socket, String host, int port, boolean autoClose) throws IOException, UnknownHostException {
//		return sslContext.getSocketFactory().createSocket(socket, host, port, autoClose);
//	}
//
//	public static LoginSSLSocketFactory getSSLSocketFactoryEx(Context mContext) {
//		InputStream finStream = mContext.getResources().openRawResource(R.raw.ssl_key);
//		try {
//			CertificateFactory cf = CertificateFactory.getInstance("X.509");
//			final X509Certificate caCertificate = (X509Certificate) cf.generateCertificate(finStream);
//
//			LoginSSLSocketFactory sf = new LoginSSLSocketFactory(caCertificate);
//			sf.setHostnameVerifier(SSLSocketFactory.STRICT_HOSTNAME_VERIFIER);
//
//			return sf;
//		} catch (NoSuchAlgorithmException e) {
//			e.printStackTrace();
//		} catch (CertificateException e) {
//			e.printStackTrace();
//		} catch (KeyManagementException e) {
//			e.printStackTrace();
//		} catch (KeyStoreException e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//		} catch (UnrecoverableKeyException e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//		} finally {
//			try {
//				finStream.close();
//			} catch (IOException e) {
//				e.printStackTrace();
//			}
//		}
//		return null;
//	}
//}
