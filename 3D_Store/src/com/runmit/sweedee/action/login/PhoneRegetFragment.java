package com.runmit.sweedee.action.login;

import java.util.Locale;
import java.util.Timer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.login.RegetPassword.Status;
import com.runmit.sweedee.manager.RegistManager;
import com.runmit.sweedee.manager.ResetPassWord;
import com.runmit.sweedee.model.OpenCountryCode;
import com.runmit.sweedee.model.OpenCountryCode.CountryInfo;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.sdkex.httpclient.simplehttp.HttpManager;
import com.runmit.sweedee.sdkex.httpclient.simplehttp.HttpManager.OnLoadFinishListener;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.StaticHandler;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.StaticHandler.MessageListener;
public class PhoneRegetFragment extends BaseFragment implements MessageListener{
	
	private EditText phone_num;
	private Button phonenum_next;
	
	private TextView textviewLocation;
	private TextView phone_countrycode;
	private RelativeLayout select_location_layout;
	
	private RegetPassword mFragmentActivity;
	private OpenCountryCode mOpenCountryCode;
	private int selectPosition;
	private String phoneNumber;

	private ResetPassWord resetManager = new ResetPassWord();
	private AlertDialog mAlertDialog;
		
	public void handleMessage(Message msg) {
		switch (msg.what) {
		case ResetPassWord.MSG_GET_VERIFYCODE:
			if(isDetached()){
				return;
			}
			mFragmentActivity.hideProgress();
			if(msg.arg1==0){
				mFragmentActivity.setValidAccount(phoneNumber, 0);	// 设置新号码
				mFragmentActivity.setStatus(Status.Reset);// 跳转下一页
			}
			else if(msg.arg1 == ResetPassWord.ERROR_CODE_ACCOUNT_ISNOT_EXIT){
				Util.showToast(mFragmentActivity, mFragmentActivity.getString(R.string.account_not_found), Toast.LENGTH_LONG); //"账号不存在"
			}else{
				Util.showToast(mFragmentActivity, mFragmentActivity.getString(R.string.play_neterror_and_trylater), Toast.LENGTH_LONG); //"网络错误" 
			}
			break;
		default:break;
		}
	}
	
