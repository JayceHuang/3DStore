/**
 * VideoCaptionManager.java 
 * com.xunlei.share.manager.VideoCaptionManager
 * @author: Administrator
 * @date: 2013-5-13 下午3:12:40
 */
package com.runmit.sweedee.player.uicontrol;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Locale;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.action.more.SettingItemActivity;
import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;
import com.runmit.sweedee.player.subtitle.FileEncodType;
import com.runmit.sweedee.player.subtitle.FormatASS;
import com.runmit.sweedee.player.subtitle.FormatSCC;
import com.runmit.sweedee.player.subtitle.FormatSRT;
import com.runmit.sweedee.player.subtitle.FormatSTL;
import com.runmit.sweedee.player.subtitle.FormatTTML;
import com.runmit.sweedee.player.subtitle.SubTilteDatabaseHelper;
import com.runmit.sweedee.player.subtitle.TimedTextFileFormat;
import com.runmit.sweedee.util.EnvironmentTool;
import com.runmit.sweedee.util.ReadUnknowCodeTxt;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

/**
 * @author 字幕加载类
 */
public class VideoCaptionManager {

	private Context mContext;

	private XL_Log log = new XL_Log(VideoCaptionManager.class);

	private OnCaptionSelect mOnCaptionSelectListener;

	private SubTitleInfo currentSelectSubTitleInfo;

	private static final int DOWN_SUBTITLE_FAIL = 1234567890;
	private static final int DOWN_SUBTITLE_SUCC = 1212;

	private ArrayList<SubTitleInfo> mSubTitleInfos;

	public interface OnCaptionSelect {
		
		public void onLoadSuccess(final String localPath);
		
		public void close();
	}

