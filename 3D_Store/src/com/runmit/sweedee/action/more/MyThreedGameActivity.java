package com.runmit.sweedee.action.more;

import java.util.List;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.GridView;

import com.runmit.sweedee.R;
import com.runmit.sweedee.manager.GetSysAppInstallListManager;
import com.runmit.sweedee.manager.GetSysAppInstallListManager.AppInfo;
import com.runmit.sweedee.util.ApkInstaller;
import com.runmit.sweedee.view.EmptyView;
import com.runmit.sweedee.view.EmptyView.Status;

public class MyThreedGameActivity extends NewBaseRotateActivity implements AdapterView.OnItemClickListener {

	private GridView mGridView;
	protected EmptyView mEmptyView;
	private GridItemInfoAdapter adapter = null;
	private static final int MSG_SUCCESS = 0;
	private static final int MSG_EMPTYDATA = 1;

	private boolean isShortCutLaunch = false;

	private Handler mHandler = new Handler() {
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case MSG_SUCCESS:
				List<AppInfo> appInfoList = (List<AppInfo>) msg.obj;
				mEmptyView.setStatus(Status.Gone);
				adapter = new GridItemInfoAdapter(appInfoList);
				mGridView.setAdapter(adapter);
				break;
			case MSG_EMPTYDATA:
				mEmptyView.setStatus(Status.Empty);
				break;
			}
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getActionBar().setDisplayHomeAsUpEnabled(true);
		getActionBar().setDisplayShowHomeEnabled(false);
		setContentView(R.layout.frg_mythreed_game);
		initViews();
		isShortCutLaunch = getIntent().getBooleanExtra("shortcut", false);
	}

	@Override
	protected void onResume() {
		super.onResume();
		
		new Thread(new Runnable() {
			
			@Override
			public void run() {
				GetSysAppInstallListManager mng = new GetSysAppInstallListManager();
				List<AppInfo> appInfoList = mng.getAppinfoListByType(getAppType());
				if (appInfoList != null && appInfoList.size() != 0) {
					mHandler.obtainMessage(MSG_SUCCESS, appInfoList).sendToTarget();
				} else {
					mHandler.obtainMessage(MSG_EMPTYDATA, null).sendToTarget();
				}
			}
		}).start();
	}

	protected int getAppType(){
		return GetSysAppInstallListManager.TYPE_GAME;
	}
	
	protected void initViews() {
		mGridView = (GridView) findViewById(R.id.myGame_item);
		mEmptyView = (EmptyView) findViewById(R.id.rl_empty_tip);
		mEmptyView.setEmptyTip(R.string.no_Game_record_tip).setEmptyPic(R.drawable.ic_game_defaut);
		mEmptyView.setStatus(Status.Loading);
		mGridView.setOnItemClickListener(this);
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

	protected void lauchAppByPackageName(String packageName) {
		// * @param appType:app 类型1:APP, 2:GAME
		// * @param startFrom：0 –未知;1– 我的游戏;2 – 我的应用，3.详情页，4.首页TAB推荐
		ApkInstaller.lanchAppByPackageName(this, packageName, 2, 1);
	}

	@Override
	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long arg3) {
		String packagename = adapter.getItem(position).packagename;
		lauchAppByPackageName(packagename);
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
	}

}
