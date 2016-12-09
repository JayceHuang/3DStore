package com.runmit.sweedee.player;

import java.util.ArrayList;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

public class LocalPlayMode extends VideoPlayMode {

    private XL_Log log = new XL_Log(LocalPlayMode.class);

    private String filePath;
    
    private SharedPreferences mSharedPreferences;
    
    private static final String LOCAL_PLAYINFO="local_play";

    public LocalPlayMode(Context context,int mode, String fileName, String path,ArrayList<SubTitleInfo> mSubTitleList) {
        super(context, mode, PlayerConstant.LOCAL_PLAY_MODE);
        this.mUrlReady = path + fileName;
        this.filePath = path;
        this.mSubTitleInfos=mSubTitleList;
        mSharedPreferences = context.getSharedPreferences(LOCAL_PLAYINFO, Context.MODE_PRIVATE);
        load(fileName);
        log.debug("LocalPlayMode filePath=" + filePath + ",fileName=" + this.mFileName);
    }

    @Override
    public void saveVideoInfo(long mMovieDuration) {
    	Editor mEditor = mSharedPreferences.edit();
    	 String spKey=Util.hashKeyForDisk(mUrlReady);
    	 mEditor.putInt(spKey, mPlayedTime);
         mEditor.commit();
    }

    private void load(String fileName) {
        this.mFileName = fileName;
        String spKey=Util.hashKeyForDisk(mUrlReady);
        this.mPlayedTime = mSharedPreferences.getInt(spKey, 0);
    }

    public String getFilePath() {
        return filePath;
    }

	@Override
	public boolean hasPlayUrlByType(int playtype) {
		return false;
	}
}
