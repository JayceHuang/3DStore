package com.runmit.sweedee.sdkex.httpclient;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.CipherInputStream;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;

import android.content.Context;

public class ServerSSLHandler extends ExceptionHandler {

	private static final String FILE_NAME = "private_3dstore.txt";

	private static final String SERVER_SSL_KEY = "http://dl.d3dstore.com/public/ssl/d3dstore.ssl";

	private boolean hasLoadServerKey = false;

	public ServerSSLHandler(Context context) {
		super(context);
		mX509Certificate = loadLocalCertificate();
	}

	/**
	 * 加载本地秘钥
	 * 
	 * @return
	 */
	private X509Certificate loadLocalCertificate() {
		File mRootPath = mContext.getFilesDir();
		if (!mRootPath.exists()) {
			mRootPath.mkdirs();
		}

		File file = new File(mRootPath, FILE_NAME);
		InputStream finStream = null;
		log.debug("file=" + file.getPath() + ",exit=" + file.exists());
		if (file.exists()) {
			try {
				finStream = new FileInputStream(file);
				return loadCertificate(finStream);
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			} finally {
				if (finStream != null) {
					try {
						finStream.close();
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
			}
		}
		return null;
	}

	public void handleFeeRequest(X509Certificate mServerX509Certificate) throws CertificateException {
		if(hasError(mServerX509Certificate)){//如果本地证书为空或者验证失败
			try {
				if(mX509Certificate == null || !hasLoadServerKey){//本地证书不存在或者没有加载过
					loadServerKey();//从服务器下载证书
					hasLoadServerKey = true;
					
					mX509Certificate = loadLocalCertificate();
					if(mX509Certificate != null){
						try {
							if (!mServerX509Certificate.equals(mX509Certificate)) {
								mServerX509Certificate.verify(mX509Certificate.getPublicKey());
							}
							mServerX509Certificate.checkValidity();
						} catch (Exception e) {// 当前的本地证书验证失败
							throw new CertificateException("we have down neweast ssl key and we check fail");
						}
					}else{
						throw new CertificateException("we have down neweast ssl key and load local fail");
					}
				}else{
					throw new CertificateException("we have load server key but we check fail");
				}
			}catch(Exception e){
				throw new CertificateException("local mX509Certificate is null and we load server sslkey fail", e);
			}
		}
	}
	
	/**
	 * 检查本地证书是否为空或者是否验证失败
	 * @param mServerX509Certificate
	 * @return
	 */
	private boolean hasError(X509Certificate mServerX509Certificate){
		if (mX509Certificate == null) {
			return true;
		}else{
			try {
				if (!mServerX509Certificate.equals(mX509Certificate)) {
					mServerX509Certificate.verify(mX509Certificate.getPublicKey());
				}
				mServerX509Certificate.checkValidity();
				return false;
			} catch (Exception e) {// 当前的本地证书验证失败
				return true;
			}
		}
	}

	/**
	 * 从服务器下载最新的sslkey，然后保存到文件 如果下载成功并且保存到本地文件，则返回true 否则返回false
	 * 
	 * @return
	 * @throws NoSuchAlgorithmException
	 * @throws NoSuchPaddingException
	 * @throws InvalidKeyException
	 * @throws BadPaddingException
	 * @throws IllegalBlockSizeException
	 */
	private boolean loadServerKey() throws IOException, NoSuchAlgorithmException, NoSuchPaddingException, InvalidKeyException, IllegalBlockSizeException, BadPaddingException {
		String url = SERVER_SSL_KEY + "?timestamp=" + System.currentTimeMillis();
		HttpGet httpGet = new HttpGet(url);
		final HttpParams httpParams = new BasicHttpParams();
		HttpConnectionParams.setConnectionTimeout(httpParams, 5000);
		HttpConnectionParams.setSoTimeout(httpParams, 5000);
		HttpClient httpClient = new DefaultHttpClient(httpParams);

		HttpResponse response = httpClient.execute(httpGet);
		int rtn_code = response.getStatusLine().getStatusCode();
		if (rtn_code == 200) {
			HttpEntity entity = response.getEntity();
			InputStream is = entity.getContent();
			FileOutputStream fos = mContext.openFileOutput(FILE_NAME, Context.MODE_PRIVATE);

			byte[] readBuffer = new byte[8192];
			Cipher deCipher = getCipher(Cipher.DECRYPT_MODE, ssl_key);
			if (deCipher != null) {
				CipherInputStream fis = null;
				BufferedOutputStream bos = null;
				int size;
				fis = new CipherInputStream(new BufferedInputStream(is), deCipher);
				bos = new BufferedOutputStream(fos);
				while ((size = fis.read(readBuffer, 0, BUFFER_SIZE)) >= 0) {
					bos.write(readBuffer, 0, size);
				}
				bos.flush();
				bos.close();
			}
			fos.close();
			is.close();
			log.debug("success url=" + url);
			return true;
		} else {
			throw new RuntimeException("rtn_code=" + rtn_code);
		}
	}

	private static String TYPE = "AES";
	private static int KeySizeAES128 = 16;
	private static int BUFFER_SIZE = 8192;

	private static Cipher getCipher(int mode, String key) {
		Cipher mCipher;
		byte[] keyPtr = new byte[KeySizeAES128];
		IvParameterSpec ivParam = new IvParameterSpec(keyPtr);
		byte[] passPtr = key.getBytes();
		try {
			mCipher = Cipher.getInstance(TYPE + "/CBC/PKCS5Padding");
			for (int i = 0; i < KeySizeAES128; i++) {
				if (i < passPtr.length)
					keyPtr[i] = passPtr[i];
				else
					keyPtr[i] = 0;
			}
			SecretKeySpec keySpec = new SecretKeySpec(keyPtr, TYPE);
			mCipher.init(mode, keySpec, ivParam);
			return mCipher;
		} catch (InvalidKeyException e) {
			e.printStackTrace();
		} catch (NoSuchAlgorithmException e) {
			e.printStackTrace();
		} catch (NoSuchPaddingException e) {
			e.printStackTrace();
		} catch (InvalidAlgorithmParameterException e) {
			e.printStackTrace();
		}
		return null;
	}
}
