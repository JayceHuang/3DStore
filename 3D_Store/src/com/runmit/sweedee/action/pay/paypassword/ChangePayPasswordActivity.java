package com.runmit.sweedee.action.pay.paypassword;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;

import com.runmit.sweedee.ActionBarActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.view.gridpasswordview.GridPasswordView;
import com.runmit.sweedee.view.gridpasswordview.GridPasswordView.OnPasswordChangedListener;

public class ChangePayPasswordActivity extends ActionBarActivity {

	private GridPasswordView enter_oldpassword_gpv;

	private AlertDialog mCancelAlertDialog;

	private FrameLayout content_framelayout;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_changepassword);
		content_framelayout = (FrameLayout) findViewById(R.id.content_framelayout);
		enter_oldpassword_gpv = (GridPasswordView) findViewById(R.id.enter_oldpassword_gpv);
		enter_oldpassword_gpv.showIM();
		
		enter_oldpassword_gpv.setOnPasswordChangedListener(new OnPasswordChangedListener() {

			@Override
			public void onMaxLength(String psw) {
				if (PayPasswordManager.checkPassword(psw)) {
					content_framelayout.removeAllViews();
					content_framelayout.addView(new SetPasswordView(ChangePayPasswordActivity.this), new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
				} else {
					AlertDialog.Builder mBuilder = new AlertDialog.Builder(ChangePayPasswordActivity.this);
					mBuilder.setTitle(R.string.confirm_title);
					mBuilder.setMessage(R.string.pay_password_error);
					mBuilder.setPositiveButton(R.string.ok, new OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
							enter_oldpassword_gpv.clearPassword();
						}
					});
					mBuilder.create().show();
				}
			}

			@Override
			public void onChanged(String psw) {

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
		mBuilder.setMessage(R.string.ensure_cancel_change_password);
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
