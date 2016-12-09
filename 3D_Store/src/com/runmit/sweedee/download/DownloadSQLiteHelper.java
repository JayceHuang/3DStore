package com.runmit.sweedee.download;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import java.util.ArrayList;

import com.runmit.sweedee.util.XL_Log;

public class DownloadSQLiteHelper extends SQLiteOpenHelper {

	private static final int VERSION = 9;

	private static final String DB_NAME = "runmitdownload.db";

	private DataBaseManager manager = null;
	
	private static Object mSyncObj = new Object();

	public static class DownloadNotes {
		public static final String TABLE_NAME = "installInfo";

		public static final String KEY_ID = "_id";
		public static final String JAVA_BEAN_KEY = "beankey";
		public static final String TITLE = "mTitle";
		public static final String STARTTIME = "mStartTime";
		public static final String FINISHTIME = "mFinishTime";
		public static final String PATH = "mPath";
		public static final String FILESIZE = "mFileSize";
		public static final String DOWNLOAD_URL = "downloadUrl";
		public static final String DOWNLOAD_TYPE = "downloadtype";
		public static final String DOWNLOAD_STATE = "state";
		public static final String ERROR_CODE = "errorcode";
		public static final String USER_DATA = "userdata";
		
		/** add by qihui start */
		public static final String DOWNLOAD_THREAD1_POS = "thread1_pos";
		public static final String DOWNLOAD_THREAD2_POS = "thread2_pos";
		public static final String DOWNLOAD_THREAD3_POS = "thread3_pos";
		public static final String DOWNLOAD_THREAD4_POS = "thread4_pos";
		public static final String DOWNLOAD_THREAD5_POS = "thread5_pos";
		/** add by qihui end */
		
	}

	public DownloadSQLiteHelper(Context context) {
		super(context, DB_NAME, null, VERSION);
		DataBaseManager.initializeInstance(this);
		manager = DataBaseManager.getInstance();
	}

	@Override
	public void onCreate(SQLiteDatabase db) {
		String CREATE_TABLE = "CREATE TABLE " + DownloadNotes.TABLE_NAME + " (" 
				+ DownloadNotes.KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT," 
				
				+ DownloadNotes.JAVA_BEAN_KEY + " TEXT," 
				+ DownloadNotes.TITLE + " TEXT," 
				+ DownloadNotes.STARTTIME + " TEXT,"
				+ DownloadNotes.FINISHTIME + " TEXT,"
				+ DownloadNotes.PATH + " TEXT," 
				+ DownloadNotes.ERROR_CODE + " INTEGER," 
				
				+ DownloadNotes.DOWNLOAD_STATE + " INTEGER," 
				+ DownloadNotes.FILESIZE + " INTEGER," 
				+ DownloadNotes.DOWNLOAD_TYPE + " INTEGER," 
				+ DownloadNotes.DOWNLOAD_URL + " TEXT,"
				+ DownloadNotes.USER_DATA + " TEXT," 
				
				+ DownloadNotes.DOWNLOAD_THREAD1_POS + " LONG,"
				+ DownloadNotes.DOWNLOAD_THREAD2_POS + " LONG,"
				+ DownloadNotes.DOWNLOAD_THREAD3_POS + " LONG," 
				+ DownloadNotes.DOWNLOAD_THREAD4_POS + " LONG," 
				+ DownloadNotes.DOWNLOAD_THREAD5_POS + " LONG" + " )";
		
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
		db.execSQL("DROP TABLE IF EXISTS " + DownloadNotes.TABLE_NAME);
		onCreate(db);
	}

	@Override
	public void onOpen(SQLiteDatabase db) {
		super.onOpen(db);
	}

