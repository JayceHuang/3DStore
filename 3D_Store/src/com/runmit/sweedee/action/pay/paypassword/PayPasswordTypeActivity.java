package com.runmit.sweedee.action.pay.paypassword;

import com.runmit.sweedee.ActionBarActivity;
import com.runmit.sweedee.R;

import android.R.integer;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;

public class PayPasswordTypeActivity extends ActionBarActivity{
    public static final int REQUESTCODE_SETPAYPASSWORD=0;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if(PayPasswordManager.isPayPasswordEmpty()){
			startActivityForResult(new Intent(PayPasswordTypeActivity.this, SettingPayPasswordActivity.class),REQUESTCODE_SETPAYPASSWORD);
		}
		
		setContentView(R.layout.activity_passwordtype);
		findViewById(R.id.change_pay_password_layout).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				startActivity(new Intent(PayPasswordTypeActivity.this, ChangePayPasswordActivity.class));
			}
		});
		findViewById(R.id.forget_pay_password_layout).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				startActivity(new Intent(PayPasswordTypeActivity.this, ForgetPayPasswordActivity.class));
			}
		});
	}
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		// TODO Auto-generated method stub
		super.onActivityResult(requestCode, resultCode, data);
		if(resultCode!= RESULT_OK){
			finish();
		}
	}
	
	
}
