package com.runmit.sweedee.report.sdk;

import java.util.ArrayList;
import java.util.List;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.text.TextUtils;

class ReportDataBase extends SQLiteOpenHelper {

	static class StatisticsNotes {// 统计字段
		public static final String TABLE_NAME = "STATISTICS";
		// 字段
		public static final String ID = "id";
		public static final String ACTION = "action";
		public static final String ENTRY_DATA = "entry_data";
	}

	private static final String DB_NAME = "report_database.db";

	private static final int version = 1;

	ReportDataBase(Context context) {
		super(context, DB_NAME, null, version);
	}

	@Override
	public void onCreate(SQLiteDatabase db) {
		String CREATE_TABLE_Statistics = "CREATE TABLE " + StatisticsNotes.TABLE_NAME + " (" + StatisticsNotes.ID + " INTEGER PRIMARY KEY," + StatisticsNotes.ACTION + " TEXT,"
				+ StatisticsNotes.ENTRY_DATA + " TEXT" + ");";
		try {
			db.beginTransaction();
			db.execSQL(CREATE_TABLE_Statistics);
			db.setTransactionSuccessful();
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			db.endTransaction();
		}
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		try {
			db.beginTransaction();
			onCreate(db);
			db.setTransactionSuccessful();
		} catch (Exception e) {

		} finally {
			db.endTransaction();
		}
	}

	/*public */List<ReportItem> findTopReportItem(int raws) {
		SQLiteDatabase db = null;
		Cursor c = null;
		ArrayList<ReportItem> mList = new ArrayList<ReportItem>();
		try {
			db = this.getReadableDatabase();
			c = db.query(StatisticsNotes.TABLE_NAME, null, null, null, null, null, StatisticsNotes.ID + " ASC ", "0," + String.valueOf(raws));
			if (null != c && c.moveToFirst()) {
				int _id = c.getColumnIndex(StatisticsNotes.ID);
				int action_index = c.getColumnIndex(StatisticsNotes.ACTION);
				int entry_data_index = c.getColumnIndex(StatisticsNotes.ENTRY_DATA);
				ReportItem mReportItem = null;

				do {
					int id = c.getInt(_id);
					String action = c.getString(action_index);
					mReportItem = new ReportItem(action);
					mReportItem.id = id;
					mReportItem.parse(c.getString(entry_data_index));
					mList.add(mReportItem);
				} while (c.moveToNext());
			}
		} catch (SQLiteException e) {
			e.printStackTrace();
		} finally {
			if (c != null) {
				c.close();
			}
			if(db != null){
				db.close();
			}
		}
		return mList;
	}

	int deleteReporterList(List<ReportItem> mList){
		SQLiteDatabase db = null;
		try {
			db =getWritableDatabase();  
	        String[] args = new String[mList.size()];  
	        for(int i = 0,size=mList.size();i<size;i++){
	        	args[i] = ""+mList.get(i).id;
	        }
	        
	        String args1 = TextUtils.join(", ", args);
	        String sql = String.format("DELETE FROM STATISTICS WHERE id IN (%s);", args1);
	        db.execSQL(sql);
	        return 0;  
		} catch (SQLiteException e) {
			e.printStackTrace();
		}finally{
			db.close();
		}
		return -1;
	}
	
	long addReporter(ReportItem mReportItem) {
		SQLiteDatabase db = null;
		long id = -1;
		try {
			ContentValues mValues = new ContentValues();
			mValues.put(StatisticsNotes.ACTION, mReportItem.action);
			mValues.put(StatisticsNotes.ENTRY_DATA, mReportItem.toJsonString());
			db = this.getWritableDatabase();
			id = db.insert(StatisticsNotes.TABLE_NAME, null, mValues);
		} catch (SQLiteException e) {
			e.printStackTrace();
		} finally {
			close(db);
		}
		return id;
	}

	private void close(SQLiteDatabase db) {
		if (null != db) {
			try {
				db.close();
			} catch (SQLiteException e) {
				e.printStackTrace();
			} finally {
				db = null;
			}
		}
	}

}