	public long insertData(DownloadInfo info) {
		SQLiteDatabase db = manager.openDatabase();
		try {
			ContentValues cv = new ContentValues();
			cv.put(DownloadNotes.JAVA_BEAN_KEY, info.getBeanKey());
			cv.put(DownloadNotes.TITLE, info.mTitle);
			cv.put(DownloadNotes.ERROR_CODE, info.errorCode);
			cv.put(DownloadNotes.STARTTIME, info.starttime);
			cv.put(DownloadNotes.FINISHTIME, info.finishtime);
			cv.put(DownloadNotes.PATH, info.mPath);
			cv.put(DownloadNotes.FILESIZE, info.mFileSize);
			cv.put(DownloadNotes.DOWNLOAD_URL, info.downloadUrl);
			cv.put(DownloadNotes.DOWNLOAD_STATE, info.getState());
			cv.put(DownloadNotes.DOWNLOAD_TYPE, info.downloadType);
			cv.put(DownloadNotes.USER_DATA, info.getUserData());
			
			cv.put(DownloadNotes.DOWNLOAD_THREAD1_POS, 0);
			cv.put(DownloadNotes.DOWNLOAD_THREAD2_POS, 0);
			cv.put(DownloadNotes.DOWNLOAD_THREAD3_POS, 0);
			cv.put(DownloadNotes.DOWNLOAD_THREAD4_POS, 0);
			cv.put(DownloadNotes.DOWNLOAD_THREAD5_POS, 0);
			long id = db.insert(DownloadNotes.TABLE_NAME, null, cv);
			info.downloadId = (int) id;
			return id;
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (db != null) {
				manager.closeDatabase();
			}
		}
		return -1;
	}

	/* find one record by hashkey */
	public DownloadInfo findDownloadInfoByBeanKey(String beanKey) {
		if (beanKey == null) {
			return null;
		}
		SQLiteDatabase db = null;
		try {
			db = manager.openDatabase();
			Cursor cursor = db.query(DownloadNotes.TABLE_NAME, null, DownloadNotes.JAVA_BEAN_KEY + "=?", new String[] { beanKey }, null, null, null);
			DownloadInfo mDownloadInfo = null;
			if (cursor != null && cursor.moveToFirst()) {
				mDownloadInfo = getDownloadInfo(cursor);
			}
			cursor.close();
			return mDownloadInfo;
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (db != null) {
				manager.closeDatabase();
			}
		}
		return null;
	}

	private XL_Log log = new XL_Log(DownloadSQLiteHelper.class);
	
	public boolean update(DownloadInfo item) {
		synchronized (mSyncObj) {
			ContentValues cv = new ContentValues();
			cv.put(DownloadNotes.DOWNLOAD_STATE, item.getState());
			cv.put(DownloadNotes.ERROR_CODE, item.errorCode);
			cv.put(DownloadNotes.FILESIZE, item.mFileSize);
			cv.put(DownloadNotes.FINISHTIME, item.finishtime);
			cv.put(DownloadNotes.DOWNLOAD_URL, item.downloadUrl);

			long posses[] = item.getHasDownloadPosses();
			cv.put(DownloadNotes.DOWNLOAD_THREAD1_POS, posses[0]);
			cv.put(DownloadNotes.DOWNLOAD_THREAD2_POS, posses[1]);
			cv.put(DownloadNotes.DOWNLOAD_THREAD3_POS, posses[2]);
			cv.put(DownloadNotes.DOWNLOAD_THREAD4_POS, posses[3]);
			cv.put(DownloadNotes.DOWNLOAD_THREAD5_POS, posses[4]);
			log.debug("DownloadSQLiteHelper update="+item);
			SQLiteDatabase db = null;
			try {
				db = getWritableDatabase();
				String[] id = new String[] { String.valueOf(item.downloadId) };
				db.update(DownloadNotes.TABLE_NAME, cv, DownloadNotes.KEY_ID + "=?", id);
				return true;
			} catch (Exception e) {
				e.printStackTrace();
			} finally {
				if (db != null) {
					db.close();
				}
			}
		}
		return false;
	}

	public void clear() {
		String sql1 = "delete from " + DownloadNotes.TABLE_NAME;
		SQLiteDatabase db = null;
		try {
			db = manager.openDatabase();
			db.execSQL(sql1);
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (db != null) {
				manager.closeDatabase();
			}
		}
	}

