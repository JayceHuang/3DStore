package com.runmit.sweedee.report.sdk;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

public class ReportItem {

	/**
	 * 数据库的主键，只针对查询时候有效
	 */
	int id;
	
	String action;

	JsonObject mReportItemMap;

	/*public*/ ReportItem(String action) {
		this.action = action;
		mReportItemMap = new JsonObject();
	}

	/*public*/ ReportItem entry(String key, int value) {
		mReportItemMap.addProperty(key, value);
		return this;
	}

	/*public*/ ReportItem entry(String key, String value) {
		mReportItemMap.addProperty(key, value);
		return this;
	}
	
	/*public*/ ReportItem entry(String key, long value) {
		mReportItemMap.addProperty(key, value);
		return this;
	}
	
	/*public*/ ReportItem entry(String key, double value) {
		mReportItemMap.addProperty(key, value);
		return this;
	}
	
	/*public*/ ReportItem parse(String jsonString) {
		JsonParser parser = new JsonParser();
		this.mReportItemMap = parser.parse(jsonString).getAsJsonObject();
		return this;
	}
	
	/*public*/ String toJsonString(){
		return mReportItemMap.toString();
	}
}
