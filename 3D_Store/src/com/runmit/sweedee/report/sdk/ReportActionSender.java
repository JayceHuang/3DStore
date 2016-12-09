package com.runmit.sweedee.report.sdk;

import java.util.UUID;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.telephony.TelephonyManager;

import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.util.Util;

public class ReportActionSender {
	
	private static ReportActionSender INSTANCE;

	private Reporter mReporter;

	private String sid;

	private int seq;

	private Context mContext;

	private ReportActionSender() {
		mReporter = new Reporter();
		mContext = StoreApplication.INSTANCE;
		sid=UUID.randomUUID().toString();
		mReporter.init(mContext);
	}
	
	public int getPlayTimeDuration(){
		return mReporter.getPlayTimeDuration();
	}

	public static ReportActionSender getInstance() {
		if (INSTANCE == null) {
			INSTANCE = new ReportActionSender();
		}
		return INSTANCE;
	}
	
	public void Uninit() {
		mReporter.Uninit();
	}
	
	private int getNetworkTypeForInt() {
		if(Util.isWifiNet(mContext)){
			return 1;
		}
		ConnectivityManager connMgr = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo networkInfo = connMgr.getActiveNetworkInfo();
		TelephonyManager telephonyManager = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
		int subType = telephonyManager.getNetworkType();
		if (networkInfo != null && networkInfo.isAvailable()) {
			int nType = networkInfo.getType();
			if (nType == ConnectivityManager.TYPE_MOBILE) {
				switch (subType) {
				case TelephonyManager.NETWORK_TYPE_GPRS:
				case TelephonyManager.NETWORK_TYPE_EDGE:
				case TelephonyManager.NETWORK_TYPE_CDMA:
				case TelephonyManager.NETWORK_TYPE_1xRTT:
				case TelephonyManager.NETWORK_TYPE_IDEN:
					return 2; // NETWORK_CLASS_2_G
				case TelephonyManager.NETWORK_TYPE_UMTS:
				case TelephonyManager.NETWORK_TYPE_EVDO_0:
				case TelephonyManager.NETWORK_TYPE_EVDO_A:
				case TelephonyManager.NETWORK_TYPE_HSDPA:
				case TelephonyManager.NETWORK_TYPE_HSUPA:
				case TelephonyManager.NETWORK_TYPE_HSPA:
				case TelephonyManager.NETWORK_TYPE_EVDO_B:
				case TelephonyManager.NETWORK_TYPE_EHRPD:
				case TelephonyManager.NETWORK_TYPE_HSPAP :
					return 3; // NETWORK_CLASS_3_G
				case TelephonyManager.NETWORK_TYPE_LTE:
					return 4;// NETWORK_CLASS_4_G ;
				default:
					return 0; // NETWORK_CLASS_UNKNOWN ;
				}

			}
		}
		return 0;
    }

