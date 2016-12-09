/**
 * 
 */
package com.runmit.sweedee.manager;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

import org.json.JSONObject;

import android.content.Context;
import android.os.Handler;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.loopj.android.http.RequestHandle;
import com.runmit.sweedee.action.pay.PayPalActivity;
import com.runmit.sweedee.manager.ObjectCacheManager.DiskCacheOption;
import com.runmit.sweedee.manager.data.EventCode.AccountEventCode;
import com.runmit.sweedee.model.PayOrder;
import com.runmit.sweedee.model.PayStyle;
import com.runmit.sweedee.model.PayStyleList;
import com.runmit.sweedee.model.Quota;
import com.runmit.sweedee.model.QuotaList;
import com.runmit.sweedee.model.RechargeHistory;
import com.runmit.sweedee.model.UserAccount;
import com.runmit.sweedee.model.VOPermission;
import com.runmit.sweedee.model.VOPrice;
import com.runmit.sweedee.model.VOUserPurchase;
import com.runmit.sweedee.model.VOUserPurchase.PurchaseRecord;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.update.Config;
import com.runmit.sweedee.util.XL_Log;

/**
 * @author Sven.Zhan 计费系统（Accounting）相关接口封装
 */
public class AccountServiceManager extends HttpServerManager {
	
	private XL_Log log = new XL_Log(AccountServiceManager.class);
	
	/**CMS 代理计费系统的BaseUrl*/
	protected static final String CMS_ACCOUNT_BASEURL = CMS_V1_BASEURL + "pay/";
	
	private static final String PayOrderHostUrl="https://poseidon.d3dstore.com/api/v1/pay/goodsSpend/add";
	
	private static final String ChargeHostUrl="https://poseidon.d3dstore.com/api/v1/pay/userCharge/add";
	
	private static final String PaySuccessUrl="http://poseidon.d3dstore.com/api/v1/pay/userPay/paying?orderId=";

	private static AccountServiceManager instance;
	
	private static final String SkyPayAppId = "7003263";

	public synchronized static AccountServiceManager getInstance() {
		if (instance == null) {
			instance = new AccountServiceManager();
		}
		return instance;
	}

	private AccountServiceManager() {
		TAG = AccountServiceManager.class.getSimpleName();
	}
	

	@Override
	public void reset() {
		super.reset();
		instance = null;
	}
	
	/**
	 *  获取商品价格
	 *  http://poseidon.d3dstore.com/api/v1/pay/goods/get?productId=89&productType=1&lg=zh&ct=cn
	 * @param productId:商品id
	 * @param productType:产品类别
	 * @param callBack
	 * @return 
	 */
	public void getProductPrice(int productId, boolean needCache,int productType,Handler callBack) {
		String url = CMS_ACCOUNT_BASEURL+"goods/get?productId="+productId+"&productType="+productType+"&"+getClientInfo(true);
		log.debug("getProductPrice url = "+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_PRICE, callBack);
		if(needCache){
			CacheParam cparam = new CacheParam(url, VOPrice.class, DiskCacheOption.NULL, CachePost.Continue);
			rhandler.setCacheParam(cparam);
		}
		sendGetRequst(url, rhandler);
	}
	
	/**
	 * 获取paypal支付结果
	 * @param paypalToken
	 * @param callback
	 */
	public void paypalCheckout(String paypalToken,Handler callback){
		String url = CMS_ACCOUNT_BASEURL+"paypal/expressCheckout?paypalToken="+paypalToken+"&"+getClientInfo(true);
		log.debug("paypalCheckout url = "+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_PAYPAL_CHECKOUT, callback);
		sendGetRequst(url, rhandler);
	}

