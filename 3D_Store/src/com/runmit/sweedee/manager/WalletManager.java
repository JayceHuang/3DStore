package com.runmit.sweedee.manager;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import org.json.JSONObject;

import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;

import com.google.gson.JsonArray;
import com.google.gson.JsonParser;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.StoreApplication.OnNetWorkChangeListener;
import com.runmit.sweedee.manager.data.EventCode.AccountEventCode;
import com.runmit.sweedee.model.UserAccount;
import com.runmit.sweedee.model.VOPrice;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.sdk.user.member.task.UserManager.LoginState;
import com.runmit.sweedee.sdk.user.member.task.UserManager.LoginStateChange;
import com.runmit.sweedee.util.XL_Log;

/**
 * 对钱包进行管理的类，因为目前用户有虚拟货币和真实货币，而且对用户来讲是全局的
 * 
 * @author Zhi.Zhang
 *
 */
public class WalletManager extends HttpServerManager implements OnNetWorkChangeListener{

	private XL_Log log = new XL_Log(WalletManager.class);

	public static final int ACCOUNT_REAL_MONEY = 1;// 真实货币

	public static final int ACCOUNT_VIRTUAL_MONEY = 2;// 虚拟货币

	private List<OnAccountChangeListener> mObseverList = new ArrayList<WalletManager.OnAccountChangeListener>();

	private List<UserAccount> mUserAccounts;// 用户钱包

	/** CMS 代理计费系统的BaseUrl */
	protected static final String CMS_ACCOUNT_BASEURL = CMS_V1_BASEURL + "pay/";

	private static WalletManager instance;

