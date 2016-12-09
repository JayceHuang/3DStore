package com.runmit.sweedee.report.sdk;


/*** * 
 * @author Sven.Zhan
 * 服务事件码和统计上报码的映射
 */
public class ReportEventCode {		
	
	/**UC 后续事件在此补充*/
	public static class UcReportCode{		
		public static final int UC_REPORT_REGISTER = 1;//注册/01
		public static final int UC_REPORT_GET_AUTHCODE = 2;//获取验证码/02
		public static final int UC_REPORT_LOGIN = 3;//登陆/03
		public static final int UC_REPORT_UNBIND = 4;//解除绑定04
		public static final int UC_REPORT_LOGOUT = 5;//注销/05
		public static final int UC_REPORT_AUTHENTICATION = 6;//鉴权/06
	}
	
	public static class DownloadReportCode{		
		public static final int APP_REPORT_DOWNLOADFAIL = 1;//下载失败
	}
}