	/**
	 * * 获取充值记录
	 * 定义:GET:http://pay.runmit-dev.com/pay/userCharge/get?uid=12&ct=cn&page=1&count=3
	 * uid:用户id
	 * ct:国家代码
	 * status:（可选参数）状态，默认2。 0，全部记录；1，充值失败记录；2，充值成功记录。
	 * page:（可选参数）页码，默认1
	 * count:（可选参数）条数，默认10
	 */
	public void getRechargeRecord(int status,int page,int count,Handler callBack){
		String url = CMS_ACCOUNT_BASEURL+"userCharge/get?page="+page+"&count="+count+"&"+getClientInfo(true);
		log.debug("getUserAccountMessage url = "+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_RECHARGE_RECORD, callBack);
		sendGetRequst(url, rhandler);
	}
	/**
	 * http://192.168.1.245/pay/present/get?uid=16&countryCode=cn&page=1&count=3
	 * @param status
	 * @param page
	 * @param count
	 * @param callBack
	 */
	public void getPresentRecord(int status,int page,int count,Handler callBack){
		String url = CMS_ACCOUNT_BASEURL+"present/get?page="+page+"&count="+count+"&countryCode="+Locale.getDefault().getCountry()+"&"+getClientInfo(true);
		log.debug("getPresentRecord url = "+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_RECHARGE_RECORD, callBack);
		sendGetRequst(url, rhandler);
	}
	
	/**
	 * 获取用户购买记录 
	 * http://poseidon.d3dstore.com/api/v1/pay/goodsSpend/get?page=1&count=3&lg=zh&ct=cn&uid=486
	 * @param currencyId 
	 * @param reqPage
	 * @param reqCount
	 * @param callBack
	 */
	public void getUserPurchases(int reqPage,int reqCount,Handler callBack) {
		String url = CMS_ACCOUNT_BASEURL+"goodsSpend/get?page="+reqPage+"&count="+reqCount+"&"+getClientInfo(true);
		log.debug("getUserPurchases url = "+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_USER_PURCHASES, callBack);
		sendGetRequst(url, rhandler);
	}

	/**
	 * 获取影片支付方式 回调Handler:AccountEventCode.EVENT_GET_USER_PURCHASES
	 * @param amount
	 * @param currencyId
	 * @param callBack
	 * @return
	 */
	public RequestHandle reqMoviePayStyle(String amount, int currencyId, Handler callBack) {
		String url = CMS_ACCOUNT_BASEURL + "typePayment/get?currencyId=" + currencyId+"&amount="+amount+"&"+getClientInfo(true);
		log.debug("getMoviePayStyle url = "+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_MOVIE_PALY_STYLE, callBack);
		RequestHandle requstHanle = sendCancableGetRequest(url, rhandler);
		CacheParam cparam = new CacheParam(url, PayStyleList.class, DiskCacheOption.NULL, CachePost.Continue);
		rhandler.setCacheParam(cparam);
		return requstHanle;
	}
	
	public void getPermission(int productId,Handler callBack,Serializable userData) {
		int productType = 3;//3是专辑的购买权限
		String url = CMS_ACCOUNT_BASEURL+"checkPermission?productId="+productId+"&productType="+productType+"&"+getClientInfo(true);
		log.debug("getUserPurchases url = "+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_PERMISSION, callBack);
		rhandler.setTag(userData);
		sendGetRequst(url, rhandler);
	}
	
	/*获取可用充值额度
	 * 定义：根据币种获取充值额度
	 * GET:http://pay.runmit-dev.com/pay/typeLimit/get?currencyId=1
	 * currencyId：币种id
	*/
	public void getRechargeQuota(int currencyId, Handler callBack){
		String url = CMS_ACCOUNT_BASEURL+"/typeLimit/get?currencyId="+currencyId+"&"+getClientInfo(true);
		log.debug("getRechargeQuota url = "+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_RECHARGE_QUOTA, callBack);
		CacheParam cparam = new CacheParam(url, QuotaList.class, DiskCacheOption.NULL, CachePost.Continue);
		rhandler.setCacheParam(cparam);
		sendGetRequst(url, rhandler);
	}
	
