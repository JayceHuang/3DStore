package com.runmit.sweedee.manager;

import java.io.Serializable;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpUriRequest;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.util.EntityUtils;

import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent.ShortcutIconResource;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.database.dao.AppInstallSQLDao;
import com.runmit.sweedee.database.dao.AppInstallSQLDao.AppDataModel;
import com.runmit.sweedee.database.dao.AppInstallSQLDao.OnInsertListener;
import com.runmit.sweedee.manager.ObjectCacheManager.DiskCacheOption;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.provider.GameConfig;
import com.runmit.sweedee.provider.GameConfigTable;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.TDString;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

/**
 * 负责应用和游戏3D配置文件的逻辑管理
 * @author Sven.Zhan
 *
 */
public class Game3DConfigManager{
	
	
	XL_Log log = new XL_Log(Game3DConfigManager.class);
	
	private static final String DOMAIN = "http://ceto.d3dstore.com/public/app/configures/";
	
	/**
	 * 获得当前型号和驱动版本下所有适配的应用配置
	 * http://{public.store}/public/app/configures/{module}_{driverVersion}.json
	 */
	private static final String WhiteListUrlBase = DOMAIN+"{0}_{1}.json";
	
	/**
	 * 获得应用配置文件方式
	 * http://{public.store}/public/app/{packageName}/configures/{module}_{driverVersion}_{intervalVersion}.json
	 */
	private static final String ConfigUrlBase = DOMAIN+"{0}/{1}_{2}_{3}";	
	
	private final int TIMEOUT = 30000;
	
	private final long REFUSE_GAP = 3*24*3600*1000L;//用户拒绝配置更新后，三天之内不提醒	
 
	private static final String LAST_REQUEST_GAMECONFIG = "LAST_REQUEST_GAMECONFIG";//白名单上次更新时间	
	private static final String LAST_REFUSE_DOWNLOAD = "LAST_REFUSE_DOWNLOAD";//用户上次拒绝下载游戏配置的时间
	private static final String GAME_3D_STATUS = "GAME_3G_STATUS";
	private static final String HAS_DOWNLAOD_TIP = "HAS_DOWNLAOD_TIP";
	private static final String LAST_REFUSEED = "LAST_REFUSEED";
	private static final String GAMEREFRENCE = "GAMEREFRENCE";
	
	private Uri uri = GameConfigTable.CONTENT_URI;    
	
	private SharePrefence mXLSharePrefence;
	
	private ExecutorService mExecutor = Executors.newCachedThreadPool();
	
	private Gson gson = null;
	private String module;
	private final String driverVersion = String.valueOf(1);//该值在需求变化的过程中已经失去意义，定为1	
	
	private static Game3DConfigManager instance = null;
	
	private DiskCacheOption mDiskCacheOption;
	
	private Context mContext;
	
	private ContentResolver mContentResolver;
	
	private List<PackageInfo> needToUpdatePack = new ArrayList<PackageInfo>();
	
	private List<PackageInfo> selectedPackges = new ArrayList<PackageInfo>();
	
	/**计数器*/
	private int downloadFinshCount = 0;
	
	private int successUpdateNum = 0;
	
	/**
	 * 1 判断是否在配置文件检测的过程中，如果该过程未完成，是无法知道要去更新哪些配置文件的
	 * 2 更新过程是否完成
	 */
	private boolean isStateAvailable = false;
	
	/**
	 * 用户上次拒绝下载游戏配置的时间
	 */
	private long mLastRefuseTime = -1;
	
	private List<GameConfig> mConfigList = new ArrayList<GameConfig>();
	
	private final static boolean CheckVersion = true;
	
	private final static int DATA_VERSION = 1;
	
	private boolean isStatusOn = true;
	
	boolean isSameWithLastRefused = false;
	
	GameWhiteList whiteList = null;		
	
