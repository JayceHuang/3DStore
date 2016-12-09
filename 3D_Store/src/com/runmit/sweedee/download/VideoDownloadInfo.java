package com.runmit.sweedee.download;

import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;
import com.google.gson.reflect.TypeToken;
import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class VideoDownloadInfo extends DownloadInfo {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

    /**
     * cms返回给客户端的albumnId
     */
    public int albumnId;
    
    public int videoId;//视频id
    
    public int mode;//视频上下左右格式

	public String poster;
    
    public ArrayList<SubTitleInfo> mSubTitleInfos;


	@Override
	public String getUserData() {
		JSONObject jObject = new JSONObject();
		try {
			jObject.put("albumnId", albumnId);
			jObject.put("videoId", videoId);
			jObject.put("mode", mode);
			jObject.put("poster", poster);
			String subtitleJson = new Gson().toJson(mSubTitleInfos);
			jObject.put("subtitleJson", subtitleJson);
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
			this.albumnId = mObject.getInt("albumnId");
			this.videoId = mObject.getInt("videoId");
			this.mode = mObject.getInt("mode");
			this.poster = mObject.getString("poster");
			String subtitleJson = mObject.getString("subtitleJson");
			
			if(subtitleJson!=null && subtitleJson.length()>0){
	        	try {
	        		this.mSubTitleInfos = new Gson().fromJson(subtitleJson,new TypeToken<ArrayList<SubTitleInfo>>(){}.getType());  
				} catch (JsonSyntaxException e) {
					e.printStackTrace();
				}
	        }
		} catch (JSONException e) {
			e.printStackTrace();
		}
	}

	@Override
	public void setBeanKey(String key) {
		this.albumnId = Integer.parseInt(key);
	}

	@Override
	public String getBeanKey() {
		return Integer.toString(this.albumnId);
	}
}
