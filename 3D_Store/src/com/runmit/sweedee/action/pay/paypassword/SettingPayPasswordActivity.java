package com.runmit.sweedee.action.pay.paypassword;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.view.MenuItem;

import com.runmit.sweedee.ActionBarActivity;
import com.runmit.sweedee.R;

public class SettingPayPasswordActivity extends ActionBarActivity {

	private AlertDialog mCancelAlertDialog;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(new SetPasswordView(this));
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
		mBuilder.setMessage(R.string.ensure_cancel_setting_password);
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
