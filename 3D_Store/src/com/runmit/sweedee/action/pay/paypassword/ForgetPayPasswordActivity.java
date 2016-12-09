package com.runmit.sweedee.action.pay.paypassword;

import com.runmit.sweedee.ActionBarActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.login.RegetPassword;
import com.runmit.sweedee.sdk.user.member.task.UserManager;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.TextView;

public class ForgetPayPasswordActivity extends ActionBarActivity {
	
	private EditText editText_login_password;
	
	private Button button_next_step;
	
	private TextView textView_forget_login_password;
	
	private TextView user_account_textview;
	
	private FrameLayout content_view;
	
	private AlertDialog mCancelAlertDialog;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_forget_password);
		
		content_view = (FrameLayout) findViewById(R.id.content_view);
		
		editText_login_password = (EditText) findViewById(R.id.editText_login_password);
		button_next_step = (Button) findViewById(R.id.button_next_step);
		user_account_textview = (TextView) findViewById(R.id.user_account_textview);
		user_account_textview.setText(UserManager.getInstance().getMemberInfo().account);
		textView_forget_login_password = (TextView) findViewById(R.id.textView_forget_login_password);
		textView_forget_login_password.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				Intent intent = new Intent(ForgetPayPasswordActivity.this, RegetPassword.class);
				startActivity(intent);
			}
		});
		
		button_next_step.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				String enterPassword = editText_login_password.getText().toString().trim();
				if(TextUtils.isEmpty(enterPassword)){
					AlertDialog.Builder mBuilder = new AlertDialog.Builder(ForgetPayPasswordActivity.this);
					mBuilder.setTitle(R.string.confirm_title);
					mBuilder.setMessage(R.string.login_password_is_incorrect);
					mBuilder.setPositiveButton(R.string.ok, new OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
						}
					});
					mBuilder.create().show();
				}else{
					if(enterPassword.equals(UserManager.getInstance().getPassword())){
						content_view.removeAllViews();
						SetPasswordView mPasswordView = new SetPasswordView(ForgetPayPasswordActivity.this);
						content_view.addView(mPasswordView, new LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.MATCH_PARENT));
					}else{
						AlertDialog.Builder mBuilder = new AlertDialog.Builder(ForgetPayPasswordActivity.this);
						mBuilder.setTitle(R.string.confirm_title);
						mBuilder.setMessage(R.string.login_password_is_incorrect);
						mBuilder.setPositiveButton(R.string.ok, new OnClickListener() {

							@Override
							public void onClick(DialogInterface dialog, int which) {
							}
						});
						mBuilder.create().show();
					}
				}
			}
		});
	}
	
	@Override
	public void onBackPressed() {
		ConfirmedDialog();
	}
	
	private void ConfirmedDialog(){
		if (mCancelAlertDialog != null && mCancelAlertDialog.isShowing()) {
			return;
		}
		AlertDialog.Builder mBuilder = new AlertDialog.Builder(this);
		mBuilder.setTitle(R.string.confirm_title);
		mBuilder.setMessage(R.string.ensure_cancel_reget_pay_password);
		mBuilder.setPositiveButton(R.string.ok, new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
				finish();
			}
		});
		mBuilder.setNegativeButton(R.string.cancel, new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
			}
		});
		mCancelAlertDialog = mBuilder.create();
		mCancelAlertDialog.show();
	}
	
	 @Override
	public boolean onOptionsItemSelected(MenuItem item) {
		        switch (item.getItemId()) {
		            case android.R.id.home:
		            	ConfirmedDialog();
		                break;
		            default:
		                break;
		        }
		        return false;
		    }

}
