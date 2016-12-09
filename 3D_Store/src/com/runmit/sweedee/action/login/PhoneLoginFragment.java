package com.runmit.sweedee.action.login;

import java.util.Locale;
import java.util.regex.Pattern;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.model.OpenCountryCode;
import com.runmit.sweedee.model.OpenCountryCode.CountryInfo;
import com.runmit.sweedee.sdkex.httpclient.simplehttp.HttpManager;
import com.runmit.sweedee.sdkex.httpclient.simplehttp.HttpManager.OnLoadFinishListener;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;

public class PhoneLoginFragment extends LoginBaseFragment{
	
	private RelativeLayout select_location_layout;
	
	private TextView textviewLocation;
	
	private TextView phoneCountryCode;
	
    private AlertDialog mAlertDialog = null;
    
    private int selectPosition;
    
//    private OpenCountryCode mOpenCountryCode;
    
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		return inflater.inflate(R.layout.phonelogin, container, false);
	}
	
	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {
		super.onViewCreated(view, savedInstanceState);
//		textviewLocation = (TextView) view.findViewById(R.id.textviewLocation);
//		phoneCountryCode = (TextView) view.findViewById(R.id.phone_countrycode);
//		select_location_layout = (RelativeLayout) view.findViewById(R.id.select_location_layout);
//		select_location_layout.setOnClickListener(new View.OnClickListener() {
//			
//			@Override
//			public void onClick(View v) {
//				selectLocation();
//			}
//		});
		
//		new HttpManager(mFragmentActivity).loadCountryCode(new OnLoadFinishListener<OpenCountryCode>() {
//			
//			@Override
//			public void onLoad(final OpenCountryCode openCountryCode) {
//				mOpenCountryCode = openCountryCode;
//				setLocation();
//			}
//		});
	}
	@Override
	protected void startRegetPage() {
		Intent intent = new Intent(getActivity(), RegetPassword.class);
//		intent.putExtra("openCountyCode", mOpenCountryCode);
		getActivity().startActivity(intent);
	}

    @Override
    public void onResume(){
//    	if(mOpenCountryCode != null){
//    		selectPosition = SharePrefence.getInstance(getActivity()).getInt(SharePrefence.REGIST_LOCATION, selectPosition);
//    		textviewLocation.setText(getCurrentLocaleInfo().name);
//            if(phoneCountryCode != null){
//            	phoneCountryCode.setText(getCurrentLocaleInfo().countrycode);
//            }
//    	}
    	
    	super.onResume();
    }
	
//    protected void selectLocation() {
//        final MySimpleArrayAdapter mySimpleArrayAdapter = new MySimpleArrayAdapter(getActivity(), mOpenCountryCode.data);
//        int position = SharePrefence.getInstance(getActivity()).getInt(SharePrefence.REGIST_LOCATION, selectPosition);
//        mAlertDialog = new AlertDialog.Builder(getActivity()).setTitle(Util.setColourText(getActivity(), R.string.select_location, Util.DialogTextColor.BLUE))
//                .setSingleChoiceItems(mySimpleArrayAdapter, position, new DialogInterface.OnClickListener() {
//
//                    @Override
//                    public void onClick(DialogInterface dialog, int which) {
//                        selectPosition = which;
//                        SharePrefence.getInstance(getActivity()).putInt(SharePrefence.REGIST_LOCATION, selectPosition);
//                        
//                        textviewLocation.setText(getCurrentLocaleInfo().name);
//                        if(phoneCountryCode != null){
//                        	phoneCountryCode.setText(getCurrentLocaleInfo().countrycode);
//                        }
//                        mAlertDialog.dismiss();
//                    }
//                }).setPositiveButton(R.string.cancel, new DialogInterface.OnClickListener() {
//                    public void onClick(DialogInterface dialog, int whichButton) {
//
//                    }
//                }).create();
//        mAlertDialog.show();
//    }
//    
//    private void setLocation(){
//    	SharePrefence mSharePrefence = SharePrefence.getInstance(mFragmentActivity);
//    	if(mSharePrefence.containKey(SharePrefence.REGIST_LOCATION)){
//    		selectPosition = mSharePrefence.getInt(SharePrefence.REGIST_LOCATION, selectPosition);
//    	}else{
//    		Locale defaultLocale = Locale.getDefault();
//    		for(int i = 0 ,size = mOpenCountryCode.data.size();i < size;i++){
//    			CountryInfo mCountryInfo = mOpenCountryCode.data.get(i);
//    			log.debug("defaultLocale="+defaultLocale.getCountry() +",lan="+defaultLocale.getLanguage());
//    			if(mCountryInfo.locale.toLowerCase().equals(defaultLocale.getCountry().toLowerCase())){
//    				selectPosition = i;
//    			}
//    		}
//    	}
//    	textviewLocation.setText(getCurrentLocaleInfo().name);
//        if(phoneCountryCode != null){
//        	phoneCountryCode.setText(getCurrentLocaleInfo().countrycode);
//        }
//    }
//    
//    protected CountryInfo getCurrentLocaleInfo() {
//    	try {
//    		return mOpenCountryCode.data.get(selectPosition);
//		} catch (IndexOutOfBoundsException e) {
//			e.printStackTrace();
//			selectPosition = 0;
//			SharePrefence mSharePrefence = SharePrefence.getInstance(mFragmentActivity);
//			mSharePrefence.remove(SharePrefence.REGIST_LOCATION);
//			return mOpenCountryCode.data.get(0);
//		}
//    }

	@Override
	protected String getCountryCode() {
		return "0086"/*getCurrentLocaleInfo().countrycode*/;
	}

	@Override
	protected boolean checkisValidate() {
		boolean isValidatePhone = isNumeric(mEditUsername.getText().toString().trim());
		if(!isValidatePhone){
			Util.showToast(mFragmentActivity, getString(R.string.regist_invalidate_password), Toast.LENGTH_SHORT);
			mEditUsername.requestFocus();
		}
		return isValidatePhone;
	}
	
	public static boolean isNumeric(String str){ 
	    Pattern pattern = Pattern.compile("[0-9]*"); 
	    return pattern.matcher(str).matches();    
	 } 
}
