/**
 * 
 */
package com.runmit.sweedee.player;
import java.io.*;
import java.net.*;
import java.security.cert.CertificateException;
import java.security.cert.CertificateNotYetValidException;

import javax.net.ssl.SSLHandshakeException;

import org.apache.http.client.*;

import android.util.Log;

import com.google.gson.*;

import org.json.JSONException;

/**
 * @author Sven.Zhan 客户端异常和错误码的映射
 */
public class ExceptionCode {

	/*--------------以下为客户端的场景错误码，如无网络，未登录等------------------*/
	public static final int ERROR_DEFUALT = 20000;
	public static final int ERR_NO_NETWORK = ERROR_DEFUALT+1;// 无网络
	
	
	/*--------------以下为客户端的异常错误码，如JSONException SocketException等 ，需要在此补充  ，以5分段------------------*/
	public static final int ClientProtocolException = 30000;
	public static final int JSONException = ClientProtocolException+1;
	public static final int SocketException = JSONException+1;
	public static final int SocketTimeoutException = SocketException+1;
	public static final int ParseException = SocketTimeoutException+1;
	
	public static final int IOException = 30005;
	public static final int UnknownHostException = IOException+1;
	public static final int UnsupportedEncodingException = UnknownHostException+1;
	public static final int JsonParseException = UnsupportedEncodingException+1;
	public static final int NullPointerException = JsonParseException+1;
	
	public static final int CertificateException = 30010;
	public static final int NoSpaceException = CertificateException + 1;//存储空间不足
	public static final int GetConnectionException = NoSpaceException + 1;//创建连接失败
	

	/**根据Throwable类型获取对应的错误码*/
	public static int getErrorCode(Throwable throwable) {
		int errorCode = ERROR_DEFUALT;
		try {
			throw throwable;
		} catch (ClientProtocolException e) {
			errorCode = UnsupportedEncodingException;
		}catch (JSONException e) {
			errorCode = JSONException;
		}catch (SocketException e) {
			errorCode = SocketException;
		}catch (SocketTimeoutException e) {
			errorCode = SocketTimeoutException;
		}catch (UnsupportedEncodingException e) {
			errorCode = UnsupportedEncodingException;
		}catch (UnknownHostException e) {
			errorCode = UnknownHostException;
		}catch (JsonParseException e) {
			errorCode = JsonParseException;
		}catch (NullPointerException e) {
			errorCode = NullPointerException;
		}catch (CertificateException e) {
			errorCode = CertificateException;
		}catch (SSLHandshakeException e) {
			errorCode = CertificateException;
		}catch (IOException e) {
			errorCode = IOException;
		}catch (Throwable e) {
			
		}
		return errorCode;
	}
}
