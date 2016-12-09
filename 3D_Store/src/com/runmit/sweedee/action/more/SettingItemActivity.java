package com.runmit.sweedee.action.more;

import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.manager.Game3DConfigManager;
import com.runmit.sweedee.player.OnLinePlayMode;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.sdk.user.member.task.UserManager.LoginState;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

import java.util.Locale;

public class SettingItemActivity extends NewBaseRotateActivity implements UserManager.LoginStateChange {

    XL_Log log = new XL_Log(SettingItemActivity.class);
    
    public static final String IS_INSTALL_AFTER_DOWNLOAD = "IS_INSTALL_AFTER_DOWNLOAD";
    
    public static final String IS_DELETE_AFTER_INSTALL = "IS_DELETE_AFTER_INSTALL";

    public static final String MOBILE_PLAY = "mobile_play";//移动网络播放

    public static final String MOBILE_DOWNLOAD = "mobile_download";//移动网络缓存

    public static final String DOWNLOAD_SOLUTION = "download_solution";//缓存视频清晰度选择

    public static final String DOWNLOAD_LANGUAGE = "download_language";//缓存视频清晰度选择  

    private SharePrefence mXLSharePrefence;

    private CheckBox checkbox_first_play_high;//优先播放高清
    private CheckBox checkbox_select_superhight;//优先播放超高清

    private CheckBox checkbox_mobile_play;
    private CheckBox checkbox_mobile_download;

    private CheckBox checkbox_download_high_first;
    private CheckBox checkbox_download_superhigh_first; 
    
//    private CheckBox checkbox_is_install_after_download;    
    private CheckBox checkbox_is_auto_delete_apk;

    private Button btnLoginOut;

//    private LinearLayout unbind_device_head;
//    private LinearLayout unbind_device_parent;

    private ProgressDialog mProgressDialog;

    private CheckBox checkbox_download_en;

    private CheckBox checkbox_download_ch;
    
    /**音轨设置CheckBox*/
    private CheckBox checkbox_audio_orin;
    private CheckBox checkbox_audio_local;
    
    private CheckBox checkbox_game_setting;
    
    private LinearLayout ll_game_header_title;
    private TextView tv_game_header_subtitle;
    private LinearLayout ll_game_header_setting;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
    	super.onCreate(savedInstanceState);
    	setContentView(R.layout.activity_setting_item);
    	getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setDisplayShowHomeEnabled(false);
        
    	UserManager.getInstance().addLoginStateChangeListener(this);
        mXLSharePrefence = SharePrefence.getInstance(getApplicationContext());
        mProgressDialog = new ProgressDialog(SettingItemActivity.this, R.style.ProgressDialogDark);
        initUI();
    }

    public void startViewAnim() {
        Intent intent = getIntent();
        Bundle bundle = intent.getExtras();
        if (bundle != null) {
            int token = bundle.getInt(TOKEN);
        }
    }

    public void closeViewAnim() {
    }

    private void initUI() {
        checkbox_first_play_high = (CheckBox) findViewById(R.id.checkbox_first_play_high);
        checkbox_select_superhight = (CheckBox) findViewById(R.id.checkbox_select_superhight);
        int select_play_resolution = mXLSharePrefence.getInt(OnLinePlayMode.SP_DEFAULT_PLAY_RESOLUTION, Constant.NORMAL_VOD_URL);
        checkbox_first_play_high.setChecked(select_play_resolution == Constant.NORMAL_VOD_URL);
        checkbox_first_play_high.setClickable(!(select_play_resolution == Constant.NORMAL_VOD_URL));
        checkbox_select_superhight.setChecked(select_play_resolution == Constant.HIGH_VOD_URL);
        checkbox_select_superhight.setClickable(!(select_play_resolution == Constant.HIGH_VOD_URL));
        
        checkbox_first_play_high.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                	buttonView.playSoundEffect(0);
                    mXLSharePrefence.putInt(OnLinePlayMode.SP_DEFAULT_PLAY_RESOLUTION, Constant.NORMAL_VOD_URL);
                    checkbox_select_superhight.setChecked(false);
                    checkbox_first_play_high.setClickable(false);
                    checkbox_select_superhight.setClickable(true);
                }
            }
        });

        checkbox_select_superhight.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {//如果選中
                	buttonView.playSoundEffect(0);
                    mXLSharePrefence.putInt(OnLinePlayMode.SP_DEFAULT_PLAY_RESOLUTION, Constant.HIGH_VOD_URL);
                    checkbox_first_play_high.setChecked(false);
                    checkbox_first_play_high.setClickable(true);
                    checkbox_select_superhight.setClickable(false);
                }else{
                	
                }
            }
        });
        
