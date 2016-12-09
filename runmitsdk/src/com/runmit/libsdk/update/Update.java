package com.runmit.libsdk.update;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.Serializable;
import java.text.MessageFormat;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.protocol.HTTP;
import org.apache.http.util.EntityUtils;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.DialogInterface.OnClickListener;
import android.content.SharedPreferences.Editor;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.libsdk.R;
import com.runmit.libsdk.util.RmLog;
import com.runmit.libsdk.util.SdkUtil;

public class Update {

	private static final String TAG = "Update";
	private static final int DOWNLOAD_FILE_INFO_ERROR = 1; // 下载数据错误
	private static final int APK_DOWNLOAD_DONE = 2; // 缓存apk完成
	private static final int MSG_UPDATE_DOWNLOAD_PROGRESS = 3; // 缓存任务interval
	private static final int EVENT_GET_APP_UPDATE_CONFIG = 4; // 获取更新信息
	private static final int NO_UPGRADE_FOUND = 5; // 获取更新信息
	private static final int IS_THE_LATEST_VER = 6; // 获取更新信息

	private ProgressDialog pBar;
	private Context mContext;
	private boolean mIsManualUpdate = false;// 是否首页更新
	private ProgressDialog mDialog;
	private Handler handler;
	private String mThisVersion;

	// 更新缓存进度
	private Timer mTimer;
	private int readLen = 0;
	private long fileSize = 0;// 原始文件大小

	private final String UPDATE_URL = "http://clotho.d3dstore.com/upgrade/getupgrade?";

	private static final int ConnectTimeOut = 10 * 1000;

	private File mDownloadFile;

	private UpdateInfo mUpdateInfo;
	
	private SharedPreferences mSharedPreferences = null;
    private Editor mEditor = null;
    public static final String PREFERENCES = "update_sp";

	public Update(Context context) {
		mContext = context;
		mDialog = new ProgressDialog(mContext, R.style.ProgressDialogDark);
		
		mSharedPreferences = mContext.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE);
        mEditor = mSharedPreferences.edit();
        