	private Handler mHandler = new Handler(StoreApplication.INSTANCE.getMainLooper()){
		
		private int countId = 0;
		
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case AccountEventCode.EVENT_GET_USER_ACCOUNT_MESSAGE:
				log.debug("msg.arg1="+msg.arg1);
				if(msg.arg1 != 0){
					if(countId++ < 5){//计数器，轮询5次
						reqGetAccount();
					}else{
						countId = 0;//计步完成后重置
					}
				}else{
					countId = 0;
				}
				break;
			default:
				break;
			}
		};
	};
	
	public static final String INTENT_ACTION_USER_WALLET_CHANGE = "com.runmit.sweedee.USER_WALLET_CHANGE_ACTION";
	
	/**
	 * 发送用户钱包改变通知
	 * @param context
	 */
	public static void sendWalletChangeBroadcast(Context context){
		Intent i = new Intent(INTENT_ACTION_USER_WALLET_CHANGE);
		context.sendBroadcast(i);
	}

	private LoginStateChange mLoginStateChange = new LoginStateChange() {

		@Override
		public void onChange(LoginState lastState, LoginState currentState, Object userdata) {
			if (currentState == LoginState.LOGINED) {
				reqGetAccount();
			} else if (currentState == LoginState.COMMON && lastState == LoginState.LOGINED) {// 此种情况判断是注销登录
				mUserAccounts.clear();
			}
		}
	};

	public synchronized static WalletManager getInstance() {
		if (instance == null) {
			instance = new WalletManager();
		}
		return instance;
	}

	private WalletManager() {
		TAG = WalletManager.class.getSimpleName();
		
		mUserAccounts = new ArrayList<UserAccount>();
		UserManager.getInstance().addLoginStateChangeListener(mLoginStateChange);
		
		StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
	}

	public void release() {
		UserManager.getInstance().removeLoginStateChangeListener(mLoginStateChange);
	}

	public void addAccountChangeListener(final OnAccountChangeListener observer) {
		mHandler.post(new Runnable() {

			@Override
			public void run() {
				if (!mObseverList.contains(observer)) {
					mObseverList.add(observer);
				}
			}
		});
	}

	public void removeAccountChangeListener(final OnAccountChangeListener observer) {
		mHandler.post(new Runnable() {

			@Override
			public void run() {
				mObseverList.remove(observer);
			}
		});
	}

	private String getCountryCode() {
		String country = Locale.getDefault().getCountry();
		return country.toLowerCase();
	}

	/**
	 * 获取真实钱包金额
	 * 
	 * @return
	 */
	public UserAccount getReadAccount() {
		for (UserAccount mAccount : mUserAccounts) {
			if (mAccount.accountType == ACCOUNT_REAL_MONEY) {
				return mAccount;
			}
		}
		return null;
	}

	/**
	 * 获取虚拟钱包金额
	 * 
	 * @return
	 */
	public UserAccount getVirtualAccount() {
		for (UserAccount mAccount : mUserAccounts) {
			if (mAccount.accountType == ACCOUNT_VIRTUAL_MONEY && mAccount.currencyId == VOPrice.VIRTUAL_CURRENCYID) {
				return mAccount;
			}
		}
		return null;
	}

	/**
	 * * 获取用户账户信息 定义:查询用户账户金额 在用户登录之后获取，充值之后，支付之后一定要重新获取账户余额
	 * GET:http://pay.runmit-dev.com/pay/userAccount/get?uid
	 * =16&ct=cn&accountType=1
	 * accountType:账户类型,ACCOUNT_REAL_MONEY:货币账户，ACCOUNT_VIRTUAL_MONEY:虚拟账户,
	 */
	public void reqGetAccount() {
		String url = CMS_ACCOUNT_BASEURL + "userAccount/get?countryCode=" + getCountryCode() + "&" + getClientInfo(true);
		log.debug("reqGetAccount url = " + url);
		ResponseHandler rhandler = new ResponseHandler(AccountEventCode.EVENT_GET_USER_ACCOUNT_MESSAGE, mHandler);
		sendGetRequst(url, rhandler);
	}

	@Override
	protected void dispatchSuccessResponse(ResponseHandler responseHandler, String responseString) {
		int eventCode = responseHandler.getEventCode();
		log.debug("responseString=" + responseString);
		try {

			switch (eventCode) {
			case AccountEventCode.EVENT_GET_USER_ACCOUNT_MESSAGE: {
				JSONObject jobj = new JSONObject(responseString);
				int rtn = jobj.getInt(JSON_RTN_FIELD);
				if (rtn == 0) {
					JsonParser parser;
					JsonArray jsonArray;
					parser = new JsonParser();
					mUserAccounts.clear();
					jsonArray = parser.parse(jobj.getJSONArray(JSON_DATA_FIELD).toString()).getAsJsonArray();
					for (int i = 0; i < jsonArray.size(); i++) {
						UserAccount mItem = gson.fromJson(jsonArray.get(i), UserAccount.class);
						log.debug("mItem="+mItem);
						mUserAccounts.add(mItem);
					}
					notifyAccountListener(rtn);
				}
			}
				break;
			default:
				break;
			}
		} catch (Exception e) {
			responseHandler.handleException(e);
		}
	}

	private void notifyAccountListener(final int rtnCode) {
		mHandler.post(new Runnable() {

			@Override
			public void run() {
				log.debug("notifyAccountListener mObseverList="+mObseverList.size());
				for (OnAccountChangeListener mOnAccountChangeListener : mObseverList) {
					log.debug("notifyAccountListener mOnAccountChangeListener="+mOnAccountChangeListener);
					mOnAccountChangeListener.onAccountChange(rtnCode, mUserAccounts);
				}
			}
		});
	}

	public interface OnAccountChangeListener {
		/**
		 * 钱包金额改变的通知
		 * 
		 * @param accountType
		 *            :ACCOUNT_REAL_MONEY,ACCOUNT_VIRTUAL_MONEY
		 * @param mAccounts
		 */
		void onAccountChange(int rtnCode, List<UserAccount> mAccounts);
	}

	@Override
	public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
		log.debug("isNetWorkAviliable="+isNetWorkAviliable+",isLogined="+UserManager.getInstance().isLogined());
		if(isNetWorkAviliable && UserManager.getInstance().isLogined()){//如果网络是好的并且已登录则重新拉取一次数据
			reqGetAccount();
		}
	}
}
