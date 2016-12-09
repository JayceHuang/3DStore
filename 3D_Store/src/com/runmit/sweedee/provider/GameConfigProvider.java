package com.runmit.sweedee.provider;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;

import com.runmit.sweedee.util.XL_Log;

public class GameConfigProvider extends ContentProvider {

	private static final int DATABASE_VERSION = 2;	
	
	private static final String DATABASE_NAME = "GameConfig.db";	
	
	private static final UriMatcher sUriMatcher;
	
	private DatabaseHelper mOpenHelper;
	
	static {
		sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
		sUriMatcher.addURI(GameConfigTable.AUTHORITY,GameConfigTable.PATH,GameConfigTable.AUTHORITY_CODE);
	}

	@Override
	public boolean onCreate() {
		mOpenHelper = new DatabaseHelper(getContext());
		return true;
	}

	@Override
	public Cursor query(Uri uri, String[] projection, String selection,String[] selectionArgs, String sortOrder) {
		SQLiteDatabase db;
		try{
			db = mOpenHelper.getWritableDatabase();
		}catch(Exception e){
			return null;
		}
		SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
		int code = sUriMatcher.match(uri);
		switch (code) {
		case GameConfigTable.AUTHORITY_CODE:
			qb.setTables(GameConfigTable.TABLE_NAME);
			break;
		default:
			throw new IllegalArgumentException("Unknown URI " + uri);
		}		
		Cursor c = qb.query(db, projection, selection, selectionArgs, null, null, sortOrder);
		return c;
	}

	@Override
	public String getType(Uri uri) {		
		return null;
	}

	@Override
	public Uri insert(Uri uri, ContentValues initialValues) {
		SQLiteDatabase db;
		try{
			db = mOpenHelper.getWritableDatabase();
		} catch(Exception e) {
			return uri;
		}
		int code = sUriMatcher.match(uri);
		String tableName = null;
		switch (code) {		
		case GameConfigTable.AUTHORITY_CODE:
			tableName = GameConfigTable.TABLE_NAME;
			break;
		default:
			throw new IllegalArgumentException("Unknown URI " + uri);
		}
		ContentValues values;
		if (initialValues != null) {
			values = new ContentValues(initialValues);
		} else {
			values = new ContentValues();
		}
		
		long rowId = db.insert(tableName, null, values);
		if(0 < rowId) {
			getContext().getContentResolver().notifyChange(uri, null);
			return uri;
		}
		throw new SQLException("Failed to insert row into " + uri);
	}

	@Override
	public int delete(Uri uri, String selection, String[] selectionArgs) {
		SQLiteDatabase db;
		try{
			db = mOpenHelper.getWritableDatabase();
		} catch(Exception e) {
			return 0;
		}
		
		int code = sUriMatcher.match(uri);
		String tableName = null;
		switch (code) {
		case GameConfigTable.AUTHORITY_CODE:
			tableName = GameConfigTable.TABLE_NAME;
			break;
		default:
			throw new IllegalArgumentException("Unknown URI " + uri);
		}
		int count = db.delete(tableName, selection, selectionArgs);
		if(0 < count) {
			getContext().getContentResolver().notifyChange(uri, null);
		}
		return count;
		
	}

	@Override
	public int update(Uri uri, ContentValues values, String selection,String[] selectionArgs) {
		SQLiteDatabase db;
		try{
			db = mOpenHelper.getWritableDatabase();
		} catch(Exception e) {
			return 0;
		}
		int code = sUriMatcher.match(uri);
		String tableName = null;
		switch (code) {
		case GameConfigTable.AUTHORITY_CODE:
			tableName = GameConfigTable.TABLE_NAME;
			break;
		default:
			throw new IllegalArgumentException("Unknown URI " + uri);
		}
		int count = db.update(tableName, values, selection, selectionArgs);
		if(0 < count) {
			getContext().getContentResolver().notifyChange(uri, null);
		}
		return count;		
	}
	
	private static class DatabaseHelper extends SQLiteOpenHelper{
		
		XL_Log log=new XL_Log(GameConfigProvider.class);
		
		private DatabaseHelper(Context context){
			super(context, DATABASE_NAME, null, DATABASE_VERSION);
			log.debug("DataBase Version:"+DATABASE_VERSION);
		}
		
		@Override
		public void onCreate(SQLiteDatabase db) {
			log.debug("[DatabaseHelper] onCreate");
			createTables(db);			
		}

		private void createTables(SQLiteDatabase db) {
			
			db.execSQL("CREATE TABLE " + GameConfigTable.TABLE_NAME + " ("
					+ GameConfigTable.GAME_ID + " INTEGER PRIMARY KEY, "
					+ GameConfigTable.PACKAGE_NAME + " TEXT NOT NULL, "
					+ GameConfigTable.PACKAGE_VERSION + " INTEGER, "
					+ GameConfigTable.X + " FLOAT, "
					+ GameConfigTable.Y + " FLOAT, "
					+ GameConfigTable.Z + " FLOAT, "
					+ GameConfigTable.W + " FLOAT, "
					+ GameConfigTable.STATUS + " BOOLEAN, "
					+ GameConfigTable.UPDATE_AT + " INTEGER, "
					+ GameConfigTable.DATA_VERSION + " INTEGER" + ");");
		}

		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
			db.execSQL("DROP TABLE IF EXISTS " + GameConfigTable.TABLE_NAME);
			createTables(db);
		}	
	}
}
