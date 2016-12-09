package com.runmit.sweedee.player;

import java.util.Timer;
import java.util.TimerTask;

import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.ViewGroup.LayoutParams;
import android.widget.Toast;

import com.runmit.mediaserver.MediaServer;
import com.runmit.mediaserver.VodTaskInfo;
import com.runmit.mediaserver.MediaServer.OnVodTaskErrorListener;
import com.runmit.mediaserver.MediaServer.OnVodTaskInfoListener;
import com.runmit.sweedee.R;
import com.runmit.sweedee.player.OnLinePlayMode.OnGetUrlListener;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.TDString;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.umeng.analytics.MobclickAgent;

public class PlayerActivity extends BasePlayActivity {

    private XL_Log log = new XL_Log(PlayerActivity.class);


    private static final int MOVIE_TIME_DISTANCE = 5;    //5s
    
    private int mVodTaskSpeed;
    
    private Timer mTimer; //刷新播放进度定时器
    private SurfaceHolder mSurfaceHolder;//播放器渲染界面
    private boolean mIsSurfaceViewCreated = false;


    public int mScreenId[] = {R.drawable.play_screen_suitable_gnr,
            R.drawable.play_screen_fill_gnr,
            R.drawable.play_screen_origin_gnr
    };


    private boolean mIsSwithch = false;//时候切换了清晰度

    public String mfilename;
    /**
     * mCallBackHandler作用：
     * 1，定时器刷新进度信息回调
     * 2，响应DownLoadEngine状态回调
     */

    private String mVodUrl;
    private String mCurPlayUrl;
    private MediaServer mMediaServer;
    private boolean isBufferBlock;//是否缓冲卡顿
    private long bufferStartTime;//卡顿开始时间

