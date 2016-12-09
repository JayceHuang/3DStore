package com.runmit.sweedee.database;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

/**
 * 数据库的生成
 */
public class AppInstallSQLiteHelper extends SQLiteOpenHelper {

    private static final int VERSION = 6;

    private static final String DB_NAME = "appInstallInfo.db";

    public AppInstallSQLiteHelper(Context context) {
        super(context, DB_NAME, null, VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        // TODO Auto-generated method stub
        String CREATE_TABLE = "CREATE TABLE  if not exists " + infoNotes.TABLE_NAME +
                " ("
                + infoNotes._PACKEGE_NAME + " TEXT PRIMARY KEY,"
                + infoNotes._APP_NAME + " TEXT,"
                + infoNotes._APP_TYPE + " Integer,"
                + infoNotes._APP_CMSID + " TEXT,"
                + infoNotes._APP_CMS_KEY + " TEXT,"
                + infoNotes._APP_START_TIME + " TEXT"
                + " )";
        try {
            db.beginTransaction();
            db.execSQL(CREATE_TABLE);
            db.setTransactionSuccessful();
        } catch (Exception e) {
            // TODO: handle exception
            e.printStackTrace();
        } finally {
            db.endTransaction();
        }
    }


    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // TODO Auto-generated method stub
    }

    public static class infoNotes {
        public static final String TABLE_NAME = "installInfo";
        public static final String _PACKEGE_NAME = "PACKEGE_NAME";           //app packege name
        public static final String _APP_NAME = "APP_NAME";                   //app name
        public static final String _APP_TYPE = "APP_TYPE";                   //app type,game or common app
        public static final String _APP_CMSID = "APP_CMSID"; //cms中的对应appid
        public static final String _APP_CMS_KEY = "APP_KEY"; //cms中的对应appkey
        public static final String _APP_START_TIME = "APP_START_TIME"; //任务开始下载时间
    }

 
}
