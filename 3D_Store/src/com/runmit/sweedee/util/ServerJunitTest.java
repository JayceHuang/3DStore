//package com.runmit.sweedee.util;
//
//import android.os.Handler;
//import android.os.HandlerThread;
//import android.test.AndroidTestCase;
//
//import com.runmit.sweedee.StoreApplication;
//import com.runmit.sweedee.manager.RegistManager;
//import com.runmit.sweedee.sdk.user.member.task.MemberInfo;
//import com.runmit.sweedee.sdk.user.member.task.OnUserListener;
//import com.runmit.sweedee.sdk.user.member.task.UserLoginTask;
//import com.runmit.sweedee.sdk.user.member.task.UserPingTask;
//import com.runmit.sweedee.sdk.user.member.task.UserPingTask.OnUserPingResultListener;
//import com.runmit.sweedee.sdk.user.member.task.UserUtil;
//
//public class ServerJunitTest extends AndroidTestCase {
//
//    XL_Log log = new XL_Log(ServerJunitTest.class);
//
////	public void testEmailRegist(){
////		for(int i=0;i<1000;i++){
////			RegistManager.getInstance().startRegister(1,System.currentTimeMillis()+"zz834730873@gmail.com", "zz2806", "",new Handler());
////		}
////	}
//
//    public void testLogin() {
//        String account = "834730873@qq.com";
//        String password = "zz2806";
//
//        UserUtil.getInstance().Init(getContext());
//        OnUserListener mXLOnUserListener = new OnUserListener() {
//            @Override
//            public boolean onUserLogin(int errorCode, MemberInfo userInfo, Object userdata) {
//                log.debug("onUserLogin=" + errorCode + ",userInfo=" + userInfo);
//                if (errorCode == 0) {
//                    for (int i = 0; i < 1000; i++) {
//                        UserPingTask pingTask = new UserPingTask();
//                        pingTask.execute(userInfo.userid, userInfo.token, Long.toString(System.currentTimeMillis()), new OnUserPingResultListener() {
//
//                            @Override
//                            public void onResult(int rtn_code, MemberInfo mPingMemberInfo) {
//                                // TODO Auto-generated method stub
//                                log.debug("OnUserPingResultListener rtn_code=" + rtn_code + ",mPingMemberInfo=" + mPingMemberInfo);
//                            }
//                        });
//                    }
//                }
//                return true;
//            }
//
//        };
//        UserUtil.getInstance().userLogin(account, password, "", mXLOnUserListener, null);
//
//    }
//}
