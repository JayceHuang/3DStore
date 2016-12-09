package com.runmit.sweedee.provider;

import android.net.Uri;

/**
 * 游戏配置数据库表
 * @author Sven.Zhan
 */
public class GameConfigTable {
	
	/*
	 * AUTHORITY
	 */
	public static final String AUTHORITY = "com.runmit.sweedee.gameConfigProvider";	
	public static final String PATH = "gameConfig";		
	public static final int AUTHORITY_CODE = 100;
	
	//访问Uri content://com.runmit.sweedee.gcProvider/gameConfig	
	public static Uri CONTENT_URI = Uri.parse("content://"+AUTHORITY+"/"+PATH);
		
	/*
	 * 表名
	 */
	public static final String TABLE_NAME = "gameConfigTable";
	
	
	/*
	 * 字段名组
	 */	
	public static final String GAME_ID = "_id";	//主键ID，系统默认	
	public static final String PACKAGE_NAME = "package_name";// 游戏包名 (String)
	public static final String PACKAGE_VERSION = "package_version";	// 游戏版本号 VersionCode （int）
	public static final String X = "x";//配置参数（float）
	public static final String Y = "y";//配置参数（float）
	public static final String Z = "z";//配置参数（float）
	public static final String W = "w";	//配置参数（float）
	public static final String STATUS = "status";	//设置的可用状态（boolean）
	public static final String DATA_VERSION = "data_version"; //该配置数据的版本（int）
	public static final String UPDATE_AT = "update_at";
	
}
