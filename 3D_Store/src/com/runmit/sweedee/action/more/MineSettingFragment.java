package com.runmit.sweedee.action.more;

import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;

import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.action.login.LoginActivity;
import com.runmit.sweedee.action.myvideo.MyVideoActivity;
import com.runmit.sweedee.action.pay.MyWalletActivity;
import com.runmit.sweedee.action.pay.RechargeActivity;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadManager;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.GetSysAppInstallListManager;
import com.runmit.sweedee.manager.WalletManager;
import com.runmit.sweedee.manager.GetSysAppInstallListManager.AppInfo;
import com.runmit.sweedee.manager.WalletManager.OnAccountChangeListener;
import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.UserAccount;
import com.runmit.sweedee.sdk.user.member.task.MemberInfo;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.sdk.user.member.task.UserManager.LoginState;
import com.runmit.sweedee.sdk.user.member.task.UserManager.LoginStateChange;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.BadgeView;

public class MineSettingFragment extends BaseFragment implements View.OnClickListener, LoginStateChange, View.OnTouchListener {

	XL_Log log = new XL_Log(MineSettingFragment.class);
	
	public static final String INTENT_ACTION_SET_UPDATEAPP_PREFERENCE = "com.runmit.sweedee.set_updateapp_preference";

	private View rootView;

	private TextView show_userName;

	private ImageView header_image;

	public ImageView new_image;

	private ProgressDialog mProgressDialog;

	private Button btn_loin_add_regist;

	private RelativeLayout setting_layoutLayout;
	
	private RelativeLayout task_layoutLayout;
	
	private LinearLayout line_between_video_game;
	// 临时测试3d游戏与3d应用所加
	private LinearLayout btn_mythreed_gameButton;
	private LinearLayout btn_mythreed_appButton;
	private LinearLayout my3DMovie;

	private Button buttonRecharge;

	private TextView textView_Balance;
	private TextView tv_virtual_balance;
	private ImageView imageView_line;
	private RelativeLayout mywallet_layout;
	private BadgeView badgeView;
	private UserCenterReceiver mUserCenterReceiver;
	private ArrayList<AppItemInfo> mAppInfoList;
	
	private Thread mThread;
	private static final int MSG_SUCCESS = 0;
    private static final int MSG_EMPTYDATA = 1;
    public static boolean isBadgeViewShow=false;

	
	private OnAccountChangeListener mAccountChangeListener = new OnAccountChangeListener() {	
		@Override
		public void onAccountChange(int rtnCode, List<UserAccount> mAccounts) {
			log.debug("rtnCode="+rtnCode+",mAccounts="+mAccounts.size());
			setAccount();
		}
	};
	