	private Game3DConfigManager(){
		//to-do 获取module和driverVersion的方法未知，暂用服务器的测试值代替
		
		
		mContext = StoreApplication.INSTANCE;
		mXLSharePrefence = SharePrefence.getInstance(mContext);
		gson = new GsonBuilder().create();
		mDiskCacheOption =  new DiskCacheOption().useExternal(false).setAvialableTime(Long.MAX_VALUE);
		mLastRefuseTime = mXLSharePrefence.getLong(LAST_REFUSE_DOWNLOAD, -1);
		
		mContentResolver = mContext.getContentResolver();
		
//		module = "InFocus".toLowerCase();		
		module = Uri.encode(Build.MODEL.toLowerCase());
		
		log.debug("module and version = "+module+" _ "+driverVersion);
		
		isStatusOn = mXLSharePrefence.getBoolean(GAME_3D_STATUS, true);
		
		AppInstallSQLDao.getInstance(mContext).setOnInsertListener(mOnInsertListener);
	}
	
	public static Game3DConfigManager getInstance(){
		if(instance == null){
			instance = new Game3DConfigManager();	
		}
		return instance;
	}
	
	public static void reset(){
		instance = null;
	}
	
	public boolean isGame3DOn(){
		return isStatusOn;
	}
	
	public void toggleGame3D(final boolean enable){
		mExecutor.execute(new Runnable() {			
			@Override
			public void run() {
				synchronized (instance) {
					
					if(isStatusOn == enable){
						return;
					}
					
					isStatusOn = enable;
					mXLSharePrefence.putBoolean(GAME_3D_STATUS, enable);
					//set3DStatusImple(enable);
				}
			}
		});
	}
	
//	//遍历一遍，逐项设置
//	private void set3DStatusImple(boolean enable){
//		 List<GameConfig> list = getAllGameConfig(mContentResolver);
//		 
//		 for(GameConfig gc:list){
//			 ContentValues values = new ContentValues();
//			 values.put(GameConfigTable.PACKAGE_NAME, gc.getPackageName());
//			 values.put(GameConfigTable.PACKAGE_VERSION, gc.getVersionCode());
//			 values.put(GameConfigTable.X, gc.getX());
//			 values.put(GameConfigTable.Y, gc.getY());
//			 values.put(GameConfigTable.Z, gc.getZ());
//			 values.put(GameConfigTable.W, gc.getW());
//			 values.put(GameConfigTable.STATUS, enable);
//			 values.put(GameConfigTable.DATA_VERSION, gc.getDataVersion()); 
//			 mContentResolver.update(uri, values, GameConfigTable.PACKAGE_NAME+"=?", new String[]{gc.getPackageName()});
//		 }
//	}
	
