package com.runmit.sweedee.download;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.runmit.sweedee.database.dao.AppInstallSQLDao;
import com.runmit.sweedee.util.XL_Log;

public class MyAppInstallerReceiver extends BroadcastReceiver {

	XL_Log log = new XL_Log(MyAppInstallerReceiver.class);

	@Override
	public void onReceive(Context context, Intent intent) {
		String packageName = intent.getDataString();
		if (packageName.startsWith("package:")){
			packageName = packageName.substring("package:".length());
		}
		log.debug("packageName="+packageName);
		if (intent.getAction().equals("android.intent.action.PACKAGE_ADDED")) {
			DownloadManager.getInstance().onAppInstallOrUnistall(packageName,true);
		}else if (intent.getAction().equals("android.intent.action.PACKAGE_REMOVED")) {
			AppInstallSQLDao dbHelper =AppInstallSQLDao.getInstance(context);
			dbHelper.deleteDataByPackageName(packageName);
			DownloadManager.getInstance().onAppInstallOrUnistall(packageName,false);
		}
	}
}
