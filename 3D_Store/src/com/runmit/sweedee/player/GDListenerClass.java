package com.runmit.sweedee.player;

import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.XL_Log;

import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;

/**
 * @author zhanjin
 *         播放时监听和处理手势操作，同时也监听音量键事件。
 *         用手势来调节音量，屏幕亮度和播放进度
 *         <p/>
 *         修改记录：修改者，修改日期，修改内容
 */
public class GDListenerClass implements GestureDetector.OnGestureListener {

    XL_Log log = new XL_Log(GDListenerClass.class);

    private Activity mActivity;//相关的播放Activity
    private AudioManager mAudioManager;
    private WindowManager.LayoutParams mWL;//改变亮度要的属性
    
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == HIDE_VOL_LAYOUT) {
                mConsumeGesture.hideLayout();
            }
        }
    };;//用来处理一些自动关闭操作
    
    private final int HIDE_VOL_LAYOUT = 0;
    private float initialBrightness;//初始亮度
    private int initialVolume;//初始音量

    private int screenWidth;//当前屏幕宽度
    private int screenHeight;//当前屏幕高度
    private int halfScreenWidth; //当前屏幕宽度的一半 
    private int briUnit;//亮度垂直滑动的单位距离
    private int volUnit;//音量垂直滑动的单位距离
    private float proUnit;//播放进度水平滑动的单位距离

    private final int BRIGHTNESS_LEVEL_NUM = 10;//亮度可调级数,默认10级
    private int VOLUME_LEVEL_NUM = 10;//音量可调级数,默认15级
    private final int PROGRESS_MAX_ADJUST = 100;//一次滑动可调的最大时间幅度，单位：秒

    private final float SCROLL_SENSITIVITY = 1f;//调节的灵敏度(0.1-1)值越大，灵敏度越低    
    private final int SYSTEM_MAX_BRIGHTNESS = 255;//系统最高亮度，为255
    private final int SYSTEM_MAX_VOLUME;//系统最高音量

    private float briPerUnit;//每级亮度的数值
    private float volPerUnit = 1f;//每级声音的数值

    private float disX;//X方向上的滑动距离,float类型，(正向计算)
    private int disY;//Y方向上的滑动距离(反向计算)

    //等级变化变量，一定要有
    private int nowCL = 0;//
    private int recordedCL = 0;
    private int nowSL = -1;
    private int recordedSL = 0;

    //处理类型的标志    
    private final int TYPE_UNKNOWN = -1;
    private final int TYPE_PROGRESS = 1;
    private final int TYPE_VOL_OR_BRI = 2;
    public int handType = TYPE_UNKNOWN;

    private int startX;//进入滑动时的X坐标
    private int startY;//进入滑动时的Y坐标

    private float disBri = 0;//亮度变化的幅度

    //private boolean isAdjustingVol = false;//是否正在操作音量，防止滑动和按键同时操作音量时冲突

    private ConsumeGesture mConsumeGesture;//由播放Activity传递的手势处理接口

    public static float FLAG_USER_NOT_SETTING_BRIGHT = 0.0f;

    private boolean checkKitKatTouch = false;

    private SharePrefence mXlSharePrefence;

    public GDListenerClass(Activity activity, ConsumeGesture consumeGesture, float initBright) {
        super();
        this.mActivity = activity;
        
        checkKitKatTouch = Util.hasVirtualKey(activity) && Util.hasKitKat();
        mWL = mActivity.getWindow().getAttributes();
        mAudioManager = (AudioManager) mActivity.getSystemService(Context.AUDIO_SERVICE);
        SYSTEM_MAX_VOLUME = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        //VOLUME_LEVEL_NUM = SYSTEM_MAX_VOLUME;//可能不同机型的音量等级不一样
        mConsumeGesture = consumeGesture;
        screenWidth = Util.getScreenWidth(mActivity);
        screenHeight = Util.getScreenHeight(mActivity);
        halfScreenWidth = screenWidth / 2;
        briUnit = (int) ((screenHeight / BRIGHTNESS_LEVEL_NUM) * SCROLL_SENSITIVITY);
        briPerUnit = (float) (SYSTEM_MAX_BRIGHTNESS) / (float) (BRIGHTNESS_LEVEL_NUM);
        volUnit = (int) ((screenHeight / VOLUME_LEVEL_NUM) * SCROLL_SENSITIVITY);
        proUnit = (((float) screenWidth / PROGRESS_MAX_ADJUST) * SCROLL_SENSITIVITY);
        log.error("proUnit = " + proUnit);

        mXlSharePrefence = SharePrefence.getInstance(mActivity);
        //保存用户之前的播放记录
        initialBrightness = (float) Settings.System.getInt(mActivity.getContentResolver(), Settings.System.SCREEN_BRIGHTNESS, 255);
        initialVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
    }

    public LayoutParams getInitBright(float lastBright) {
        initialBrightness = lastBright;
        int bright = (int) (initialBrightness * BRIGHTNESS_LEVEL_NUM / SYSTEM_MAX_BRIGHTNESS);
        if (bright < 1) bright = 1;//此处是为了避免screenbrightness=0，从而导致屏幕自动休眠锁屏
        mWL.screenBrightness = bright / (float) (BRIGHTNESS_LEVEL_NUM);
        Settings.System.putInt(mActivity.getContentResolver(), Settings.System.SCREEN_BRIGHTNESS, (int) initialBrightness);
        mWL.flags |= WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;
        return mWL;
    }

    /**
     * 设置屏幕亮度，优先使用用户上次记忆的亮度值
     * 如果用户没有设置过亮度值，则使用推荐亮度值
     * 推荐亮度值是系统当前的亮度
     *
     * @param suggestLight:推荐亮度值
     */
    public void setScreenLight(float suggestLight) {
//    	if(mWL.screenBrightness >= 0){//大于0 表示用户自己设置过
//            log.debug("mWL.screenBrightness=" + mWL.screenBrightness);
//            mWL.flags |= WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;
//            mActivity.getWindow().setAttributes(mWL);
//            return;
//    	}
//        float lastBright = mXlSharePrefence.getFloat(SharePrefence.KEY_SCREEN_BRIGHT_RECORD, GDListenerClass.FLAG_USER_NOT_SETTING_BRIGHT);
//        log.debug("setScreenLight suggestLight=" + suggestLight + ",lastBright=" + lastBright);
//        if (lastBright == FLAG_USER_NOT_SETTING_BRIGHT) {//此时说明用户从没有使用过自定义亮度值，则使用推荐亮度值
//            lastBright = suggestLight;
//        }
    	float lastBright = SYSTEM_MAX_BRIGHTNESS * 0.7f;
        log.debug("setScreenLight lastBright=" + lastBright);
        if (lastBright > 0) {
            initialBrightness = lastBright;
            int bright = (int) (initialBrightness * BRIGHTNESS_LEVEL_NUM / SYSTEM_MAX_BRIGHTNESS);
            log.debug("setScreenLight bright=" + bright);
            if (bright < 1) {
                bright = 1;//此处是为了避免screenbrightness=0，从而导致屏幕自动休眠锁屏
            }
            log.debug("BRIGHTNESS_LEVEL_NUM bright=" + (bright / (float) (BRIGHTNESS_LEVEL_NUM)));
            mWL.screenBrightness = bright / (float) (BRIGHTNESS_LEVEL_NUM);
            log.debug("mWL.screenBrightness=" + mWL.screenBrightness);
            Settings.System.putInt(mActivity.getContentResolver(),Settings.System.SCREEN_BRIGHTNESS, (int) initialBrightness);

            mWL.flags |= WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;
            mActivity.getWindow().setAttributes(mWL);
        }

    }

    @Override
    public boolean onSingleTapUp(MotionEvent e) {
        //log.error("onSingleTapUp");
        return false;
    }

    @Override
    public void onShowPress(MotionEvent e) {
        //log.error("onShowPress");
    }

    private boolean is_adjust_pro = false;

    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        //log.error("onScroll");
        //log.error("distanceX = "+distanceX+" > distanceY ="+distanceY+" slope = "+ (distanceY/distanceX));
        //Log.e("eeee","e1x = "+e1.getX()+" e1y = "+e1.getY() +" e2x = "+e2.getX()+ "e2y = "+e2.getY()+" distanceX = "+distanceX + " distanceY = "+distanceY);
        //先计算斜率，决定调那种,后面的流程全由初始斜率决定
        if (Math.abs(distanceY / distanceX) < 1) {
            if (handType == TYPE_UNKNOWN) handType = TYPE_PROGRESS;
        } else {
            if (handType == TYPE_UNKNOWN) handType = TYPE_VOL_OR_BRI;
        }
        if (e1 == null || e2 == null) {
            return false;
        }
        //开始按类型执行
        if (handType == TYPE_PROGRESS) {//调节进度
            if (checkKitKatTouch && e1.getX() > screenWidth - 100) {
                return true;
            }
            disX = e2.getX() - e1.getX();
            //log.error("disX = "+disX);
            recordedCL = nowCL;
            if (Math.abs(disX) >= 5 * proUnit) {
                nowCL = (int) (disX / proUnit);
                nowSL = nowCL - recordedCL;//返回递增量
                if (nowSL != 0) {
                	mConsumeGesture.adjustPlayProgress(nowSL);
                    is_adjust_pro = true;
                }
            }
        } else if (handType == TYPE_VOL_OR_BRI) {
            if (checkKitKatTouch && e1.getY() < 50) {
                return true;
            }
            if (startX < halfScreenWidth) {//在屏幕左边则，调节亮度；
                disY = (int) (e1.getY() - e2.getY());
                recordedCL = nowCL;
                nowCL = disY / briUnit;//变化级数,可为负
                //if(nowCL - recordedCL != 0){
                initialBrightness = initialBrightness + (nowCL - recordedCL) * briPerUnit;
                initialBrightness = Math.max(initialBrightness, 0);
                
                if (initialBrightness > SYSTEM_MAX_BRIGHTNESS){
                	initialBrightness = SYSTEM_MAX_BRIGHTNESS;
                }
                recordedSL = nowSL;

                nowSL = (int) (initialBrightness * BRIGHTNESS_LEVEL_NUM / SYSTEM_MAX_BRIGHTNESS);//维持相应等级的关系
                log.error("initialBrightness ="+initialBrightness);
                if (nowSL - recordedSL != 0) {//只有等级变化了才调用界面
                    mWL.screenBrightness = nowSL / (float) (BRIGHTNESS_LEVEL_NUM);
                    log.debug("BRIGHTNESS_LEVEL_NUM="+BRIGHTNESS_LEVEL_NUM+",nowSL="+nowSL);
                    mConsumeGesture.adjustBrightness(mWL, BRIGHTNESS_LEVEL_NUM, nowSL);//交给界面处理
                }
                //}
            } else {//在屏幕的右边，则调节音量
                //isAdjustingVol = true;
                disY = (int) (e1.getY() - e2.getY());
                recordedCL = nowCL;
                nowCL = disY / volUnit;
                if (nowCL - recordedCL != 0) {
                    initialVolume = (int) (initialVolume + (nowCL - recordedCL) * volPerUnit);
                    if (initialVolume < 0) initialVolume = 0;
                    if (initialVolume > SYSTEM_MAX_VOLUME) initialVolume = SYSTEM_MAX_VOLUME;
                    recordedSL = nowSL;

                    mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, initialVolume, 0);
                    if(initialVolume == 1){
                    	nowSL = 1;
                    }else{
                    	nowSL = (int) (initialVolume * VOLUME_LEVEL_NUM / SYSTEM_MAX_VOLUME);//维持相应等级的关系
                    }
                    log.debug("initialVolume="+initialVolume+",nowSL="+nowSL+",VOLUME_LEVEL_NUM="+VOLUME_LEVEL_NUM+",SYSTEM_MAX_VOLUME="+SYSTEM_MAX_VOLUME);

                    if (nowSL - recordedSL != 0) {//只有等级变化了才调用界面
                    	mConsumeGesture.adjustVolume(mAudioManager, VOLUME_LEVEL_NUM, nowSL);//交给界面处理
                    }
                }
            }
        }
        return false;
    }

    @Override
    public void onLongPress(MotionEvent e) {
    }

    @Override
    //该函数有可能不触发
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        //log.error("onFling");
        asureFields();
        mConsumeGesture.hideLayout();
        is_adjust_pro = false;
        return true;
    }

    @Override
    public boolean onDown(MotionEvent e) {
        //log.error("onDown");
        asureFields();
        startX = (int) e.getX();
        startY = (int) e.getY();
        return false;
    }

    public void hideLayout() {
        if (is_adjust_pro) {
            mHandler.removeMessages(HIDE_VOL_LAYOUT);
            mHandler.sendEmptyMessage(HIDE_VOL_LAYOUT);
            is_adjust_pro = false;
        }
    }

    private void asureFields() {
        nowCL = 0;
        recordedCL = 0;
        nowSL = -1;
        recordedSL = 0;
        disX = 0;
        disY = 0;
        handType = TYPE_UNKNOWN;
        is_adjust_pro = false;
        //isAdjustingVol = false;
    }

    //处理音量键事件,注意onFling没触发的情况
    @SuppressWarnings("static-access")
    public boolean consumeVolumeKeyDown(int keyCode, KeyEvent keyEvent) {
        if (keyCode == keyEvent.KEYCODE_VOLUME_UP) {
            initialVolume++;
        } else if (keyCode == keyEvent.KEYCODE_VOLUME_DOWN) {
            initialVolume--;
        } else {
            return false;
        }
        if (initialVolume < 0) initialVolume = 0;
        if (initialVolume > SYSTEM_MAX_VOLUME) initialVolume = SYSTEM_MAX_VOLUME;
        mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, initialVolume, 0);

        if(initialVolume == 1){
        	nowSL = 1;
        }else{
        	nowSL = (int) (initialVolume * VOLUME_LEVEL_NUM / SYSTEM_MAX_VOLUME);//维持相应等级的关系
        }
    	mConsumeGesture.adjustVolume(mAudioManager, VOLUME_LEVEL_NUM, nowSL);

        mHandler.removeMessages(HIDE_VOL_LAYOUT);
        mHandler.sendEmptyMessageDelayed(HIDE_VOL_LAYOUT, 1500);//一定时间后自动关闭界面View
        return true;
    }

    public void saveCurBright() {
        SharePrefence.getInstance(mActivity).putFloat(SharePrefence.KEY_SCREEN_BRIGHT_RECORD, initialBrightness);
    }


    public interface ConsumeGesture {
    	
        public void adjustBrightness(WindowManager.LayoutParams wl, int maxlevel, int level);//调整亮度

        public void adjustVolume(AudioManager am, int maxlevel, int level);//调整媒体音量

        public void adjustPlayProgress(int adjustTime);//调整播放进度,返回调整时间的递增量（单位：秒）,由界面响应

        public void hideLayout();
    }

}