	/**
	 * 检测是否配置文件需要更新，包含以下流程：
	 * 1 根据If-Modified-Since从网络获取配置白名单，若无结果，取本地缓存白名单
	 * 2 比较本地非系统预制应用列表和白名单列表，得到一份在白名单中的应用列表
	 * 3 剔除配置文件已经存在的应用
	 */
	public void checkConfigUpdate(){
		//蜗牛手机无此功能
		if(StoreApplication.isSnailPhone){
			return;
		}		
		mExecutor.submit(new Runnable() {			
			@Override
			public void run() {		
				synchronized (instance) {
					long startTime = System.currentTimeMillis();
					log.debug( "loadWhiteList start");
					
					mConfigList = getAllGameConfig(mContentResolver);
					
					String url  = MessageFormat.format(WhiteListUrlBase, module,driverVersion);//http://ceto.d3dstore.com/public/app/configures/infocus%20m550%203d_1.json
					log.debug("checkConfigUpdate url = "+url);
					
					HttpGet request = new HttpGet(url);
					
					//请求头添加 If-Modified-Since
					String lastModifySince = mXLSharePrefence.getString(LAST_REQUEST_GAMECONFIG, null);
					if(lastModifySince != null){
						request.addHeader("If-Modified-Since", lastModifySince);
					}	
					
					ResultData result = executeGetRequest(request);
							
					
					if(result.resultCode == HttpStatus.SC_OK){
						String jsonStr = null;
						try {
							jsonStr = EntityUtils.toString(result.response.getEntity(),"UTF-8");
							whiteList = gson.fromJson(jsonStr, GameWhiteList.class);
							String lastModify = result.response.getFirstHeader("Last-Modified").getValue();
							log.debug( "loadWhiteList Last-Modified = "+lastModify);
							mXLSharePrefence.putString(LAST_REQUEST_GAMECONFIG, lastModify);
							ObjectCacheManager.getInstance().pushIntoCache(url, whiteList,mDiskCacheOption);
						} catch (Exception e) {						
							e.printStackTrace();
						} 
					}				
					
					if(whiteList == null){
						whiteList = (GameWhiteList) ObjectCacheManager.getInstance().pullFromCache(url, GameWhiteList.class, mDiskCacheOption);
					}					
										
					List<PackageInfo> installedPackageInfo = new ArrayList<PackageInfo>();
					if(whiteList != null && whiteList.data != null){
						List<PackageInfo> packages = getAllApps(mContext);
						for(GameWhiteListItem item : whiteList.data){						
							for(PackageInfo packageInfo : packages){
								if(item.contans(packageInfo)){
									installedPackageInfo.add(packageInfo);
									if(!isConfigExist(packageInfo)){
										needToUpdatePack.add(packageInfo);
									}
								}
							}						
							//for test
//							needToUpdatePack.add(item.toPackageInfo());
						}
					}	
					isStateAvailable = true;	
					
					//检测上次拒绝更新的
					String value = mXLSharePrefence.getString(LAST_REFUSEED, null);
					if(value != null){
						String[] hashcodes = value.split("_");						
						if(hashcodes != null && hashcodes.length > 0){
							List<String> strs = Arrays.asList(hashcodes);
							isSameWithLastRefused = true;
							for(PackageInfo pack : needToUpdatePack){								
								if(!strs.contains(pack.packageName.hashCode()+"")){									
									isSameWithLastRefused = false;
								}
							}
						}
					}
					
					
					//程序卸载后已安装的游戏数据被清除，通过这种方式恢复，其中appId，appKey,startTime 是不可恢复字段
					AppInstallSQLDao appDao = AppInstallSQLDao.getInstance(mContext);
					List<AppDataModel> appModels = appDao.getInstallAppRecordByPackge();
					for(PackageInfo pack : needToUpdatePack){
						boolean hasExist = hasAppModel(appModels, pack);
						if(!hasExist){
							AppDataModel appModel = new AppDataModel();
							appModel.app_type = AppItemInfo.TYPE_GAME;
							appModel.package_name = pack.packageName;
							appModel.app_name = pack.applicationInfo.loadLabel(mContext.getPackageManager()).toString();
							appModel.appId = "";
							appModel.appKey = "";
							appModel.startTime = System.currentTimeMillis()+"";
							appDao.insertData(appModel);
						}
					}					
					log.debug( "loadWhiteList end time is "+(System.currentTimeMillis() - startTime)+
							" needToUpdatePack "+ (needToUpdatePack == null ? "null":needToUpdatePack.size())+" isSameWithLastRefused = "+isSameWithLastRefused);	
				}			
			}
		});
	}	
	
	
	private boolean hasAppModel(List<AppDataModel> models,PackageInfo pack){
		for(AppDataModel model : models){
			if(model.package_name != null && model.package_name.equals(pack.packageName)){
				return true;
			}
		}
		return false;
	}
	
	/**
	 * 判断当前状态下是否有配置更新可用
	 * @return
	 */
	public boolean hasConfigUpdate(){
		
		if(StoreApplication.isSnailPhone){
			return false;
		}
		
		if(!isStateAvailable){
			return false;
		}
		
		//离上次用户拒绝更新还未满三天
		if(mLastRefuseTime > 0 && System.currentTimeMillis() - mLastRefuseTime < REFUSE_GAP && isSameWithLastRefused){
			return false;
		}
		
		if(needToUpdatePack.isEmpty()){
			return false;
		}
		isStateAvailable = false;//只会有一次提醒
		return true;
	}
	
