package com.runmit.sweedee.player.subtitle;


import java.util.ArrayList;
import java.util.HashMap;
import java.util.TreeMap;


import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteException;
import android.util.Log;

public class SubTilteDatabaseHelper extends SDSQLiteOpenHelper {

    public static final int VERSION_UNLOAD = 1;
    public static final int VERSION_LOADED = VERSION_UNLOAD + 1;
    /**
     * 版本号。没有加载时为1，加载完成设置为2.不设置onUpgrade
     * 由于字幕文件数据库不可能有版本号增加和数据更改。只有第一次的导入数据和之后的读数据
     * 所以以此来判断是否加载过数据是可行的
     */
    private static final int version = 1;

    private static HashMap<String, SubTilteDatabaseHelper> mHashMap = new HashMap<String, SubTilteDatabaseHelper>(10);

    public static synchronized SubTilteDatabaseHelper getInstance(Context context, String name) {
        if (mHashMap.get(name) != null) {
            return mHashMap.get(name);
        }
        SubTilteDatabaseHelper helper = new SubTilteDatabaseHelper(context, name);
        mHashMap.put(name, helper);
        return helper;
    }


    public SubTilteDatabaseHelper(Context context, String name) {
        super(context, name, null, VERSION_UNLOAD);
    }

    public SubTilteDatabaseHelper(Context context, String name, CursorFactory factory,
                                  int version) {
        super(context, name, factory, version);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        String CREATE_TABLE_CAPTION = "CREATE TABLE " + Caption.TABLE_NAME + " (" + Caption._ID
                + " INTEGER PRIMARY KEY," + Caption.START_TIME + " INTEGER," + Caption.END_TIME + " INTEGER,"
                + Caption.CONTENT + " TEXT" + ");";
        try {
            db.beginTransaction();
            db.execSQL(CREATE_TABLE_CAPTION);
            db.setVersion(VERSION_UNLOAD);
            db.setTransactionSuccessful();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            db.endTransaction();
        }
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // TODO Auto-generated method stub
    }

    @Override
    public void onOpen(SQLiteDatabase db) {
        // TODO Auto-generated method stub
        super.onOpen(db);
    }

    /**
     * 插入一行数据
     *
     * @return 返回新插入的这行数据的Raw ID，如果插入失败返回-1
     */
    public long addCaption(ContentValues values) {
        SQLiteDatabase db = null;
        long id = -1;
        try {
            db = this.getWritableDatabase();
            id = db.insert(Caption.TABLE_NAME, null, values);
            db.close();
        } catch (SQLiteException e) {
            e.printStackTrace();
        }
        return id;
    }


    /**
     * 查找起始时间大于等于startduration的数据列，指定size
     * 方法的一句话概述
     * <p>方法详述（简单方法可不必详述）</p>
     *
     * @param startduration
     * @param size
     * @return
     */
    public ArrayList<Caption> findCaption(int startduration, int size) {
        SQLiteDatabase db = null;
        Cursor c = null;
        ArrayList<Caption> mList = new ArrayList<Caption>(size);

        try {
            String table = Caption.TABLE_NAME;
            String[] columns = new String[]{Caption.START_TIME, Caption.END_TIME, Caption.CONTENT};
            String selection = "start>=?";
            String[] selectionArgs = new String[]{Integer.toString(startduration)};
            String groupBy = null;
            String having = null;
            String orderBy = Caption.START_TIME;
            db = this.getReadableDatabase();
            c = db.query(table, columns, selection, selectionArgs, groupBy, having, orderBy, Integer.toString(size));

            if (null != c && c.moveToFirst()) {
                int istart = c.getColumnIndex(Caption.START_TIME);
                int iend = c.getColumnIndex(Caption.END_TIME);
                int icontent = c.getColumnIndex(Caption.CONTENT);

                Caption mCaption = null;
                do {
                    mCaption = new Caption();
                    mCaption.start = new Time(c.getInt(istart));
                    mCaption.end = new Time(c.getInt(iend));
                    mCaption.content = c.getString(icontent);

                    mList.add(mCaption);
                } while (c.moveToNext());
            }

        } catch (SQLiteException e) {
            e.printStackTrace();
        } finally {
            if (db != null) {
                db.close();
            }
            if (c != null) {
                c.close();
            }

        }
        return mList;
    }
}
