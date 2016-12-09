package com.runmit.sweedee.download;

import org.json.JSONException;
import org.json.JSONObject;


public class AppDownloadInfo extends DownloadInfo {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	public String mAppkey;
	
	public String mAppId;
	
	public int versioncode;// 当前下载的版本号

	public String pkgName;
	
	public int installState;	// 安装状态

	@Override
	public String getUserData() {
		JSONObject jObject = new JSONObject();
		try {
			jObject.put("mAppkey", mAppkey);
			jObject.put("mAppId", mAppId);
			jObject.put("versioncode", versioncode);
			jObject.put("pkgName", pkgName);
		} catch (JSONException e) {
			e.printStackTrace();
		}
		return jObject.toString();
	}

	@Override
	public void decodeUserData(String userdata) {
		JSONObject mObject;
		try {
			mObject = new JSONObject(userdata);
			this.mAppkey = mObject.getString("mAppkey");
			this.mAppId = mObject.getString("mAppId");
			this.versioncode = mObject.getInt("versioncode");
			this.pkgName = mObject.getString("pkgName");
		} catch (JSONException e) {
			e.printStackTrace();
		}
	}

	@Override
	public void setBeanKey(String key) {
		this.pkgName = key;
	}

	@Override
	public String getBeanKey() {
		return this.pkgName;
	}
}
