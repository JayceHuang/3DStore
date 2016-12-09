package com.runmit.sweedee.report.sdk;


public class ReportConfig {
	
	int hb;//延时上报间隔，单位为秒，客户端应该更新本地保存的延迟上报的时间间隔（详见下面的批量延时上报接口）。默认180秒
	
	int phb;//播放心跳检测间隔，单位为秒，客户端应该更新本地保存的延迟上报的时间间隔。默认180秒
	
	int limit;//一次批量上报的最大事件数量，默认200

	@Override
	public String toString() {
		return "ReportConfig [hb=" + hb + ", phb=" + phb + ", limit=" + limit + "]";
	}
	
	
}
