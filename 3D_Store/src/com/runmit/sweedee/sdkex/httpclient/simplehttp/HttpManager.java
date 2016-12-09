package com.runmit.sweedee.sdkex.httpclient.simplehttp;

import android.app.ProgressDialog;
import android.content.Context;

import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;
import com.google.gson.reflect.TypeToken;
import com.runmit.sweedee.R;
import com.runmit.sweedee.model.OpenCountryCode;
import com.runmit.sweedee.sdkex.httpclient.simplehttp.LoadProxy.OnloadCallBackListener;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

public class HttpManager {
	
	public final static String CountryCodeUrl = "http://clotho.d3dstore.com/countryCode/getlist";
	
	private Gson gson = new Gson();

	private static OpenCountryCode mOpenCountryCode;
	
	private Context mContext;
	
	public HttpManager(Context context){
		mContext = context;
	}
	
	public void loadCountryCode(final OnLoadFinishListener<OpenCountryCode> mOnLoadFinishListener) {
		if(mOpenCountryCode != null){
			mOnLoadFinishListener.onLoad(mOpenCountryCode);
		}else{
			final ProgressDialog mProgressDialog = new ProgressDialog(mContext,R.style.ProgressDialogDark);
			Util.showDialog(mProgressDialog, mContext.getString(R.string.loading));
			
			LoadProxy.getInstance().loadData(CountryCodeUrl,R.raw.countrycode, new OnloadCallBackListener() {
				
				@Override
				public void onLoadSuccess(String data) throws JsonSyntaxException{
					Util.dismissDialog(mProgressDialog);
					mOpenCountryCode = gson.fromJson(data, new TypeToken<OpenCountryCode>() {}.getType());
					mOnLoadFinishListener.onLoad(mOpenCountryCode);
				}
			});
		}
	}
	
	public interface OnLoadFinishListener<T> {
		void onLoad(T t);
	}
}