	/**
	 * 请求生成订单
	 * 返回消息AccountEventCode.EVENT_GET_MOVIE_PALY_ORDER
	 * 请求格式
	 * {
	       "uid": 12,
	       "productId": 1234,
	       "productType": 1,
	       "paymentId": 1,
	       "amount": 500,
	       "currencyId": 1,
	       "channelId":123
		}		
	 * @param userid
	 * @param productId
	 * @param productType
	 * @param paymentId
	 * @param amount
	 * @param currencyId
	 * @param channelId
	 * @param callBack
	 * @return
	 */
	public void getMoviePayOrder(Context context,String productName,long userid,long productId,int productType,int paymentId,String amount,int currencyId,int channelId,Handler callBack){
		JsonObject mJsonObject=new JsonObject();
		mJsonObject.addProperty("uid", userid);
		mJsonObject.addProperty("productId", productId);
		mJsonObject.addProperty("productName", productName);
		mJsonObject.addProperty("productType", productType);
		mJsonObject.addProperty("paymentId", paymentId);
		mJsonObject.addProperty("amount", amount);
		mJsonObject.addProperty("currencyId", currencyId);
		mJsonObject.addProperty("channelId", channelId);
		mJsonObject.addProperty("appName", Config.getAppName(context));
		mJsonObject.addProperty("appVersion", Config.getVerCode(context));
		mJsonObject.addProperty("appId", SkyPayAppId);
		mJsonObject.addProperty("productUrl", PayPalActivity.PRODUCTURL_CALLBACL);
		mJsonObject.addProperty("backUrl", PayPalActivity.BACKURL_USER_GIVEUP);
		
		String url=PayOrderHostUrl+"?"+getClientInfo(true);
		log.debug("getMoviePayOrder="+mJsonObject.toString()+",url="+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_MOVIE_PALY_ORDER, callBack);
		sendPostRequest(url, mJsonObject.toString(), rhandler);
	}
	
