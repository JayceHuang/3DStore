package com.runmit.sweedee;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.content.ContentValues;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

import com.runmit.sweedee.downloadinterface.DownloadEngine;
import com.runmit.sweedee.manager.GetSysAppInstallListManager;
import com.runmit.sweedee.provider.GameConfigTable;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.update.Config;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.util.gif.GifImageView;
import com.runmit.sweedee.util.gif.GifImageView.OnAnimationEndListener;
import com.runmit.sweedee.util.gif.GifMovieView;
import com.superd.sdsdk.SDDevice;

public class LoadingActivity extends BaseActivity {

    private XL_Log log = new XL_Log(LoadingActivity.class);

    private final static int GOTO_XLSHAREACTIVITY = 1;
    private final static int GOTO_FLIPACTIVITY = GOTO_XLSHAREACTIVITY + 1;

    public final static String CURRENT_VERSION = "current_version";
    public static int lastUserVersion = 0;

    private LaunchType mInstallState = LaunchType.FIRST_INSTALL;// 安装启动模式,0，普通启动，1，覆盖安装启动,2,首次安装。3.覆盖安装但是之前版本并没有此字段


    public static long appStartTime;

    private static final String SP_SHOW_NET_CHARGE = "isshownetcharge";//是否显示流量提示

    //安装启动模式
    private enum LaunchType {
        FIRST_INSTALL,//首次安装
        UPDATE_INSTALL,//覆盖安装启动
        NORMAL_LANUCH,//普通启动
        UNKNOWN,//覆盖安装但是之前版本并没有此字段
    }

    private Bitmap loadingBitmap = null;