//        checkbox_is_install_after_download = (CheckBox) findViewById(R.id.checkbox_is_install_after_download);
//        boolean is_install_after_download = mXLSharePrefence.getBoolean(IS_INSTALL_AFTER_DOWNLOAD, true);
//        checkbox_is_install_after_download.setChecked(is_install_after_download);
//        checkbox_is_install_after_download.setOnCheckedChangeListener(new OnCheckedChangeListener() {
//
//            @Override
//            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
//                mXLSharePrefence.putBoolean(IS_INSTALL_AFTER_DOWNLOAD, isChecked);
//                buttonView.playSoundEffect(0);
//            }
//        });
        
        checkbox_is_auto_delete_apk = (CheckBox) findViewById(R.id.checkbox_is_auto_delete_apk);
        boolean is_auto_delete_apk = mXLSharePrefence.getBoolean(IS_DELETE_AFTER_INSTALL, false);
        checkbox_is_auto_delete_apk.setChecked(is_auto_delete_apk);
        checkbox_is_auto_delete_apk.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mXLSharePrefence.putBoolean(IS_DELETE_AFTER_INSTALL, isChecked);
                buttonView.playSoundEffect(0);
            }
        });

        checkbox_mobile_play = (CheckBox) findViewById(R.id.checkbox_mobile_play);
        checkbox_mobile_play.setChecked(mXLSharePrefence.getBoolean(MOBILE_PLAY, false));
        checkbox_mobile_play.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mXLSharePrefence.putBoolean(MOBILE_PLAY, isChecked);
                buttonView.playSoundEffect(0);
            }
        });

        checkbox_mobile_download = (CheckBox) findViewById(R.id.checkbox_mobile_download);
        checkbox_mobile_download.setChecked(mXLSharePrefence.getBoolean(MOBILE_DOWNLOAD, false));
        checkbox_mobile_download.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mXLSharePrefence.putBoolean(MOBILE_DOWNLOAD, isChecked);
                buttonView.playSoundEffect(0);
            }
        });

        checkbox_download_high_first = (CheckBox) findViewById(R.id.checkbox_download_high_first);
        checkbox_download_superhigh_first = (CheckBox) findViewById(R.id.checkbox_download_superhigh_first);
        int downloadSolution = mXLSharePrefence.getInt(DOWNLOAD_SOLUTION, Constant.NORMAL_VOD_URL);
        checkbox_download_high_first.setChecked(downloadSolution == Constant.NORMAL_VOD_URL);
        checkbox_download_high_first.setClickable(downloadSolution != Constant.NORMAL_VOD_URL);
        checkbox_download_superhigh_first.setChecked(downloadSolution == Constant.HIGH_VOD_URL);
        checkbox_download_superhigh_first.setClickable(downloadSolution != Constant.HIGH_VOD_URL);
        
        checkbox_download_high_first.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                	buttonView.playSoundEffect(0);
                    mXLSharePrefence.putInt(DOWNLOAD_SOLUTION, Constant.NORMAL_VOD_URL);
                    checkbox_download_high_first.setClickable(false);
                    checkbox_download_superhigh_first.setChecked(false);
                    checkbox_download_superhigh_first.setClickable(true);
                }
            }
        });
        checkbox_download_superhigh_first.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                	buttonView.playSoundEffect(0);
                    mXLSharePrefence.putInt(DOWNLOAD_SOLUTION, Constant.HIGH_VOD_URL);
                    checkbox_download_superhigh_first.setClickable(false);
                    checkbox_download_high_first.setChecked(false);
                    checkbox_download_high_first.setClickable(true);
                }

            }
        });

        checkbox_download_ch = (CheckBox) findViewById(R.id.checkbox_download_ch);
        checkbox_download_en = (CheckBox) findViewById(R.id.checkbox_download_en);
        String downloadLanguage = mXLSharePrefence.getString(DOWNLOAD_LANGUAGE, Locale.getDefault().getLanguage());
        log.debug("downloadLanguage=" + downloadLanguage);
        checkbox_download_ch.setChecked(Constant.DOWNLOAD_LAN_CH.equalsIgnoreCase(downloadLanguage));
        checkbox_download_ch.setClickable(!Constant.DOWNLOAD_LAN_CH.equalsIgnoreCase(downloadLanguage));
        checkbox_download_en.setChecked(Constant.DOWNLOAD_LAN_EN.equalsIgnoreCase(downloadLanguage));
        checkbox_download_en.setClickable(!Constant.DOWNLOAD_LAN_EN.equalsIgnoreCase(downloadLanguage));
        
        checkbox_download_ch.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                	buttonView.playSoundEffect(0);
                    mXLSharePrefence.putString(DOWNLOAD_LANGUAGE, Constant.DOWNLOAD_LAN_CH);
                    checkbox_download_ch.setClickable(false);
                    checkbox_download_en.setChecked(false);
                    checkbox_download_en.setClickable(true);
                }
            }
        });
        checkbox_download_en.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                	buttonView.playSoundEffect(0);
                    mXLSharePrefence.putString(DOWNLOAD_LANGUAGE, Constant.DOWNLOAD_LAN_EN);
                    checkbox_download_en.setClickable(false);
                    checkbox_download_ch.setChecked(false);
                    checkbox_download_ch.setClickable(true);
                }

            }
        });
        
        /**音轨部分的设置---------------start--**/
        checkbox_audio_orin = (CheckBox) findViewById(R.id.checkbox_audio_orin);
        checkbox_audio_local = (CheckBox) findViewById(R.id.checkbox_audio_local);
        int audioType = mXLSharePrefence.getInt(Constant.AUDIO_TRACK_SETTING, Constant.AUDIO_FIRST_LOCAL);
        log.debug("audioTrack setting =" + audioType);
        checkbox_audio_orin.setChecked(audioType == Constant.AUDIO_FIRST_ORIN);
        checkbox_audio_orin.setClickable(audioType != Constant.AUDIO_FIRST_ORIN);
        checkbox_audio_local.setChecked(audioType == Constant.AUDIO_FIRST_LOCAL);
        checkbox_audio_local.setClickable(audioType != Constant.AUDIO_FIRST_LOCAL);
        checkbox_audio_orin.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                	buttonView.playSoundEffect(0);
                    mXLSharePrefence.putInt(Constant.AUDIO_TRACK_SETTING, Constant.AUDIO_FIRST_ORIN);
                    checkbox_audio_orin.setClickable(false);
                    checkbox_audio_local.setChecked(false);
                    checkbox_audio_local.setClickable(true);
                }
            }
        });
        checkbox_audio_local.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                	buttonView.playSoundEffect(0);
                	mXLSharePrefence.putInt(Constant.AUDIO_TRACK_SETTING, Constant.AUDIO_FIRST_LOCAL);
                    checkbox_audio_local.setClickable(false);
                    checkbox_audio_orin.setChecked(false);
                    checkbox_audio_orin.setClickable(true);
                }

            }
        });
        /**音轨部分的设置---------------end--**/
        
        checkbox_game_setting = (CheckBox) findViewById(R.id.checkbox_game_setting);
        ll_game_header_title = (LinearLayout) findViewById(R.id.ll_game_header_title);
        tv_game_header_subtitle = (TextView) findViewById(R.id.tv_game_header_subtitle);
        ll_game_header_setting = (LinearLayout) findViewById(R.id.ll_game_header_setting);
        
        boolean isChecked = Game3DConfigManager.getInstance().isGame3DOn();
        checkbox_game_setting.setChecked(isChecked);
        checkbox_game_setting.setOnCheckedChangeListener(new OnCheckedChangeListener() {			
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				Game3DConfigManager.getInstance().toggleGame3D(isChecked);
				buttonView.playSoundEffect(0);
			}
		});
        if(StoreApplication.isSnailPhone){
        	ll_game_header_title.setVisibility(View.GONE);	
        	tv_game_header_subtitle.setVisibility(View.GONE);	
        	ll_game_header_setting.setVisibility(View.GONE);	
        }
