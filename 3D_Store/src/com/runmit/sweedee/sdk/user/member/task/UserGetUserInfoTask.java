//package com.runmit.sweedee.sdk.user.member.task;
//
//import java.util.ArrayList;
//import java.util.HashMap;
//import java.util.List;
//import java.util.Map;
//
//import org.apache.http.Header;
//import org.apache.http.entity.ByteArrayEntity;
//import org.json.JSONArray;
//import org.json.JSONException;
//import org.json.JSONObject;
//
//import android.os.Bundle;
//import android.util.Log;
//
//import com.runmit.sweedee.sdk.user.member.task.MemberInfo.USERINFOKEY;
//import com.runmit.sweedee.sdkex.httpclient.LoginAsyncHttpProxy;
//import com.runmit.sweedee.sdkex.httpclient.LoginAsyncHttpProxyListener;
//
//public class UserGetUserInfoTask extends UserTask {
//
//	private List<USERINFOKEY> catchedInfoList = new ArrayList<USERINFOKEY>();
//	private List<USERINFOKEY> reqInfoKeyList = null;
//	private Map<USERINFOKEY, String> mapUserInfoName = new HashMap<USERINFOKEY, String>();
//
//	public UserGetUserInfoTask(UserUtil util) {
//		super(util);
//	}
//
//	private void initMapUserInfoName() {
//		mapUserInfoName.put(USERINFOKEY.NickName, "nickName");
//		mapUserInfoName.put(USERINFOKEY.Account, "account");
//		mapUserInfoName.put(USERINFOKEY.Rank, "rank");
//		mapUserInfoName.put(USERINFOKEY.Order, "order");
//		mapUserInfoName.put(USERINFOKEY.IsSubAccount, "isSubAccount");
//		mapUserInfoName.put(USERINFOKEY.Country, "country");
//		mapUserInfoName.put(USERINFOKEY.City, "city");
//		mapUserInfoName.put(USERINFOKEY.Province, "province");
//		mapUserInfoName.put(USERINFOKEY.IsSpecialNum, "isSpecialNum");
//		mapUserInfoName.put(USERINFOKEY.Role, "role");
//		mapUserInfoName.put(USERINFOKEY.TodayScore, "todayScore");
//		mapUserInfoName.put(USERINFOKEY.AllowScore, "allowScore");
//		mapUserInfoName.put(USERINFOKEY.PersonalSign, "personalSign");
//		mapUserInfoName.put(USERINFOKEY.IsVip, "isVip");
//		mapUserInfoName.put(USERINFOKEY.ExpireDate, "expireDate");
//		mapUserInfoName.put(USERINFOKEY.VasType, "vasType");
//		mapUserInfoName.put(USERINFOKEY.PayId, "payId");
//		mapUserInfoName.put(USERINFOKEY.PayName, "payName");
//		mapUserInfoName.put(USERINFOKEY.VipGrow, "vipGrow");
//		mapUserInfoName.put(USERINFOKEY.VipDayGrow, "vipDayGrow");
//		mapUserInfoName.put(USERINFOKEY.IsAutoDeduct, "isAutoDeduct");
//		mapUserInfoName.put(USERINFOKEY.IsRemind, "isRemind");
//		mapUserInfoName.put(USERINFOKEY.ImgURL, "imgURL");
//		mapUserInfoName.put(USERINFOKEY.vip_level, "vipLevel");
//		mapUserInfoName.put(USERINFOKEY.JumpKey, "jumpKey");
//
//	}
//
//	public void initTask() {
//		super.initTask();
//		reqInfoKeyList = null;
//		initMapUserInfoName();
//	}
//
//	public boolean fireEvent(OnUserListener listen, Bundle bundle) {
//		if (bundle == null || bundle.getString("action") != "userGetDetailTask")
//			return false;
//		return listen.onUserInfoCatched(bundle.getInt("errorCode"), catchedInfoList, getUser(), getUserData());
//	}
//
//	public void putReqInfoKeyList(List<MemberInfo.USERINFOKEY> infoList) {
//		reqInfoKeyList = infoList;
//	}
//
//	public boolean execute() {
//		if (!UserManager.getInstance().isLogined()) {
//			Bundle bundle = new Bundle();
//			bundle.putString("action", "userGetDetailTask");
//			bundle.putInt("errorCode", UserErrorCode.OPERATION_INVALID);
//			getUserUtil().notifyListener(UserGetUserInfoTask.this, bundle);
//			return false;
//		}
//
//		putTaskState(TASKSTATE.TS_DOING);
//		JSONArray infoNameArray = new JSONArray();
//		for (USERINFOKEY infoKey : reqInfoKeyList) {
//			Object obj = mapUserInfoName.get(infoKey);
//			if (obj != null) {
//				infoNameArray.put(obj);
//			}
//		}
//		JSONObject jsonRequObj = new JSONObject();
//		try {
//			jsonRequObj.put("protocolVersion", getProtocolVersion());
//			jsonRequObj.put("sequenceNo", getSequenceNo());
//			jsonRequObj.put("platformVersion", getPlatformVersion());
//			jsonRequObj.put("peerID", getPeerId());
//			jsonRequObj.put("businessType", getUserUtil().getBusinessType());
//			jsonRequObj.put("clientVersion", getUserUtil().getClientVersion());
//			jsonRequObj.put("isCompressed", 0);
//			jsonRequObj.put("cmdID", 0x0003);
//			jsonRequObj.put("userID", getUser().userid);
//			jsonRequObj.put("sessionID", getUser().token);
//			jsonRequObj.put("extensionList", infoNameArray);
//
//		} catch (JSONException e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//			catchedInfoList.clear();
//			Bundle bundle = new Bundle();
//			bundle.putString("action", "userGetDetailTask");
//			bundle.putInt("errorCode", UserErrorCode.PACKAGE_ERROR);
//			getUserUtil().notifyListener(UserGetUserInfoTask.this, bundle);
//			return false;
//		}
//
//		catchedInfoList.clear();
//		String jsonContent = jsonRequObj.toString();
//		Log.d("json", "jsonContent=" + jsonContent);
//		ByteArrayEntity byteEntity = new ByteArrayEntity(jsonContent.getBytes());
//		getUserUtil().getHttpProxy().post(byteEntity, new LoginAsyncHttpProxyListener() {
//
//			public void onSuccess(int responseCode, Header[] headers, String result) {
//				try {
//					JSONObject jsonRespObj = new JSONObject(result);
//					int errorCode = jsonRespObj.getInt("errorCode");
//					Log.d("onSuccess", "result=" + result);
//					// boolean isHasUserLevel = false;
//					if (errorCode == UserErrorCode.SUCCESS) {
//						for (USERINFOKEY infoKey : reqInfoKeyList) {
//							Object obj = mapUserInfoName.get(infoKey);
//							if (obj != null && jsonRespObj.has((String) obj) == true) {
//								catchedInfoList.add(infoKey);
//								// getUser().putUserData(infoKey,jsonRespObj.get((String)obj));
//							}
//						}
//						Bundle bundle = new Bundle();
//						bundle.putInt("errorCode", UserErrorCode.SUCCESS);
//						bundle.putString("action", "userGetDetailTask");
//						getUserUtil().notifyListener(UserGetUserInfoTask.this, bundle);
//					} else {
//						catchedInfoList.clear();
//						Bundle bundle = new Bundle();
//						bundle.putInt("errorCode", UserErrorCode.GETUSERINFO_ERROR);
//						bundle.putString("action", "userGetDetailTask");
//						getUserUtil().notifyListener(UserGetUserInfoTask.this, bundle);
//					}
//				} catch (JSONException e) {
//					// TODO Auto-generated catch block
//					e.printStackTrace();
//					catchedInfoList.clear();
//					Bundle bundle = new Bundle();
//					bundle.putInt("errorCode", UserErrorCode.UNPACKAGE_ERROR);
//					bundle.putString("action", "userGetDetailTask");
//					getUserUtil().notifyListener(UserGetUserInfoTask.this, bundle);
//				}
//			}
//
//			public void onFailure(Throwable error) {
//				Log.d("onFailure", "result=" + error.getLocalizedMessage());
//				catchedInfoList.clear();
//				Bundle bundle = new Bundle();
//				bundle.putString("action", "userGetDetailTask");
//				bundle.putInt("errorCode", UserErrorCode.UNKNOWN_ERROR);
//				getUserUtil().notifyListener(UserGetUserInfoTask.this, bundle);
//			}
//		});
//		return true;
//	}
//}
