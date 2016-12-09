package com.runmit.sweedee.action.more;

import com.runmit.sweedee.R;
import com.runmit.sweedee.manager.GetSysAppInstallListManager;
import com.runmit.sweedee.util.ApkInstaller;

public class MyThreeDAppActivity extends MyThreedGameActivity{

	protected void initViews() {
		super.initViews();
		mEmptyView.setEmptyTip(R.string.no_App_record_tip).setEmptyPic(R.drawable.ic_app_defaut);
	}

	protected int getAppType(){
		return GetSysAppInstallListManager.TYPE_APP;
	}

	protected void lauchAppByPackageName(String packageName) {
		// * @param appType:app 类型1:APP, 2:GAME
		// * @param startFrom：0 –未知;1– 我的游戏;2 – 我的应用，3.详情页，4.首页TAB推荐
		ApkInstaller.lanchAppByPackageName(this, packageName, 1, 2);
	}
}