	@SuppressLint("HandlerLeak") Handler mHandler = new Handler() {

		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case DOWN_SUBTITLE_FAIL:
				Util.showToast(mContext, mContext.getString(R.string.load_caption_fail), Toast.LENGTH_SHORT);
				if (mOnCaptionSelectListener != null) {
					mOnCaptionSelectListener.close();
				}
				break;
			case DOWN_SUBTITLE_SUCC://加载字幕成功
				String localPath=(String) msg.obj;
				log.debug("DOWN_SUBTITLE_SUCc="+localPath);
				if(mOnCaptionSelectListener!=null){
					mOnCaptionSelectListener.onLoadSuccess(localPath);
				}
				break;
			default:
				break;
			}
		}

	};

	public VideoCaptionManager(Context context) {
		mContext = context;
	}

	public void loadVideoCaption(final ArrayList<SubTitleInfo> mTitleInfos, OnCaptionSelect mOnCaptionSelect) {
		this.mOnCaptionSelectListener = mOnCaptionSelect;
		this.mSubTitleInfos = mTitleInfos;
		
		currentSelectSubTitleInfo = getUserSelectSubtitle(mTitleInfos);
		if(currentSelectSubTitleInfo!=null){
			new Thread(new Runnable() {
				@Override
				public void run() {
					loadVideoCaption(currentSelectSubTitleInfo);
				}
			}).start();
		}
	}
	
	private SubTitleInfo getUserSelectSubtitle(final ArrayList<SubTitleInfo> mTitleInfos){
		 String downloadLanguage = SharePrefence.getInstance(mContext).getString(SettingItemActivity.DOWNLOAD_LANGUAGE, Locale.getDefault().getLanguage());
		 if(mTitleInfos!=null){
			 for(int i=0,size=mTitleInfos.size();i<size;i++){
				 SubTitleInfo mSubTitleInfo=mTitleInfos.get(i);
				 if(downloadLanguage.equalsIgnoreCase(mSubTitleInfo.language)){
					 return mSubTitleInfo;
				 }
			 }
			 return mSubTitleInfos.get(0);
		}
		 return null;
	}

	private synchronized void loadVideoCaption(SubTitleInfo mSubTitleInfo) {
		final String cachePath = EnvironmentTool.getDataPath("caption");
		File f = new File(cachePath);
		if (!f.exists()) {
			f.mkdirs();
		}

		// 用文件名的hash值保存本地，防止错误
		int index = mSubTitleInfo.title.lastIndexOf(".");
		log.debug("loadVideoCaption sname:" + mSubTitleInfo.title);
		if (index < 0 || index > mSubTitleInfo.title.length()) {//此处是为了防止无法判断文件名后缀
			mHandler.obtainMessage(DOWN_SUBTITLE_FAIL).sendToTarget();
			return;
		}

		String subfile = mSubTitleInfo.title.substring(index);
		String defaultDownloadPath = cachePath + Util.hashKeyForDisk(mSubTitleInfo.title) + subfile;
		log.debug("defaultDownloadPath=" + defaultDownloadPath);

		SubTilteDatabaseHelper mSubTilteDatabaseHelper = SubTilteDatabaseHelper.getInstance(mContext, defaultDownloadPath + ".db");
		log.debug("mSubTilteDatabaseHelper.getVersion()=" + mSubTilteDatabaseHelper.getVersion());

		if (mSubTilteDatabaseHelper.getVersion() != SubTilteDatabaseHelper.VERSION_LOADED) {// 版本号不匹配，则需要倒数据到数据库
			File tf = new File(defaultDownloadPath);
			if (!tf.exists()) {// 不存在则缓存文件，保存到本地
				if (!loadVideoCaptionFile(defaultDownloadPath, currentSelectSubTitleInfo.url)) {
					log.debug("fail-one");
					mHandler.obtainMessage(DOWN_SUBTITLE_FAIL).sendToTarget();// 缓存失败，则提示用户加载字幕失败
					return;
				}
			}
			// 导入文件到数据库
			ReadUnknowCodeTxt readUnknowCodeTxt = new ReadUnknowCodeTxt();
			String charsetName = "utf-8";
			if (readUnknowCodeTxt.isUTF8(readUnknowCodeTxt.readTxtFile(defaultDownloadPath))) {
				
			} else {
				File file = new File(defaultDownloadPath);
				charsetName = FileEncodType.getFilecharset(file);
			}
			log.debug("charsetName="+charsetName);
			TimedTextFileFormat mTimedTextFileFormat;
			if (defaultDownloadPath.endsWith("srt") || defaultDownloadPath.endsWith("chs")) {
				mTimedTextFileFormat = new FormatSRT();
			} else if (defaultDownloadPath.endsWith("ssa") || defaultDownloadPath.endsWith("ass")) {
				mTimedTextFileFormat = new FormatASS();
			} else if (defaultDownloadPath.endsWith("xml")) {
				mTimedTextFileFormat = new FormatTTML();
			} else if (defaultDownloadPath.endsWith("scc")) {
				mTimedTextFileFormat = new FormatSCC();
			} else if (defaultDownloadPath.endsWith("stl")) {
				mTimedTextFileFormat = new FormatSTL();
			} else {
				mHandler.obtainMessage(DOWN_SUBTITLE_FAIL).sendToTarget();
				return;
			}
			boolean isFinish = mTimedTextFileFormat.loadFileToDB(defaultDownloadPath, charsetName, mContext);
			log.debug("isFinish="+isFinish);
			if (!isFinish) {
				log.debug("fail-two");
				mHandler.obtainMessage(DOWN_SUBTITLE_FAIL).sendToTarget();
				return;
			} else {
				mHandler.obtainMessage(DOWN_SUBTITLE_SUCC,defaultDownloadPath).sendToTarget();
			}
		} else {
			log.debug("database had videoCaption");
			mHandler.obtainMessage(DOWN_SUBTITLE_SUCC,defaultDownloadPath).sendToTarget();
		}

	}

	private boolean loadVideoCaptionFile(String filepath, String url) {
		InputStream is = null;
		HttpClient client = null;
		try {
			HttpGet httpGet = new HttpGet(url);
			client = new DefaultHttpClient();
			HttpResponse resp = client.execute(httpGet);
			if (HttpStatus.SC_OK == resp.getStatusLine().getStatusCode()) {
				HttpEntity entity = resp.getEntity();
				is = entity.getContent();
				FileOutputStream fos = new FileOutputStream(new File(filepath));
				byte[] buff = new byte[1024]; // buff用于存放循环读取的临时数据
				int len = 0;
				try {
					while ((len = is.read(buff, 0, 1024)) > 0) {
						fos.write(buff, 0, len);
					}
				} catch (IOException e) {
					e.printStackTrace();
				}
				is.close();
				fos.flush();
				fos.close();
				log.debug("success");
				return true;
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
		log.debug("fail");
		return false;
	}
}
