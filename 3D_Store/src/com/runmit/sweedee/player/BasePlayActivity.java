/**
 * BasePlayActivity.java 
 * com.xunlei.share.player.BasePlayActivity
 * @author: zhanjin
 * @date: 2012-11-8 下午4:35:27
 */
package com.runmit.sweedee.player;

import java.text.NumberFormat;
import java.util.ArrayList;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.graphics.PixelFormat;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.sweedee.CaptionTextView;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.more.SettingItemActivity;
import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;
import com.runmit.sweedee.player.OnLinePlayMode.OnGetUrlListener;
import com.runmit.sweedee.player.ScreenObserver.ScreenStateListener;
import com.runmit.sweedee.player.TDGLSurfaceView.On3DStateChangeListener;
import com.runmit.sweedee.player.uicontrol.ResolutionView;
import com.runmit.sweedee.player.uicontrol.VideoCaptionManager;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.TDString;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.umeng.analytics.MobclickAgent;

/**
 */
public abstract class BasePlayActivity extends Activity {

	private XL_Log log = new XL_Log(BasePlayActivity.class);
	// 常量
	private final int SEEK_BAR_DEFAULT = 1000;// SeekBar的默认最大值
	public static final int START_SET_HINT_DIALOG = 100;
	private final int SET_LOCKVIEW_INVISIABLE = 11;
	private final int MSG_MOVE_LOCKLAYOUT = 12;
	private final int MSG_MOVE_DIALOG_RESOLUTION = 13;
	protected final int MSG_MOVE_DIALOG_CAPTION = 14;
	private final int MSG_HIDE_NAVIGATION = 15;
	private final int MSG_HIDE_DIALOG_SELECT_EPISODE = 16;

	// 播放界面相关
	private View mMainView;
	protected SeekBar mSeekBar;// 进度条
	protected TextView mPlaySpeed; // 在线点播缓存速度
	protected TextView mPlayedTime;// 已观看时长
	protected TextView mDurationTime;// 影片片长
	protected TextView mPlayTitle; // 影片名称
	protected LinearLayout mBufferingRL;// 缓冲等待框
	protected RelativeLayout mPlayerLayout;// 控制界面的容器
	protected ImageView mStartPausePlay;// 播放或暂停按钮
	protected Button mPlayerBack;// 返回按钮
	protected Button play_open_or_disable3d;

	protected TDGLSurfaceView mSurfaceView;// 系统播放器专用播放界面
	protected ImageView mIvLock;

	// 手势相关
	protected RelativeLayout mAdjustRL; // 音量，亮度调整布局容器
	protected LinearLayout mAdjustProRL;// 进度调整布局容器
	protected GestureDetector mGesture;
	protected GDListenerClass mGDListener;
	protected GestureHandler mGestureHandler = new GestureHandler();
	protected ImageView mAdjustIV;
	protected ImageView mAdjustProgress;
	protected ProgressBar mAdjustPBVol;
	protected ProgressBar mAdjustPBBri;
	protected TextView mAdjustProTime;
	protected TextView mAdjustProCHTime;
	protected long mMovieDuration = 0; // 影片总时长，单位：秒,播放库定义类型为long型
	protected int mPlayingTime;// 影片正在播放的时长，单位：秒

	protected int mHasPlayedTime = 0;

	protected CaptionTextView mCaptionTextView;
	ScreenObserver mScreenObserver;

	Button ivCurMode;

	protected String mBatterty = "";// 当前电量显示字符串

	protected VideoPlayMode mCurPlayMode; // 在线点播 ；本地播放

	protected ProgressDialog mWaittingDialog;// 启动时加载影片等待框

	protected int DIALOG_TIME_UP = 15 * 1000;
	public final static int MSG_DIALOG_WAITING = 1009;

	protected boolean mAutoBrightness = false;

	protected boolean mCanSeek = true;

	protected int mBeginTime;// 影片保存的时间进度
	protected int shouldBeginTime = 0;

	private PlayerBroadcastReceiver mBroadcastReceiver;// 处理网络状态改变的广播接收器

	private boolean mIsOnBack = false;// 判断播放activity是否在后台

	private LinearLayout layoutLock; // 加入的锁屏界面

	protected boolean is_screen_locked = false;
	private boolean hasVirtualKey = false;// 是否应该处理虚拟按键
	private boolean isPad = false;
	private boolean hasKitKat = Util.hasKitKat();// 是否是android 4.4以上版本

	protected SharePrefence mXLSharePrefence;

	protected VideoCaptionManager mVideoCaptionManager;
	protected ResolutionView resolutionView;

	protected PlayerAudioMgr mPlayerAudioMgr;
	protected boolean beforeScreenOff = false; // 为解决一进入锁屏没有暂停的处理

	// 屏幕模式
	public static final int ORIGIN_SIZE = 0;
	public static final int SUITABLE_SIZE = ORIGIN_SIZE + 1;
	public static final int FILL_SIZE = SUITABLE_SIZE + 1;
	protected int mScreenType = SUITABLE_SIZE;

	private final static int BRIGHT_NOT_SETTING = -1;
	private int initBrightAtAuto = BRIGHT_NOT_SETTING;// 播放器上次进入时候的亮度值，此记录是系统的亮度值，当我们退出播放器时候还原系统亮度值

	protected ReportPlayerData mReportPlayerData;
	protected long startActivityTime;
	protected long setUrlTime;
	protected long startPlayTime;

	protected boolean isFirstPlay = true;// 判断是否是第一次播放，决定是否显示提示浮层
	protected boolean isShowViewSign = false;
	protected final String IS_FIRST_PLAY = "is_first_play";