	private Handler mHandler =new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case CmsEventCode.EVENT_GET_APP_UPDATE_LIST:
				 ArrayList<AppItemInfo> appItemInfos=(ArrayList<AppItemInfo>) msg.obj;
				 if(appItemInfos==null){
				
				 }else{
					 appItemInfos=getUpdateAppList(appItemInfos);
					 String pkgNameJson=SharePrefence.getInstance(getActivity()).getString(SharePrefence.TASKMANAGER_UPDATE_TIPS,"[]");
					 Gson gson = new Gson();  
					 ArrayList<String> pkgNameList=gson.fromJson(pkgNameJson,new TypeToken<ArrayList<String>>() {}.getType());
					 showBadgeTips(pkgNameList,appItemInfos);
					 mAppInfoList=appItemInfos;
					 StoreApplication.INSTANCE.mAppInfoList=appItemInfos;
				 }
		    break; 	
			case MSG_SUCCESS:
				List<AppInfo> appInfoList = (List<AppInfo>)msg.obj;
				Gson gson = new Gson();  
			    String result = gson.toJson(getPkgNameListFromAppInfo(appInfoList));  
				CmsServiceManager.getInstance().getAppUpdateList(result, mHandler);     
	        break;  
	        case MSG_EMPTYDATA:
	        break;
			}
		}
	};
	
	private void showBadgeTips(ArrayList<String> pkgNameList,ArrayList<AppItemInfo> appInfoList){
		for (AppItemInfo appItemInfo : appInfoList) {
			if(!pkgNameList.contains(appItemInfo.packageName)){
				badgeView.show();
				isBadgeViewShow=true;
				return;
			}else{
				badgeView.hide();
			}
		}
	}
	
	private ArrayList<String> getPkgNameListFromAppItemInfo(List<AppItemInfo> appItemInfoList){
		ArrayList<String> pkgList=new ArrayList<String>();
		for (AppItemInfo appItemInfo : appItemInfoList) {
			pkgList.add(appItemInfo.packageName);
		}
		return pkgList;
	}
	
	private ArrayList<String> getPkgNameListFromAppInfo(List<AppInfo> appInfoList){
		ArrayList<String> pkgList=new ArrayList<String>();
		for (AppInfo appInfo : appInfoList) {
			pkgList.add(appInfo.packagename);
		}
		return pkgList;
	}
	
	private ArrayList<AppItemInfo> getUpdateAppList( ArrayList<AppItemInfo> appItemList){
		ArrayList<AppItemInfo> updateAppItemList=new ArrayList<AppItemInfo>();
		for (AppItemInfo appItem : appItemList) {
		if(DownloadManager.getInstallState(appItem)== AppDownloadInfo.STATE_UPDATE){
			updateAppItemList.add(appItem);
		}
		}
		return updateAppItemList;
	}
	
	private void registerReceiver(){
		mUserCenterReceiver = new UserCenterReceiver();
		IntentFilter intentFilter = new IntentFilter(INTENT_ACTION_SET_UPDATEAPP_PREFERENCE);
		getActivity().registerReceiver(mUserCenterReceiver, intentFilter);
	}
	    
	private void unregisterReceiver(){
        getActivity().unregisterReceiver(mUserCenterReceiver);
	}

	// 如果支付完成，继续下载
	public class UserCenterReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			if(badgeView.isShown()){
				if(mAppInfoList!=null&&mAppInfoList.size()>0){
					Gson gson = new Gson();  
				    String result = gson.toJson(getPkgNameListFromAppItemInfo(mAppInfoList));  
				    SharePrefence.getInstance(getActivity()).putString(SharePrefence.TASKMANAGER_UPDATE_TIPS,result);
				    badgeView.hide();
				    isBadgeViewShow=false;
				}
			}
		}
	}

	// 发送支付完成的广播
	public static void sendUserCenterBroadcast(Context context) {
		Intent i = new Intent(INTENT_ACTION_SET_UPDATEAPP_PREFERENCE);
		context.sendBroadcast(i);
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mProgressDialog = new ProgressDialog(this.getActivity(), R.style.ProgressDialogDark);
		WalletManager.getInstance().addAccountChangeListener(mAccountChangeListener);
		if(UserManager.getInstance().isLogined()){
			WalletManager.getInstance().reqGetAccount();
		}
		registerReceiver();
		new Thread(runnable).start();
	}

	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		rootView = inflater.inflate(R.layout.activity_user_center, container, false);
		initViews();
		UserManager.getInstance().addLoginStateChangeListener(this);
		return rootView;
	}

	private void initViews() {
		header_image = (ImageView) rootView.findViewById(R.id.head_imgview);
		show_userName = (TextView) rootView.findViewById(R.id.show_userName);
		setting_layoutLayout = (RelativeLayout) rootView.findViewById(R.id.setting_layout);
		task_layoutLayout= (RelativeLayout) rootView.findViewById(R.id.task_layout);
		btn_loin_add_regist = (Button) rootView.findViewById(R.id.btn_loin_add_regist);
		
		line_between_video_game = (LinearLayout) rootView.findViewById(R.id.line_between_video_game);
		btn_mythreed_gameButton = (LinearLayout) rootView.findViewById(R.id.my_game_layout);
		btn_mythreed_gameButton.setOnClickListener(this);
		btn_mythreed_gameButton.setOnTouchListener(this);
		
		if(StoreApplication.isSnailPhone){
			line_between_video_game.setVisibility(View.GONE);
			btn_mythreed_gameButton.setVisibility(View.GONE);
		}
		
		TextView iv_task_indicator = (TextView)rootView.findViewById(R.id.my_task_badage);

		badgeView = new BadgeView(this.getActivity(),iv_task_indicator);
		badgeView.setBackgroundResource(R.drawable.icon_badge);
		badgeView.setBadgePosition(BadgeView.POSITION_CENTER);
		badgeView.setHeight(10);
		badgeView.setWidth(10);

		btn_mythreed_appButton = (LinearLayout) rootView.findViewById(R.id.my_apk_layout);
		btn_mythreed_appButton.setOnClickListener(this);
		btn_mythreed_appButton.setOnTouchListener(this);

		my3DMovie = (LinearLayout) rootView.findViewById(R.id.my_video_layout);
		my3DMovie.setOnClickListener(this);
		my3DMovie.setOnTouchListener(this);

		rootView.findViewById(R.id.username_layout).setOnClickListener(this);
		rootView.findViewById(R.id.username_layout).setOnTouchListener(this);

		rootView.findViewById(R.id.payhistory_layout).setOnClickListener(this);
		rootView.findViewById(R.id.payhistory_layout).setOnTouchListener(this);

		btn_loin_add_regist.setOnClickListener(this);
		btn_loin_add_regist.setOnTouchListener(this);

		setting_layoutLayout.setOnClickListener(this);
		setting_layoutLayout.setOnTouchListener(this);
		
		task_layoutLayout.setOnClickListener(this);
		task_layoutLayout.setOnTouchListener(this);
		

		header_image.setOnClickListener(this);
		header_image.setOnTouchListener(this);

		buttonRecharge = (Button) rootView.findViewById(R.id.buttonRecharge);
		buttonRecharge.setOnClickListener(this);
		
		View v = rootView.findViewById(R.id.user_suggest_layout);
		v.setOnClickListener(this);
		v.setOnTouchListener(this);
		
		v = rootView.findViewById(R.id.aboutus_layout);
		v.setOnClickListener(this);
		v.setOnTouchListener(this);

		mywallet_layout = (RelativeLayout) rootView.findViewById(R.id.mywallet_layout);
		textView_Balance = (TextView) rootView.findViewById(R.id.tv_account_balance);
		tv_virtual_balance = (TextView) rootView.findViewById(R.id.tv_virtual_balance);
		imageView_line = (ImageView) rootView.findViewById(R.id.imageView_line);
		setAccount();
	}

	private void setAccount(){
		if(UserManager.getInstance().isLogined()){
			mywallet_layout.setVisibility(View.VISIBLE);
			UserAccount mUserAccount = WalletManager.getInstance().getReadAccount();
			log.debug("mUserAccount="+mUserAccount);
			if(mUserAccount != null){
				int userAmount = (int) (mUserAccount.amount/100);
				textView_Balance.setText(MessageFormat.format(mFragmentActivity.getString(R.string.usercenter_account_balance), userAmount));
				textView_Balance.setVisibility(View.VISIBLE);
			}else{
				textView_Balance.setVisibility(View.GONE);
			}
			mUserAccount = WalletManager.getInstance().getVirtualAccount();
			log.debug("VirtualAccount="+mUserAccount);
	        if(mUserAccount != null){
	        	int amount = (int) mUserAccount.amount;
	        	tv_virtual_balance.setText(Integer.toString(amount));
	        	tv_virtual_balance.setVisibility(View.VISIBLE);
	        	imageView_line.setVisibility(View.VISIBLE);
	        }else{
	        	tv_virtual_balance.setVisibility(View.GONE);
	        	imageView_line.setVisibility(View.GONE);
			}
		}else{
			mywallet_layout.setVisibility(View.INVISIBLE);
		}
	}
	
	private void initStatus() {
		if (!isAdded()) {
			return;
		}
		if (UserManager.getInstance().getLoginState() == LoginState.LOGINED) {
			btn_loin_add_regist.setVisibility(View.GONE);
			show_userName.setVisibility(View.VISIBLE);
			final MemberInfo mMemberInfo = UserManager.getInstance().getMemberInfo();
			if (mMemberInfo.nickname != null && mMemberInfo.nickname.length() > 0) {
				show_userName.setText(mMemberInfo.nickname);
			}
		} else {
			show_userName.setVisibility(View.GONE);
			btn_loin_add_regist.setVisibility(View.VISIBLE);
			
			mywallet_layout.setVisibility(View.INVISIBLE);
		}

		mHandler.postDelayed(new Runnable() {

			@Override
			public void run() {
				// 需要重新buildcache一次
				View view = mFragmentActivity.getWindow().getDecorView();
				view.setDrawingCacheEnabled(true);
				view.destroyDrawingCache();
				view.buildDrawingCache();
			}
		}, 50);
	}

	@Override
	public void onResume() {
		super.onResume();
		initStatus();
	}

	@Override
	public void onPause() {
		super.onPause();
	}

	@Override
	public void onStart() {
		super.onStart();
	}

	@Override
	public void onStop() {
		super.onStop();
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		UserManager.getInstance().removeLoginStateChangeListener(this);
		WalletManager.getInstance().removeAccountChangeListener(mAccountChangeListener);
		unregisterReceiver();
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.btn_loin_add_regist:
			rotateStartActivity(LoginActivity.class);
			break;
		case R.id.head_imgview:
			if(!UserManager.getInstance().isLogined()){
				rotateStartActivity(LoginActivity.class);
			}
			break;
		case R.id.my_video_layout: // 缓存记录
			rotateStartActivity(MyVideoActivity.class);
			break;
		case R.id.payhistory_layout: // 购买记录
			if (UserManager.getInstance().isLogined()) {
				rotateStartActivity(MyWalletActivity.class);
			} else {
				rotateStartActivity(LoginActivity.class);
			}
			break;
		case R.id.setting_layout: // 设置
			rotateStartActivity(SettingItemActivity.class);
			break;
		case R.id.task_layout: // 下载与更新
			rotateStartActivity(TaskAndUpdateActivity.class);
			break;
		case R.id.my_game_layout: // 我的3d游戏
			rotateStartActivity(MyThreedGameActivity.class);
			break;
		case R.id.my_apk_layout: 
			rotateStartActivity(MyThreeDAppActivity.class);
			break;
		case R.id.buttonRecharge:
			if (UserManager.getInstance().isLogined()) {
				UserAccount mUserAccount = WalletManager.getInstance().getReadAccount();
				if(mUserAccount != null){
					Intent intent = new Intent(mFragmentActivity, RechargeActivity.class);
					intent.putExtra(UserAccount.class.getSimpleName(), mUserAccount);
					startActivity(intent);
				}else{
					//TODO show toast;
				}
			}else{
				Util.showToast(mFragmentActivity, getString(R.string.user_no_login), Toast.LENGTH_SHORT);
			}
			break;
			
		case R.id.user_suggest_layout:
			rotateStartActivity(FeedBackActivity.class);
			break;
		case R.id.aboutus_layout:
			rotateStartActivity(AboutActivity.class);
			break;
		default:
			break;
		}
	}

	/**
	 * 3D翻转界面的启动类
	 *
	 * @param target
	 */
	public void rotateStartActivity(Class target) {
		((UserAndSettingActivity) mFragmentActivity).createViewBitmap();
		NewBaseRotateActivity.rotateStartActivity(mFragmentActivity, target);
	}

	@Override
	public void onChange(LoginState lastState, LoginState currentState, final Object userdata) {
		if (currentState == LoginState.LOGINED) {
			initStatus();
		} else if (currentState == LoginState.COMMON && lastState == LoginState.LOGINED) {// 此种情况判断是注销登录
			initStatus();
		}
	}

	// 按下的时候截屏,避免截到状态点击器
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		log.debug("event acation = " + event.getAction());
		if (event.getAction() == MotionEvent.ACTION_DOWN) {
			((UserAndSettingActivity) mFragmentActivity).createViewBitmap();
		}
		return false;
	}
	
	private Runnable runnable = new Runnable(){
		@Override
		public void run() {
			GetSysAppInstallListManager mng = new GetSysAppInstallListManager();

			List<AppInfo> appInfoList =  mng.getAppinfoListByType(GetSysAppInstallListManager.TYPE_ALL);
    		if (appInfoList != null && appInfoList.size() != 0) {
    			mHandler.obtainMessage(MSG_SUCCESS,appInfoList).sendToTarget();
	    	}else {
	    		mHandler.obtainMessage(MSG_EMPTYDATA, null).sendToTarget();
			}
		}
    };
}
