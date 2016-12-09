package com.runmit.sweedee.player;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import com.runmit.sweedee.report.sdk.ReportActionSender;

public class ReportPlayerData {

    private ReportActionSender mReportActionSender = null;

    private String upid;
    
    private int online;
    
    private String mVideoName;//视频名称
    
    private int mVideoId;//
    
    private int videoResolution;//视频码率
    
    private int blockReason;

	public ReportPlayerData(String name,int id,int online) {
    	this.mVideoName=name;
    	this.mVideoId=id;
    	this.online=online;
    	
    	mReportActionSender = ReportActionSender.getInstance();
    	upid=UUID.randomUUID().toString();
    }
    
    public void setResolution(int type){
    	this.videoResolution=type;
    }

    /**
     * 启动播放器的缓冲上报
     * @param uta
     * @param utb
     */
    private boolean isBeginPlay = false;
    public void setBeginPlay(long uta,long utb,int continueplay,int isusercancel) {
    	if(isBeginPlay){
    		return;
    	}
    	isBeginPlay = true;
    	mReportActionSender.playInit(upid, online, mVideoId, mVideoName, videoResolution, uta, utb,continueplay,isusercancel);
    }
    
    /**
     * 开始播放上报
     */
    public void playStart(){
    	mReportActionSender.playStart(upid, online, mVideoId, mVideoName, videoResolution);
    }
    
    /**
     * reason卡顿原因：0-未知；1-正常播放；2-起播；3-拖拽；4-切换清晰度；5-切换片源
     * @param blockReason
     */
    public void setBlockReason(int blockReason) {
		this.blockReason = blockReason;
	}
    
    /**
     * 播放卡顿
     * @param flag卡顿标记：是否卡顿未结束就外部退出，1 - 卡顿正常结束，2 - 本次卡顿由于用户操作提前结束
     * @param speed当前的缓存速度，单位bytes/s
     */
    public void playBlock(int flag,int dur,int speed){
    	mReportActionSender.playBlock(upid, online, mVideoId, mVideoName, videoResolution, dur, blockReason, flag, speed);
    	setBlockReason(1);//重置
    }
    
    /**
     * drt0-开始方向拖拽；1-结束方向拖拽
     * @param direct
     */
    public void playSeek(int direct){
    	mReportActionSender.playSeek(upid, online, mVideoId, mVideoName, videoResolution, direct);
    }
    
    public void setPlaySpeed(int speed){
    	if(isRunning){
    		if(mSpeedList.size()<mReportActionSender.getPlayTimeDuration()){//60s上报一次
        		mSpeedList.add(speed);
        	}else{
        		playTime();
        	}
    	}
    }
    
    /**
     * 
     * @param type 1-起播 2-播放中
     */
    public void changeCdn(){
    	mReportActionSender.changeVideoCDN(upid, isBeginPlay ? 2 : 1, videoResolution, mVideoId, mVideoName);
    }
    
    /**
     * 
     */
    public void playError(int errwhat,int errc){
    	mReportActionSender.playErrors(upid, online, videoResolution, mVideoId, mVideoName, blockReason, errwhat, errc);
    }
    
    public void changeSolutions(int beforecrt,int aftercrt){
    	mReportActionSender.changeSolutions(upid, beforecrt, aftercrt, mVideoId, mVideoName);
    }
    
    
    /**
     * 播放速度上报
     */
    public void playTime(){
    	int size = mSpeedList.size();
    	if(size>0){
    		int totalSpeed = 0;
    		for(int i=0;i<size;i++){
    			totalSpeed+=mSpeedList.get(i);
    		}
    		mReportActionSender.playTime(upid, online, mVideoId, mVideoName, videoResolution, size, totalSpeed/size);
    		mSpeedList.clear();
    	}
    }
    
    
    
    public void onStop(){
    	isRunning=false;
    	playTime();
    }
    
    public void onResume(){
    	isRunning=true;
    }
    
    private boolean isRunning=false;
    
    private List<Integer> mSpeedList=new ArrayList<Integer>();
}
