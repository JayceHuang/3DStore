package com.runmit.sweedee;

import java.io.File;
import java.lang.ref.WeakReference;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;

import android.annotation.TargetApi;
import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.Signature;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;

import com.nostra13.universalimageloader.cache.disc.impl.UnlimitedDiscCache;
import com.nostra13.universalimageloader.cache.disc.naming.Md5FileNameGenerator;
import com.nostra13.universalimageloader.cache.memory.impl.LruMemoryCache;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.ImageLoaderConfiguration;
import com.nostra13.universalimageloader.core.assist.QueueProcessingType;
import com.nostra13.universalimageloader.utils.StorageUtils;
import com.runmit.sweedee.StoreApplication.OnNetWorkChangeListener.NetType;
import com.runmit.sweedee.datadao.DataBaseManager;
import com.runmit.sweedee.download.MyAppInstallerReceiver;
import com.runmit.sweedee.downloadinterface.DownloadEngine;
import com.runmit.sweedee.imageloaderex.TDImageDownloader;
import com.runmit.sweedee.manager.Game3DConfigManager;
import com.runmit.sweedee.manager.InitDataLoader;
import com.runmit.sweedee.manager.ObjectCacheManager;
import com.runmit.sweedee.manager.WalletManager;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.sdk.user.member.task.UserUtil;
import com.runmit.sweedee.util.CrashHandler;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.umeng.analytics.MobclickAgent;

public class StoreApplication extends Application {

	XL_Log log = new XL_Log(StoreApplication.class);

	public static StoreApplication INSTANCE = null;

	private IntentFilter mWifiStateFilter;

	private MyAppInstallerReceiver mAppInstallerReceiver;

	private boolean isInitData = false;

	public ArrayList<AppItemInfo> mAppInfoList;

	public static boolean isSnailPhone = false;

	public StoreApplication() {
		super();
		INSTANCE = this;
	}

	@Override
	public void onCreate() {
		super.onCreate();

		if(Build.MODEL.toLowerCase().contains("w3d")){
			isSnailPhone = true;
		}
		log.debug("Build.MODEL="+Build.MODEL);
		mWifiStateFilter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
		this.registerReceiver(mWifiStateReceiver, mWifiStateFilter);

		// 根据签名去判断是否捕获异常
		String sha1 = initializeSignature(this, getPackageName());
		if ("4fe8b97cbede14436f81a0f74a6f4232".equalsIgnoreCase(sha1)) {
			CrashHandler crashHandler = CrashHandler.getInstance();
			crashHandler.init(this);
			MobclickAgent.setCatchUncaughtExceptions(false);
		}
		initImageLoader();
		UserUtil.getInstance().Init(this);
		InitDataLoader.getInstance().init(this);
		ObjectCacheManager.init();
		WalletManager.getInstance();// 初始化
		DataBaseManager.getInstance().init(this);
	}

	@Override
	public void onTerminate() {
		uninit();
		super.onTerminate();
	}

	/**
	 * 改為Loading界面去init
	 */
	public void initData() {
		if (isInitData) {
			return;
		}
		isInitData = true;

		InitDataLoader.getInstance().initHomeFakeData();
		Game3DConfigManager.getInstance().checkConfigUpdate();
		UserManager.getInstance().autoLogin();
		log.debug("init end");
	}

	private void uninit() {
		unregisterInstallerReceiver();
	}

	private void unregisterInstallerReceiver() {
		unregisterReceiver(mAppInstallerReceiver);
	}