//        unbind_device_layout = (RelativeLayout)findViewById(R.id.unbind_device_layout);
//        unbind_device_layout.setOnClickListener(new OnClickListener() {
//
//            @Override
//            public void onClick(View v) {
//                android.app.AlertDialog.Builder mBuilder = new android.app.AlertDialog.Builder(SettingItemActivity.this);
//                mBuilder.setTitle(Util.setColourText(SettingItemActivity.this, R.string.unbind_device, Util.DialogTextColor.BLUE));
//                mBuilder.setMessage(getString(R.string.confirm_unbind));
//                mBuilder.setNegativeButton(Util.setColourText(SettingItemActivity.this, R.string.ok, Util.DialogTextColor.BLUE), new DialogInterface.OnClickListener() {
//
//                    @Override
//                    public void onClick(DialogInterface dialog, int which) {
//                        Util.showDialog(mProgressDialog, getString(R.string.unbind_processing));
//                        UserManager.getInstance().unbindDevice();
//                    }
//                });
//                mBuilder.setPositiveButton(R.string.cancel, new DialogInterface.OnClickListener() {
//
//                    @Override
//                    public void onClick(DialogInterface dialog, int which) {
//                        // TODO Auto-generated method stub
//
//                    }
//                });
//                mBuilder.create().show();
//            }
//        });
        btnLoginOut = (Button)findViewById(R.id.btnLoginOut);
        btnLoginOut.setVisibility(UserManager.getInstance().isLogined() ? View.VISIBLE : View.GONE);
        btnLoginOut.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                Util.showDialog(mProgressDialog, getString(R.string.logout_processing));
                UserManager.getInstance().userLogout(true);
            }
        });

//        unbind_device_head = (LinearLayout) findViewById(R.id.unbind_device_head);
//        unbind_device_parent = (LinearLayout) findViewById(R.id.unbind_device_parent);
        invalidateLoginState();
    }

    private void invalidateLoginState() {
        if (UserManager.getInstance().isLogined()) {
            btnLoginOut.setVisibility(View.VISIBLE);
//            unbind_device_head.setVisibility(View.VISIBLE);
//            unbind_device_parent.setVisibility(View.VISIBLE);
        } else {
            btnLoginOut.setVisibility(View.GONE);
//            unbind_device_head.setVisibility(View.GONE);
//            unbind_device_parent.setVisibility(View.GONE);
        }
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onDestroy() {
    	super.onDestroy();
    	UserManager.getInstance().removeLoginStateChangeListener(this);
    }
    
    @Override
    public void onChange(LoginState lastState, LoginState currentState, Object userdata) {
        log.debug("onChange lastState=" + lastState + ",currentState=" + currentState);
        invalidateLoginState();
        if (currentState == LoginState.COMMON && lastState == LoginState.LOGINED) {
            if (mProgressDialog.isShowing()) {
                Util.dismissDialog(mProgressDialog);
            }
            finish();
        }
    }
}