	public void refuseUpdate(){
		 isStateAvailable = false;
		 mXLSharePrefence.putLong(LAST_REFUSE_DOWNLOAD, System.currentTimeMillis());
		 
		 onRefuseUpdate();
	}
	
	/**
	 * 下载游戏配置文件
	 */
	public void downloadConfigFiles(List<PackageInfo> spackages){
		selectedPackges.clear();
		selectedPackges.addAll(spackages);
		isStateAvailable = false;//锁住状态
		for(final PackageInfo pak : selectedPackges){
			mExecutor.submit(new Runnable() {				
				@Override
				public void run() {
					boolean downResult = downloadFileImpl(pak);
					if(downResult){
						successUpdateNum++;
					}
					onUpdateFinished();
				}
			});
		}
	}
	
	public void downloadSingleConfigFile(final String packageName,final int versionCode){
		
		//处理下弹框提示问题
		boolean hasTiped = mXLSharePrefence.getBoolean(HAS_DOWNLAOD_TIP, false);
		if(!hasTiped){
			mXLSharePrefence.putBoolean(HAS_DOWNLAOD_TIP, true);
			new Handler(mContext.getMainLooper()).post(new Runnable() {				
				@Override
				public void run() {
					AlertDialog.Builder builder = new AlertDialog.Builder(mContext,R.style.AlertDialog);
					builder.setTitle(Util.setColourText(mContext, R.string.confirm_title, Util.DialogTextColor.BLUE));
					builder.setMessage(R.string.first_download_gameconfig_tip);
					builder.setNeutralButton(R.string.known, new OnClickListener() {						
						@Override
						public void onClick(DialogInterface dialog, int which) {
							dialog.dismiss();
						}
					});
					AlertDialog alertDialog = builder.create();
					alertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
					alertDialog.show();
				}
			});
		}

		mExecutor.execute(new Runnable() {			
			@Override
			public void run() {
				PackageInfo pak = new PackageInfo();
				pak.packageName = packageName;
				pak.versionCode = versionCode;
				downloadFileImpl(pak);
			}
		});
	}
	
	private boolean downloadFileImpl(PackageInfo pak){
		boolean downResult = false;
		//http://ceto.d3dstore.com/public/app/configures/com.siulun.Camera3D/infocus_1_201411140
		String version = String.format("%s", pak.versionCode);
		String url = MessageFormat.format(ConfigUrlBase, pak.packageName,module,driverVersion,version);	
		
		log.debug("downloadFileImpl url = "+url);
		Log.d("downloadFileImpl", "downloadFileImpl url = "+url);
		HttpGet request = new HttpGet(url);
		ResultData result = executeGetRequest(request);
		if(result.resultCode == HttpStatus.SC_OK){//下载成功			
			String jsonStr;
			try {
				jsonStr = EntityUtils.toString(result.response.getEntity(),"UTF-8");
				ConfigAgs args = gson.fromJson(jsonStr, ConfigAgs.class);
				long updateAt = getUpdateAt(pak.packageName);
				GameConfig config = new GameConfig(pak.packageName, pak.versionCode, args.x, args.y, args.z, args.w, isStatusOn, DATA_VERSION,updateAt);
				insertOrUpdateConfig(config);
				downResult = true;
			} catch (Exception e) {			
				e.printStackTrace();
				log.error("downloadFileImpl name = "+pak.packageName+" e = "+e.getCause());
			}	
		}
		return downResult;
	}
	
	private long getUpdateAt(String packageName){
		return  whiteList == null ? 0 : whiteList.getLastUpdate(packageName);
	}
	
	public int getConverNum(){
		return needToUpdatePack.size();
	}
	
	public List<PackageInfo> getUpdatePackage(){
		return needToUpdatePack;
	}
	