	private String initializeSignature(Context context, String packageName) {
		/** 通过包管理器获得指定包名包含签名的包信息 **/
		PackageInfo packageInfo;
		try {
			packageInfo = context.getPackageManager().getPackageInfo(packageName, PackageManager.GET_SIGNATURES);
			/******* 通过返回的包信息获得签名数组 *******/
			Signature[] signatures = packageInfo.signatures;
			/******* 循环遍历签名数组拼接应用签名 *******/
			byte[] hexBytes = signatures[0].toByteArray();
			byte[] md5digest;
			try {
				md5digest = MessageDigest.getInstance("MD5").digest(hexBytes);
				return new String(convertByteArrayToHexArray(md5digest));
			} catch (NoSuchAlgorithmException e) {
				e.printStackTrace();
			}
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		return null;
	}

	/* 将byte数组中的每一个byte转化为0-F表示的两个char输出 */
	public static char[] convertByteArrayToHexArray(byte[] inputBuffer) {
		char[] output = new char[inputBuffer.length * 2];
		int index = 0;
		for (byte b : inputBuffer) {
			byte pre = b;
			pre = (byte) (pre >> 4);
			pre = (byte) (pre & 0x0f);
			byte post = b;
			post = (byte) (post & 0x0f);

			if (pre >= 0 && pre <= 9) {
				output[index++] = (char) ('0' + pre);
			} else if (pre >= 10 && pre <= 15) {
				output[index++] = (char) ('a' + pre - 10);
			}

			if (post >= 0 && post <= 9) {
				output[index++] = (char) ('0' + post);
			} else if (post >= 10 && post <= 15) {
				output[index++] = (char) ('a' + post - 10);
			}
		}
		return output;
	}

	public void killSelf() {
		INSTANCE = null;
	}

	private void initImageLoader() {
		File imageCacheDir = StorageUtils.getCacheDirectory(this);
		ImageLoaderConfiguration config = new ImageLoaderConfiguration.Builder(this).threadPriority(Thread.NORM_PRIORITY).threadPoolSize(5).memoryCache(new LruMemoryCache(16 * 1024 * 1024))
				.denyCacheImageMultipleSizesInMemory().diskCache(new UnlimitedDiscCache(imageCacheDir)).diskCacheFileNameGenerator(new Md5FileNameGenerator()).diskCacheSize(50 * 1024 * 1024) // 50
				.imageDownloader(new TDImageDownloader(INSTANCE)).tasksProcessingOrder(QueueProcessingType.LIFO)
				// .writeDebugLogs() // Remove for release app
				.build();
		// Initialize ImageLoader with configuration.
		ImageLoader.getInstance().init(config);
	}

	public static boolean isNetAvilible;

	private List<WeakReference<OnNetWorkChangeListener>> mOnNetWorkChangeListenerList = new ArrayList<WeakReference<OnNetWorkChangeListener>>();

	public void addOnNetWorkChangeListener(OnNetWorkChangeListener mListener) {
		mOnNetWorkChangeListenerList.add(new WeakReference<StoreApplication.OnNetWorkChangeListener>(mListener));
	}

	public void removeOnNetWorkChangeListener(OnNetWorkChangeListener mListener) {
		mOnNetWorkChangeListenerList.remove(new WeakReference<StoreApplication.OnNetWorkChangeListener>(mListener));
	}

	private BroadcastReceiver mWifiStateReceiver = new BroadcastReceiver() {

		@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
		public void onReceive(Context context, Intent intent) {

			if (ConnectivityManager.CONNECTIVITY_ACTION.equals(intent.getAction())) {
				Util.net_type_changed = true;
				isNetAvilible = !intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, false);
				int networkType = intent.getExtras().getInt(ConnectivityManager.EXTRA_NETWORK_TYPE);
				ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
				NetworkInfo mNetworkInfo = cm.getNetworkInfo(networkType);
				log.debug("isNetAvilible=" + isNetAvilible + ",networkType=" + networkType + ",mNetworkInfo=" + mNetworkInfo);
				NetType mtype = NetType.None;
				if (mNetworkInfo.getType() == ConnectivityManager.TYPE_WIFI) {
					mtype = NetType.WifiNet;
				} else {
					mtype = NetType.MobileNet;
				}
				for (WeakReference<OnNetWorkChangeListener> mReference : mOnNetWorkChangeListenerList) {
					OnNetWorkChangeListener mOnNetWorkChangeListener = mReference.get();
					if (mOnNetWorkChangeListener != null) {
						mOnNetWorkChangeListener.onChange(isNetAvilible, mtype);
					}
				}
			}
		}
	};

	public interface OnNetWorkChangeListener {
		enum NetType {
			MobileNet, WifiNet, None
		}

		/*
		 * 网络状态改变通知，首先判断isNetWorkAviliable 如果true则网络可用 后面判断wifi或者移动网络
		 */
		void onChange(boolean isNetWorkAviliable, NetType mNetType);
	}
}
