/**
 * 
 */
package com.runmit.sweedee.player;

import java.util.ArrayList;

import android.content.Context;

import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;

/**
 * @author Sven.Zhan
 * 试播播放模式
 */
public class TralerPlayMode extends OnLinePlayMode {
	
	public TralerPlayMode(Context context,int mode, String fileName, RealPlayUrlInfo realPlayUrlInfo, ArrayList<SubTitleInfo> mSubTitleList, String picUrl, int albumid, int movieid) {
		super(context, mode , fileName, realPlayUrlInfo, mSubTitleList, picUrl, albumid, movieid);
		this.mPlayMode = PlayerConstant.TRAILER_PLAY_MODE;
	}
	
	@Override
	public void saveVideoInfo(long mMovieDuration) {
	}

}