	private synchronized void onUpdateFinished(){
		downloadFinshCount++;
		int allSize = selectedPackges.size();
		log.debug( "onUpdateFinished downloadFinshCount = "+downloadFinshCount+" allSize = "+allSize);
		if(downloadFinshCount >= allSize){//游戏列表更新完成
			log.debug( "Update success  allSize = "+allSize+" successUpdateNum = "+successUpdateNum);
			String tip;
			if(successUpdateNum > 0){
				tip = MessageFormat.format(TDString.getStr(R.string.gameconfig_updated_tip),successUpdateNum);
			}else{
				tip = TDString.getStr(R.string.gameconfig_updated_tip_error);
			}
			Util.showToast(mContext, tip, Toast.LENGTH_SHORT);
		}
		
		onRefuseUpdate();
	}	
	
	private void onRefuseUpdate(){
		// 检测出未下载的配置文件
		List<PackageInfo> leftList = new ArrayList<PackageInfo>(needToUpdatePack);
		leftList.removeAll(selectedPackges);
		String value = "";
		if (!leftList.isEmpty()) {			
			for (PackageInfo pack : leftList) {
				value = value + pack.packageName.hashCode() + "_";
			}
		}
		mXLSharePrefence.putString(LAST_REFUSEED, value);
		log.debug("onRefuseUpdate value = "+value);
	}
	
	private List<PackageInfo> getAllApps(Context context) {  
	    List<PackageInfo> apps = new ArrayList<PackageInfo>();  
	    PackageManager pManager = context.getPackageManager();  
	    //获取手机内所有应用  
	    List<PackageInfo> paklist = pManager.getInstalledPackages(0);  
	    for (int i = 0; i < paklist.size(); i++) {  
	        PackageInfo pak = (PackageInfo) paklist.get(i);  
	        //判断是否为非系统预装的应用程序  
	        if ((pak.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) <= 0) {  
	            // customs applications  
	            apps.add(pak);  
	        }  
	    }  
	    return apps;  
	}  
	
	
	private boolean isConfigExist(PackageInfo pak){
		for(GameConfig gc : mConfigList){
			if(pak.packageName.equals(gc.getPackageName())
					&& (!CheckVersion || pak.versionCode == gc.getVersionCode()) 
					&& (gc.getUpdateAt() != 0 && gc.getUpdateAt() >= getUpdateAt(pak.packageName)) ){
				return true;
			}
		}		
		return false;
	}
	
	private void insertOrUpdateConfig(GameConfig config){
		boolean insert = true;
		for(GameConfig gc : mConfigList){
			if(gc.getPackageName().equals(config.getPackageName()) &&  (!CheckVersion || gc.getVersionCode() == config.getVersionCode())){
				insert = false;
				break;
			}
		}	
		
		ContentValues values = new ContentValues();
		values.put(GameConfigTable.PACKAGE_NAME, config.getPackageName());
		values.put(GameConfigTable.PACKAGE_VERSION, config.getVersionCode());
		values.put(GameConfigTable.X, config.getX());
		values.put(GameConfigTable.Y, config.getY());
		values.put(GameConfigTable.Z, config.getZ());
		values.put(GameConfigTable.W, config.getW());
		values.put(GameConfigTable.STATUS, config.isStatus());
		values.put(GameConfigTable.DATA_VERSION, config.getDataVersion());
		values.put(GameConfigTable.UPDATE_AT, config.getUpdateAt());
		
		if(insert){			
			Uri resultUri = mContentResolver.insert(uri, values);
			log.debug("insertOrUpdateConfig insert uri = "+resultUri);
		}else{
			int num = mContentResolver.update(uri, values, GameConfigTable.PACKAGE_NAME+"=?", new String[]{config.getPackageName()});
			log.debug("insertOrUpdateConfig update num = "+num);
		}
	}
	
