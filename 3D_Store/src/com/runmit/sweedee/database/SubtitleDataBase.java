package com.runmit.sweedee.database;

import java.util.ArrayList;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;

import com.runmit.sweedee.model.VideoCaption;
import com.runmit.sweedee.util.XL_Log;

public class SubtitleDataBase extends SQLiteOpenHelper {

    private XL_Log log = new XL_Log(SubtitleDataBase.class);

    public final int MAX_SIZE = 500;// 限制数据大小，应该没这个大把


    public static class SubTitleNotes {// 网站的字段,收藏夹也在此
        public static final String TABLE_NAME = "subtitle";
        // 字段
        public static final String _ID = "_id";
        public static final String FILENAME = "filename";
        public static final String Local_PATH = "local_path";
        public static final String S_NAME = "sname";
        public static final String LANGUAGE = "language";
        public static final String S_CID = "scid";
        public static final String S_URL = "surl";
    }


    private static final String DB_NAME = "subtitle.db";
    private static final int version = 1;

    public SubtitleDataBase(Context context) {
        super(context, DB_NAME, null, version);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {

        String CREATE_TABLE = "CREATE TABLE " + SubTitleNotes.TABLE_NAME + " (" + SubTitleNotes._ID
                + " INTEGER PRIMARY KEY," + SubTitleNotes.FILENAME + " TEXT," + SubTitleNotes.Local_PATH + " TEXT,"
                + SubTitleNotes.S_NAME + " TEXT,"
                + SubTitleNotes.LANGUAGE + " TEXT,"
                + SubTitleNotes.S_CID + " TEXT,"
                + SubTitleNotes.S_URL + " TEXT " + ");";

        try {
            db.beginTransaction();
            db.execSQL(CREATE_TABLE);
            db.setTransactionSuccessful();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            db.endTransaction();
        }
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {

    }


    public void addSubTitle(ArrayList<VideoCaption> mList) {
        SQLiteDatabase db = null;
        long id = -1;
        try {
            db = this.getWritableDatabase();
            for (int i = 0, size = mList.size(); i < size; i++) {
                VideoCaption mVideoCaption = mList.get(i);
                ContentValues cv = new ContentValues();
                cv.put(SubTitleNotes.FILENAME, mVideoCaption.filename);
                cv.put(SubTitleNotes.Local_PATH, mVideoCaption.local_path);
                cv.put(SubTitleNotes.S_NAME, mVideoCaption.sname);
                cv.put(SubTitleNotes.LANGUAGE, mVideoCaption.language);
                cv.put(SubTitleNotes.S_CID, mVideoCaption.scid);
                cv.put(SubTitleNotes.S_URL, mVideoCaption.surl);

                id = db.insert(SubTitleNotes.TABLE_NAME, null, cv);
            }


        } catch (SQLiteException e) {
            e.printStackTrace();
        } finally {
            if (db != null) {
                db.close();
            }
        }


    }

    public ArrayList<VideoCaption> getVideoCaptionByName(String filename) {
        if (filename == null || filename.equals("")) {
            return null;
        }
        SQLiteDatabase db = null;
        Cursor c = null;
        ArrayList<VideoCaption> sites = new ArrayList<VideoCaption>(MAX_SIZE);
        try {
            db = this.getReadableDatabase();

            c = db.query(SubTitleNotes.TABLE_NAME, null, SubTitleNotes.FILENAME + "=?", new String[]{filename}, null, null, SubTitleNotes._ID + " DESC");
            if (null != c && c.moveToFirst()) {

                int iname = c.getColumnIndex(SubTitleNotes.FILENAME);
                int iLocal_PATH = c.getColumnIndex(SubTitleNotes.Local_PATH);
                int iS_NAME = c.getColumnIndex(SubTitleNotes.S_NAME);
                int iLANGUAGE = c.getColumnIndex(SubTitleNotes.LANGUAGE);
                int iS_CID = c.getColumnIndex(SubTitleNotes.S_CID);
                int iS_URL = c.getColumnIndex(SubTitleNotes.S_URL);

                VideoCaption mVideoCaption = null;
                do {
                    mVideoCaption = new VideoCaption();
                    mVideoCaption.filename = c.getString(iname);
                    mVideoCaption.local_path = c.getString(iLocal_PATH);
                    mVideoCaption.sname = c.getString(iS_NAME);
                    mVideoCaption.language = c.getString(iLANGUAGE);
                    mVideoCaption.scid = c.getString(iS_CID);
                    mVideoCaption.surl = c.getString(iS_URL);
                    sites.add(mVideoCaption);
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
        return sites;
    }


}
