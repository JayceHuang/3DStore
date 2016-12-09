//package com.runmit.sweedee.sdk.user.member.task;
//import org.apache.http.Header;
//import org.apache.http.HeaderElement;
//import android.os.Bundle;
//import com.loopj.android.http.BinaryHttpResponseHandler;
//
//public class UserVerifyCodeTask extends UserTask{
//	private final int VERIFY_JPEG_IMAGE = 1;
//	private final int VERIFY_PNG_IMAGE = 2;
//	private final int VERIFY_BMP_IMAGE = 3;
//	private final int VERIFY_UNKNOWN_IMAGE = 4;
//	public UserVerifyCodeTask(UserUtil util) {
//		super(util);
//		
//	}
//	@Override
//	public boolean execute() {
//		if(TASKSTATE.TS_CANCELED == getTaskState()){
//			return false;
//		}
//		putTaskState(TASKSTATE.TS_DOING);
//		getUserUtil().getHttpProxy().get(getUserUtil().getVerifyCodeUrl(),new BinaryHttpResponseHandler() {			
//			@Override
//			public void onSuccess(int responseCode, Header[] headers, byte[] result) {
//				String verifyKey = "";
//				int imageType = 1;
//				Bundle bundle = new Bundle();
//				int i = 0;
//				for(i = 0;i < headers.length;i++){
//					Header item = headers[i];
//					if(item.getName().compareToIgnoreCase("Set-Cookie")==0){
//						HeaderElement []elements = item.getElements();
//						int j = 0;
//						for(j = 0;j < elements.length;j++){
//							HeaderElement itemElement = elements[j];
//							if(itemElement.getName().compareToIgnoreCase("VERIFY_KEY")==0){
//								verifyKey = itemElement.getValue();
//							}
//						}
//						
//					}
//					if(item.getName().compareToIgnoreCase("Content-Type")==0){
//						if(item.getValue().compareToIgnoreCase("image/jpeg")==0){
//							imageType = VERIFY_JPEG_IMAGE;
//						}
//						else if(item.getValue().compareToIgnoreCase("image/png")==0){
//							imageType = VERIFY_PNG_IMAGE;
//						}
//						else if(item.getValue().compareToIgnoreCase("image/bmp")==0){
//							imageType = VERIFY_BMP_IMAGE;
//						}
//						else{
//							imageType = VERIFY_UNKNOWN_IMAGE;
//						}
//					}
//				}
//				bundle.putString("verifyKey",verifyKey);
//				bundle.putInt("imageType",imageType);
//				bundle.putByteArray("imageContent", result);
//				bundle.putInt("errorCode",UserErrorCode.SUCCESS );
//    			bundle.putString("action","UserVerifyCodeTask");
//    			getUserUtil().notifyListener(UserVerifyCodeTask.this, bundle); 
//			}
//			
//			@Override
//			public void onFailure(int arg0, Header[] arg1, byte[] arg2, Throwable arg3) {
//				Bundle bundle = new Bundle();
//				bundle.putString("verifyKey","");
//				bundle.putInt("imageType",-1);
//				bundle.putString("imageContent","");
//				bundle.putInt("errorCode",UserErrorCode.UNKNOWN_ERROR );
//    			bundle.putString("action","UserVerifyCodeTask");
//    			getUserUtil().notifyListener(UserVerifyCodeTask.this, bundle);  
//			}
//		});
//		putTaskState(TASKSTATE.TS_DONE);
//		return true;
//	}
//
//	@Override
//	public boolean fireEvent(OnUserListener listen, Bundle bundle) {
//		if(bundle == null || bundle.getString("action") != "UserVerifyCodeTask")
//			return false;
//		
//		
//		return listen.onUserVerifyCodeUpdated(bundle.getInt("errorCode"),bundle.getString("verifyKey"),
//				             bundle.getInt("imageType"),bundle.getByteArray("imageContent"),getUserData());
//	}
//
//	public void initTask(){
//		super.initTask();
//	}
//}
