package com.runmit.sweedee.player;

import java.util.ArrayList;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.runmit.sweedee.R;
import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;

public class PlayModeFactory {
    public static VideoPlayMode createPlayModeObj(Context context, Intent intent) {
        VideoPlayMode playModeObj = null;
        //以下两个属性为各个播放模式所共有，提取出来
        int playMode = intent.getIntExtra(PlayerConstant.INTENT_PLAY_MODE, PlayerConstant.ONLINE_PLAY_MODE);
        String fileName = intent.getStringExtra(PlayerConstant.INTENT_PLAY_FILE_NAME);
        int mode=intent.getIntExtra(PlayerConstant.INTENT_VIDEO_SOURCE_MODE, 0);
        Log.d("createPlayModeObj", "fileName="+fileName+",mode="+mode);
        ArrayList<SubTitleInfo> mSubTitleInfos=(ArrayList<SubTitleInfo>) intent.getSerializableExtra(PlayerConstant.INTENT_PLAY_SUBTITLE);
        switch (playMode) {
            case PlayerConstant.LOCAL_PLAY_MODE://本地缓存完成播放
                String filePath = intent.getStringExtra(PlayerConstant.LOCAL_PLAY_PATH);
                playModeObj = new LocalPlayMode(context, mode, fileName, filePath,mSubTitleInfos);
                break;
            case PlayerConstant.ONLINE_PLAY_MODE://在线点播
                RealPlayUrlInfo mRealPlayUrlInfo=intent.getParcelableExtra(PlayerConstant.PLAY_URL_LISTINFO);
                String picurl=intent.getStringExtra(PlayerConstant.INTENT_PLAY_POSTER);
                int albumid=intent.getIntExtra(PlayerConstant.INTENT_PLAY_ALBUMID, 0);
                int videoid=intent.getIntExtra(PlayerConstant.INTENT_PLAY_VIDEOID, 0);
                if(!fileName.equals(context.getResources().getString(R.string.detail_movie_prevue))){
                	 playModeObj = new OnLinePlayMode(context, mode, fileName, mRealPlayUrlInfo,mSubTitleInfos,picurl,albumid,videoid);
                }else{
                	playModeObj = new TralerPlayMode(context, mode , fileName, mRealPlayUrlInfo,mSubTitleInfos,picurl,albumid,videoid);
                }
                break;
        }
        return playModeObj;
    }
}
