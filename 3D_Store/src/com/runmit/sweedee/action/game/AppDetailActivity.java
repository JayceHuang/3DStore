package com.runmit.sweedee.action.game;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.graphics.drawable.BitmapDrawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.FragmentTransaction;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.RoundedBitmapDisplayer;
import com.nostra13.universalimageloader.core.display.SimpleBitmapDisplayer;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.action.search.Activity3DEffectTool;
import com.runmit.sweedee.action.search.DefaultEffectFaces;
import com.runmit.sweedee.action.search.FlipViewGroup;
import com.runmit.sweedee.download.DownloadCreater;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadInfo;
import com.runmit.sweedee.download.DownloadManager;
import com.runmit.sweedee.download.DownloadManager.OnDownloadDataChangeListener;
import com.runmit.sweedee.download.OnStateChangeListener;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.util.ApkInstaller;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.HorizontalListView;
import com.tdviewsdk.activity.transforms.AnimActivity;

/**
 * 游戏和应用详情页面
 */
public class AppDetailActivity extends AnimActivity implements View.OnClickListener, StoreApplication.OnNetWorkChangeListener, AdapterView.OnItemClickListener {

	public static final String APP_ID = "appid";
	public static final String APP_DATA = "app_data";
	private int appid = 16;

	static XL_Log log = new XL_Log(AppDetailActivity.class);

	private TextView btn_download;
	private ImageButton btn_actionbar_return;
	private ProgressBar download_progress_bar;
	private RelativeLayout btn_download_rl;
	private TextView app_detail_title_tv;
	private TextView download_message_tv;
	private TextView file_size_tv;
	private TextView app_news_tv;
	private View app_news_ll;
	private TextView app_intro_tv;
	private TextView app_type_tv;
	private TextView app_developer_tv;
	private TextView app_update_tv;
	private TextView app_version_tv;
	private TextView app_type2_tv;
	private TextView app_file_size_tv;
	private ImageView show_all_msg_click_iv;
	private ImageView mBlurImageView;
	private HorizontalListView screen_shots_list;
	private LinearLayout popupll;
	private PopupShortcutFrag fragment;
	private ScreenShotsAdapter ssAdapter;
	private AppItemInfo mAppInfo;

	private AppDownloadInfo mDownloadInfo;
	
	private DisplayImageOptions mOptions;

	private Handler mHandler = new Handler(){
        public void handleMessage(Message msg) {
	        switch(msg.what){
	        case FlipViewGroup.MSG_ANIMATOIN_FIN:
	        	Activity3DEffectTool.isLaunching = false;
	        	break;
	        }
        }};

	private boolean isAnimLaunch = false;

	private OnDownloadDataChangeListener mOnDownloadChangeListener = new OnDownloadDataChangeListener() {

		@Override
		public void onDownloadDataChange(List<DownloadInfo> mDownloadInfos) {
			if (mDownloadInfo != null) {
				for (DownloadInfo info : mDownloadInfos) {
					if (mDownloadInfo.downloadId == info.downloadId) {
						mDownloadInfo = (AppDownloadInfo) info;
						mAppInfo.installState = DownloadManager.getInstallState(mAppInfo);
						updateStatus();
						return;
					}
				}
				//如果没有查找到则会走到这里来
				mDownloadInfo = null;
				mAppInfo.installState = DownloadManager.getInstallState(mAppInfo);
				updateStatus();
			}
		}

		@Override
		public void onAppInstallChange(AppDownloadInfo mDownloadInfo,String packagename,boolean isInstall) {
			if(mAppInfo.packageName.equals(packagename)){
				log.debug("packagename="+packagename);
				mAppInfo.mDownloadInfo = mDownloadInfo;
				mAppInfo.installState = DownloadManager.getInstallState(mAppInfo);
				updateStatus();
				return;
			}
		}
	};

	private OnStateChangeListener mOnStateChangeListener = new OnStateChangeListener() {

		@Override
		public void onDownloadStateChange(DownloadInfo downloadInfo, int new_state) {
			log.debug("new_state="+new_state);
			mDownloadInfo = (AppDownloadInfo) downloadInfo;
			updateStatus();
		}
	};

	private void checkIfAnim() {
		Intent intent = getIntent();
		isAnimLaunch = intent.getBooleanExtra(ANIM_GAME_ENABLED, false);
		log.debug("isAnimLaunch app = " + isAnimLaunch);
		if (isAnimLaunch) {
			DefaultEffectFaces.DefaultEffectFacesParams params = new DefaultEffectFaces.DefaultEffectFacesParams(DefaultEffectFaces.DefaultEffectFacesParams.EffectType.Scale,
					DefaultEffectFaces.DefaultEffectFacesParams.EasyingType.Easying);
			params.scaleRange = intent.getIntArrayExtra(ANIM_SCALE_DATA);
			effectView = new FlipViewGroup(AppDetailActivity.this, actView, false, mHandler, params);
			setContentView(effectView);
		} else {
			setContentView(actView);
		}
	}