	private BroadcastReceiver mBatInfoReceiver;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		startActivityTime = System.currentTimeMillis();
		getLayoutInflater();
		mMainView = LayoutInflater.from(this).inflate(R.layout.player_new, null);

		setContentView(mMainView);
		hasVirtualKey = Util.hasVirtualKey(this);
		isPad = isPad();
		log.debug("onCreate hasVirtualKey = " + hasVirtualKey);
		mPlayerAudioMgr = new PlayerAudioMgr();
		mPlayerAudioMgr.requestAudioFocus(this);

		mXLSharePrefence = SharePrefence.getInstance(this);
		initViews();

		mBroadcastReceiver = new PlayerBroadcastReceiver();
		IntentFilter filter = new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
		filter.addAction(Intent.ACTION_BATTERY_CHANGED);
		filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
		filter.setPriority(IntentFilter.SYSTEM_HIGH_PRIORITY - 1);// 不能高过系统优先级
		this.registerReceiver(mBroadcastReceiver, filter);

		mVideoCaptionManager = new VideoCaptionManager(this);

		// 锁屏处理：对正在播放进行暂停
		mScreenObserver = new ScreenObserver(this);
		mScreenObserver.requestScreenStateUpdate(new ScreenStateListener() {
			@Override
			public void onScreenOn() {

			}

			@Override
			public void onScreenOff() {
				if (mSurfaceView != null && mSurfaceView.isPlaying()) {
					pausePlayer(true); // 云播放器操作
					SharePrefence.getInstance(BasePlayActivity.this).putBoolean(SharePrefence.KEY_SCREEN_PAUSE_PLAYER, true);
				}
			}
		});
		intoPlayer();
		// 网络太差的时候30秒后跳出框来处理
		if (mCurPlayMode != null && PlayerConstant.LOCAL_PLAY_MODE != mCurPlayMode.getPlayMode()) {
			mLayoutOperationHandler.postDelayed(mRunnableCloseDialog, DIALOG_TIME_UP);
		}
		// 电池温度监听 add by f0rest
		mBatInfoReceiver = new BroadcastReceiver() {
			public void onReceive(Context context, Intent intent) {
				String action = intent.getAction();
				if (Intent.ACTION_BATTERY_CHANGED.equals(action)) {
					double t = intent.getIntExtra("temperature", 0) * 0.1; // 电池温度
					log.debug("温度为" + (t * 0.1) + "℃");
					if (t >= 50.0) {
						ReportActionSender.getInstance().hotWoring(t, mSurfaceView.getEnable3DCoastTime());
					}
				}
			}
		};
		registerReceiver(mBatInfoReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
	}

	protected void showFirstToast() {
		isShowViewSign = true;
		final WindowManager mWindowManager = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
		LayoutInflater mLayoutInflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);
		final ViewGroup mViewFirstToast = (ViewGroup) mLayoutInflater.inflate(R.layout.play_view_angle_toast, null, true);
		final ImageView imageView_show_angle = (ImageView) mViewFirstToast.findViewById(R.id.imageView_show_angle);
		final ViewGroup show_handle_gusture = (ViewGroup) mViewFirstToast.findViewById(R.id.show_handle_gusture);
		final WindowManager.LayoutParams mParams = new WindowManager.LayoutParams();
		mParams.gravity = Gravity.FILL_VERTICAL | Gravity.FILL_HORIZONTAL;
		mParams.height = WindowManager.LayoutParams.MATCH_PARENT;
		mParams.width = WindowManager.LayoutParams.MATCH_PARENT;
		mParams.flags = WindowManager.LayoutParams.FLAG_FULLSCREEN;
		mParams.format = PixelFormat.TRANSLUCENT;
		try {
			mWindowManager.addView(mViewFirstToast, mParams);
			mWindowManager.updateViewLayout(mViewFirstToast, mParams);
		} catch (Exception e) {
			e.printStackTrace();
		} catch (Error e) {
			// TODO: handle exception
		}