    private Handler mCallBackHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case PlayerConstant.PLAYER_UPDATE_PROGRESS:
                    mCaptionTextView.loadText(mPlayingTime);
                    mSeekBar.setProgress(mPlayingTime);
                    mPlayedTime.setText(Util.getFormatTime(mPlayingTime));
                    break;
                default:
                    break;
            }
        }
    };

    private void stopTimer() {
        log.debug("stopTimer begin (null != mTimer) : " + (null != mTimer));
        if (null != mTimer) {
            mTimer.cancel();
            mTimer = null;
        }
    }

    public synchronized void exit() {
    	log.debug("exit start");
        if (mCallBackHandler != null){
        	mCallBackHandler.removeCallbacksAndMessages(null);
        }
        saveVideoPlayInfo();    //保存播放记录
        if(isBufferBlock){
        	mReportPlayerData.playBlock(2, (int) (System.currentTimeMillis()-bufferStartTime),mVodTaskSpeed);
        }
        mReportPlayerData.setBeginPlay(setUrlTime-startActivityTime, startPlayTime-setUrlTime,mBeginTime > 0 ? 1 : 0 , 1);
        stopTimer();//关闭定时器
        if (mCurPlayMode != null && PlayerConstant.LOCAL_PLAY_MODE != mCurPlayMode.getPlayMode()) {
            if (mMediaServer != null){
            	mMediaServer.stopGetVodTaskInfo();
            }
        }
        finish();
       
    }


    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initSurfaceHolder();
    }

    private boolean isFirstLoad = true;
    @Override
    protected void onStart() {
    	super.onStart();
    	if (isFirstLoad) {
    		isFirstPlay = mXLSharePrefence.getBoolean(IS_FIRST_PLAY, true);
            if(isFirstPlay){
            	showFirstToast();
            }else{
            	if (mCurPlayMode != null && mCurPlayMode.isCloudPlay()) {
        			showWaittingDialog(TDString.getStr(R.string.play_loading_movie));
        	    }
            }
    		isFirstLoad = false;
    	}
    }

    protected void getOnLinePlayUrl(final boolean isLocal,String url) {
  		getPlayUrlAndStartPlayTask(isLocal,1200, url);
  	}

    private void getPlayUrlAndStartPlayTask(final boolean isLocal,final int errorCode, final String url) {
        mCallBackHandler.post(new Runnable() {

            @Override
            public void run() {
                log.debug("player getUrl and will get playurl and original url=" + url);
                mVodUrl=url;
                String peerId = UserManager.getInstance().getPeerId() + "b";
                if (mMediaServer == null) {
                    mMediaServer = MediaServer.getInstance(PlayerActivity.this, Constant.PRODUCT_ID, peerId);
                }
                if (!mMediaServer.isInitFinished()) {
                    mMediaServer.reInit(PlayerActivity.this, Constant.PRODUCT_ID, peerId);
                }
                mCurPlayUrl = mMediaServer.getVodPlayURL(isLocal,mVodUrl, null, 0);
                log.debug("mCurPlayUrl="+mCurPlayUrl+",mIsSurfaceViewCreated="+mIsSurfaceViewCreated);
                if (mIsSurfaceViewCreated) {
                    startPlayer();
                } else {
                    initSurfaceHolder();
                }
            }
        });

    }

    public void initSurfaceHolder() {
        log.debug("player initSurfaceHolder begin");
        mSurfaceHolder = mSurfaceView.getHolder();
        mSurfaceView.setOnSurfaceCallBacListener(new TDGLSurfaceView.OnSurfaceCallBackListener() {
            
        	public boolean isDestroyed = false;

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                mIsSurfaceViewCreated = false;
                log.debug("surfaceDestroyed holder=" + holder+",isfinish="+isFinishing());
                mSurfaceHolder = null;
                isDestroyed = true;
            }

            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                mIsSurfaceViewCreated = true;
                mSurfaceHolder = holder;
                log.debug("surfaceCreated holder = " + holder + ",isDestroyed=" + isDestroyed);
                if(isDestroyed){
                	resumePlayer();
                } else if (mCurPlayMode != null && mCurPlayMode.isLocalPlay()) {
                    LocalPlayMode mode = (LocalPlayMode) mCurPlayMode;
                    String mPlayUrl = mode.getFilePath() + mode.getFileName();
                    dismissDialog();
                    getOnLinePlayUrl(true,mPlayUrl);
                    return;
                }else if (mCurPlayMode != null && mCurPlayMode.isCloudPlay()) {
                	((OnLinePlayMode)mCurPlayMode).loadPlayUrlByDefault(new OnGetUrlListener() {
						
						@Override
						public void onGetUrlFinish(String mPlayUrl) {
							getOnLinePlayUrl(false,mPlayUrl);
						}
					});
                }
                isDestroyed = false;
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                log.debug("surfaceChanged");
            }
        });
        mSurfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }
    
    boolean isStop=false;

    @Override
    protected void onPause() {
        super.onPause();
        isStop = true;
        mScreenObserver.keepScreenLightOff();
        mReportPlayerData.onStop();
        pausePlayer(true);
        if(mMediaServer!=null){
        	mMediaServer.stopVodTaskDownload(mVodUrl);
        }
        shouldBeginTime = mBeginTime = mSeekBar.getProgress();
        log.debug("go to background time =shouldtime：" + shouldBeginTime + " mbegintime=" + mBeginTime);
    }

    @Override
    public void onResume() {
        log.debug("onResume");
        super.onResume();
        mScreenObserver.tryAndWaitWhileScreenLocked(this);
        mScreenObserver.keepScreenLightOn(this); //保持屏幕一直高亮
        mReportPlayerData.onResume();
        if(isStop){
        	if(mMediaServer!=null){
            	mMediaServer.startVodTaskDownload(mVodUrl);
            }
        	isStop=false;
        	resumePlayer();
        }
        if (!is_screen_locked){
        	mLayoutOperationHandler.sendEmptyMessage(PlayerConstant.VideoPlayerStatus.PLAYER_SHOW_CONTROL_BAR);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        dismissDialog();
        destroyPlayer();
        mPlayerAudioMgr.abandonAudioFocus(this);
        log.debug("End onDestroy ");
    }

    private void startPlayer() {
        log.debug("player startPlayer mUrl=" + mCurPlayUrl + "mSurfaceHolder=" + mSurfaceHolder);
        if (mMediaServer != null && mCurPlayMode.isCloudPlay()) {
            //如果为空则判断为本地播放
            mMediaServer.startGetVodTaskInfo(new OnVodTaskInfoListener() {
                @Override
                public void onGetVodTask(VodTaskInfo vodTaskInfo) {
                    if (mCurPlayMode != null && mCurPlayMode.isCloudPlay()) {
                    	mVodTaskSpeed=vodTaskInfo.getSpeed();
                    	mReportPlayerData.setPlaySpeed(mVodTaskSpeed);
                        mPlaySpeed.setText(convertNetSpeed(mVodTaskSpeed, 1));
                    }
                }
            }, mVodUrl);
        }
        
        mSurfaceView.setOnPlayerCallBackListener(new TDGLSurfaceView.OnPlayerCallBackListener() {
			
			@Override
			public void onVideoSizeChanged(android.media.MediaPlayer mp, int width, int height) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void onSeekComplete(android.media.MediaPlayer mp) {
                log.error("player onSeekComplete");
                setSeeking(false);
                if (mIsSwithch) {
                	dismissDialog();
                    mIsSwithch = false;
                }
                showLoadingState(false, 0);
                log.debug("mScreenObserver.isScreenStateOff()="+mScreenObserver.isScreenStateOff());
                if (mScreenObserver.isScreenStateOff()) {
                    beforeScreenOff = true;
                    pausePlayer(true);
                }
            }
			
			@Override
			public void onPrepared(android.media.MediaPlayer player) {            
				dismissDialog();        
                
                //clear 超时对话框
                if (mRunnableCloseDialog.dialog != null && mRunnableCloseDialog.dialog.isShowing()) {
                    mRunnableCloseDialog.dialog.dismiss();
                }
                mLayoutOperationHandler.removeCallbacks(mRunnableCloseDialog);
                continueAfterShowTip();
			}
			
			@Override
			public boolean onInfo(android.media.MediaPlayer mp, int what, int extra) {
                log.debug("player onInfo what=" + what + ",extra=" + extra);
                switch (what) {
                    case MediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START://开始播放
                    	setSeeking(false);
                        if (mIsSwithch) {
                        	dismissDialog();
                            mIsSwithch = false;//切换时设置初始值
                        }
                        if (mScreenObserver.isScreenStateOff()) {
                            beforeScreenOff = true;
                            pausePlayer(true);
                        }
                        
                        startPlayTime=System.currentTimeMillis();
                        mReportPlayerData.setBlockReason(1);
                        mReportPlayerData.setBeginPlay(setUrlTime-startActivityTime, startPlayTime-setUrlTime,mBeginTime > 0 ? 1 : 0 , 0);
                        mReportPlayerData.playStart();
                        if (mCurPlayMode.isCloudPlay()) {
                        	((OnLinePlayMode)mCurPlayMode).onVideoPlay();
                        }
                        break;
                    case MediaPlayer.MEDIA_INFO_BUFFERING_START://开始缓冲
                        //显示加载框
                        if (mCurPlayMode.isCloudPlay()) {
                            if (mIsSwithch) {
                            	dismissDialog();
                            }
                            showLoadingState(true, 0);
                            isBufferBlock=true;
                            bufferStartTime = System.currentTimeMillis();
                            
    						mLayoutOperationHandler.removeCallbacks(mRunnableCloseDialog);
    			            mLayoutOperationHandler.postDelayed(mRunnableCloseDialog, DIALOG_TIME_UP);
                        }
                        break;
                    case MediaPlayer.MEDIA_INFO_BUFFERING_END://缓冲结束
                        //消失加载框
                    	setSeeking(false);
                        if (mCurPlayMode.isCloudPlay()) {
                            mReportPlayerData.playBlock(1,(int) (System.currentTimeMillis()-bufferStartTime), mVodTaskSpeed);
                            isBufferBlock=false;
                            mLayoutOperationHandler.removeCallbacks(mRunnableCloseDialog);
                            if (mRunnableCloseDialog.dialog != null && mRunnableCloseDialog.dialog.isShowing()) {
                                mRunnableCloseDialog.dialog.dismiss();
                            }
                        }
                        showLoadingState(false, 0);
                        break;
                }

                return false;
            }
			
			@Override
			public boolean onError(android.media.MediaPlayer mp, final int what, final int extra) {
                log.error("onError:<" + what + ", " + extra + ">");
                if (mMediaServer != null && what != -38) {
                	mReportPlayerData.playError(what, extra);
                	if(mCurPlayMode.isCloudPlay() && ((OnLinePlayMode)mCurPlayMode).hasResourceForResolution()){
                		final OnLinePlayMode mOnLinePlayMode = (OnLinePlayMode) mCurPlayMode;
                		mOnLinePlayMode.loadNextIndexUrlByCurrentResolution(new OnGetUrlListener() {
							
							@Override
							public void onGetUrlFinish(String mPlayUrl) {
								switchVideoUrl(mPlayUrl);
								mLayoutOperationHandler.removeCallbacks(mRunnableCloseDialog);
					            mLayoutOperationHandler.postDelayed(mRunnableCloseDialog, DIALOG_TIME_UP);
							}
						});
                	}else{
                		mMediaServer.getVodTaskErrorInfo(mVodUrl, new OnVodTaskErrorListener() {

                            @Override
                            public void onGetVodTaskErrorCode(int errorCode) {
                                log.debug("player from mediaserver error and errorNO =" + errorCode);
                                onErrorInfo(what, extra, errorCode);
                            }
                        });
                	}
                } else {
                    onErrorInfo(what, extra, 0);
                }
                if (isFinishing()) {
                    return true;
                }
				return false;
            }
			
			@Override
			public void onCompletion(android.media.MediaPlayer mp) {
                log.debug("complition: all_time:" + mMovieDuration + " curtime=" + mBeginTime);
                if (!is_screen_locked){
                	mLayoutOperationHandler.sendEmptyMessage(PlayerConstant.VideoPlayerStatus.PLAYER_SHOW_CONTROL_BAR);
                }
                Util.showToast(PlayerActivity.this, TDString.getStr(R.string.play_complete), Toast.LENGTH_LONG);
                exit();
            }
			
			@Override
			public void onBufferingUpdate(android.media.MediaPlayer mp, int percent) {
				if(mCurPlayMode.isLocalPlay()){
					mPlaySpeed.setText(percent+"%");
				}
			}
		});

        try {
            if (!is_screen_locked){
            	mLayoutOperationHandler.sendEmptyMessage(PlayerConstant.VideoPlayerStatus.PLAYER_SHOW_CONTROL_BAR);
            }
            mLayoutOperationHandler.sendEmptyMessage(PlayerConstant.VideoPlayerStatus.PLAYER_SHOW_WAITING_BAR);
            mSurfaceView.prepareAsync(mCurPlayUrl);
            log.error("prepareAsync mCurPlayUrl="+mCurPlayUrl);
            setUrlTime=System.currentTimeMillis();
        } catch (Exception e) {
            e.printStackTrace();
            return;
        }
    }
    
    private void onErrorInfo(int what, int extra, int errorCode) {
        switch (what) {
        	case -38:
        		return;
            case 100:
                Util.showToast(PlayerActivity.this, TDString.getStr(R.string.playerr_service_died)+"(" + extra + "-" + errorCode + ")", Toast.LENGTH_SHORT);
                break;
            case -22://ffmpeg也解不出来高宽导致崩溃
                Util.showToast(PlayerActivity.this, TDString.getStr(R.string.playerr_source_error) + "-" + errorCode + ")", Toast.LENGTH_SHORT);
                break;
            default:
                if (!Util.isNetworkAvailable()) {
                    Util.showToast(PlayerActivity.this, TDString.getStr(R.string.no_net_connection), Toast.LENGTH_SHORT);
                } else {
                    Util.showToast(PlayerActivity.this, TDString.getStr(R.string.unknown_error)+"(" + what + "-" + errorCode + ")", Toast.LENGTH_SHORT);
                }
                break;
        }
        exit();
    }

    @Override
    protected void continueAfterShowTip() {
    	long start = System.currentTimeMillis();
        log.debug("continueAfterShowTip mBeginTime = " + mBeginTime + "shouldBeginTime = " + shouldBeginTime);
        mMovieDuration = mSurfaceView.getDuration() / 1000;

        if (mCurPlayMode != null) {
            mCurPlayMode.setDuration((int) mMovieDuration);
        }
        setVideoBeginTime();
        mSeekBar.setMax((int) mMovieDuration);
        mDurationTime.setText(Util.getFormatTime((int) mMovieDuration));
        mLayoutOperationHandler.sendEmptyMessage(PlayerConstant.VideoPlayerStatus.PLAYER_HIDE_WAITING_BAR);
        
        startTimer();
        if (shouldBeginTime >= 0) {//开始时间赋值为上次播放时间点
            mBeginTime = shouldBeginTime;
            shouldBeginTime = 0;
        }

        if (mBeginTime == 0) {
            mBeginTime = mCurPlayMode.getPlayedTime();
            log.debug("continueAfterShowTip getPlayedTime = " + mBeginTime);
        }

        log.debug("continueAfterShowTip mIsSwithch=" + mIsSwithch + ",mBeginTime=" + mBeginTime);
        if (mIsSwithch) {
            seekToBySecond(mBeginTime, false);
        } else {
            if (mBeginTime >= 0 && mBeginTime < mMovieDuration - MOVIE_TIME_DISTANCE) {
                if (!is_screen_locked){
                	mLayoutOperationHandler.sendEmptyMessage(PlayerConstant.VideoPlayerStatus.PLAYER_SHOW_CONTROL_BAR);
                }
                mLayoutOperationHandler.sendEmptyMessage(PlayerConstant.VideoPlayerStatus.PLAYER_SHOW_WAITING_BAR);
                seekToBySecond(mBeginTime, false);
                toastShowLastPlayTime(mBeginTime);
            }
        }
        
        if(!isStop && !isShowViewSign){//这个解决偶尔出现花屏现象（当清晰度转换时）
        	mCallBackHandler.postDelayed(new Runnable() {
        		@Override
        		public void run() {
        			resumePlayer();
                }
           }, 300);
        }
        hideNavigationBar();
        
        log.debug("continueAfterShowTip end ="+(System.currentTimeMillis()-start));
    }

    private void startTimer() {
        if (null == mTimer) {
            mTimer = new Timer();
            mTimer.schedule(new PlayTimerTask(), 0, 1000);
        }
    }

    private class PlayTimerTask extends TimerTask {
        @Override
        public void run() {
            if (mSurfaceView == null) {
                return;
            }

            try {
                if (mSurfaceView.isPlaying()) {
                    mHasPlayedTime++;

                    int time_minis = mSurfaceView.getCurrentPosition();
                    mPlayingTime = time_minis / 1000;

                    if (0 != mMovieDuration && !mSeekBar.isPressed()) {
                        mCallBackHandler.sendEmptyMessage(PlayerConstant.PLAYER_UPDATE_PROGRESS);
                    }
                }
            } catch (IllegalStateException e) {
                e.printStackTrace();
            }
        }

    }
    
    @Override
    public void resumePlayer() {
    	log.debug("resumePlayer isUserPauseVideo="+isUserPauseVideo);
        if (mSurfaceView != null && !mSurfaceView.isPlaying() && !isShowViewSign && !isUserPauseVideo) {
            log.debug("player resumePlayer");
            try {//确保万一，同时catch一下可能抛出的异常
            	mSurfaceView.start();
            } catch (Exception e) {
                e.printStackTrace();
            }
            disappearInSeconds(6000);
            mStartPausePlay.setBackgroundResource(R.drawable.play_pause_selector);//暂停状态图片
        }
    }

    @Override
    public void pausePlayer(boolean isdisable3d) {
    	log.debug("pausePlayer="+isdisable3d+",isPlaying="+mSurfaceView.isPlaying());
        mBeginTime = mPlayingTime;
        log.debug("player pause and mBeginTime=" + mBeginTime);
        mSurfaceView.pause(isdisable3d);
        disappearInSeconds(6000);
        mStartPausePlay.setBackgroundResource(R.drawable.play_start_selector);//播放状态图片
    }

    private void destroyPlayer() {
        if (mSurfaceView != null) {
        	mSurfaceView.stopPlayback();
        }
    }

    @Override
    public void seekToBySecond(int whichTimeToPlay, boolean isDrag) {
        if (mSurfaceView != null) {
            if (mCurPlayMode.isCloudPlay()) {
                //如果不是本地播放，seek的时候需要弹出等待框
                showLoadingState(true, 0);
            }
            mSurfaceView.seekTo(whichTimeToPlay * 1000);
        }
    }

    @Override
    protected void switchVideoResolution(int type, String tipStr) {
    	if(mIsSwithch){
    		return;
    	}
        mLayoutOperationHandler.sendEmptyMessage(PlayerConstant.VideoPlayerStatus.PLAYER_HIDE_CONTROL_BAR);
        showWaittingDialog(tipStr);
        shouldBeginTime = mBeginTime = mPlayingTime;
        if (mSurfaceView != null) {
        	mSurfaceView.stop();
        }
        stopTimer();
        mCurPlayMode.setUrl_type(type);
        resolutionView.setCurPlayMode(type);
        mReportPlayerData.setResolution(mCurPlayMode.getUrl_type());
        mReportPlayerData.setBlockReason(4);
        log.debug("player switchVideoResolution and urlType=" + mCurPlayMode.getUrl_type());
        if(mCurPlayMode.isCloudPlay()){
        	((OnLinePlayMode)mCurPlayMode).loadUrlByType(type, 0, new OnGetUrlListener() {
				
				@Override
				public void onGetUrlFinish(String mPlayUrl) {
					 getOnLinePlayUrl(false,mPlayUrl);
				}
			});
        }
        mIsSwithch = true;
    }
    
    protected void switchVideoUrl(String mPlayUrl){
    	Log.d("switchVideoUrl", "mPlayUrl="+mPlayUrl);
    	mLayoutOperationHandler.sendEmptyMessage(PlayerConstant.VideoPlayerStatus.PLAYER_HIDE_CONTROL_BAR);
        showWaittingDialog(getString(R.string.player_switching_video_resouce));
        shouldBeginTime = mBeginTime = mPlayingTime;
        if (mSurfaceView != null) {
        	mSurfaceView.stop();
        }
        stopTimer();
        getOnLinePlayUrl(false,mPlayUrl);
    }
    
    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
    	super.onRestoreInstanceState(savedInstanceState);
    	shouldBeginTime=savedInstanceState.getInt("mPlayingTime");
    }
    
    @Override
    protected void onSaveInstanceState(Bundle outState) {
    	super.onSaveInstanceState(outState);
    	outState.putSerializable("mPlayingTime", mPlayingTime);
    }
}

