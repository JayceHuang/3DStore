package com.runmit.sweedee.player;

import java.lang.reflect.Method;

import android.app.Activity;
import android.app.KeyguardManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;

/**
 * @author zhangyg
 */
public class ScreenObserver {
    private static String TAG = "ScreenObserver";
    private Context mContext;
    //private ScreenBroadcastReceiver mScreenReceiver;
    private static ScreenStateListener mScreenStateListener;
    private Method mReflectScreenState;
    private Method mReflectScreenOff;
    private PowerManager mPowerManager = null;
    private WakeLock mWakeLock = null;
    private static boolean isScreenStateOff = false;

    public ScreenObserver(Context context) {
        mContext = context;
        //mScreenReceiver = new ScreenBroadcastReceiver();
        mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = mPowerManager.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "My Lock");
        try {
            mReflectScreenState = PowerManager.class.getMethod("isScreenOn",
                    new Class[]{});

            mReflectScreenOff = PowerManager.class.getMethod("isScreenOff",
                    new Class[]{});
        } catch (NoSuchMethodException nsme) {
            Log.d(TAG, "API < 7," + nsme);
        }
    }

    /**
     * screen状态广播接收者
     *
     * @author zhangyg
     */
    public static class ScreenBroadcastReceiver extends BroadcastReceiver {
        private String action = null;

        @Override
        public void onReceive(Context context, Intent intent) {
            action = intent.getAction();
            if (Intent.ACTION_SCREEN_ON.equals(action)) {
                Log.d("LJD", "screen on");
                isScreenStateOff = false;
                if (mScreenStateListener != null) {
                    mScreenStateListener.onScreenOn();
                }
            } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                if (mScreenStateListener != null) {
                    mScreenStateListener.onScreenOff();
                }
                isScreenStateOff = true;
                Log.d("LJD", "screen off");
            }
        }
    }


    /**
     * 请求screen状态更新
     *
     * @param listener
     */
    public void requestScreenStateUpdate(ScreenStateListener listener) {
        mScreenStateListener = listener;
        //startScreenBroadcastReceiver();

        firstGetScreenState();
    }

    /**
     * 第一次请求screen状态
     */
    private void firstGetScreenState() {

        if (isScreenOn()) {
            if (mScreenStateListener != null) {
                mScreenStateListener.onScreenOn();
            }
        } else {
            if (mScreenStateListener != null) {
                mScreenStateListener.onScreenOff();
            }
        }
    }

    /**
     * 停止screen状态更新
     */
    public void stopScreenStateUpdate() {
        //	mContext.unregisterReceiver(mScreenReceiver);
        mContext = null;
    }

    /**
     * 启动screen状态广播接收器
     */
    private void startScreenBroadcastReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        //mContext.registerReceiver(mScreenReceiver, filter);
    }

    /**
     * screen是否打开状态
     *
     * @param pm
     * @return
     */
    private boolean isScreenOn() {
        boolean screenState;
        try {
            screenState = (Boolean) mReflectScreenState.invoke(mPowerManager);
        } catch (Exception e) {
            screenState = false;
        }
        return screenState;
    }

    /**
     * screen是否打开状态
     *
     * @param pm
     * @return
     */
    public boolean isScreenOff() {
        boolean screenState;
        try {
            screenState = (Boolean) mReflectScreenOff.invoke(mPowerManager);
        } catch (Exception e) {
            screenState = false;
        }
        return screenState;
    }

    public interface ScreenStateListener {
        public void onScreenOn();

        public void onScreenOff();
    }

    public boolean isScreenStateOff() {
        return isScreenStateOff;
    }


    public void initPowerManager(Context context) {
        mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = mPowerManager.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "My Lock");
    }

    public void keepScreenLightOn(Context context) {
        if (null == mPowerManager) {
            initPowerManager(context);
        }
        mWakeLock.acquire();
    }

    public void keepScreenLightOff() {
        if (null == mPowerManager) {
            return;
        }
        try {
            mWakeLock.release();
        } catch (RuntimeException e) {
            e.printStackTrace();
        }
    }

    public boolean isScreenLocked(Context context) {
        KeyguardManager mKeyguardManager = (KeyguardManager) context.getSystemService(context.KEYGUARD_SERVICE);
        if (mKeyguardManager.inKeyguardRestrictedInputMode()) {
            return true;
        }
        return false;
    }

    public void tryAndWaitWhileScreenLocked(Context context) {
        while (isScreenLocked(context)) {
            try {
                Thread.sleep(700);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }


}
