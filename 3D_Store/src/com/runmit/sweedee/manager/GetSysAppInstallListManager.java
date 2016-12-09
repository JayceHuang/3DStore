package com.runmit.sweedee.manager;

import java.util.ArrayList;
import java.util.List;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;

import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.database.dao.AppInstallSQLDao;
import com.runmit.sweedee.database.dao.AppInstallSQLDao.AppDataModel;
import com.runmit.sweedee.util.XL_Log;

public class GetSysAppInstallListManager {
	
	public static final int TYPE_ALL=0;
	public static final int TYPE_APP=1;
	public static final int TYPE_GAME=2;
	
    public static final String STEREO_PLAYER = "com.runmit.player";
    public static final String STEREO_GALLERY = "com.runmit.gallery";
    
	XL_Log log = new XL_Log(GetSysAppInstallListManager.class);

    public static class AppInfo {
        public int versionCode = 0;
        public String appName = "";
        public String packagename = "";
        public String versionname = "";
        public Drawable appicon = null;
        public int type;
    }
    
    public GetSysAppInstallListManager(){
    	checkDefaultApp();
    }
    
    private void checkDefaultApp(){
    	PackageManager pkMng = StoreApplication.INSTANCE.getPackageManager();
    	AppInstallSQLDao mAppInstallSQLDao = AppInstallSQLDao.getInstance(StoreApplication.INSTANCE);
    	
    	try {
    		if(mAppInstallSQLDao.findAppInfoByPackageName(STEREO_PLAYER) == null || pkMng.getPackageInfo(STEREO_PLAYER, 0) != null){
    			AppDataModel model = new AppDataModel();
    	        model.package_name = STEREO_PLAYER;
    	        model.app_type = 1;//默认为1
    	        model.appId = "";//cms中的对应appid
    	        model.appKey = "";//cms中的对应appkey
    	        model.startTime = System.currentTimeMillis() + "";//任务开始下载时间
    	        mAppInstallSQLDao.insertData(model);
    		}
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
    	
    	try {
    		if(mAppInstallSQLDao.findAppInfoByPackageName(STEREO_GALLERY) == null || pkMng.getPackageInfo(STEREO_GALLERY, 0) != null){
    			AppDataModel model = new AppDataModel();
    			model = new AppDataModel();
    	        model.package_name = STEREO_GALLERY;
    	        model.app_type = 1;//默认为1
    	        model.appId = "";//cms中的对应appid
    	        model.appKey = "";//cms中的对应appkey
    	        model.startTime = System.currentTimeMillis() + "";//任务开始下载时间
    	        mAppInstallSQLDao.insertData(model);
    		}
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
    }
    
    public List<AppInfo> getAppinfoListByType(int request_type) {
    	ArrayList<AppInfo> mAppInfoList = new ArrayList<AppInfo>();
    	
    	AppInstallSQLDao dbHelper = AppInstallSQLDao.getInstance(StoreApplication.INSTANCE);
        ArrayList<AppDataModel> installedApp = dbHelper.getInstallAppRecordByPackge();
    	PackageManager pkMng = StoreApplication.INSTANCE.getPackageManager();
    	for(AppDataModel mItem : installedApp){
    		PackageInfo packageInfo;
            try {
                packageInfo = pkMng.getPackageInfo(mItem.package_name, 0);
                AppInfo tmpInfo = new AppInfo();
                
                tmpInfo.appName = packageInfo.applicationInfo.loadLabel(pkMng).toString();
                tmpInfo.packagename = packageInfo.packageName;
                tmpInfo.versionname = packageInfo.versionName;
                tmpInfo.versionCode = packageInfo.versionCode;
                tmpInfo.appicon = packageInfo.applicationInfo.loadIcon(pkMng);
                
                tmpInfo.type = mItem.app_type;
                if(request_type==TYPE_ALL || tmpInfo.type==request_type){
                	mAppInfoList.add(tmpInfo);
                }
                
            } catch (Exception e) {
                e.printStackTrace();
            }
    	}
    	return mAppInfoList;
    }
}
