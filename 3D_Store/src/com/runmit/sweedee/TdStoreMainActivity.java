
package com.runmit.sweedee;

import java.util.Locale;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.widget.Toast;

import com.runmit.libsdk.update.Update;
import com.runmit.mediaserver.MediaServer;
import com.runmit.sweedee.download.DownloadManager;
import com.runmit.sweedee.manager.AccountServiceManager;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.Game3DConfigManager;
import com.runmit.sweedee.manager.InitDataLoader;
import com.runmit.sweedee.player.ScreenObserver;
import com.runmit.sweedee.player.ScreenObserver.ScreenBroadcastReceiver;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.tdviewsdk.activity.transforms.AnimActivity;
import com.umeng.analytics.MobclickAgent;

public class TdStoreMainActivity extends AnimActivity {

    private XL_Log log = new XL_Log(TdStoreMainActivity.class);

    private long waitTime = 2000;

    private long touchTime = 0;

    private TdStoreMainView mXlMainView;

    private ScreenBroadcastReceiver mScreenReceiver;

    private static final String FRAGMENTS_TAG = "android:support:fragments";

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        if (savedInstanceState != null) {//清理掉数据，防止FragmentActivity restore数据，造成崩溃
            savedInstanceState.remove(FRAGMENTS_TAG);
        }
        super.onCreate(savedInstanceState);
        log.debug("TdStoreMainActivity onCreate");
        mXlMainView = new TdStoreMainView(this, null);
        setContentView(mXlMainView);
        update();
        //初始化点播库
        String peerId = UserManager.getInstance().getPeerId() + "b";
        MediaServer.getInstance(this, Constant.PRODUCT_ID, peerId);
        initBroadCast();
    }

    private void initBroadCast() {
        mScreenReceiver = new ScreenObserver.ScreenBroadcastReceiver();
        IntentFilter screenfilter = new IntentFilter();
        screenfilter.addAction(Intent.ACTION_SCREEN_ON);
        screenfilter.addAction(Intent.ACTION_SCREEN_OFF);
        registerReceiver(mScreenReceiver, screenfilter);

        IntentFilter localfilter = new IntentFilter();
        localfilter.addAction(Intent.ACTION_LOCALE_CHANGED);
        registerReceiver(mReceiver, localfilter);
    }

    /**
     * 勿删，用来保存被系统kill之后的一些重要数据
     * 1.保存主界面当前的tab
     * 2.保存用户登陆信息
     */
    public void onSaveInstanceState(Bundle outState) {
        log.debug("onSaveInstanceState");
        super.onSaveInstanceState(outState);
        if (mXlMainView != null) {
            mXlMainView.onSaveInstanceState(outState);
        }
        UserManager.getInstance().onSaveInstanceState(outState);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        UserManager.getInstance().onRestoreInstanceState(savedInstanceState);
    }

    public boolean dispatchKeyEvent(KeyEvent event) {
        log.debug("dispatchKeyEvent >> event.code = " + event.getKeyCode() + ">> event.action = " + event.getAction());
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN) {
            if (mXlMainView != null && mXlMainView.onBackPress()) {
                return true;
            }
            if (DownloadManager.getInstance().haveTaskRunning()) {
                showExitNotiDialog();
            } else {
                long currentTime = System.currentTimeMillis();
                if ((currentTime - touchTime) >= waitTime) {
                    Util.showToast(this, getString(R.string.press_again_to_exit_app), Toast.LENGTH_SHORT);
                    touchTime = currentTime;
                } else {
                    releaseAll();
                }
            }
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    private void showExitNotiDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this, R.style.AlertDialog);
        builder.setTitle(Util.setColourText(this, R.string.download_cancle, Util.DialogTextColor.BLUE));
        builder.setPositiveButton(R.string.cancel,
                new android.content.DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                }
        );
        builder.setNegativeButton(Util.setColourText(this, R.string.ok, Util.DialogTextColor.BLUE),
                new android.content.DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        releaseAll();
                    }
                }
        );
        if (!isFinishing()) {
            AlertDialog mExitNotiDialog = builder.create();
            mExitNotiDialog.setCanceledOnTouchOutside(true);
            mExitNotiDialog.show();
        }
    }

    public void update() {
        if (Util.isNetworkAvailable(TdStoreMainActivity.this)) {
            new Update(this).checkUpdate(Constant.PRODUCT_ID, false);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mXlMainView != null) {
            mXlMainView.onResume();
        }
    }

    @Override
    protected void onPause() {
        log.debug("onPause");
        super.onPause();
        if (mXlMainView != null) {
            mXlMainView.onPause();
        }
    }

    @Override
    protected void onDestroy() {
        log.debug("-----onDestroy start********************");
        super.onDestroy();
        
        unregisterReceiver(mScreenReceiver);
        unregisterReceiver(mReceiver);
        
    }

    /**
     * 这个是真正释放所有资源，包括kill虚拟机
     */
    private void releaseAll() {
        MobclickAgent.onKillProcess(this);
        //置空静态对象
        ReportActionSender.getInstance().openDuration(System.currentTimeMillis() - LoadingActivity.appStartTime);
        ReportActionSender.getInstance().Uninit();
        DownloadManager.getInstance().onDestroy();
        Game3DConfigManager.reset();
        finish();
      
        CmsServiceManager.getInstance().reset();
        AccountServiceManager.getInstance().reset();
        InitDataLoader.getInstance().reset();
        Game3DConfigManager.reset();
        log.debug("-----releaseAll********************");
        StoreApplication.INSTANCE.killSelf();
        android.os.Process.killProcess(android.os.Process.myPid());
    }

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            log.debug("mReceiver  onReceive action =  " + intent.getAction());
            if (intent.getAction().equals(Intent.ACTION_LOCALE_CHANGED)) {
            	Resources res = context.getResources();
                DisplayMetrics dm = res.getDisplayMetrics();
                android.content.res.Configuration conf = res.getConfiguration();
                conf.locale = Locale.getDefault();
                res.updateConfiguration(conf, dm);
                
                InitDataLoader.getInstance().initHomeFakeData();
            }
        }
    };
}
