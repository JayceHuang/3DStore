package com.runmit.sweedee.datadao;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.sqlite.SQLiteDatabase;
import android.preference.PreferenceManager;

import com.runmit.sweedee.datadao.DaoMaster.DevOpenHelper;

/**
 * @author Sven.Zhan
 * 
 * 数据库Dao统一管理类
 */
public class DataBaseManager {

	private static final String SP_DB_VERSION = "DB_VERSION";

	private static final String DB_NAME = "3DStroeDB";

	private static final int INITAIL_VERSION = 1000;

	private SQLiteDatabase db;
	private DaoMaster daoMaster;
	private DaoSession daoSession;

	/** 饿汉式单例 */
	private static DataBaseManager instance = new DataBaseManager();

	public static DataBaseManager getInstance() {
		return instance;
	}

	private DataBaseManager() {
	
	}

	public boolean init(Context ctx) {
		DevOpenHelper helper = new DaoMaster.DevOpenHelper(ctx, DB_NAME, null);
		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(ctx);

		int lastDBVersion = sp.getInt(SP_DB_VERSION, INITAIL_VERSION);

		db = helper.getWritableDatabase();
		if (db == null) {
			return false;
		}

		daoMaster = new DaoMaster(db);
		daoSession = daoMaster.newSession();
		playRecordDao = daoSession.getPlayRecordDao();
		searchHistoryDao = daoSession.getSearchHisRecordDao();

		if (lastDBVersion < DaoMaster.SCHEMA_VERSION) {
			// reCreateTableOfPlayRecord();
			// sp.edit().putInt(SP_DB_VERSION,DaoMaster.SCHEMA_VERSION).commit();
		}
		return true;
	}

	public PlayRecordDao getPlayRecordDao() {
		return playRecordDao;
	}

	public void reCreateTableOfPlayRecord() {
		PlayRecordDao.dropTable(db, true);
		PlayRecordDao.createTable(db, true);
	}

	/** 表对象 后续新增 */
	private PlayRecordDao playRecordDao;
	
	// 搜索本地记录 
	public SearchHisRecordDao getSearchHisDao() {
		return searchHistoryDao;
	}
	
	/** 表对象 后续新增 */
	private SearchHisRecordDao searchHistoryDao;
}
