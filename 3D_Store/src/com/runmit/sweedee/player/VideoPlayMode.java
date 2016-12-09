package com.runmit.sweedee.player;

import java.util.ArrayList;

import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;
import com.runmit.sweedee.util.Constant;

import android.content.Context;

public abstract class VideoPlayMode {

    protected int mCurrentPlayUrlType = Constant.NORMAL_VOD_URL;

    //本地和云点播公用字段
    protected String mUrlReady;
    
    protected String mFileName;
    
    protected int mPlayMode;
   
    protected int mPlayedTime;
    
    protected int mDuration;
    
    protected Context mContext;
    
    protected long fileSize;
    
    protected int videoid;
    
    protected int mode;//视频上下左右格式
    
    /**
     * 获取上下左右格式
     * @return
     */
    public int getMode() {
		return mode;
	}

	public int getVideoid() {
		return videoid;
	}

	protected ArrayList<SubTitleInfo> mSubTitleInfos;//字幕信息

	/**
	 * 
	 * @param context
	 * @param mode:视频上下左右格式
	 * @param playMode:播放模式
	 */
	public VideoPlayMode(Context context,int mode, int playMode) {
        this.mPlayMode = playMode;
        this.mContext = context;
        this.mode = mode;
        this.mPlayedTime = 0;
        this.mDuration = 0;
    }

	/**在线点播  正片和片花的点播*/
    public boolean isCloudPlay() {
        return mPlayMode == PlayerConstant.ONLINE_PLAY_MODE || mPlayMode == PlayerConstant.TRAILER_PLAY_MODE;
    }

    /**本地点播*/
    public boolean isLocalPlay() {
        return mPlayMode == PlayerConstant.LOCAL_PLAY_MODE;
    }    
    
    public int getPlayMode() {
        return mPlayMode;
    }

    public String getFileName() {
        return mFileName;
    }
    
    public int getPlayedTime() {
        return this.mPlayedTime;
    }

    public void setPlayedTime(int time) {
        this.mPlayedTime = time;
    }

    public int getDuration() {
        return this.mDuration;
    }

    public void setDuration(int duration) {
        this.mDuration = duration;
    }

    public String getPreparedUrl() {
        return this.mUrlReady;
    }

    public long getFileSize() {
        return this.fileSize;
    }

    public int getUrl_type() {
        return mCurrentPlayUrlType;
    }

    public void setUrl_type(int url_type) {
        this.mCurrentPlayUrlType = url_type;
    }
    
    public ArrayList<SubTitleInfo> getSubTitleInfos() {
		return mSubTitleInfos;
	}

    abstract public void saveVideoInfo(long mMovieDuration);
    
    abstract public boolean hasPlayUrlByType(int playtype);
}