	public void clearDownloaded(int downloadId) {
		SQLiteDatabase db = null;
		try {
			db = manager.openDatabase();
			String[] id = new String[] { String.valueOf(downloadId) };
			db.delete(DownloadNotes.TABLE_NAME, DownloadNotes.KEY_ID + "=?", id);
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (db != null) {
				manager.closeDatabase();
			}
		}
	}

	/* get all record */
	public ArrayList<DownloadInfo> getAllDownloadLists() {
		ArrayList<DownloadInfo> list = new ArrayList<DownloadInfo>();
		SQLiteDatabase db = null;
		Cursor cursor = null;
		try {
			db = manager.openDatabase();
			String orderBy = "_id desc";
			cursor = db.query(DownloadNotes.TABLE_NAME, null, null, null, null, null, orderBy);

			if (cursor != null) {
				while (cursor.moveToNext()) {
					list.add(getDownloadInfo(cursor));
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (db != null) {
				manager.closeDatabase();
			}
			if (cursor != null) {
				cursor.close();
			}
		}
		return list;
	}


	private DownloadInfo getDownloadInfo(Cursor cursor) {
		DownloadInfo downloadInfo = null;
		int type = cursor.getInt(cursor.getColumnIndex(DownloadNotes.DOWNLOAD_TYPE));
		if(type == DownloadInfo.DOWNLOAD_TYPE_VIDEO){
			downloadInfo = new VideoDownloadInfo();
		} else {
			downloadInfo = new AppDownloadInfo();
		}
		downloadInfo.downloadId = cursor.getInt(cursor.getColumnIndex(DownloadNotes.KEY_ID));
		
		downloadInfo.setBeanKey(cursor.getString(cursor.getColumnIndex(DownloadNotes.JAVA_BEAN_KEY)));
		downloadInfo.mTitle = cursor.getString(cursor.getColumnIndex(DownloadNotes.TITLE));
		downloadInfo.starttime = cursor.getString(cursor.getColumnIndex(DownloadNotes.STARTTIME));
		downloadInfo.finishtime = cursor.getString(cursor.getColumnIndex(DownloadNotes.FINISHTIME));
		downloadInfo.mPath = cursor.getString(cursor.getColumnIndex(DownloadNotes.PATH));
		downloadInfo.mFileSize = cursor.getLong(cursor.getColumnIndex(DownloadNotes.FILESIZE));
		
		downloadInfo.downloadUrl = cursor.getString(cursor.getColumnIndex(DownloadNotes.DOWNLOAD_URL));
		downloadInfo.downloadType = cursor.getInt(cursor.getColumnIndex(DownloadNotes.DOWNLOAD_TYPE));
		downloadInfo.setDownloadState(cursor.getInt(cursor.getColumnIndex(DownloadNotes.DOWNLOAD_STATE)));
		downloadInfo.errorCode = cursor.getInt(cursor.getColumnIndex(DownloadNotes.ERROR_CODE));
		downloadInfo.decodeUserData(cursor.getString(cursor.getColumnIndex(DownloadNotes.USER_DATA)));

		
		downloadInfo.mHasDownloadPosses[0] = cursor.getLong(cursor.getColumnIndex(DownloadNotes.DOWNLOAD_THREAD1_POS));
		downloadInfo.mHasDownloadPosses[1] = cursor.getLong(cursor.getColumnIndex(DownloadNotes.DOWNLOAD_THREAD2_POS));
		downloadInfo.mHasDownloadPosses[2] = cursor.getLong(cursor.getColumnIndex(DownloadNotes.DOWNLOAD_THREAD3_POS));
		downloadInfo.mHasDownloadPosses[3] = cursor.getLong(cursor.getColumnIndex(DownloadNotes.DOWNLOAD_THREAD4_POS));
		downloadInfo.mHasDownloadPosses[4] = cursor.getLong(cursor.getColumnIndex(DownloadNotes.DOWNLOAD_THREAD5_POS));
		return downloadInfo;
	}
}