	private StaticHandler mHandler;
    
    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        this.mFragmentActivity = (RegetPassword) activity;
        mHandler = new StaticHandler(this);
    }
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		return inflater.inflate(R.layout.reget_pwd_phone, container, false);
	}
	
	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {
		phone_num = (EditText)view.findViewById(R.id.phone_num);
		phonenum_next = (Button)view.findViewById(R.id.phonenum_next);
		phonenum_next.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View v) {
				phoneNumber= phone_num.getText().toString();
				if(isPhoneValidate(phoneNumber)){
					resetManager.getVerifyCode(phoneNumber, getCurrentLocaleInfo().countrycode, 2, mHandler);
					mFragmentActivity.showProgress(getActivity().getString(R.string.sending_verify_code)); //"正在发送验证码"
				}else{
		    		Util.showToast(getActivity(), getString(R.string.regist_invalidate_password), Toast.LENGTH_LONG);
		    	}
			}
		});
		
		
		select_location_layout = (RelativeLayout) view.findViewById(R.id.select_location_layout);
		select_location_layout.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				selectLocation();
			}
		});
		textviewLocation = (TextView)view.findViewById(R.id.textviewLocation);
		phone_countrycode = (TextView)view.findViewById(R.id.phone_countrycode);
		
		if(mOpenCountryCode != null){
			textviewLocation.setText(getCurrentLocaleInfo().name);
			phone_countrycode.setText(getCurrentLocaleInfo().countrycode);
		}
		
		super.onViewCreated(view, savedInstanceState);
	}
	
	private void selectLocation() {
		if(mOpenCountryCode == null)return;
        MySimpleArrayAdapter mySimpleArrayAdapter = new MySimpleArrayAdapter(getActivity(), mOpenCountryCode.data);
        int position = SharePrefence.getInstance(getActivity()).getInt(SharePrefence.REGIST_LOCATION, selectPosition);
        mAlertDialog = new AlertDialog.Builder(getActivity()).setTitle(Util.setColourText(getActivity(), R.string.select_location, Util.DialogTextColor.BLUE))
                .setSingleChoiceItems(mySimpleArrayAdapter, position, new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        selectPosition = which;
                        SharePrefence.getInstance(getActivity()).putInt(SharePrefence.REGIST_LOCATION, selectPosition);
                        updateUI();
                        mAlertDialog.dismiss();
                    }
                }).setPositiveButton(R.string.cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {

                    }
                }).create();
        mAlertDialog.show();
    }
	
	@Override
    public void onResume(){
    	if(mOpenCountryCode != null){
    		selectPosition = SharePrefence.getInstance(getActivity()).getInt(SharePrefence.REGIST_LOCATION, selectPosition);
            if(phone_countrycode != null){
            	textviewLocation.setText(getCurrentLocaleInfo().name);
    			phone_countrycode.setText(getCurrentLocaleInfo().countrycode);
            }
    	}else{
    		new HttpManager(mFragmentActivity).loadCountryCode(new OnLoadFinishListener<OpenCountryCode>() {
    			
    			@Override
    			public void onLoad(final OpenCountryCode openCountryCode) {
    				mOpenCountryCode = openCountryCode;
    				setLocation();
    				if(phone_countrycode != null){
    	            	textviewLocation.setText(getCurrentLocaleInfo().name);
    	    			phone_countrycode.setText(getCurrentLocaleInfo().countrycode);
    	            }
    			}
    		});
    	}
    	
    	super.onResume();
    }
	
	
	private void setLocation(){
    	SharePrefence mSharePrefence = SharePrefence.getInstance(getActivity());
    	if(mSharePrefence.containKey(SharePrefence.REGIST_LOCATION)){
    		selectPosition = mSharePrefence.getInt(SharePrefence.REGIST_LOCATION, selectPosition);
    	}else{
    		Locale defaultLocale = Locale.getDefault();
    		for(int i = 0 ,size = mOpenCountryCode.data.size();i < size;i++){
    			CountryInfo mCountryInfo = mOpenCountryCode.data.get(i);
    			if(mCountryInfo.locale.toLowerCase().equals(defaultLocale.getCountry().toLowerCase())){
    				selectPosition = i;
    			}
    		}
    	}
    	
    	
    }
	
	public CountryInfo getCurrentLocaleInfo() {
        try {
    		return mOpenCountryCode.data.get(selectPosition);
		} catch (IndexOutOfBoundsException e) {
			e.printStackTrace();
			selectPosition = 0;
			SharePrefence mSharePrefence = SharePrefence.getInstance(mFragmentActivity);
			mSharePrefence.remove(SharePrefence.REGIST_LOCATION);
			return mOpenCountryCode.data.get(0);
		}
    }
	
	private void updateUI() {
		if(textviewLocation != null){
	    	textviewLocation.setText(getCurrentLocaleInfo().name);
		}
        if(phone_countrycode != null){
        	phone_countrycode.setText(getCurrentLocaleInfo().countrycode);
        }
	}
	
	/**
     * 手机号是否正确的正则表达式
     *
     * @param phoneNumber
     * @return
     */
    private boolean isPhoneValidate(String phoneNumber) {
    	if(getCurrentLocaleInfo().locale.equalsIgnoreCase(Locale.CHINA.getCountry())){
          String check = "^((13[0-9])|(15[0-9])|(17[0-9])|(18[0-9]))\\d{8}$";
          Pattern regex = Pattern.compile(check);
          Matcher matcher = regex.matcher(phoneNumber);
          boolean isMatched = matcher.matches();
          return isMatched;
    	}else if(getCurrentLocaleInfo().locale.equalsIgnoreCase(Locale.TAIWAN.getCountry())){
    		String check = "^(09|9)\\d{8}$";
            Pattern regex = Pattern.compile(check);
            Matcher matcher = regex.matcher(phoneNumber);
            boolean isMatched = matcher.matches();
            return isMatched;
    	}else{
    		return phoneNumber != null && phoneNumber.length() > 0;
    	}
    }
	
	// ===================== 外部调用 =====================
	
	public void setOpenCountryCode(OpenCountryCode openCountryCode) {
		mOpenCountryCode = openCountryCode;
		setLocation();
		updateUI();
	}
}