		handler = new Handler(context.getMainLooper()) {
			public void handleMessage(Message msg) {
				switch (msg.what) {
				case EVENT_GET_APP_UPDATE_CONFIG:
					RmLog.debug(TAG, "mUpdateInfo = " + mUpdateInfo);
					doGotUpdateInfo();
					break;
				case APK_DOWNLOAD_DONE:
					Log.d(TAG, "APK_DOWNLOAD_DONE");
					apkFetched();
					break;
				case NO_UPGRADE_FOUND:
					if (mIsManualUpdate) {
						showIsLastMsg(mContext.getString(R.string.no_found_new_ver));
					}
					break;
				case IS_THE_LATEST_VER:
					if (mIsManualUpdate) {
						String versionInfo = mContext.getString(R.string.is_lattest_ver);
						showIsLastMsg(versionInfo);
					}
					break;
				case DOWNLOAD_FILE_INFO_ERROR:// 下载出错
					dismissProgressBar();
					stopTimer();
					Toast.makeText(mContext, mContext.getString(R.string.update_failed), Toast.LENGTH_SHORT).show();
					break;
				case MSG_UPDATE_DOWNLOAD_PROGRESS:// 显示进度条
					if (pBar == null) {
						return;
					}
					Bundle b = msg.getData();
					int progress = b.getInt("progress");
					RmLog.debug(TAG, "progress=" + progress);
					String upFormat = mContext.getString(R.string.update_loading);
					String upStr = MessageFormat.format(upFormat, progress);
					pBar.setMessage(upStr);
					break;
				default:
					break;
				}
			}
		};
	}

	private void doGotUpdateInfo() {
		mThisVersion = SdkUtil.getVerName(mContext);
		RmLog.debug(TAG, "mUpdateInfo = " + mUpdateInfo);
		if (mUpdateInfo.upgrade_type > 0) {
			doNewVersionUpdate(mUpdateInfo.upgrade_type);
		} else {
			if (mIsManualUpdate) {
				String versionInfo = "";
				if (null != mThisVersion || !("".equals(mThisVersion))) {
					versionInfo = mContext.getString(R.string.is_lattest_ver) + "(" + mContext.getString(R.string.app_name) + mThisVersion + ")";
				}
				dismissDialog(versionInfo);
			}
		}
	}

	private void showIsLastMsg(String message) {
		if (mDialog != null) {
			SdkUtil.dismissDialog(mDialog);
		}
		AlertDialog.Builder builder = new AlertDialog.Builder(mContext, R.style.AlertDialog);
		builder.setTitle(SdkUtil.setColourText(mContext, R.string.update_info, SdkUtil.DialogTextColor.BLUE));
		builder.setMessage(message);
		builder.setNegativeButton(SdkUtil.setColourText(mContext, R.string.ok, SdkUtil.DialogTextColor.BLUE), new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
			}
		});
		builder.create().show();
	}

	private void dismissDialog(String promptMsg) {
		if (mDialog != null) {
			SdkUtil.dismissDialog(mDialog);
		}
		if (!TextUtils.isEmpty(promptMsg))
			Toast.makeText(mContext, promptMsg, Toast.LENGTH_SHORT).show();
	}

	private void doNewVersionUpdate(final int value) {
		dismissDialog(null);
		boolean isShowCurrentVersion = mSharedPreferences.getBoolean(mUpdateInfo.new_version, false);
		if(isShowCurrentVersion && value != 2 && !mIsManualUpdate){//不再显示当前版本并且不是强制升级时候才会返回
			return;
		}
		String showMessage = mContext.getString(R.string.update_show_msg_f) + mUpdateInfo.introduction;
		RmLog.debug(TAG, "showMessage="+showMessage+",mMessage="+mUpdateInfo.introduction);
		
		LayoutInflater mLayoutInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		View view = (ViewGroup) mLayoutInflater.inflate(R.layout.more_update_info, null, true);
		TextView tvInfo = (TextView) view.findViewById(R.id.tvInfo);
		tvInfo.setText(showMessage);
		CheckBox checkbox_ignore_current_version = (CheckBox) view.findViewById(R.id.checkbox_ignore_current_version);
		checkbox_ignore_current_version.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				mEditor.putBoolean(mUpdateInfo.new_version, isChecked);
		        mEditor.commit();
			}
		});
		RmLog.debug(TAG, "value="+value+",mIsManualUpdate="+mIsManualUpdate);
		if(value == 2 || mIsManualUpdate){//強制升級隱藏checkbox
			checkbox_ignore_current_version.setVisibility(View.GONE);
		}
		
		TextView download_filesize = (TextView) view.findViewById(R.id.tvFileSize);
		download_filesize.setText(mContext.getString(R.string.update_download_filesize) + SdkUtil.convertFileSize(mUpdateInfo.filesize, 2, false));
		
		AlertDialog.Builder builder = new AlertDialog.Builder(mContext, R.style.AlertDialog);
		builder.setTitle(SdkUtil.setColourText(MessageFormat.format(mContext.getString(R.string.update_tip_titile), mUpdateInfo.new_version), SdkUtil.DialogTextColor.BLUE));
		builder.setView(view);
		builder.setNegativeButton(SdkUtil.setColourText(mContext, R.string.update, SdkUtil.DialogTextColor.BLUE), new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				
				if (SdkUtil.isSDCardExist()) {
					mDownloadFile = getDownloadFile();
					if(isSameFile(mDownloadFile)){//如果从本地文件获取的版本号和最新的版本号一致
						handler.sendEmptyMessage(APK_DOWNLOAD_DONE);
					}else{
						downFile(mUpdateInfo.upgrade_url);
					}
				} else {
					Toast.makeText(mContext, mContext.getString(R.string.no_found_sdcard), Toast.LENGTH_SHORT).show();
				}

			}
		});
		builder.setPositiveButton(R.string.cancel, new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
				RmLog.debug(TAG, "value=" + value);
				if (value == 2) {// value==2强制更新
					android.os.Process.killProcess(android.os.Process.myPid());
				}
			}
		});
		final AlertDialog dialog = builder.create();
		dialog.setCanceledOnTouchOutside(false);
		if (!((Activity) mContext).isFinishing()) {
			if ((value == 1 && mUpdateInfo.show_type == 2) || mIsManualUpdate || value == 2) {// 推荐升级
				try {
					dialog.show();
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		}

		dialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
			@Override
			public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
				if (KeyEvent.KEYCODE_BACK == keyCode || KeyEvent.KEYCODE_SEARCH == keyCode) {
					return true;
				}
				return false;
			}
		});
	}

	private void downFile(final String url) {
		pBar = new ProgressDialog(mContext, R.style.ProgressDialogDark);
		pBar.setMessage(mContext.getString(R.string.update_downloading));
		pBar.setProgressStyle(ProgressDialog.STYLE_SPINNER);
		pBar.setCanceledOnTouchOutside(false);
		pBar.setOnKeyListener(new DialogInterface.OnKeyListener() {
			@Override
			public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
				if (KeyEvent.KEYCODE_BACK == keyCode || KeyEvent.KEYCODE_SEARCH == keyCode) {
					return true;
				}
				return false;
			}
		});
		pBar.show();
		new Thread() {
			public void run() {
				try {
					HttpClient client = new DefaultHttpClient();
					HttpGet get = new HttpGet(url);
					BasicHttpParams httpParameters = new BasicHttpParams();
					HttpConnectionParams.setConnectionTimeout(httpParameters, ConnectTimeOut);
					HttpConnectionParams.setSoTimeout(httpParameters, ConnectTimeOut);
					get.setParams(httpParameters);
					HttpResponse response = client.execute(get);
					HttpEntity entity = response.getEntity();
					fileSize = entity.getContentLength();
					startTimer();// 获取文件大小后才启动定时任务
					InputStream is = entity.getContent();
					FileOutputStream fileOutputStream = null;
					if (is != null) {
						mDownloadFile = getDownloadFile();
						RmLog.debug(TAG, "file=" + mDownloadFile.getAbsolutePath());
						fileOutputStream = new FileOutputStream(mDownloadFile);
						byte[] buf = new byte[1024];
						int ch = -1;
						while ((ch = is.read(buf)) != -1) {
							fileOutputStream.write(buf, 0, ch);
							readLen = readLen + ch;
						}
						fileOutputStream.flush();
						if (fileOutputStream != null) {
							fileOutputStream.close();
						}
						if (readLen > 0) {
							handler.sendEmptyMessage(APK_DOWNLOAD_DONE);
						} else {
							handler.sendEmptyMessage(DOWNLOAD_FILE_INFO_ERROR);
						}
					} else {
						handler.sendEmptyMessage(DOWNLOAD_FILE_INFO_ERROR);
					}
				} catch (Exception e) {
					e.printStackTrace();
					handler.sendEmptyMessage(DOWNLOAD_FILE_INFO_ERROR);
				}
			}
		}.start();
	}

	private File getDownloadFile() {
		File downloadDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
		if (!downloadDir.exists()) {
			downloadDir.mkdirs();
		}
		return new File(downloadDir, mContext.getPackageName() + ".apk");
	}

	/**
	 * apk缓存完成
	 */
	private void apkFetched() {
		dismissProgressBar();
		stopTimer();
		installAPK();
	}

	/**
	 * 安装apk
	 */
	private void installAPK() {
		Intent intent = new Intent(Intent.ACTION_VIEW);
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		intent.setDataAndType(Uri.fromFile(mDownloadFile), "application/vnd.android.package-archive");
		
		String appName = mContext.getApplicationInfo().loadLabel(mContext.getPackageManager()).toString();
		int notificationId = mContext.hashCode();
		
		PendingIntent pendingIntent = PendingIntent.getActivity(mContext, 0, intent, 0);  
		NotificationManager mNotificationManager = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
		Notification notification = new Notification.Builder(mContext)
        .setContentTitle(appName)
        .setContentText(mContext.getString(R.string.download_success_click_install))
        .setContentIntent(pendingIntent)
        .setSmallIcon(R.drawable.ic_launcher)
        .build();
		notification.flags |= Notification.FLAG_AUTO_CANCEL;
        mNotificationManager.notify(notificationId, notification);

		mContext.startActivity(intent);
		((Activity)mContext).finish();
	}
	
	

	/**
	 * 取消进度条
	 */
	private void dismissProgressBar() {
		if (pBar != null && pBar.isShowing()) {
			pBar.dismiss();
		}
	}

	// 缓存进度
	// ========================================================================
	private void startTimer() {
		if (null == mTimer) {
			mTimer = new Timer();
			mTimer.schedule(new PlayTimerTask(), 0, 1000);
		}
	}

	private void stopTimer() {
		if (null != mTimer) {
			mTimer.cancel();
			mTimer = null;
		}
	}

	private class PlayTimerTask extends TimerTask {
		@Override
		public void run() {
			Message msg = handler.obtainMessage(MSG_UPDATE_DOWNLOAD_PROGRESS);
			Bundle b = new Bundle();
			int progress = (int) (((float) readLen / fileSize) * 100);
			b.putInt("progress", progress);
			msg.setData(b);
			handler.sendMessage(msg);
		}
	}

	// 公开接口
	// ================================================================================
	/**
	 * 请求检查更新
	 * 
	 * @param clientid
	 *            :客户端id
	 * @param isCommonCheck
	 *            :是否首页检测，首页检测时候如果失败不弹toast或者dialog，首页false，设置界面true
	 */
	public void checkUpdate(final int clientid, boolean isCommonCheck) {
		this.mIsManualUpdate = isCommonCheck;
		if (mIsManualUpdate) {
			SdkUtil.showDialog(mDialog, mContext.getString(R.string.update_fetching_msg));
		}
		new Thread(new Runnable() {

			@Override
			public void run() {
				try {
					final String version = SdkUtil.getVerName(mContext);
					final String url = new StringBuilder(UPDATE_URL).append("version=").append(version).append("&clientid=").append(clientid).append("&lang=").append(getLanguage()).toString();
					RmLog.debug(TAG, "update url = " + url);

					HttpGet httpGet = new HttpGet(url);
					BasicHttpParams httpParameters = new BasicHttpParams();
					HttpConnectionParams.setConnectionTimeout(httpParameters, ConnectTimeOut);
					HttpConnectionParams.setSoTimeout(httpParameters, ConnectTimeOut);
					httpGet.setParams(httpParameters);

					HttpResponse response = new DefaultHttpClient().execute(httpGet);
					if (response.getStatusLine().getStatusCode() == 200) {
						HttpEntity entity = response.getEntity();
						String responseString = EntityUtils.toString(entity, HTTP.UTF_8);
						RmLog.debug(TAG, "responseString=" + responseString);

						mUpdateInfo = new UpdateInfo(responseString);
						if (mUpdateInfo.rtn != 0) {
							handler.sendEmptyMessage(IS_THE_LATEST_VER);// 已经是最新版本
							return;
						}
						handler.sendEmptyMessage(EVENT_GET_APP_UPDATE_CONFIG); // 更新内容
					} else {
						handler.sendEmptyMessage(NO_UPGRADE_FOUND);// 更新出错
					}
				} catch (Exception e) {
					e.printStackTrace();
					handler.sendEmptyMessage(NO_UPGRADE_FOUND); // 更新出错
					return;
				}
			}
		}).start();
	}

	private String getLanguage() {
		String lg = Locale.getDefault().getLanguage();
		String country = Locale.getDefault().getCountry();
		if (country.toLowerCase().equals("tw") && lg.toLowerCase().equals("zh")) {// 台湾的语言加上地区号
			lg = "zh_tw";
		}
		return lg;
	}

	private boolean isSameFile(File file) {
		if(file != null && file.exists()){
			PackageManager pm = mContext.getPackageManager();
			PackageInfo packageInfo = pm.getPackageArchiveInfo(file.getPath(), PackageManager.GET_ACTIVITIES);
			if (packageInfo!= null && packageInfo.versionName.equals(mUpdateInfo.new_version)) {
				return true;
			}
		}
		return false;

	}

	public static class UpdateInfo implements Serializable {

		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;

		public int rtn = -1;
		public String introduction;
		public String new_version;
		public int show_type;
		public int upgrade_type;
		public String upgrade_url;
		public long filesize;

		public UpdateInfo(String jsonString) {
			try {
				JSONObject jsonObject = new JSONObject(jsonString);
				rtn = jsonObject.getInt("rtn");
				if (rtn == 0) {
					new_version = jsonObject.getString("new_version");

					String tmpStr = jsonObject.getString("upgrade_type");
					if (null != tmpStr && !"null".equals(tmpStr)) {
						upgrade_type = Integer.valueOf(tmpStr).intValue();
					}

					tmpStr = jsonObject.getString("show_type");
					if (null != tmpStr && !"null".equals(tmpStr)) {
						show_type = Integer.valueOf(tmpStr).intValue();
					}

					upgrade_url = jsonObject.getString("upgrade_url");
					introduction = jsonObject.getString("introduction");
					filesize = jsonObject.getLong("filesize");
				}
			} catch (JSONException e) {
				e.printStackTrace();
			}
		}

		@Override
		public String toString() {
			return "UpdateInfo [rtn=" + rtn + ", introduction=" + introduction + ", new_version=" + new_version + ", show_type=" + show_type + ", upgrade_type=" + upgrade_type + ", upgrade_url="
					+ upgrade_url + ", filesize=" + filesize + "]";
		}

	}
}