	/**
	 * 启动应用的上报
	 */
	public void startUp() {
		ReportItem mReportItem = new ReportItem("startups")
		.entry("sid", sid)
		.entry("dts", System.currentTimeMillis())
		.entry("seq", Integer.toString(seq++))
		.entry("nt", getNetworkTypeForInt());
		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 
	 * @param upid:本次播放过程的唯一标识，在一次播放过程中保持一致，由客户端生成，建议UUID type4
	 * @param online 是在线播放还是本地播放，0 – 未知；1 - 在线；2-本地
	 * @param vid 视频唯一标识，VRS中的videoid，如果不是VRS中的视频，上报为0
	 * @param vnm:视频名称，URLEncode
	 * @param crt:当前的码率，0-未知；1-720p；2-1080p
	 * @param uta:从用户触发播放到播放器收到播放地址所需要的时间，单位为毫秒
	 * @param utb:从播放器收到播放地址到画面播放开始所需要的时间，单位为毫秒
	 * @param continueplay 是否续播：1为续播0为非续播
	 * @param isusercancel 是否用户取消：1为用户取消0为非用户取消
	 */
	public void playInit(String upid,int online,int vid,String vnm,int crt,long uta,long utb,int continueplay,int isusercancel){
		ReportItem mReportItem = new ReportItem("playinits").
				entry("sid", sid).
				entry("dts", System.currentTimeMillis()).
				entry("seq", Integer.toString(seq++))
				.entry("nt", getNetworkTypeForInt())
				.entry("upid", upid)
				.entry("online", online)
				.entry("vid", vid)
				.entry("vnm", vnm)
				.entry("crt", crt)
				.entry("uta", uta)
				.entry("utb", utb)
				//.entry("userid", UserManager.getInstance().getUserId())
				//.entry("continueplay", continueplay)
				//.entry("isusercancel", isusercancel)
				;
		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 播放开始的上报
	 * @param upid
	 * @param online
	 * @param vid
	 * @param vnm
	 * @param crt
	 */
	public void playStart(String upid,int online,int vid,String vnm,int crt){
		ReportItem mReportItem = new ReportItem("playstarts").
				entry("sid", sid).
				entry("dts", System.currentTimeMillis()).
				entry("seq", Integer.toString(seq++))
				.entry("nt", getNetworkTypeForInt())
				.entry("upid", upid)
				.entry("online", online)
				.entry("vid", vid)
				.entry("vnm", vnm)
				.entry("crt", crt)
				//.entry("userid", UserManager.getInstance().getUserId())
				;
		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 播放心跳，在生命周期之外不要计时，在用户暂停播放也不要计时
	 * @param upid
	 * @param online
	 * @param vid
	 * @param vnm
	 * @param crt
	 * @param dur
	 * @param speed
	 */
	public void playTime(String upid,int online,int vid,String vnm,int crt,int dur,int speed){
		ReportItem mReportItem = new ReportItem("playtimes").
				entry("sid", sid).
				entry("dts", System.currentTimeMillis()).
				entry("seq", Integer.toString(seq++))
				.entry("nt", getNetworkTypeForInt())
				.entry("upid", upid)
				.entry("online", online)
				.entry("vid", vid)
				.entry("vnm", vnm)
				.entry("crt", crt)
				.entry("dur", dur)
				.entry("speed", speed);
		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 播放卡顿，卡顿结束后上报。播放器loading状态结束，在buffer_start->buffer_end后上报该事件，记录一次卡顿动作发生
	 * @param upid
	 * @param online
	 * @param vid
	 * @param vnm
	 * @param crt
	 * @param dur 卡顿时间，单位为秒。
	 * @param reason卡顿原因：0-未知；1-正常播放；2-起播；3-拖拽
	 * @param flag卡顿标记：是否卡顿未结束就外部退出，1 - 卡顿正常结束，2 - 本次卡顿由于用户操作提前结束
	 * @param speed当前的缓存速度，单位bytes/s
	 */
	public void playBlock(String upid,int online,int vid,String vnm,int crt,int dur,int reason,int flag,int speed){
		ReportItem mReportItem = new ReportItem("playblocks").
				entry("sid", sid).
				entry("dts", System.currentTimeMillis()).
				entry("seq", Integer.toString(seq++))
				.entry("nt", getNetworkTypeForInt())
				.entry("upid", upid)
				.entry("online", online)
				.entry("vid", vid)
				.entry("vnm", vnm)
				.entry("crt", crt)
				.entry("dur", dur)
				.entry("reason", reason)
				.entry("flag", flag)
				.entry("speed", speed);
		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 
	 * @param upid
	 * @param online
	 * @param vid
	 * @param vnm
	 * @param crt
	 * @param drt0-开始方向拖拽；1-结束方向拖拽
	 */
	public void playSeek(String upid,int online,int vid,String vnm,int crt,int drt){
		ReportItem mReportItem = new ReportItem("playseeks").
				entry("sid", sid).
				entry("dts", System.currentTimeMillis()).
				entry("seq", Integer.toString(seq++))
				.entry("nt", getNetworkTypeForInt())
				.entry("upid", upid)
				.entry("online", online)
				.entry("vid", vid)
				.entry("vnm", vnm)
				.entry("crt", crt)
				.entry("drt", drt);
		mReporter.addReport(mReportItem);
	}
	
	public void clickAction(){
		//TODO 点击事件上报，保留
		throw new RuntimeException();
	}
	
	/**
	 * 
	 * @param dur应用使用时长，单位为毫秒。参见上报时机中关于使用时长的说明
	 */
	public void openDuration(long dur){
		ReportItem mReportItem = new ReportItem("opendurs").
				entry("sid", sid).
				entry("dts", System.currentTimeMillis()).
				entry("seq", Integer.toString(seq++))
				.entry("nt", getNetworkTypeForInt())
				.entry("dur", dur);
		mReporter.addReport(mReportItem);
	}
	
	
	/**
	 * 缓存上报
	 * @param vid 视频id
	 * @param vnm 视频名称
	 * @param crt 码率
	 */
	public void download(int vid,String vnm,int crt){
		ReportItem mReportItem = new ReportItem("downloads").
				entry("sid", sid).
				entry("dts", System.currentTimeMillis()).
				entry("seq", Integer.toString(seq++))
				.entry("nt", getNetworkTypeForInt())
				.entry("vid", vid)
				.entry("vnm", vnm)
				.entry("crt", crt)
				//.entry("userid", UserManager.getInstance().getUserId())
				;
		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 缓存失败上报
	 * @param vid		视频id
	 * @param videoname 视频名称
	 * @param crt		码率
	 */
	public void videoDownloadfail(int vid,String videoname, int crt, int errc){
//		ReportItem mReportItem = new ReportItem("downloadfail").
//				entry("sid", "").
//				entry("dts", System.currentTimeMillis()).
//				entry("seq", Integer.toString(seq++))
//				.entry("nt", getNetworkTypeForInt())
//				.entry("vid", vid)
//				.entry("vnm", videoname)
//				.entry("crt", crt)
//				.entry("errc", errc);
//		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 播放心跳，在生命周期之外不要计时，在用户暂停播放也不要计时
	 * @param upid
	 * @param online
	 * @param vid
	 * @param vnm
	 * @param crt
	 * @param dur
	 * @param speed
	 */
	public void videoDownloadSpeed(int vid, String vnm, int crt, int speed, String cdnurl){
//		ReportItem mReportItem = new ReportItem("downloadspeed").
//				entry("sid", "").
//				entry("dts", System.currentTimeMillis()).
//				entry("seq", Integer.toString(seq++))
//				.entry("nt", getNetworkTypeForInt())
//				.entry("vid", vid)
//				.entry("vnm", vnm)
//				.entry("crt", crt)
//				.entry("speed", speed)
//				.entry("cdnurl", cdnurl);
//		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 错误码上报
	 * @param evtc错误源事件唯一标识，由客户端维护源事件信息
	 * @param errc错误唯一标识，由客户端维护错误信息
	 * 错误码上报模块id和协议id规则，基本规则，四位整数，前两位表示模块id，比如cms模块用01，uc用02，
	 * 后两位代表具体协议，依次累加，比如获取cms详情01，
	 * 则0101表示cms获取详情协议的errors上报
	 */
	public void error(int moduleId,int actionId,int errc){
		int evtc=moduleId*100+actionId;
		ReportItem mReportItem = new ReportItem("errors").
				entry("sid", sid).
				entry("dts", System.currentTimeMillis()).
				entry("seq", Integer.toString(seq++))
				.entry("nt", getNetworkTypeForInt())
				.entry("evtc", evtc)
				.entry("errc", errc);
		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 错误码上报，和上面的区别是直接传入evtc，和handler的msg.what对应
	 * @param evtc
	 * @param errc
	 */
	public void error(int evtc,int errc){
		ReportItem mReportItem = new ReportItem("errors").
				entry("sid", sid).
				entry("dts", System.currentTimeMillis()).
				entry("seq", Integer.toString(seq++))
				.entry("nt", getNetworkTypeForInt())
				.entry("evtc", evtc)
				.entry("errc", errc);
		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 注册上报
	 * @param errc
	 * @param userid
	 */
	public void regist(int errc,int userid){
//		ReportItem mReportItem = new ReportItem("regists").
//				entry("sid", sid).
//				entry("dts", System.currentTimeMillis()).
//				entry("seq", Integer.toString(seq++))
//				.entry("nt", getNetworkTypeForInt())
//				.entry("userid", userid)
//				.entry("errc", errc);
//		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 登陆上报
	 * @param errc
	 * @param userid
	 */
	public void login(int errc,long userid){
//		ReportItem mReportItem = new ReportItem("logins").
//				entry("sid", sid).
//				entry("dts", System.currentTimeMillis()).
//				entry("seq", Integer.toString(seq++))
//				.entry("nt", getNetworkTypeForInt())
//				.entry("userid", userid)
//				.entry("errc", errc);
//		mReporter.addReport(mReportItem);
	}
	
	
	/**
	 * 游戏/应用下载完成上报
	 * 
	 * @param eappkey
	 *            外部应用唯一标识，对应cms系统中的appkey
	 * @param eappver
	 *            app version 外部应用版本
	 * @param eappid
	 *            对应cms系统中的appid
	 * @param eapptype
	 *            应用类型,0 –未知;1– 游戏;2 – 应用
	 * @param dts
	 *            客户端下载完成的时间，单位：毫秒
	 * @param dur
	 *            下载时长，单位:毫秒
	 * @param userid
	 *            登录返回userid 未登录返回 0 
	 */
	public void appdownload(String eappkey, String eappver, String eappid, int eapptype, long dts, long dur,String userid) {
		ReportItem mReportItem = new ReportItem("eappdownloads")
								.entry("eappkey", eappkey)
								.entry("eappver", eappver)
								.entry("eappid", eappid)
								.entry("eapptype", eapptype)
								.entry("dts", dts)
								.entry("dur", dur)
								//.entry("userid", userid)
								;
		mReporter.addReport(mReportItem);
	}

	/**
	 * 游戏/应用下载中换源上报
	 * @param appId
	 *            对应cms系统中的appid
	 * @param appKey
	 *            对应cms系统中的appkey
	 * @param appType
	 *            应用类型,0 –未知;1– 游戏;2 – 应用
	 * @param beforeCDNURL
	 *            换源前的CDNURL
	 * @param afterCNDURL
	 *            换源后的CDNURL
	 * @param dts
	 *            当前时间,单位:毫秒
	 */
	public void appdownloadChangeCDN(String appId, String appKey, int appType, long dts,String beforeCDNURL,String afterCNDURL) {
//		ReportItem reportItem = new ReportItem("eappdownloadchangecdns")
//								.entry("appid", appId)
//								.entry("appkey", appKey)
//								.entry("apptype", appType)
//								.entry("beforecdnurl", beforeCDNURL)
//								.entry("aftercdnurl", afterCNDURL)
//								.entry("userid", UserManager.getInstance().getUserId())
//								.entry("dts", dts);
//		mReporter.addReport(reportItem);	
	}

	/**
	 * 游戏/应用下载失败
	 * @param eappver
	 *            app version 外部应用版本
	 * @param appId
	 *            对应cms系统中的appid
	 * @param appKey
	 *            对应cms系统中的appkey
	 * @param appType
	 *            应用类型,0 –未知;1– 游戏;2 – 应用
	 * @param dts
	 *            当前时间,单位:毫秒
	 * @param errorCode
	 *            错误代码           
	 */

	public void appdownloadFailed(String eappver, String appId, String appKey, int appType, long dts ,int errorCode) {
//		ReportItem reportItem = new ReportItem("eappdownloadfail")
//								.entry("eappver", eappver)
//								.entry("eappid", appId)
//								.entry("eappkey", appKey)
//								.entry("eapptype", appType)
//								.entry("userid", UserManager.getInstance().getUserId())
//								.entry("dts", dts)
//								.entry("errc", errorCode);
//		mReporter.addReport(reportItem);
	}

	/**
	 * 游戏/应用下载速度上报
	 * @param eappver
	 *            app version 外部应用版本
	 * @param appId
	 *            对应cms系统中的appid
	 * @param appKey
	 *            对应cms系统中的appkey
	 * @param appType
	 *            应用类型,0 –未知;1– 游戏;2 – 应用
	 * @param dts
	 *            当前时间,单位:毫秒
	 * @param cdnUrl
	 *            cdnUrl
	 * @param nt
	 *            网络类型
	 */

	public void appdownloadSpeed(String eappver, String appId, String appKey, int appType, long size,long dur,long dts, String cdnUrl) {
//		ReportItem reportItem = new ReportItem("appdownloadspeed")
//								.entry("eappver", eappver)
//								.entry("eappkey", appKey)
//								.entry("eappid", appId)
//								.entry("eapptype", appType)
//								.entry("size", size)
//								.entry("dur", dur)
//								.entry("dts", dts)
//								.entry("cdnurl", cdnUrl)
//								.entry("userid", UserManager.getInstance().getUserId())
//								.entry("nt", getNetworkTypeForInt());
//		mReporter.addReport(reportItem);
	}

	/**
	 * 游戏/应用卸载上报
	 * @param eappver
	 *            app version 外部应用版本
	 * @param appId
	 *            对应cms系统中的appid
	 * @param appKey
	 *            对应cms系统中的appkey
	 * @param appType
	 *            应用类型,0 –未知;1– 游戏;2 – 应用
	 * @param dts
	 *            当前时间,单位:毫秒
	 */

	public void appUninstall(String eappver, String appId, String appKey, int appType, long dts) {
//		ReportItem reportItem = new ReportItem("eappuninstall")
//									.entry("eappver", eappver)
//									.entry("eappkey", appKey)
//									.entry("eappid", appId)
//									.entry("eapptype", appType)
//									.entry("userid", UserManager.getInstance().getUserId())
//									.entry("dts", dts);
//		mReporter.addReport(reportItem);
	}

	/**
	 * 游戏和应用打开/关闭3D功能上报
	 * @param action
	 *            1-打开，0-关闭
	 * @param type
	 *            对象类型：1-视频，2-游戏，3-应用
	 * @param fromuser
	 *            打开关闭操作是否来自用户 ：1-用户，0—非用户
	 * @param rusult
	 *            操作结果：1-成功 ，0-失败
	 */
	public void enable3D(int action, int type, int fromuser, int result,long dts) {
//		ReportItem reportItem = new ReportItem("enable3d")
//								.entry("action", action)
//								.entry("type", type)
//								.entry("fromuser", fromuser)
//								.entry("userid", UserManager.getInstance().getUserId())
//								.entry("sid", sid)
//								.entry("seq", seq)
//								.entry("dts", dts)
//								.entry("result", result);
//		mReporter.addReport(reportItem);
	}

	/**
	 *	当前设备温度
	 * @param temperature
	 *            当前温度
	 * @param dur
	 *            开启3d后的使用时间，单位毫秒
	 */
	public void hotWoring(double temperature, long dts) {
//		ReportItem reportItem = new ReportItem("hotwarning")
//								.entry("temperature", temperature)
//								.entry("userid", UserManager.getInstance().getUserId())
//								.entry("sid", sid)
//								.entry("seq", seq)
//								.entry("dts", dts);
//		
//		mReporter.addReport(reportItem);
	}

	/**
	 * 人眼跟踪失败上报
	 * 
	 * @param errc
	 *            追踪错误代码
	 * @param dur
	 *            眼球追踪时间，单位毫秒
	 */
	public void eyeTrack(int errc, long dur) {
//		ReportItem reportItem = new ReportItem("eyetrack")
//								.entry("errc", errc)
//								.entry("userid", UserManager.getInstance().getUserId())
//								.entry("sid", sid)
//								.entry("seq", seq)
//								.entry("dur", dur);
//		mReporter.addReport(reportItem);
	}

	/**
	 * 游戏/应用安装完成上报
	 * 
	 * @param eappkey
	 *            外部应用唯一标识，对应cms系统中的appkey
	 * @param eappver
	 *            app version 外部应用版本
	 * @param eappid
	 *            对应cms系统中的appid
	 * @param eapptype
	 *            应用类型,0 –未知;1– 游戏;2 – 应用
	 * @param dts
	 *            客户端安装完成时间，单位：毫秒
	 */
	public void appinstall(String eappkey, String eappver, String eappid, int eapptype, long dts) {
		ReportItem mReportItem = new ReportItem("eappinstalls")
								.entry("eappkey", eappkey)
								.entry("eappver", eappver)
								.entry("eappid", eappid)
								.entry("eapptype", eapptype)
								//.entry("userid", UserManager.getInstance().getUserId())
								.entry("dts", dts);
		mReporter.addReport(mReportItem);
	}
	
	/**
	 *搜索上报
	 *@param keyword 搜索关键字,格式:utf-8,禁用json转义字符
	 *@param dts 搜索的时间，单位：毫秒
	 */
	public void search(String keyword, long dts, boolean result) {
		ReportItem mReportItem = new ReportItem("searchs").
				entry("keyword", keyword).
				entry("dts", dts)
				//.entry("result", result ? 1 : 0)
				;
		mReporter.addReport(mReportItem);
	}
	
	
	/**
	 * 应用/游戏启动上报
	 * @param eappkey 外部应用唯一标识，对应cms系统中的appkey
	 * @param pkgName 包名
	 * @param eappver app version 外部应用版本
	 * @param eappid 对应cms系统中的appid
	 * @param eapptype 应用类型,0 –未知;1:APP, 2:GAME
	 * @param dts 启动的时间，单位：毫秒
	 * @param ref 0 –未知;1– 我的游戏;2 – 我的应用，3.详情页，4.首页TAB推荐
    */
	public void appstart(String eappkey,
			String eappver, 
			String eappid, 
			int eapptype, 
			long dts, 
			int ref) {
		ReportItem mReportItem = new ReportItem("eappstarts").
				entry("eappkey", eappkey).
				entry("eappver", eappver).
				entry("eappid", eappid).
				entry("eapptype", eapptype).
				entry("dts", dts).
				//entry("userid", UserManager.getInstance().getUserId()).
				entry("ref", ref);
		mReporter.addReport(mReportItem);
	}

	/**
	 * 支付上报
	 * @param errc:错误码
	 * @param payment_id支付方式
			PayOrder.PAYMENT_ALIPAY=1;//阿里支付
			PayOrder.PAYMENT_UNIONPAY=2;//银联
			PayOrder.PAYMENT_SKYSMS=3;//指易付短信支付
			PayOrder.PAYMENT_WALLET=4;//钱包支付
			PayOrder.PAYMENT_VIRTUAL_WALLET=5;//虚拟货币支付
			PayOrder.PAYMENT_PAYPAL = 8;//paypal
			PayOrder.PAYMENT_WEIXIN = 9;//微信
	 * @param currencyId:币种类型
	 * 1;//人民币
	 * 2;//美元
	 * 3;//金币
	 * 4;//sd币
	 * 5;//新台币
	 * 6;//虚拟币,水滴币
	 * 客户端没有金币和sd币
	 * @param amount支付额度，以分计算
	 * @param productid购买的商品id
	 * @param orderid订单id
	 * @param pay_type
	 * 购买类型
	 * 电影支付1.
	 * 钱包充值2
	 * @param channelId渠道id
	 */
	public void pay(int errc,int payment_id,int currencyId,int amount,int productid,int orderid,int pay_type,int channelId){
//		ReportItem mReportItem = new ReportItem("pays").
//				entry("sid", sid).
//				entry("userid", UserManager.getInstance().getUserId()).
//				entry("dts", System.currentTimeMillis()).
//				entry("seq", Integer.toString(seq++))
//				.entry("nt", getNetworkTypeForInt())
//				.entry("errc", errc)
//				.entry("paymentid", payment_id)
//				.entry("currencyid", currencyId)
//				.entry("amount", amount)
//				.entry("productid", productid)
//				.entry("orderid", orderid)
//				.entry("paytype", pay_type)
//				.entry("channelid", channelId);
//		mReporter.addReport(mReportItem);
	}
	
	/**
	 * 
	 * @param upid本次播放过程的唯一标识，在一次播放过程中保持一致，由客户端生成，建议UUID type4
	 * @param online是否在线
	 * @param crt当前的清晰度，0-未知；1-720p-高清；2-1080p-超清
	 * @param videoid视频唯一标识，VRS中的videoid，如果不是VRS中的视频，上报为0
	 * @param videoname视频名称，URLEncode
	 * @param when0-未知；1-正常播放；2-起播；3-拖拽；4-切换清晰度；5-切换片源
	 * @param errtype错误分类
	 * @param errc播放错误码
	 */
	public void playErrors(String upid,int online,int crt,int videoid,String videoname,int when,int errwhat,int errc){
//		ReportItem mReportItem = new ReportItem("playerrors")
//				.entry("crt", crt)
//				.entry("vid", videoid)
//				.entry("vnm", videoname)
//				.entry("when", when)
//				.entry("errtype", errwhat)
//				.entry("errc", errc)
//				.entry("dts", System.currentTimeMillis())
//				.entry("sid", sid)
//				.entry("seq", Integer.toString(seq++))
//				.entry("nt", getNetworkTypeForInt())
//				.entry("upid", upid)
//				.entry("userid", UserManager.getInstance().getUserId())
//				.entry("online", online);
//		mReporter.addReport(mReportItem);
	}
	
	public void changeSolutions(String upid,int beforecrt,int aftercrt,int videoid,String videoname){
//		ReportItem mReportItem = new ReportItem("changesolutions")
//		.entry("beforecrt", beforecrt)
//		.entry("aftercrt", aftercrt)
//		.entry("vid", videoid)
//		.entry("vnm", videoname)
//		.entry("dts", System.currentTimeMillis())
//		.entry("sid", sid)
//		.entry("seq", Integer.toString(seq++))
//		.entry("nt", getNetworkTypeForInt())
//		.entry("upid", upid)
//		.entry("userid", UserManager.getInstance().getUserId());
//		mReporter.addReport(mReportItem);
	}

	/**
	 * 
	 * @param upid:本次播放过程的唯一标识，在一次播放过程中保持一致，由客户端生成，建议UUID type4
	 * @param type:换源类型。1-起播 2-播放中 3-视频缓存
	 * @param crt:视频码率，0-未知；1-720p；2-1080p
	 * @param videoid
	 * @param videoname
	 */
	public void changeVideoCDN(String upid,int type,int crt,int videoid,String videoname){
//		ReportItem mReportItem = new ReportItem("changecdns")
//		.entry("type", type)
//		.entry("sid", sid)
//		.entry("dts", System.currentTimeMillis())
//		.entry("seq", Integer.toString(seq++))
//		.entry("nt", getNetworkTypeForInt())
//		.entry("vid", videoid)
//		.entry("vnm", videoname)
//		.entry("crt", crt)
//		.entry("upid", upid)
//		.entry("userid", UserManager.getInstance().getUserId());
//		mReporter.addReport(mReportItem);
	}
	
	public static final int MODULE_CMS = 1;
	public static final int MODULE_UC = 2;
	public static final int MODULE_ACCOUNT = 3;	
	
	
}