	private FlipViewGroup effectView;
	private View actView;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
        }
		float density = getResources().getDisplayMetrics().density;
		mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.game_detail_icon_h324).displayer(new RoundedBitmapDisplayer((int) (18*density))).build();
		
		int result = 0;
	      int resourceId = getResources().getIdentifier("status_bar_height", "dimen", "android");
	      if (resourceId > 0) {
	          result = getResources().getDimensionPixelSize(resourceId);
	      } 
		Log.i("*** Jorgesys :: ", "StatusBar Height= " + result +",density="+getResources().getDisplayMetrics().density);
		
		actView = View.inflate(this, R.layout.activity_app_detail, null);
		checkIfAnim();
		initUI();
		initData();
		StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
		DownloadManager.getInstance().addOnDownloadDataChangeListener(mOnDownloadChangeListener);
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		DownloadManager.getInstance().removeOnDownloadDataChangeListener(mOnDownloadChangeListener);
		Activity3DEffectTool.isLaunching = false;
	}

	@Override
	protected boolean isFinishAnimEnable() {
		return isAnimLaunch;
	}

	private static final String ANIM_GAME_ENABLED = "anim_game_enabled";
	private static final String ANIM_SCALE_DATA = "anim_scale_data";

	// ---------------------------------------------------------
	public static void start(Context activity, View clickItemView, AppItemInfo item) {
		Intent intent = new Intent(activity, AppDetailActivity.class);
		// 存储app的相关信息
		Bundle bundle = new Bundle();
		bundle.putSerializable(AppDetailActivity.APP_DATA, item);
		intent.putExtras(bundle);
		// 开启动画
		intent.putExtra(ANIM_GAME_ENABLED, true);
		int[] loc = new int[2];
		clickItemView.getLocationOnScreen(loc);
		int l = loc[0];
		int t = Constant.SCREEN_HEIGHT - loc[1];
		int r = l + clickItemView.getMeasuredWidth();
		int b = Constant.SCREEN_HEIGHT - (loc[1] + clickItemView.getMeasuredHeight());
		log.debug("l=" + l + " t=" + t + "r =" + r + "b=" + b);
		intent.putExtra(ANIM_SCALE_DATA, new int[] { l, t, r, b });
		Activity3DEffectTool.startActivity((Activity) activity, intent);
	}

	private void initData() {
		Intent intent = getIntent();
		if (intent == null)
			finish();
		mAppInfo = (AppItemInfo) intent.getExtras().get(APP_DATA);
		if (mAppInfo == null)
			finish();
		appid = mAppInfo.id;
		if (appid == NO_TASK) {
			Util.showToast(this, getString(R.string.no_albumid), Toast.LENGTH_SHORT);
		} else {
			getScreenShoots(appid, mHandler);
		}
		updateUI(mAppInfo);
	}

	/**
	 * 模拟获取海报
	 */
	private void getScreenShoots(int appid, Handler handler) {
		ArrayList<String> lists = TDImageWrap.getAppSnapshotUrls(appid);
		ssAdapter = new ScreenShotsAdapter(this, lists);
		screen_shots_list.setAdapter(ssAdapter);
	}

	private void initUI() {
		app_detail_title_tv = (TextView) findViewById(R.id.app_detail_title); // 详情标题
		btn_download = (TextView) findViewById(R.id.btn_download_tv); // 下载按钮
		download_progress_bar = (ProgressBar) findViewById(R.id.download_progress_bar); // 进度条
		btn_download_rl = (RelativeLayout) findViewById(R.id.btn_download_rl); // 下载按钮容器
		btn_actionbar_return = (ImageButton) findViewById(R.id.btn_actionbar_return); // 返回按钮
		download_message_tv = (TextView) findViewById(R.id.download_message); // 下载次数
		file_size_tv = (TextView) findViewById(R.id.file_size); // 文件大小
		app_type_tv = (TextView) findViewById(R.id.app_type); // 应用类型
		app_news_tv = (TextView) findViewById(R.id.app_news); // 最新动态
		app_news_ll = findViewById(R.id.app_news_ll); // 最新动态
		app_intro_tv = (TextView) findViewById(R.id.app_intro); // 应用描述
		app_developer_tv = (TextView) findViewById(R.id.app_developer); // 应用开发商
		app_update_tv = (TextView) findViewById(R.id.app_update); // 更新时间
		app_version_tv = (TextView) findViewById(R.id.app_version); // 跟新版本
		app_type2_tv = (TextView) findViewById(R.id.app_type2); // 应用类型 下
		app_file_size_tv = (TextView) findViewById(R.id.app_file_size); // 文件大小
		// 下
		show_all_msg_click_iv = (ImageView) findViewById(R.id.show_all_msg_click_iv); // 下拉
		mBlurImageView = (ImageView) findViewById(R.id.app_icon); // app_icon
		screen_shots_list = (HorizontalListView) findViewById(R.id.screen_shots_list); // 截图列表
		popupll = (LinearLayout) findViewById(R.id.popup_pic_viewLL); // 大图浮层

		// 按钮点击
		btn_download_rl.setOnClickListener(this);
		btn_actionbar_return.setOnClickListener(this);
		screen_shots_list.setOnItemClickListener(this);
	}

	private View.OnTouchListener toggleAllInfoTouchListener = new View.OnTouchListener() {
		float px;
		float py;

		@Override
		public boolean onTouch(View view, MotionEvent motionEvent) {
			switch (motionEvent.getAction()) {
			case MotionEvent.ACTION_DOWN:
				px = motionEvent.getX();
				py = motionEvent.getY();
				break;
			case MotionEvent.ACTION_UP:
				float dx = Math.abs(motionEvent.getX() - px),
				dy = Math.abs(motionEvent.getY() - py);
				if (dx < 0.1 && dy < 0.1) {
					toggleDescription();
					return true;
				}
				break;
			}
			return false;
		}
	};
	private View.OnClickListener toggleAllInfoListener = new View.OnClickListener() {
		@Override
		public void onClick(View view) {
			toggleDescription();
		}
	};

	private static final int FoldingStringSize = 50;

	private void updateUI(AppItemInfo info) {
		app_detail_title_tv.setText(info.title);
		download_message_tv.setText(" " + Util.addComma(info.installationTimes, this));
		String size = Util.convertFileSize(info.fileSize, 2, false); //(info.fileSize / 1024 / 1024) + "M";
		file_size_tv.setText(" " + size);
		ImageLoader.getInstance().displayImage(TDImageWrap.getAppIcon(info), mBlurImageView, mOptions);

		String str = "";
		for (AppItemInfo.AppGenre gen : info.genres) {
			str += gen.title + " ";
		}
		app_type_tv.setText(" " + str);
		app_type2_tv.setText(" " + str);
		boolean cut = info.description.length() > FoldingStringSize;
		if (cut) {
			app_intro_tv.setText(info.description.substring(0, FoldingStringSize) + "...");
			Bitmap bitmap = ((BitmapDrawable) getResources().getDrawable(R.drawable.list_enter)).getBitmap();
			Matrix matrix = new Matrix();
			matrix.setRotate(90);
			bitmap = Bitmap.createBitmap(bitmap, 0, 0,bitmap.getWidth(), bitmap.getHeight() , matrix, true);
			show_all_msg_click_iv.setImageBitmap(bitmap);
			show_all_msg_click_iv.setVisibility(View.VISIBLE);
			app_intro_tv.setOnTouchListener(toggleAllInfoTouchListener);
			show_all_msg_click_iv.setOnClickListener(toggleAllInfoListener);
			toggleFlag = true;
		} else {
			app_intro_tv.setText(info.description);
		}
		if (info.upgradeMessage.equals("")) {
			app_news_ll.setVisibility(View.GONE);
		} else {
			app_news_ll.setVisibility(View.VISIBLE);
			app_news_tv.setText(" " + info.upgradeMessage);
		}

		app_developer_tv.setText(" " + info.developerTitle);
		app_update_tv.setText(" " + Util.corverTimeWithYMD(info.updateDate));
		app_version_tv.setText(" " + info.versionName);
		app_file_size_tv.setText(" " + size);

		mDownloadInfo = DownloadManager.getInstance().bindDownloadInfo(mAppInfo);
		updateStatus();
	}

	boolean toggleFlag;

	private void toggleDescription() {
		Bitmap bitmap = ((BitmapDrawable) getResources().getDrawable(R.drawable.list_enter)).getBitmap();
		Matrix matrix = new Matrix();
		if (toggleFlag) {
			matrix.setRotate(-90);
			bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
			show_all_msg_click_iv.setImageBitmap(bitmap);
			app_intro_tv.setText(mAppInfo.description);
		} else {
			matrix.setRotate(90);
			bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
			show_all_msg_click_iv.setImageBitmap(bitmap);
			app_intro_tv.setText(mAppInfo.description.substring(0, FoldingStringSize) + "...");
		}
		toggleFlag = !toggleFlag;
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.btn_download_rl:
			if (Util.isFastDoubleClick()) {
				log.debug("任务正在执行,请勿频繁点击！");
				return;
			}
			execDownload();
			break;
		case R.id.btn_actionbar_return:
			finish();
			break;
		}
	}

	/**
	 * 安装完成时要更新一次按钮状态
	 */
	protected void onResume() {
		updateStatus();
		super.onResume();
		if(ssAdapter != null && ssAdapter.getCount() > 0){
			// 刷新数据源，解决打开应用后返回到详情页，horizontListView划不动的bug
			ssAdapter.notifyDataSetChanged();
		}
	}

	public static final int NO_TASK = -1;

	/**
	 * 按钮点击的处理逻辑
	 */
	private void execDownload() {
		log.debug("installState="+mAppInfo.installState+",mDownloadInfo="+mDownloadInfo);
		if (mAppInfo.installState == AppDownloadInfo.STATE_INSTALL) {
			ApkInstaller.lanchAppByPackageName(AppDetailActivity.this, mAppInfo.appKey, mAppInfo.appId, mAppInfo.packageName, mAppInfo.type, 3);
		} else if (mAppInfo.installState == AppDownloadInfo.STATE_UPDATE && (mDownloadInfo == null || (mDownloadInfo != null && mDownloadInfo.getState() != AppDownloadInfo.STATE_RUNNING && mDownloadInfo.getState() != AppDownloadInfo.STATE_PAUSE))) {
			new DownloadCreater(AppDetailActivity.this).startDownload(mAppInfo, mOnStateChangeListener);
		} else {
			if (mDownloadInfo == null) {
				new DownloadCreater(AppDetailActivity.this).startDownload(mAppInfo, mOnStateChangeListener);
			} else {
				int statue = mDownloadInfo.getState();
				switch (statue) {
				case AppDownloadInfo.STATE_PAUSE:
				case AppDownloadInfo.STATE_FAILED:
					if (!Util.isNetworkAvailable()) {
						Toast.makeText(this, this.getString(R.string.no_network_toast), Toast.LENGTH_SHORT).show();
						return;
					}
					DownloadManager.getInstance().startDownload(mDownloadInfo, mOnStateChangeListener);
					break;
				case AppDownloadInfo.STATE_WAIT:
				case AppDownloadInfo.STATE_RUNNING:
					DownloadManager.getInstance().pauseDownloadTask(mDownloadInfo);
					break;
				case AppDownloadInfo.STATE_FINISH:
					ApkInstaller.installApk(this, mDownloadInfo);
					break;
				default:
					break;
				}

			}
		}
	}

	/**
	 * 更新按钮状态
	 */
	private void updateStatus() {
		int downloadState = mDownloadInfo != null ? mDownloadInfo.getState() : -1;
		log.debug("installState="+mAppInfo.installState+",mDownloadInfo="+mDownloadInfo);
		if (mAppInfo.installState == AppDownloadInfo.STATE_INSTALL) {
			btn_download.setTextColor(getResources().getColor(R.color.white));
			btn_download_rl.setBackgroundResource(R.drawable.progress_bg_blue_selector);
			btn_download.setText(R.string.download_app_open);
			download_progress_bar.setVisibility(View.GONE);
		} else if (mAppInfo.installState == AppDownloadInfo.STATE_UPDATE && (mDownloadInfo == null || (mDownloadInfo != null && mDownloadInfo.getState() != AppDownloadInfo.STATE_RUNNING && mDownloadInfo.getState() != AppDownloadInfo.STATE_PAUSE))) {
			btn_download.setTextColor(getResources().getColor(R.color.white));
			btn_download_rl.setBackgroundResource(R.drawable.progress_bg_blue_selector);
			btn_download.setText(R.string.download_app_update);
			download_progress_bar.setVisibility(View.GONE);
		} else {
			if (mDownloadInfo == null) {
				btn_download_rl.setBackground(getResources().getDrawable(R.drawable.progress_bg_green_selector));
				btn_download.setTextColor(getResources().getColor(R.color.white));
				btn_download.setText(R.string.appdetail_download_app);
				download_progress_bar.setVisibility(View.GONE);
			} else {
				int state = mDownloadInfo.getState();
				switch (mDownloadInfo.getState()) {
				case AppDownloadInfo.STATE_PAUSE:
					btn_download.setText(getString(R.string.download_app_paused));
					break;
				case AppDownloadInfo.STATE_RUNNING:
					btn_download.setText(R.string.download_app_running);
					break;
				case AppDownloadInfo.STATE_FAILED:
					btn_download_rl.setBackground(getResources().getDrawable(R.drawable.progress_bg_blue_selector));
					btn_download.setText(R.string.appdetail_download_error);
					break;
				case AppDownloadInfo.STATE_FINISH:
					btn_download.setText(getString(R.string.download_app_install));
					btn_download.setTextColor(getResources().getColor(R.color.white));
					btn_download_rl.setBackgroundResource(R.drawable.progress_bg_blue_selector);
					break;
				case AppDownloadInfo.STATE_WAIT:
					btn_download.setText(R.string.download_app_wait_down);
				default:
					break;
				}

				if (state == AppDownloadInfo.STATE_RUNNING || state == AppDownloadInfo.STATE_PAUSE) { // 进度
					int prog = mDownloadInfo.getDownloadProgress();
					log.debug("prog=" + prog + ",speed=" + mDownloadInfo.downloadSpeed);
					download_progress_bar.setProgress(prog);
					download_progress_bar.setVisibility(View.VISIBLE);
					btn_download.setTextColor(getResources().getColor(R.color.textview_deep_black));
				} else {
					download_progress_bar.setVisibility(View.GONE);
				}
			}
		}
	}

	boolean flag = false;

	/**
	 * 网络状态改变时的处理方式
	 *
	 * @param isNetWorkAviliable
	 *            网络是否可用
	 * @param mNetType
	 *            网络类型
	 */
	@Override
	public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
		if (isNetWorkAviliable && !flag) {
			initData();
			flag = true;
		} else {
			flag = false;
		}
	}

	private boolean isShowPicViewer = false;

	@Override
	public void onItemClick(AdapterView<?> adapterView, View view, int pos, long l) {
		if (isShowPicViewer)
			return;
		FragmentTransaction mFragmentTransaction = getSupportFragmentManager().beginTransaction();
		fragment = new PopupShortcutFrag(ssAdapter);
		mFragmentTransaction.add(R.id.popup_pic_viewLL, fragment);
		mFragmentTransaction.commitAllowingStateLoss();
		fragment.show(pos);
		popupll.setVisibility(View.VISIBLE);
		isShowPicViewer = true;
	}

	public void hideFrag() {
		if (fragment != null) {
			FragmentTransaction mFragmentTransaction = getSupportFragmentManager().beginTransaction();
			mFragmentTransaction.remove(fragment);
			mFragmentTransaction.commit();
			fragment = null;
		}
		popupll.setVisibility(View.GONE);
		isShowPicViewer = false;
	}

	/**
	 * horizontal list view 的数据适配器
	 */
	public class ScreenShotsAdapter extends BaseAdapter {
		private final Context mContext;
		private final LayoutInflater mInflater;
		ArrayList<String> mList;
		private DisplayImageOptions pOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_app_detai_listitem).displayer(new SimpleBitmapDisplayer()).build();

		public ScreenShotsAdapter(Context context, ArrayList<String> data) {
			mContext = context;
			mInflater = LayoutInflater.from(mContext);
			mList = new ArrayList<String>();
			mList.addAll(data);
		}

		@Override
		public int getCount() {
			return mList.size();
		}

		@Override
		public String getItem(int i) {
			return mList.get(i);
		}

		@Override
		public long getItemId(int i) {
			return i;
		}

		@Override
		public View getView(int pos, View cacheView, ViewGroup parent) {
			ViewHolder holder;

			if (cacheView == null) {
				holder = new ViewHolder();
				cacheView = mInflater.inflate(R.layout.item_app_screen_shoot, parent, false);
				holder.screenShoot = (ImageView) cacheView.findViewById(R.id.item_screen_shoot);
				cacheView.setTag(holder);
			} else {
				holder = (ViewHolder) cacheView.getTag();
			}
			String url = mList.get(pos);
			ImageLoader.getInstance().displayImage(url, holder.screenShoot, pOptions);
			return cacheView;
		}

		private class ViewHolder {
			ImageView screenShoot;
		}
	}

	/**
	 * 如果显示弹出大图, 在返回时退出大图
	 *
	 * @param event
	 *            keyevent
	 * @return bool
	 */
	public boolean dispatchKeyEvent(KeyEvent event) {
		if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN) {
			if (fragment != null) {
				hideFrag();
				return true;
			}
		}
		return super.dispatchKeyEvent(event);
	}
}