	private List<GameConfig> getAllGameConfig(ContentResolver resolver){
		
		if(resolver == null){
			throw new NullPointerException("resolver is null");
		}
		
		List<GameConfig> resultList = new ArrayList<GameConfig>();
		
		Cursor cursor = resolver.query(uri, null,null,null,null);
		
		if (cursor != null && cursor.moveToFirst()) {
			int count = cursor.getCount();
			log.debug("getAllGameConfig Cursor count = "+count);
			for (int i = 0; i < cursor.getCount(); i++) {
				cursor.moveToPosition(i);
				GameConfig gconfig = getGameConfigInstance(cursor);
				resultList.add(gconfig);
			}
		}
		return resultList;
	}	
	
    private GameConfig queryConfig(ContentResolver resolver, String packageName,int versionCode){
			
		Cursor cursor = null;		
		if(versionCode <= 0){
			cursor = resolver.query(uri, null , GameConfigTable.PACKAGE_NAME+"=?", new String[]{packageName}, null);
		}else{
			cursor = resolver.query(uri, null , GameConfigTable.PACKAGE_NAME+"=? and "+GameConfigTable.PACKAGE_VERSION+"=?", new String[]{packageName,String.valueOf(versionCode)}, null);
		}
		
		if (cursor != null && cursor.moveToFirst()) {
			int count = cursor.getCount();
			log.debug("queryConfig Cursor count = "+count);
			for (int i = 0; i < cursor.getCount(); i++) {
				cursor.moveToPosition(i);
				GameConfig gconfig = getGameConfigInstance(cursor);
				if(gconfig != null){
					return gconfig;
				}
			}
		}
		return null;
	}
	
	private static GameConfig getGameConfigInstance(Cursor currentCursor){		
		try{
			int PACKAGE_NAME_index = currentCursor.getColumnIndex(GameConfigTable.PACKAGE_NAME);
			int PACKAGE_VERSION_index = currentCursor.getColumnIndex(GameConfigTable.PACKAGE_VERSION);
			int X_index = currentCursor.getColumnIndex(GameConfigTable.X);
			int Y_index = currentCursor.getColumnIndex(GameConfigTable.Y);
			int Z_index = currentCursor.getColumnIndex(GameConfigTable.Z);
			int W_index = currentCursor.getColumnIndex(GameConfigTable.W);
			int STATUS_index = currentCursor.getColumnIndex(GameConfigTable.STATUS);
			int DATA_VERSION_index = currentCursor.getColumnIndex(GameConfigTable.DATA_VERSION);
			int update_index = currentCursor.getColumnIndex(GameConfigTable.UPDATE_AT);
			
			String packageName = currentCursor.getString(PACKAGE_NAME_index);
			int versionCode = currentCursor.getInt(PACKAGE_VERSION_index);
			float x = currentCursor.getFloat(X_index);
			float y = currentCursor.getFloat(Y_index);
			float z = currentCursor.getFloat(Z_index);
			float w = currentCursor.getFloat(W_index);
			boolean status = currentCursor.getInt(STATUS_index) == 1;
			int dataVersion = currentCursor.getInt(DATA_VERSION_index);
			long updateAt = currentCursor.getLong(update_index);	
			
			return new GameConfig(packageName, versionCode, x, y, z, w, status, dataVersion,updateAt);
		}catch(Exception e){
			e.printStackTrace();
		}
		return null;
	}
		
	private ResultData executeGetRequest(HttpUriRequest request){
		
		//通用头部
		request.addHeader("Content-Type","application/json");
		
		BasicHttpParams httpParams = new BasicHttpParams();
		HttpConnectionParams.setConnectionTimeout(httpParams,TIMEOUT);
		HttpConnectionParams.setSoTimeout(httpParams, TIMEOUT);
		HttpClient client = new DefaultHttpClient(httpParams);		
		int resultCode = ExceptionCode.ERROR_DEFUALT;
		try {
			HttpResponse httpResponse = client.execute(request);
			resultCode = httpResponse.getStatusLine().getStatusCode();
			return new ResultData(resultCode, httpResponse);
		} catch (Exception e) {			
			e.printStackTrace();
			resultCode = ExceptionCode.getErrorCode(e);
		}
		return new ResultData(resultCode, null);
	}	
	
