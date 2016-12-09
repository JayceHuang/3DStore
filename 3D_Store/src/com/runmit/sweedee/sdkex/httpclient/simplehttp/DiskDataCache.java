/**
 * DiskDataCache.java 
 * com.xunlei.share.manager.DiskDataCache
 * @author: Administrator
 * @date: 2013-5-30 下午5:15:49
 */
package com.runmit.sweedee.sdkex.httpclient.simplehttp;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import android.content.Context;
import android.util.Log;

import com.runmit.sweedee.downloadinterface.DownloadEngine;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

/**
 * 
 * @author Administrator 文件缓存
 * 
 *         修改记录：修改者，修改日期，修改内容
 */
public class DiskDataCache {

	private static Object mSyncObj = new Object();

	private Context mContex;

	private static final String DISK_PATH = "data";

	/**
	 * @param context
	 * @param dataType
	 *            :数据type,唯一的字符串即可,作为文件名的hash
	 */
	public DiskDataCache(Context context) {
		this.mContex = context;
	}

	/**
	 * 加密加压保存数据到本地磁盘
	 * 
	 * @param data
	 *            :数据内容
	 * @param DataType
	 *            :数据type,唯一的字符串即可
	 */
	public void saveCacheData(final String data,final String key) {

		new Thread(new Runnable() {
			@Override
			public void run() {
				synchronized (mSyncObj) {
					File dirs = getDiskCacheDir(mContex, DISK_PATH);
					if (!dirs.exists()) {
						dirs.mkdirs();
					}
					final String hashKey = hashKeyForDisk(key);
					try {
						FileOutputStream fos = new FileOutputStream(new File(dirs, hashKey));
						fos.write(data.getBytes("utf-8"));
						fos.flush();
						fos.close();
					} catch (FileNotFoundException e) {
						e.printStackTrace();
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
			}
		}).start();

	}

	public String loadDataFromDiskImpl(String key) {
		synchronized (mSyncObj) {
			File dirs = getDiskCacheDir(mContex, DISK_PATH);
			if (dirs.exists()) {
				dirs.mkdirs();
			}
			final String hashKey = hashKeyForDisk(key);
			FileInputStream fis = null;
			ByteArrayOutputStream swapStream = new ByteArrayOutputStream();
			byte[] buff = new byte[1024];
			int len = 0;
			String decompressData = null;
			try {
				fis = new FileInputStream(dirs.getPath() + File.separator + hashKey);
				while ((len = fis.read(buff, 0, 1024)) > 0) {
					swapStream.write(buff, 0, len);
				}
				byte[] getserverdata = swapStream.toByteArray();
				String data = new String(getserverdata);
				swapStream.flush();
				swapStream.close();
				fis.close();
				return data;
			} catch (Exception e) {
				e.printStackTrace();
			}
			return decompressData;
		}

	}

	public String loadDataFromResource(int resId) {
		synchronized (mSyncObj) {
			InputStream fis = mContex.getResources().openRawResource(resId);
			ByteArrayOutputStream swapStream = new ByteArrayOutputStream();
			byte[] buff = new byte[1024];
			int len = 0;
			String decompressData = null;
			try {
				while ((len = fis.read(buff, 0, 1024)) > 0) {
					swapStream.write(buff, 0, len);
				}
				byte[] getserverdata = swapStream.toByteArray();
				decompressData = new String(getserverdata);
				swapStream.flush();
				swapStream.close();
				fis.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
			return decompressData;
		}

	}

	/**
	 * A hashing method that changes a string (like a URL) into a hash suitable
	 * for using as a disk filename.
	 */
	public static String hashKeyForDisk(String key) {
		String cacheKey;
		try {
			final MessageDigest mDigest = MessageDigest.getInstance("MD5");
			mDigest.update(key.getBytes());
			cacheKey = bytesToHexString(mDigest.digest());
		} catch (NoSuchAlgorithmException e) {
			cacheKey = String.valueOf(key.hashCode());
		}
		return cacheKey;
	}

	private static String bytesToHexString(byte[] bytes) {
		// http://stackoverflow.com/questions/332079
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < bytes.length; i++) {
			String hex = Integer.toHexString(0xFF & bytes[i]);
			if (hex.length() == 1) {
				sb.append('0');
			}
			sb.append(hex);
		}
		return sb.toString();
	}

	/**
	 * Get a usable cache directory (external if available, internal otherwise).
	 * 
	 * @param context
	 *            The context to use
	 * @param uniqueName
	 *            A unique directory name to append to the cache dir
	 * @return The cache dir
	 */
	private File getDiskCacheDir(Context context, String uniqueName) {
		String cachePath = null;
		try {
			cachePath = context.getCacheDir().getPath();
		} catch (NullPointerException e) {
			cachePath = Util.getSDCardDir() + XL_Log.LOG_PATH;
		}

		String path = cachePath + File.separator + uniqueName;
		Log.d("getDiskCacheDir", "getDiskCacheDir=" + path);
		return new File(path);
	}

	public interface OnDiskCacheLoad {
		/**
		 * 
		 * 数据加载完成回调。如果失败返回null
		 * 
		 * @param data
		 */
		public void onLoad(String data);
	}
}