    @SuppressLint("HandlerLeak")
    private Handler loadingCompleteHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            Intent intent = new Intent();
            log.debug("handleMessage=" + msg.what);
            switch (msg.what) {
                case GOTO_FLIPACTIVITY:
                    //TODO:方便测试，直接进界面，屏蔽欢迎界面
                    intent.setClass(LoadingActivity.this, TdStoreMainActivity.class);
                    intent.putExtra("first_intall_state", mInstallState == LaunchType.UPDATE_INSTALL ? true : false);
                    startActivity(intent);
                    LoadingActivity.this.finish();
                    break;
                case GOTO_XLSHAREACTIVITY:
                    intent.setClass(LoadingActivity.this, TdStoreMainActivity.class);
                    startActivity(intent);
                    LoadingActivity.this.finish();
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setBackgroundDrawableResource(R.color.gif_bkgcolor);
        DisplayMetrics metric = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metric);
        Constant.SCREEN_WIDTH = metric.widthPixels; // 屏幕宽度（像素）
        Constant.SCREEN_HEIGHT = metric.heightPixels; // 屏幕高度（像素）
        float density = metric.density; // 屏幕密度（0.75 / 1.0 / 1.5）        
        Constant.DENSITY = density;
        log.debug("density=" + density + "Constant.SCREEN_WIDTH=" + Constant.SCREEN_WIDTH + ",Constant.SCREEN_WIDTH=" + Constant.SCREEN_HEIGHT);
        loadingCompleteHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
            	setLoadingView();
                appStartTime = System.currentTimeMillis();
                initLaunchType();

            }
        }, 300);

        //游戏配置数据库测试
        // insertTest();

        //获取Hwid测试
        //long id =  SDDevice.getICFWID(); 
        //Log.d("getICFWID", "getICFWID id = "+id);

    }

    private void insertTest() {
        Uri uri = GameConfigTable.CONTENT_URI;
        ContentValues values = new ContentValues();
        values.put(GameConfigTable.PACKAGE_NAME, "com.siulun.Camera3D");
        values.put(GameConfigTable.PACKAGE_VERSION, 1);
        values.put(GameConfigTable.X, 1.0f);
        values.put(GameConfigTable.Y, 1.0f);
        values.put(GameConfigTable.Z, 1.0f);
        values.put(GameConfigTable.W, 4.0f);
        values.put(GameConfigTable.STATUS, true);
        values.put(GameConfigTable.DATA_VERSION, 1);
        getContentResolver().insert(uri, values);
    }

    private void setLoadingView(){
    	 ViewGroup.LayoutParams lpParam = new ViewGroup.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
         View loadingView = new View(this);
         loadingView.setBackgroundResource(R.drawable.start_animation);	
         setContentView(loadingView, lpParam);
         new Handler().postDelayed(new Runnable() {
             public void run() {
            	 checkNetCharge();
             }
         }, 1900);

    }

    private void setAnimView() {
        ViewGroup.LayoutParams lpParam = new ViewGroup.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
        GifMovieView movieView = new GifMovieView(this);
        int duration = movieView.setMovieResource(R.drawable.start_animation);
        log.debug("setAnimView duration  = " + duration);
        if (duration > 0) {
            movieView.setOnEndListener(new GifMovieView.EndListener() {
                @Override
                public void onEnd() {
                    log.debug("GifMovieView anim end ");
                    checkNetCharge();
                }
            });
            setContentView(movieView, lpParam);
        } else {//手机系统不支持，用原有的方式
            GifImageView gifImageView = new GifImageView(this);
            gifImageView.setOnAnimationEndListener(new OnAnimationEndListener() {
                @Override
                public void onEnd() {
                    log.debug("GifImageView anim end ");
                    loadingCompleteHandler.post(new Runnable() {
						
						@Override
						public void run() {
							checkNetCharge();
						}
					});
                }
            });
            gifImageView.setGifImage(R.drawable.start_animation);
            gifImageView.startAnimation();
            setContentView(gifImageView, lpParam);
        }
    }

    private void initLaunchType() {
        int currentVersion = Config.getVerCode(getApplicationContext());// 当前版本号
        lastUserVersion = mXLSharePrefence.getInt(CURRENT_VERSION, 0);// sp中保存的版本号
        log.debug("lastUserVersion=" + lastUserVersion + ",currentVersion=" + currentVersion);
        if (lastUserVersion == 0) {// 首次安装或者清除数据
            mInstallState = LaunchType.FIRST_INSTALL;
            mXLSharePrefence.putInt(CURRENT_VERSION, currentVersion);
            mXLSharePrefence.putBoolean("REGIST_GIFT_ISTOAST", false);//此值用于注册界面弹出送水滴的提示用
        } else if (lastUserVersion == currentVersion) {// 这种情况是普通启动
            mInstallState = LaunchType.NORMAL_LANUCH;
            mXLSharePrefence.putBoolean("REGIST_GIFT_ISTOAST", true);
        } else if (lastUserVersion < currentVersion) {// 这种情况是覆盖安装
            mInstallState = LaunchType.UPDATE_INSTALL;
            mXLSharePrefence.putInt(CURRENT_VERSION, currentVersion);// 接着保存当前版本号到sp
            mXLSharePrefence.putBoolean("REGIST_GIFT_ISTOAST", false);
            
            new DownloadEngine().initEtmAsync();
        }
    }

    private void obtainMessage() {
        log.debug("obtainMessage mInstallState=" + mInstallState);
        if (mInstallState != LaunchType.NORMAL_LANUCH) {// 第一次启动跳转到Filp页面
            loadingCompleteHandler.obtainMessage(GOTO_FLIPACTIVITY).sendToTarget();
        } else {// 不是第一次启动，做两件事1.上报服务器。2跳转
            loadingCompleteHandler.sendEmptyMessage(GOTO_XLSHAREACTIVITY);
        }
        ReportActionSender.getInstance().startUp();
        StoreApplication.INSTANCE.initData();
    }

    /**
     * 检测显示流量提示对话框
     */
    private void checkNetCharge() {
        final SharePrefence mPrefence = SharePrefence.getInstance(this);
        boolean isShow = mPrefence.getBoolean(SP_SHOW_NET_CHARGE, false);
        if (!isShow) {//没有提示
            AlertDialog.Builder builder = new AlertDialog.Builder(this, R.style.AlertDialog);
            builder.setTitle(Util.setColourText(this, R.string.reminder, Util.DialogTextColor.BLUE));
            View view = LayoutInflater.from(this).inflate(R.layout.dialog_firsti_flow_toast, null);
            final CheckBox checkView = (CheckBox) view.findViewById(R.id.checkbox_flow_show);
            builder.setView(view);

            builder.setPositiveButton(R.string.real_quit,
                    new android.content.DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            finish();
                        }
                    }
            );
            builder.setNegativeButton(Util.setColourText(this, R.string.agree, Util.DialogTextColor.BLUE),
                    new android.content.DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            if (checkView.isChecked()) {
                                mPrefence.putBoolean(SP_SHOW_NET_CHARGE, true);
                            }
                            obtainMessage();
                        }
                    }
            );
            AlertDialog mExitNotiDialog = builder.create();
            mExitNotiDialog.setCanceledOnTouchOutside(false);
            mExitNotiDialog.setCancelable(false);
            mExitNotiDialog.show();
        } else {
            obtainMessage();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (loadingBitmap != null && !loadingBitmap.isRecycled()) {
            loadingBitmap.recycle();
            loadingBitmap = null;
        }
    }

    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK
                && event.getAction() == KeyEvent.ACTION_DOWN) {
            LoadingActivity.this.finish();
            ((StoreApplication) getApplication()).killSelf();
            android.os.Process.killProcess(android.os.Process.myPid());// 这种方式不会调用onDestroy，不知道会不会有影响
            return true;
        }
        return super.dispatchKeyEvent(event);
    }
}