		imageView_show_angle.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				imageView_show_angle.setVisibility(View.GONE);
				show_handle_gusture.setVisibility(View.VISIBLE);
			}
		});

		show_handle_gusture.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View v) {
				mWindowManager.removeView(mViewFirstToast);
				mXLSharePrefence.putBoolean(IS_FIRST_PLAY, false);
				isFirstPlay = false;
				isShowViewSign = false;

				if (!mSurfaceView.isPlaying()) {
					resumePlayer();
				}
			}
		});
	}

	private void intoPlayer() {
		Intent intent = getIntent();
		initPlayMode(intent);
	}

	/**
	 * 对于没有物理按键的手机，处理虚拟导航栏的逻辑（4.4KikKat可以完全隐藏掉）
	 */
	@SuppressLint("NewApi")
	protected void hideNavigationBar() {
		if (!hasVirtualKey) {
			return;
		}
		if (!isPad) {
			mMainView.setSystemUiVisibility(uiOption);
		} else {
			mMainView.setSystemUiVisibility(padUiOption);
		}
	}

	@SuppressLint("InlinedApi")
	static final int padUiOption = View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
	// View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
			View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

	@SuppressLint("InlinedApi")
	static final int uiOption = padUiOption | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;

	@SuppressLint("NewApi")
	protected void showNavigationBar() {
		if (!Util.isSpecialDeviceOfVirtualKey()) {
			return;
		} else {
			mMainView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_VISIBLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_FULLSCREEN
					| View.SYSTEM_UI_LAYOUT_FLAGS);
		}
	}

	private boolean isPad() {
		WindowManager wm = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
		Display display = wm.getDefaultDisplay();
		// 屏幕宽度
		float screenWidth = display.getWidth();
		// 屏幕高度
		float screenHeight = display.getHeight();
		DisplayMetrics dm = new DisplayMetrics();
		display.getMetrics(dm);
		double x = Math.pow(dm.widthPixels / dm.xdpi, 2);
		double y = Math.pow(dm.heightPixels / dm.ydpi, 2);
		// 屏幕尺寸
		double screenInches = Math.sqrt(x + y);
		log.debug("isPad screenInches = " + screenInches);
		// 大于6尺寸则为Pad
		if (screenInches >= 6.0) {
			return true;
		}
		return false;
	}

	@Override
	protected void onPause() {
		super.onPause();
		mIsOnBack = true;
		MobclickAgent.onPause(this);
		startAutoBrightness();
	}

	@Override
	protected void onResume() {
		super.onResume();

		if (is_screen_locked) {
			mPlayerLayout.setVisibility(View.GONE);
		}

		mLayoutOperationHandler.postDelayed(new Runnable() {
			@Override
			public void run() {
				hideNavigationBar();
			}
		}, 500);
		mIsOnBack = false;
		MobclickAgent.onResume(this);
		setBrightness();
	}

	private void disssmissAllDialog() {
		// 对话框关闭以防没有关闭
		if (resolutionView != null) {
			resolutionView.dismiss();
		}
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		mGDListener.saveCurBright(); // 保存当前用户使用的亮度
		mLayoutOperationHandler.removeCallbacks(mRunnableCloseDialog);
		this.unregisterReceiver(mBroadcastReceiver);
		disssmissAllDialog();
		if (mScreenObserver != null) {
			mScreenObserver.stopScreenStateUpdate();
			mScreenObserver = null;
		}
	}

	private void initViews() {
		// 初始化界面控制View
		mPlayerLayout = (RelativeLayout) findViewById(R.id.play_view_info);
		mStartPausePlay = (ImageView) findViewById(R.id.play_start_pause_gnr);
		mPlayerBack = (Button) findViewById(R.id.play_back_gnr);
		play_open_or_disable3d = (Button) findViewById(R.id.play_open_or_disable3d);
		mBufferingRL = (LinearLayout) findViewById(R.id.wait_bar_gnr);

		mSurfaceView = (TDGLSurfaceView) findViewById(R.id.play_surface_view);
		mIvLock = (ImageView) findViewById(R.id.iv_lock);

		mSeekBar = (SeekBar) findViewById(R.id.seek_bar_gnr);

		mPlaySpeed = (TextView) findViewById(R.id.play_speed_gnr);
		mPlayedTime = (TextView) findViewById(R.id.play_playtime_gnr);
		mDurationTime = (TextView) findViewById(R.id.play_playduration_gnr);
		mPlayTitle = (TextView) findViewById(R.id.play_title_text_gnr);

		mPlayTitle.setOnClickListener(mOperateBtnListener);

		ivCurMode = (Button) findViewById(R.id.iv_play_video_mode);
		ivCurMode.setOnClickListener(mOperateBtnListener);
		// 设置相关控制View
		mSeekBar.setMax(SEEK_BAR_DEFAULT);
		mSeekBar.setOnSeekBarChangeListener(mSeekBarListener);
		mStartPausePlay.setOnClickListener(mOperateBtnListener);
		mPlayerBack.setOnClickListener(mOperateBtnListener);
		play_open_or_disable3d.setOnClickListener(mOperateBtnListener);

		mIvLock.setOnClickListener(mOperateBtnListener);
		// 初始化手势控制View
		mAdjustRL = (RelativeLayout) findViewById(R.id.supreme_adjust);
		mAdjustIV = (ImageView) findViewById(R.id.adjustViewIcon);
		mAdjustProgress = (ImageView) findViewById(R.id.iv_progress);
		mAdjustProRL = (LinearLayout) findViewById(R.id.adJustProgressRL);
		mAdjustProTime = (TextView) findViewById(R.id.adJustProgressPlayTime);
		mAdjustProCHTime = (TextView) findViewById(R.id.adJustProgressChangeTime);
		mAdjustPBVol = (ProgressBar) findViewById(R.id.adjustPorgressBarVol);
		mAdjustPBBri = (ProgressBar) findViewById(R.id.adjustPorgressBarBri);
		// 设置手势监听

		mGDListener = new GDListenerClass(this, mGestureHandler, mXLSharePrefence.getFloat(SharePrefence.KEY_SCREEN_BRIGHT_RECORD, GDListenerClass.FLAG_USER_NOT_SETTING_BRIGHT));
		mGesture = new GestureDetector(this, mGDListener);

		mCaptionTextView = (CaptionTextView) findViewById(R.id.captionTextView);

		layoutLock = (LinearLayout) findViewById(R.id.layout_lock);

		findViewById(R.id.player_header_info).setOnClickListener(mOperateBtnListener); // 标题栏
		findViewById(R.id.play_info).setOnClickListener(mOperateBtnListener); // 底部菜单栏

		mSurfaceView.setOn3DStateChangeListener(new On3DStateChangeListener() {

			@Override
			public void onChange(final boolean isenable3d) {
				mLayoutOperationHandler.post(new Runnable() {

					@Override
					public void run() {
						play_open_or_disable3d.setText(isenable3d ? R.string.open_three_d : R.string.disable_three_d);
					}
				});
			}
		});
	}

	private void initPlayMode(Intent intent) {
		mCurPlayMode = PlayModeFactory.createPlayModeObj(this, intent);
		String fileName = mCurPlayMode.getFileName();
		log.debug("fileName=" + fileName + ",mPlayTitle=" + mPlayTitle);
		mPlayTitle.setText(fileName.trim()); // 设置标题(影片名称)
		resolutionView = new ResolutionView(ivCurMode, this, mCurPlayMode);
		if (mCurPlayMode.isCloudPlay()) {
			initResolutionBar();
		}
		loadVideoCaption();
		int online = mCurPlayMode.isCloudPlay() ? 1 : 2;
		mReportPlayerData = new ReportPlayerData(fileName, mCurPlayMode.getVideoid(), online);
		mReportPlayerData.setResolution(mCurPlayMode.getUrl_type());
		mReportPlayerData.setBlockReason(2);

		mSurfaceView.setMode(mCurPlayMode.getMode());
		Log.d("Mode", "mode = " + mCurPlayMode.getMode());
	}

	private void initResolutionBar() {
		resolutionView.setOnChangePlayModule(new ResolutionView.OnChangePlayModule() {
			@Override
			public void onPlayModule(int type) {
				resolutionView.setMiniMode();
				if (mCurPlayMode != null && type == mCurPlayMode.getUrl_type()) {
					return;
				} else if (mCurPlayMode != null && !mCurPlayMode.hasPlayUrlByType(type)) {
					return;
				}
				mLayoutOperationHandler.removeCallbacks(mRunnableCloseDialog);
				if (mCurPlayMode != null && PlayerConstant.LOCAL_PLAY_MODE != mCurPlayMode.getPlayMode()) {
					mLayoutOperationHandler.postDelayed(mRunnableCloseDialog, DIALOG_TIME_UP);
				}

				mReportPlayerData.changeSolutions(mCurPlayMode.getUrl_type(), type);
				switchVideoResolution(type, getString(R.string.player_switching_video));
			}
		});

	}

	private void loadVideoCaption() {
		ArrayList<SubTitleInfo> mSubTitleInfos = mCurPlayMode.getSubTitleInfos();

		if (mSubTitleInfos == null || mSubTitleInfos.isEmpty()) {
			return;
		}
		mVideoCaptionManager.loadVideoCaption(mSubTitleInfos, new VideoCaptionManager.OnCaptionSelect() {
			@Override
			public void onLoadSuccess(final String localPath) {
				mLayoutOperationHandler.post(new Runnable() {
					@Override
					public void run() {
						mCaptionTextView.setVideoCaptionPath(localPath);
					}
				});
			}

			@Override
			public void close() {
				mCaptionTextView.close();
			}
		});
	}

	private void trigScreenLock() {
		// log.error("trigScreenLock is_screen_locked = "+is_screen_locked);
		if (is_screen_locked) {
			is_screen_locked = false;
			mIvLock.setImageResource(R.drawable.lock_no);
			mPlayerLayout.setVisibility(View.VISIBLE);
			disappearInSeconds(6000);
		} else {
			is_screen_locked = true;
			mIvLock.setImageResource(R.drawable.lock_yes);
			mPlayerLayout.setVisibility(View.GONE);

			// 将所有对话框消失
			if (resolutionView != null) {
				resolutionView.dismiss();
			}
		}
	}

	private void setPlayerViewAndLockViewVisiable(boolean visiable) {
		if (visiable) {
			mPlayerLayout.setVisibility(View.VISIBLE);
			if (!is_screen_locked) {
				mIvLock.setVisibility(View.VISIBLE);
				mLayoutOperationHandler.removeMessages(SET_LOCKVIEW_INVISIABLE);
			}
		} else {
			if (resolutionView != null && !resolutionView.isShowing()) {
				mPlayerLayout.setVisibility(View.GONE);
			}

			mIvLock.setVisibility(View.GONE);
			mLayoutOperationHandler.removeMessages(SET_LOCKVIEW_INVISIABLE);
		}
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		if (event.getAction() == MotionEvent.ACTION_DOWN) {
			showNavigationBar();
		}
		log.debug("onTouchEvent action = " + event.getAction());
		if (hasVirtualKey && !hasKitKat && event.getAction() == MotionEvent.ACTION_UP) {
			mLayoutOperationHandler.sendEmptyMessageDelayed(MSG_HIDE_NAVIGATION, 600);// 600毫秒后发送隐藏状态栏的消息
		}
		// 处理锁屏状态
		if (is_screen_locked) {
			if (event.getAction() == MotionEvent.ACTION_DOWN) {
				if (mIvLock.getVisibility() == View.VISIBLE) {
					mIvLock.setVisibility(View.GONE);
					mLayoutOperationHandler.removeMessages(SET_LOCKVIEW_INVISIABLE);
				} else {
					mIvLock.setVisibility(View.VISIBLE);
					mLayoutOperationHandler.sendEmptyMessageDelayed(SET_LOCKVIEW_INVISIABLE, 6000);
				}
			}
			return true;
		}

		// 高清等分辨率对话框消失
		disssmissAllDialog();

		boolean result = mGesture.onTouchEvent(event);
		if (!result) {
			if (event.getAction() == MotionEvent.ACTION_UP) {
				disappearInSeconds(6000);
				if (View.VISIBLE == mPlayerLayout.getVisibility()) {
					setPlayerViewAndLockViewVisiable(false);
				} else {
					setPlayerViewAndLockViewVisiable(true);
				}
				if (resolutionView != null) {
					resolutionView.setMiniMode();
				}
				mAdjustRL.setVisibility(View.GONE);// 确保onfling没调用时也会隐藏
				mGDListener.hideLayout();
				mAdjustProRL.setVisibility(View.GONE);
			}
		}
		return super.onTouchEvent(event);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent keyEvent) {
		boolean ret = mGDListener.consumeVolumeKeyDown(keyCode, keyEvent);
		log.debug("onKeyDown ret=" + ret + ",is_screen_locked=" + is_screen_locked);
		if (!ret) {
			if (KeyEvent.KEYCODE_BACK == keyCode) {
				if (is_screen_locked) {
					// Util.showToast(this,"屏幕已锁定，请先解锁", Toast.LENGTH_SHORT);
					layoutLock.setVisibility(View.VISIBLE);
					if (mIvLock.getVisibility() != View.VISIBLE) {
						mIvLock.setVisibility(View.VISIBLE);
						mLayoutOperationHandler.sendEmptyMessageDelayed(SET_LOCKVIEW_INVISIABLE, 6000);
					}
					mLayoutOperationHandler.sendEmptyMessageDelayed(MSG_MOVE_LOCKLAYOUT, 2000);
				} else {
					exit();
				}
			}
		}
		return true;
	}

	protected boolean isUserPauseVideo = false;

	public void manualPlayOrPause() // 通过手工操作播放界面的方式来改变播放状态
	{

		if ((mSurfaceView != null && mSurfaceView.isPlaying())) {
			isUserPauseVideo = true;
			pausePlayer(false);
		} else {
			isUserPauseVideo = false;// 此處可以理解為用戶關閉的必須用戶打開，所以只在此改變開關
			resumePlayer();
		}
	}

	/**
	 * 打开或者关闭3D功能
	 */
	public void manualOpenOrDisable3D() {
		if ((mSurfaceView != null && mSurfaceView.isEnable3D())) {
			mSurfaceView.disable3D(true);
		} else {
			mSurfaceView.enable3D(true);
		}
	}

	protected void disappearInSeconds(int time) {
		mLayoutOperationHandler.removeMessages(PlayerConstant.VideoPlayerStatus.PLAYER_HIDE_CONTROL_BAR);
		mLayoutOperationHandler.sendEmptyMessageDelayed(PlayerConstant.VideoPlayerStatus.PLAYER_HIDE_CONTROL_BAR, time);
	}

	protected void saveVideoPlayInfo() {
		if (mHasPlayedTime > 0) {// 保存进度的条件是本次播放有效
			log.debug("saveVideoPlayInfo--->start to save" + ",mPlayingTime = " + mPlayingTime);
			if (mPlayingTime >= (mMovieDuration - 5)) {// 如果播放到最号的5秒钟内则保存为开头
				mPlayingTime = 0;
			}

			if (mCurPlayMode != null) {
				mCurPlayMode.setPlayedTime(mPlayingTime);
				mCurPlayMode.saveVideoInfo(mMovieDuration);
			}
		}

	}

	protected void setVideoBeginTime() {
		if (mBeginTime == 0 && mCurPlayMode != null) {
			mBeginTime = mCurPlayMode.getPlayedTime();
		}
		log.error("setVideoBeginTime, mBeginTime = " + mBeginTime);
	}

	// unit_type： 0表示KB，1表示B
	protected String convertNetSpeed(int speed, int unit_type) {
		int speedKB = 0;
		if (unit_type == 0) {
			speedKB = speed;
		} else if (unit_type == 1) {
			speedKB = speed / 1024;
		}
		if (speedKB > 1024) {
			float sp = (float) speedKB / 1024f;
			NumberFormat format = NumberFormat.getNumberInstance();
			format.setMaximumFractionDigits(2);
			String s = format.format(sp);
			return s + "MB/S";
		} else {
			return speedKB + "KB/S";
		}
	}

	// 判断系统是否开启了自动亮度调节
	public boolean isAutoBrightnesOpened(Activity activity) {
		boolean res = false;
		try {
			res = Settings.System.getInt(activity.getContentResolver(), Settings.System.SCREEN_BRIGHTNESS_MODE) == Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC;
		} catch (SettingNotFoundException e) {
			log.error("catch a SettingNotFoundException");
			e.printStackTrace();
		}
		return res;
	}

	// 开启自动亮度调节
	private void startAutoBrightness() {
		log.debug("startAutoBrightness=" + mAutoBrightness + ",initBrightAtAuto=" + initBrightAtAuto);
		if (mAutoBrightness) {// 恢复用户的自动亮度设置
			getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
			Settings.System.putInt(getContentResolver(), Settings.System.SCREEN_BRIGHTNESS_MODE, Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC);
		}

		if (initBrightAtAuto != BRIGHT_NOT_SETTING) {
			Settings.System.putInt(getContentResolver(), Settings.System.SCREEN_BRIGHTNESS, initBrightAtAuto);
		}
	}

	/**
	 * 获取自动亮度下当前屏幕的亮度值
	 * 
	 * @return
	 */
	private void setAutoScreenBrightnessLight() {
		try {
			final SensorManager mSensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
			Sensor mLight = mSensorManager.getDefaultSensor(Sensor.TYPE_LIGHT);
			SensorEventListener mSensorEventListener = new SensorEventListener() {
				@Override
				public void onSensorChanged(SensorEvent event) {
					if (event.sensor.getType() == Sensor.TYPE_LIGHT) {
						float X_lateral = event.values[0];
						log.debug("onSensorChanged=" + X_lateral);
						int autoScreenBrightnessLight = 0;
						if (X_lateral < SensorManager.LIGHT_CLOUDY)
							autoScreenBrightnessLight = (120);
						else if (X_lateral < SensorManager.LIGHT_SUNRISE)
							autoScreenBrightnessLight = (150);
						else if (X_lateral < SensorManager.LIGHT_OVERCAST)
							autoScreenBrightnessLight = (180);
						else if (X_lateral < SensorManager.LIGHT_SHADE)
							autoScreenBrightnessLight = (200);
						else if (X_lateral < SensorManager.LIGHT_SUNLIGHT)
							autoScreenBrightnessLight = (230);
						else
							autoScreenBrightnessLight = (255);

						initBrightAtAuto = Settings.System.getInt(getContentResolver(), Settings.System.SCREEN_BRIGHTNESS, 255);

						mGDListener.setScreenLight(autoScreenBrightnessLight);
						Settings.System.putInt(getContentResolver(), Settings.System.SCREEN_BRIGHTNESS_MODE, Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);
						mSensorManager.unregisterListener(this);
					}
				}

				@Override
				public void onAccuracyChanged(Sensor sensor, int accuracy) {
				}
			};
			mSensorManager.registerListener(mSensorEventListener, mLight, SensorManager.SENSOR_DELAY_NORMAL);
		} catch (Exception e) {
			e.printStackTrace();
			log.debug("setAutoScreenBrightnessLight=" + e.getMessage());
			mGDListener.setScreenLight(0);
			Settings.System.putInt(getContentResolver(), Settings.System.SCREEN_BRIGHTNESS_MODE, Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);
		}
	}

	protected Handler mLayoutOperationHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			log.debug("handler recv msg.what=" + msg.what);
			switch (msg.what) {
			case PlayerConstant.VideoPlayerStatus.PLAYER_HIDE_WAITING_BAR:
				log.debug("PLAYER_HIDE_WAITING_BAR");
				showLoadingState(false, 0);
				break;
			case PlayerConstant.VideoPlayerStatus.PLAYER_SHOW_CONTROL_BAR:
				if (resolutionView != null) {
					resolutionView.setMiniMode();// 已弃用
				}
				setPlayerViewAndLockViewVisiable(true);
				break;
			case PlayerConstant.VideoPlayerStatus.PLAYER_HIDE_CONTROL_BAR:
				setPlayerViewAndLockViewVisiable(false);
				if (resolutionView != null) {
					resolutionView.setMiniMode();
				}
				if (View.VISIBLE == mAdjustProRL.getVisibility()) {
					mAdjustProRL.setVisibility(View.GONE);
				}
				break;
			case SET_LOCKVIEW_INVISIABLE:
				mIvLock.setVisibility(View.GONE);
				break;
			case MSG_MOVE_LOCKLAYOUT:
				layoutLock.setVisibility(View.GONE);
				break;
			case MSG_MOVE_DIALOG_RESOLUTION:
				mPlayerLayout.setVisibility(View.GONE);
				if (resolutionView != null) {
					resolutionView.dismiss();
				}
				break;
			case MSG_MOVE_DIALOG_CAPTION:
				mPlayerLayout.setVisibility(View.GONE);
				break;
			case MSG_HIDE_NAVIGATION:
				hideNavigationBar();
				break;
			}
		}
	};

	private long mLastSeekTime = 0;

	private boolean isSeeking = false;

	/**
	 * 设置拖动时间，从缓冲结束算起
	 * 
	 * @param seek
	 */
	public void setSeeking(boolean seek) {
		this.isSeeking = seek;
		mLastSeekTime = System.currentTimeMillis();
	}

	private SeekBar.OnSeekBarChangeListener mSeekBarListener = new SeekBar.OnSeekBarChangeListener() {

		private String willPlayTime = null;

		@Override
		public void onStopTrackingTouch(SeekBar seekBar) {
			String hardware = android.os.Build.HARDWARE;
			long timeDuration = System.currentTimeMillis() - mLastSeekTime;

			log.debug("hardware=" + hardware + ",isSeeking=" + isSeeking + ",timeDuration=" + timeDuration);
			if (hardware != null && hardware.startsWith("mt") && (isSeeking || timeDuration < 200)) {
				mSeekBar.setProgress(mPlayingTime);
				return;
			}
			isSeeking = true;
			int time = seekBar.getProgress();
			mReportPlayerData.setBlockReason(3);
			mReportPlayerData.playSeek(time * 1000 > mSurfaceView.getCurrentPosition() ? 1 : 0);
			seekToBySecond(time, true);
			willPlayTime = null;
			if (View.VISIBLE == mAdjustProRL.getVisibility()) {
				mAdjustProRL.setVisibility(View.GONE);
			}
			disappearInSeconds(3000);
		}

		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {
			mLayoutOperationHandler.removeMessages(SET_LOCKVIEW_INVISIABLE);
			mLayoutOperationHandler.removeMessages(PlayerConstant.VideoPlayerStatus.PLAYER_HIDE_CONTROL_BAR);
			setPlayerViewAndLockViewVisiable(true);
		}

		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
			willPlayTime = Util.getFormatTime(progress);
			mPlayedTime.setText(willPlayTime);
		}
	};

	/**
     *
     */
	protected void onStart() {
		super.onStart();
		log.debug("baseplayactivit onstart");
	}

	// 界面按钮类控件的监听器
	View.OnClickListener mOperateBtnListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			switch (v.getId()) {
			case R.id.play_start_pause_gnr:// 开始暂停按钮
				manualPlayOrPause();
				break;
			case R.id.iv_play_video_mode:
				if (!resolutionView.isShowing()) {
					disssmissAllDialog();
					resolutionView.showChoiceMode(v);
					setPlayerViewAndLockViewVisiable(true);
					disappearInSeconds(6000);
					mLayoutOperationHandler.removeMessages(MSG_MOVE_DIALOG_RESOLUTION);
					mLayoutOperationHandler.sendEmptyMessageDelayed(MSG_MOVE_DIALOG_RESOLUTION, 6000);
				} else {
					resolutionView.dismiss();
				}
				break;
			case R.id.play_back_gnr:
				exit();
				break;
			case R.id.iv_lock:
				trigScreenLock();
				break;
			case R.id.player_header_info:
			case R.id.play_info:
				setPlayerViewAndLockViewVisiable(true);
				disappearInSeconds(6000);
				break;
			case R.id.play_open_or_disable3d:
				manualOpenOrDisable3D();
				break;
			default:
				break;
			}
		}
	};

	class PlayerBroadcastReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			if (AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(intent.getAction())) {// 耳机插拔广播
				if (mSurfaceView != null && mSurfaceView.isPlaying()) {
					pausePlayer(false);
				}
			} else if (ConnectivityManager.CONNECTIVITY_ACTION.equals(intent.getAction())) {
				// 本地播放模式忽略网络切换
				if (mCurPlayMode != null && mCurPlayMode.isLocalPlay()) {
					return;
				}
				// 如果现在当前是3G网络
				boolean isMobileNet = Util.isMobileNet(context);
				log.debug("isMobileNet=" + isMobileNet);
				if (isMobileNet) {
					boolean canPlayIn3G = SharePrefence.getInstance(context).getBoolean(SettingItemActivity.MOBILE_PLAY, false);
					if (!canPlayIn3G) {// 移动网络下面不可播放
						if (mIsOnBack) {
							Util.showToast(BasePlayActivity.this, getString(R.string.player_exit_in_3g), Toast.LENGTH_LONG);
						} else if (!mIsOnBack) {
							Util.showToast(BasePlayActivity.this, getString(R.string.player_exit_in_wifi_to_3g), Toast.LENGTH_LONG);
						}
						exit();
					} else {// 移动网络下面可以点播
						Util.showToast(BasePlayActivity.this, getString(R.string.player_in_3g), Toast.LENGTH_LONG);
					}
				}
			} else if (Intent.ACTION_BATTERY_CHANGED.equals(intent.getAction())) {
				int current = intent.getExtras().getInt("level");// 获得当前电量
				int total = intent.getExtras().getInt("scale");// 获得总电量
				float percent = current / (total * 1.00f);
				mBatterty = TDString.getStr(R.string.battery) + percent + "%";
			}
		}
	}

	private void setBrightness() {
		mAutoBrightness = isAutoBrightnesOpened(this);
		log.debug("setBrightness mAutoBrightness=" + mAutoBrightness);
		mLayoutOperationHandler.postDelayed(new Runnable() {
			@Override
			public void run() {
				if (isFinishing()) {
					return;
				}
				if (mAutoBrightness) {// 如果设置了自动亮度
					setAutoScreenBrightnessLight();
				} else {
					// 没有设置自动亮度,则第一次使用系统亮度
					initBrightAtAuto = Settings.System.getInt(getContentResolver(), Settings.System.SCREEN_BRIGHTNESS, 255);
					mGDListener.setScreenLight(initBrightAtAuto);
				}
			}
		}, 1000);
	}

	class GestureHandler implements GDListenerClass.ConsumeGesture {
		@Override
		public void adjustBrightness(LayoutParams wl, int maxLevel, int level) {
			if (mAdjustRL.getVisibility() != View.VISIBLE) {
				mAdjustRL.setVisibility(View.VISIBLE);
				mAdjustPBBri.setMax(maxLevel);
				mAdjustPBVol.setVisibility(View.GONE);
				mAdjustPBBri.setVisibility(View.GONE);
				mAdjustIV.setImageResource(R.drawable.adjust_brightness);
			}

			setVoiceOrBrightProgress(mAdjustPBBri, level);
			getWindow().setAttributes(wl);

		}

		@Override
		public void adjustVolume(AudioManager am, int maxLevel, int level) {
			if (mAdjustRL.getVisibility() != View.VISIBLE) {
				mAdjustRL.setVisibility(View.VISIBLE);
				mAdjustPBVol.setVisibility(View.GONE);
				mAdjustPBBri.setVisibility(View.GONE);
				mAdjustPBVol.setMax(maxLevel);
			}

			mAdjustIV.setImageResource(R.drawable.ic_volume);
			setVoiceOrBrightProgress(mAdjustPBVol, level);
		}

		boolean isAdjustprogress = false;
		String willPlayTime = null;
		int changeTime = 0;
		int p_time = -1;

		@Override
		public void adjustPlayProgress(int adjustTime) {
			if (p_time == -1)
				p_time = mPlayingTime;
			isAdjustprogress = true;
			// pausePlayer();
			p_time += adjustTime;
			if (p_time < 0) {// 确定时长在正确的范围内
				p_time = 0;
			} else if (p_time > mMovieDuration) {
				p_time = (int) mMovieDuration;
			} else {
				changeTime += adjustTime;
			}
			mSeekBar.setProgress(p_time);
			willPlayTime = Util.getFormatTime(p_time);
			mPlayedTime.setText(willPlayTime);
			mAdjustProTime.setText(willPlayTime);
			mAdjustProCHTime.setText(Util.getFormatTimeProgressChange(changeTime));
			if (View.VISIBLE != mPlayerLayout.getVisibility()) {
				// setPlayerViewAndLockViewVisiable(true);
			}
			if (View.VISIBLE != mAdjustProRL.getVisibility()) {
				mAdjustProRL.setVisibility(View.VISIBLE);
			}
		}

		@Override
		public void hideLayout() {
			// log.error("hideLayout isAdjustprogress = "+isAdjustprogress);
			if (View.VISIBLE == mAdjustRL.getVisibility()) {
				mAdjustRL.setVisibility(View.GONE);
			}

			mAdjustProRL.setVisibility(View.GONE);

			if (View.VISIBLE == mPlayerLayout.getVisibility()) {
				disappearInSeconds(500);
			}
			if (isAdjustprogress) {

				seekToBySecond(p_time, true);
				// resumePlayer();
				isAdjustprogress = false;

				p_time = -1;
			}
			willPlayTime = null;
			changeTime = 0;
		}
	}

	private int[] progressResIds = new int[] { R.drawable.volume_0, R.drawable.volume_1, R.drawable.volume_2, R.drawable.volume_3, R.drawable.volume_4, R.drawable.volume_5, R.drawable.volume_6,
			R.drawable.volume_7, R.drawable.volume_8, R.drawable.volume_9, R.drawable.volume_10 };

	private void setVoiceOrBrightProgress(ProgressBar progressBar, int curLevel) {
		log.debug("setVoiceOrBrightProgress curLevel =" + curLevel);
		if (curLevel >= progressResIds.length) // LJD-BUG:后续修改
			return;

		progressBar.setMax(progressResIds.length - 1);
		progressBar.setProgress(curLevel);
		mAdjustProgress.setImageResource(progressResIds[curLevel]);
	}

	protected void showLoadingState(boolean isShow, int percent) {
		log.debug("showLoadingState isShow=" + isShow + ",percent=" + percent);
		if (isShow) {
			mBufferingRL.setVisibility(View.VISIBLE);
		} else {
			mBufferingRL.setVisibility(View.GONE);
		}
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);

		if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
			// 屏幕加锁时暂停了播放，则要重新启动播放

			if (mXLSharePrefence.getBoolean(SharePrefence.KEY_SCREEN_PAUSE_PLAYER, false)) {
				mXLSharePrefence.putBoolean(SharePrefence.KEY_SCREEN_PAUSE_PLAYER, false);
				resumePlayer();

			} else if (beforeScreenOff) {
				resumePlayer();
			}
		}

	}

	// 提示框，如缓冲等待太久，加载时间过长，网络状况不佳
	protected CloseDialogTask mRunnableCloseDialog = new CloseDialogTask();

	class CloseDialogTask implements Runnable {

		AlertDialog dialog = null;

		public void run() {
			final OnLinePlayMode mOnLinePlayMode = (OnLinePlayMode) mCurPlayMode;
			boolean hasNextSource = mOnLinePlayMode.hasResourceForResolution();
			if (hasNextSource) {// 如果有下一个片源，则直接切换
				mOnLinePlayMode.loadNextIndexUrlByCurrentResolution(new OnGetUrlListener() {

					@Override
					public void onGetUrlFinish(String mPlayUrl) {
						log.debug("mPlayUrl=" + mPlayUrl);
						mReportPlayerData.setBlockReason(5);
						mReportPlayerData.changeCdn();
						switchVideoUrl(mPlayUrl);
						mLayoutOperationHandler.removeCallbacks(mRunnableCloseDialog);
						mLayoutOperationHandler.postDelayed(mRunnableCloseDialog, DIALOG_TIME_UP);
					}
				});
			} else {// 否则，提示对话框
				AlertDialog.Builder builder = new AlertDialog.Builder(BasePlayActivity.this, AlertDialog.THEME_DEVICE_DEFAULT_DARK);
				builder.setTitle(Util.setColourText(BasePlayActivity.this, R.string.confirm_title, Util.DialogTextColor.BLUE));
				builder.setMessage(TDString.getStr(R.string.paly_net_is_bad_and_wait_tip));
				builder.setPositiveButton(TDString.getStr(R.string.wait), new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						log.debug("onClick");
					}
				});
				builder.setNegativeButton(Util.setColourText(BasePlayActivity.this, R.string.real_quit, Util.DialogTextColor.BLUE), new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						exit();
					}
				});
				dialog = builder.setCancelable(false).create();
				dialog.show();
			}
		}
	}

	private boolean hasToasted = false;

	protected void toastShowLastPlayTime(int second) {
		log.debug("toastShowLastPlayTime=" + second);
		if (hasToasted) {
			return;
		}
		if (second > 0) {
			String lastPlayTip = TDString.getStr(R.string.play_last_play_to) + Util.getFormatTime(second);
			Util.showToast(this, lastPlayTip, Toast.LENGTH_SHORT);
		}
		hasToasted = true;
	}

	protected void showWaittingDialog(String tipStr) {
		if (mWaittingDialog == null) {
			mWaittingDialog = new ProgressDialog(this, AlertDialog.THEME_DEVICE_DEFAULT_DARK);
		}
		mWaittingDialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
			@Override
			public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
				if (keyCode == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN) {
					exit();
					dialog.dismiss();
				}
				return false;
			}
		});
		mWaittingDialog.setMessage(tipStr);
		mWaittingDialog.setCancelable(true);
		mWaittingDialog.setCanceledOnTouchOutside(false);
		mWaittingDialog.show();
	}

	protected void dismissDialog() {
		if (null != mWaittingDialog) {
			mWaittingDialog.dismiss();
		}
	}

	protected abstract void resumePlayer();// 继续播放

	protected abstract void pausePlayer(boolean isdisable3d);// 暂停播放

	protected abstract void seekToBySecond(int whichTimeToPlay, boolean isDrag);// 跳到指定时间，单位：秒

	protected abstract void exit();// 退出

	protected abstract void continueAfterShowTip();// 第一次提示完成后调用方法

	protected abstract void switchVideoResolution(int type, String tipStr);// 切换影片清晰度

	protected abstract void switchVideoUrl(String mPlayUrl);// 切换影片播放地址
}
