package com.runmit.mediaserver;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;

import android.R.integer;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.runmit.mediaserver.MediaServer.OnVodTaskInfoListener;

public class VodTaskInfoService {

	private static Socket mSocket = null;
	private InputStream mInputStream;
	private OutputStream mOutputStream;
	private boolean isStop = false;
	private OnVodTaskInfoListener mOnVodTaskInfoListener;
	private VodTaskInfo mVodTaskInfo;
	private String mServer;
	private int mPort;
	private Handler mHandler;

	private final static String TAG = "VodTaskInfoService";
	private static final byte CR = '\r';
	public final int MSG_VODINFO = 1;

	private Handler mCallBackHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case MSG_VODINFO:
				if (mOnVodTaskInfoListener != null) {
					mOnVodTaskInfoListener.onGetVodTask((VodTaskInfo) msg.obj);
				}
				break;
			}
		}
	};

	public VodTaskInfoService(OnVodTaskInfoListener listner, String server,
			int port) {

		this.mOnVodTaskInfoListener = listner;
		this.mServer = server;
		this.mPort = port;

		mVodTaskInfo = new VodTaskInfo();
	}

	/**
	 * init
	 * 
	 * @param server
	 * @param port
	 */
	public void init(String server, int port) {
		Log.i(TAG, "init socket client andr server=" + server + ":" + port);

		try {
			mSocket = new Socket(server, port);

			mInputStream = mSocket.getInputStream();
			mOutputStream = mSocket.getOutputStream();

			receiveThead.start();
			
		} catch (UnknownHostException e) {
			e.printStackTrace();
			uninit();
		} catch (IOException e) {
			e.printStackTrace();
			uninit();
		} catch (Exception e) {
			e.printStackTrace();
			uninit();
		}

	}

	public void sendCommand(String head) {
		try {
			if (mOutputStream == null) {
				if (mSocket != null) {
					mOutputStream = mSocket.getOutputStream();
				} else {
					init(mServer, mPort);
				}
			}
			BufferedOutputStream out = new BufferedOutputStream(mOutputStream);
			if (out != null) {
				out.write(buildHttpHead(head).getBytes());
				if (out != null)
					out.flush();
			}

		} catch (IOException e) {
			e.printStackTrace();
			uninit();
		} catch (NullPointerException e) {
			e.printStackTrace();
			uninit();
		}
	}

	
	Thread receiveThead = new Thread() {
		String msgStr = null;

		public void run() {
			while (!isStop) {
				try {
					msgStr = readResponse(mInputStream);
					if (msgStr != null) {
						boolean ret = mVodTaskInfo.setJsonToObject(
								msgStr.toString(), mVodTaskInfo);
						if (ret) {
							Message msg = new Message();
							msg.what = MSG_VODINFO;
							msg.obj = mVodTaskInfo;
							mCallBackHandler.sendMessage(msg);
						}
						msgStr = null;
					}
				} catch (IOException e) {
					e.printStackTrace();
					uninit();
				}// 检测输入流中是否有新的数据
			}
		}
	};

	public String buildHttpHead(String content) {
		StringBuffer sb = new StringBuffer();
		sb.append("GET ");
		sb.append(content);
		sb.append(" HTTP/1.1\r\n");
		// sb.append("Host: localhost:8088\r\n");
		sb.append("Connection: Keep-Alive\r\n");
		sb.append("\r\n");

		return sb.toString();
	}

	private String readResponse(InputStream in) throws IOException {
		if (in == null)
			return null;

		// 消息报头
		Map<String, String> headers = readHeaders(in);

		String tmpContentString = headers.get("Content-Length");

		if (tmpContentString != null) {
			int contentLength = Integer.valueOf(tmpContentString);
			// 可选的响应正文
			byte[] body = readResponseBody(in, contentLength);

			String charset = headers.get("Content-Type");
			if (charset.matches(".+;charset=.+")) {
				charset = charset.split(";")[1].split("=")[1];
			} else {
				charset = "ISO-8859-1"; // 默认编码
			}
			return new String(body, charset);
		} else {
			return null;
		}

	}

	private Map<String, String> readHeaders(InputStream in) throws IOException {
		Map<String, String> headers = new HashMap<String, String>();

		String line;

		while (!("".equals(line = readLine(in)))) {
			String[] nv = line.split(": "); // 头部字段的名值都是以(冒号+空格)分隔的
			if (nv.length >= 2) {
				headers.put(nv[0], nv[1]);
			}
		}

		return headers;
	}

	// 读取内容
	private byte[] readResponseBody(InputStream in, int contentLength)
			throws IOException {
		ByteArrayOutputStream buff = new ByteArrayOutputStream(contentLength);

		int b;
		int count = 0;
		while (count++ < contentLength) {
			b = in.read();
			buff.write(b);
		}

		return buff.toByteArray();
	}

	/**
	 * 读取以CRLF分隔的一行，返回结果不包含CRLF
	 */
	private String readLine(InputStream in) throws IOException {
		int b;
		ByteArrayOutputStream buff = new ByteArrayOutputStream();
		b = in.read();
		while (b != CR && b != -1) {
			buff.write(b);
			b = in.read();
		}
		if (b != -1) {
			in.read(); // 读取 LF
		}

		String line = buff.toString();

		return line;
	}

	public void uninit() {
		try {
			isStop = true;

			if (mSocket != null) {
				mSocket.shutdownInput();
				mSocket.shutdownOutput();
				mSocket.close();
				mSocket = null;
			}

			if (mSocket != null) {
				mInputStream.close();
				mInputStream = null;
			}

			if (mOutputStream != null) {
				mOutputStream.close();
				mOutputStream = null;
			}

			mCallBackHandler.removeMessages(MSG_VODINFO);

			// receiveThead = null;
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
