package com.runmit.sweedee.action.pay.paypassword;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.gridpasswordview.GridPasswordView;
import com.runmit.sweedee.view.gridpasswordview.GridPasswordView.OnPasswordChangedListener;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.text.Html;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

public class SetPasswordView extends FrameLayout{

	public SetPasswordView(Context context, AttributeSet attrs, int defStyleAttr) {
		super(context, attrs, defStyleAttr);
		initView(context,R.layout.activity_setting_password);
	}

	public SetPasswordView(Context context, AttributeSet attrs) {
		super(context, attrs);
		initView(context,R.layout.activity_setting_password);
	}

	public SetPasswordView(Context context) {
		super(context);
		initView(context,R.layout.activity_setting_password);
	}
	
	public SetPasswordView(Context context,int layoutId) {
		super(context);
		initView(context,layoutId);
	}
	

	private XL_Log log = new XL_Log(SettingPayPasswordActivity.class);

	private LinearLayout enter_new_password_layout;
	private LinearLayout ensure_new_password_layout;

	private GridPasswordView enter_new_password_gpv;
	private GridPasswordView ensure_new_password_gpv;

	private int mCurrentState = 1;
	
	private String mFirstPassword;
	
	private String mSecondPassword;
	
	private OnDialogDismissListener mOnDialogDismissListener;

	public void setOnDialogDismissListener(OnDialogDismissListener mOnDialogDismissListener) {
		this.mOnDialogDismissListener = mOnDialogDismissListener;
	}

	private void initView(Context context,int layoutId) {
		LayoutInflater inflater = LayoutInflater.from(context);
	    inflater.inflate(layoutId, this);

		enter_new_password_layout = (LinearLayout) findViewById(R.id.enter_new_password_layout);
		ensure_new_password_layout = (LinearLayout) findViewById(R.id.ensure_new_password_layout);

		enter_new_password_gpv = (GridPasswordView) findViewById(R.id.enter_new_password_gpv);
		ensure_new_password_gpv = (GridPasswordView) findViewById(R.id.ensure_new_password_gpv);
		TextView setting_new_pay_password = (TextView) findViewById(R.id.setting_new_pay_password);
		setting_new_pay_password.setText(Html.fromHtml(getResources().getString(R.string.please_end_new_password)));
		
		enter_new_password_gpv.setOnPasswordChangedListener(new OnPasswordChangedListener() {

			@Override
			public void onMaxLength(String psw) {
				log.debug("psw=" + psw);
				if(isSame(psw)){
					AlertDialog.Builder mBuilder = new AlertDialog.Builder(getContext());
					mBuilder.setTitle(R.string.confirm_title);
					mBuilder.setMessage(R.string.password_is_too_young_too_simple);
					mBuilder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
							enter_new_password_gpv.clearPassword();
						}
					});
					mBuilder.create().show();
				}else{
					mFirstPassword = psw;
					setState(2);
				}
			}

			@Override
			public void onChanged(String psw) {
				log.debug("psw=" + psw);
			}
		});

		ensure_new_password_gpv.setOnPasswordChangedListener(new OnPasswordChangedListener() {

			@Override
			public void onMaxLength(String psw) {
				mSecondPassword = psw;
				if(mSecondPassword.equals(mFirstPassword)){
					PayPasswordManager.savePassword(mSecondPassword);
					
					AlertDialog.Builder mBuilder = new AlertDialog.Builder(getContext());
					mBuilder.setTitle(R.string.confirm_title);
					mBuilder.setMessage(R.string.pay_password_set_success);
					mBuilder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
							if(mOnDialogDismissListener != null){
								mOnDialogDismissListener.onDismiss();
							}else{
								Activity mActivity = (Activity) getContext();
								mActivity.setResult(Activity.RESULT_OK);
								mActivity.finish();
							}
						}
					});
					mBuilder.create().show();
				}else{
					AlertDialog.Builder mBuilder = new AlertDialog.Builder(getContext());
					mBuilder.setTitle(R.string.confirm_title);
					mBuilder.setMessage(R.string.two_password_is_not_the_same);
					mBuilder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
							enter_new_password_gpv.clearPassword();
							ensure_new_password_gpv.clearPassword();
							setState(1);
						}
					});
					mBuilder.create().show();
				}
			}

			@Override
			public void onChanged(String psw) {
			}
		});
		setState(1);
	}

	// 判断是否相同
	public static boolean isSame(String str) {
		String regex = str.substring(0, 1) + "{" + str.length() + "}";
		return str.matches(regex);
	}

	private void setState(int state) {
		mCurrentState = state;
		enter_new_password_layout.setVisibility(state == 1 ? View.VISIBLE : View.GONE);
		ensure_new_password_layout.setVisibility(state == 2 ? View.VISIBLE : View.GONE);
		if (mCurrentState == 1) {
			enter_new_password_gpv.showIM();
		} else {
			ensure_new_password_gpv.showIM();
		}
	}

	public static interface OnDialogDismissListener{
		void onDismiss();
	}

}
