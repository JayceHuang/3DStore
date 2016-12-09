package com.runmit.sweedee.database.dao;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

import com.runmit.sweedee.database.AppInstallSQLiteHelper;
import com.runmit.sweedee.model.AppItemInfo;

import java.util.ArrayList;

/**
 * 数据库的操作类
 */
public class AppInstallSQLDao {
    private AppInstallSQLiteHelper helper;

    private AppInstallSQLDao(Context context) {
        helper = new AppInstallSQLiteHelper(context);
    }
    
    private static AppInstallSQLDao instance;
    
    private OnInsertListener mOnInsertListener;

    public static AppInstallSQLDao getInstance(Context context) {
        if (instance == null) {
            instance = new AppInstallSQLDao(context);
        }
        return instance;
    }

    public static class AppDataModel {
        public String package_name;
        public String app_name;
        public int app_type = 1;//默认为1
        public String appId;//cms中的对应appid
        public String appKey;//cms中的对应appkey
        public String startTime;//任务开始下载时间
    }



    /*insert one record*/
    public void insertData(AppDataModel data) {
        SQLiteDatabase db = null;
        try {
            db = helper.getReadableDatabase();
            String insertString = "insert into " +  AppInstallSQLiteHelper.infoNotes.TABLE_NAME + " values ( ? , ? , ? , ? , ? , ? ) ";
            db.execSQL(insertString, new Object[]{data.package_name, data.app_name, data.app_type, data.appId, data.appKey, data.startTime});
            if(mOnInsertListener != null && data.app_type == AppItemInfo.TYPE_GAME){
            	mOnInsertListener.onInsert();
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (db != null) {
                db.close();
            }
        }
    }


    /*delete one record by package name*/
    public void deleteDataByPackageName(String packageName) {
        SQLiteDatabase db = null;
        try {
            db = helper.getReadableDatabase();
            String whereClause = "PACKEGE_NAME=?";
            int nret = db.delete(AppInstallSQLiteHelper. infoNotes.TABLE_NAME, whereClause, new String[]{packageName});
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (db != null) {
                db.close();
            }
        }
    }

    /*find one record by package name*/
    public AppDataModel findAppInfoByPackageName(String packageName) {
        if (packageName == null) {
            return null;
        }
        SQLiteDatabase db = null;
        try {
            db = helper.getReadableDatabase();
            Cursor cursor = db.query(AppInstallSQLiteHelper .infoNotes.TABLE_NAME, null, AppInstallSQLiteHelper.infoNotes._PACKEGE_NAME + "=?", new String[]{String.valueOf(packageName)}, null, null, null);
            AppDataModel dataModel = null;
            if (cursor != null && cursor.moveToFirst()) {
                dataModel = new AppDataModel();
                dataModel.package_name = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._PACKEGE_NAME));
                dataModel.app_name = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_NAME));
                dataModel.app_type = cursor.getInt(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_TYPE));

                dataModel.appId = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_CMSID));
                dataModel.appKey = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_CMS_KEY));
                dataModel.startTime = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_START_TIME));
            }
            return dataModel;
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (db != null) {
                db.close();
            }
        }
        return null;
    }

    /*get all record*/
    public ArrayList<AppDataModel> getInstallAppRecordByPackge() {
        ArrayList<AppDataModel> list = new ArrayList<AppDataModel>();
        SQLiteDatabase db = null;
        Cursor cursor = null;
        try {
        	db = helper.getReadableDatabase();
            cursor = db.query(AppInstallSQLiteHelper.infoNotes.TABLE_NAME, null, null, null, null, null, null);
            if (cursor != null) {
                while (cursor.moveToNext()) {
                    AppDataModel dataModel = new AppDataModel();
                    dataModel.package_name = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._PACKEGE_NAME));
                    dataModel.app_name = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_NAME));
                    dataModel.app_type = cursor.getInt(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_TYPE));

                    dataModel.appId = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_CMSID));
                    dataModel.appKey = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_CMS_KEY));
                    dataModel.startTime = cursor.getString(cursor.getColumnIndex(AppInstallSQLiteHelper.infoNotes._APP_START_TIME));

                    list.add(dataModel);
                }
            }
		} catch (Exception e) {
			e.printStackTrace();
		}finally{
			if(cursor != null){
				cursor.close();
			}
			if(db != null){
				db.close();
			}
		}
        return list;
    }
    
    public void setOnInsertListener(OnInsertListener mOnInsertListener) {
		this.mOnInsertListener = mOnInsertListener;
	}

	public interface OnInsertListener{
    	public void onInsert();
    }
}