	private OnInsertListener mOnInsertListener = new OnInsertListener() {
		
		boolean hasInsert = false;
		
		@Override
		public void onInsert() {
			if(hasInsert) return;
			
			boolean hasSpInsert = mXLSharePrefence.getBoolean(GAMEREFRENCE, false);
			if(hasSpInsert || StoreApplication.isSnailPhone){
				return;
			}
        	
			Intent sIntent = new Intent(Intent.ACTION_MAIN);
			sIntent.addCategory(Intent.CATEGORY_DEFAULT);// 加入action,和category之后，程序卸载的时候才会主动将该快捷方式也卸载
			sIntent.setClass(mContext, com.runmit.sweedee.action.more.MyThreedGameActivity.class);

			ShortcutIconResource iconRes = Intent.ShortcutIconResource.fromContext(mContext, R.drawable.games_icon);        	
			Intent installer = new Intent();
			installer.putExtra("duplicate", false);
			installer.putExtra(Intent.EXTRA_SHORTCUT_INTENT, sIntent);
			installer.putExtra(Intent.EXTRA_SHORTCUT_NAME, mContext.getString(R.string.stereo_game));
			installer.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE, iconRes);
			installer.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
			mContext.sendBroadcast(installer);
			hasInsert = true;
			mXLSharePrefence.putBoolean(GAMEREFRENCE, true);
		}
	};
	
	private class ResultData{
		
		public int resultCode;
		
		public HttpResponse response;
		
		public ResultData(int resultCode, HttpResponse response) {
			this.resultCode = resultCode;
			this.response = response;
		}
	}
	
	public static class GameWhiteList implements Serializable{
		
		private static final long serialVersionUID = -3433144761491716565L;

		//更新时间，timemillis
		public long updatedAt;
		
		public ArrayList<GameWhiteListItem> data;
		
		public long getLastUpdate(String packageName){
			GameWhiteListItem temp = new GameWhiteListItem();
			temp.packageName = packageName;
			if(data != null){
				int index = data.indexOf(temp);
				if(index >= 0){
					return data.get(index).updatedAt;
				}
			}
			return 0;
		}
	}
	
	public static class GameWhiteListItem implements Serializable{
		
		private static final long serialVersionUID = -5761371149816233484L;

		public String packageName;
		
		public ArrayList<String> internalVersions;
		
		public long updatedAt;
		
		public boolean contans(PackageInfo packageInfo){
			boolean rule1 = packageName != null && packageName.equals(packageInfo.packageName);
			boolean rule2 = !CheckVersion || (internalVersions != null && internalVersions.contains(String.valueOf(packageInfo.versionCode)));
			return  rule1 && rule2;			
		}
		
		//for test method
		public PackageInfo toPackageInfo(){
			PackageInfo pack = new PackageInfo();
			pack.packageName = packageName;
			pack.versionCode = toInt(internalVersions.get(0));
			return pack;
		}
		
		private int toInt(String s){
			try{
				int version = Integer.parseInt(s);
				return version;
			}catch(Exception e){
				return 0;
			}
		}

		@Override
		public int hashCode() {
			final int prime = 31;
			int result = 1;
			result = prime * result + ((packageName == null) ? 0 : packageName.hashCode());
			return result;
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj)
				return true;
			if (obj == null)
				return false;
			if (getClass() != obj.getClass())
				return false;
			GameWhiteListItem other = (GameWhiteListItem) obj;
			if (packageName == null) {
				if (other.packageName != null)
					return false;
			} else if (!packageName.equals(other.packageName))
				return false;
			return true;
		}
	}
	
	public static class ConfigAgs {
		public float x;
		public float y;
		public float z;
		public float w;
	}
}