	/**
	 * 充值,根据金额获取服务器可用支付方式
	 * 返回值和getMoviePayOrder生成订单接口一样，返回订单id，所以回调类型为EVENT_GET_MOVIE_PALY_ORDER
	 * @param context
	 * @param paymentId
	 * @param accountType
	 * @param amount
	 * @param currencyId
	 * @param callBack
	 */
	public void rechargeOrder(Context context,int paymentId,int accountType,int amount,int currencyId,Handler callBack){
		JsonObject mJsonObject=new JsonObject();
		mJsonObject.addProperty("uid", UserManager.getInstance().getUserId());
		mJsonObject.addProperty("paymentId", paymentId);
		mJsonObject.addProperty("accountType", accountType);
		mJsonObject.addProperty("amount", amount);
		mJsonObject.addProperty("currencyId", currencyId);
		mJsonObject.addProperty("appId", SkyPayAppId);
		mJsonObject.addProperty("appName", Config.getAppName(context));
		mJsonObject.addProperty("appVersion", Config.getVerCode(context));
		mJsonObject.addProperty("productUrl", PayPalActivity.PRODUCTURL_CALLBACL);
		mJsonObject.addProperty("backUrl", PayPalActivity.BACKURL_USER_GIVEUP);
		
		String url=ChargeHostUrl+"?"+"countryCode="+Locale.getDefault().getCountry()+"&"+getClientInfo(true);
		log.debug("rechargeOrder="+mJsonObject.toString()+",url="+url);
		
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_MOVIE_PALY_ORDER, callBack);
		sendPostRequest(url, mJsonObject.toString(), rhandler);
	}
	
	/**
	 * 支付成功后上报服务器，下次再次支付时候提示
	 * @param context
	 * @param orderId
	 * @param callBack
	 */
	public void reportPaySuccess(Context context,int orderId,Handler callBack){
		String url=PaySuccessUrl+orderId+"&"+getClientInfo(true);
		log.debug("rechargeOrder url="+url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_REPORT_PAY_SUCCESS, callBack);
		sendGetRequst(url, rhandler);
	}
	
	
	@Override
	protected void dispatchSuccessResponse(ResponseHandler responseHandler,String responseString) {
		int eventCode = responseHandler.getEventCode();
		log.debug("responseString="+responseString);
		try {
			JSONObject jobj = new JSONObject(responseString);
			int rtn = jobj.optInt(JSON_RTN_FIELD,-1);
			if(rtn != 0){
				responseHandler.sendCallBackMsg(eventCode,rtn,null);
				return;
			}
			switch (eventCode) {
			case AccountEventCode.EVENT_GET_USER_PURCHASES:// 用户购买记录
				VOUserPurchase voPuchase = gson.fromJson(jobj.getJSONObject(JSON_DATA_FIELD).toString(), VOUserPurchase.class);
				//暂时只提取成功的记录				
				if(voPuchase.data != null){
					ArrayList<PurchaseRecord> succeedList = new ArrayList<PurchaseRecord>();
					for(PurchaseRecord record:voPuchase.data){
						if(record.status == VOUserPurchase.STATUS_SUCCEED){
							succeedList.add(record);
						}
					}
					voPuchase.data = succeedList;
				}				
				responseHandler.sendCallBackMsg(eventCode, rtn, voPuchase);
				break;
			case AccountEventCode.EVENT_GET_PRICE:// 商品定价
				VOPrice price = gson.fromJson(jobj.getJSONObject(JSON_DATA_FIELD).toString(), VOPrice.class);
				responseHandler.sendCallBackMsg(eventCode, rtn, price);
				break;
			case AccountEventCode.EVENT_GET_MOVIE_PALY_STYLE:// 支付方式
				PayStyleList mPayStyleList = gson.fromJson(jobj.toString(), PayStyleList.class);
				responseHandler.sendCallBackMsg(eventCode,rtn, mPayStyleList);
				break;
			case AccountEventCode.EVENT_GET_MOVIE_PALY_ORDER:
				PayOrder mPayOrder=gson.fromJson(jobj.getJSONObject(JSON_DATA_FIELD).toString(), PayOrder.class);
				responseHandler.sendCallBackMsg(eventCode,rtn, mPayOrder);
				WalletManager.getInstance().reqGetAccount();//请求生成订单后刷新钱包
				if(mPayOrder.paymentId==PayOrder.PAYMENT_WALLET || mPayOrder.paymentId==PayOrder.PAYMENT_VIRTUAL_WALLET){
					//如果是钱包支付则代表已经支付成功了
				}
				break;				
			case AccountEventCode.EVENT_GET_PERMISSION:
				VOPermission permission = gson.fromJson(jobj.getJSONObject(JSON_DATA_FIELD).toString(), VOPermission.class);
				responseHandler.sendCallBackMsg(eventCode,rtn, permission);
				break;
			case AccountEventCode.EVENT_GET_RECHARGE_RECORD:
				RechargeHistory mRechargeHistory = gson.fromJson(jobj.getJSONObject(JSON_DATA_FIELD).toString(), RechargeHistory.class);
				responseHandler.sendCallBackMsg(eventCode,rtn, mRechargeHistory);
				break;
			case AccountEventCode.EVENT_GET_RECHARGE_QUOTA:
				QuotaList mQuotaList = gson.fromJson(jobj.toString(), QuotaList.class);
				responseHandler.sendCallBackMsg(eventCode,rtn, mQuotaList);
				break;
			case AccountEventCode.EVENT_PAYPAL_CHECKOUT://没有实质内容
				responseHandler.sendCallBackMsg(eventCode,rtn, null);
				break;
			default:
				break;
			}
		}catch (Exception e){
			responseHandler.handleException(e);
		} 
	}
}
