package com.runmit.sweedee.action.pay;

import java.text.DateFormat;
import java.text.MessageFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import com.umeng.analytics.MobclickAgent;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.more.TaskManagerFragment;
import com.runmit.sweedee.action.pay.paypassword.ForgetPayPasswordActivity;
import com.runmit.sweedee.action.pay.paypassword.PayPasswordManager;
import com.runmit.sweedee.action.pay.paypassword.PayPasswordTypeActivity;
import com.runmit.sweedee.action.pay.paypassword.SetPasswordView;
import com.runmit.sweedee.action.pay.paypassword.SetPasswordView.OnDialogDismissListener;
import com.runmit.sweedee.manager.AccountServiceManager;
import com.runmit.sweedee.manager.WalletManager;
import com.runmit.sweedee.manager.WalletManager.OnAccountChangeListener;
import com.runmit.sweedee.manager.data.EventCode.AccountEventCode;
import com.runmit.sweedee.model.PayOrder;
import com.runmit.sweedee.model.PayStyle;
import com.runmit.sweedee.model.UserAccount;
import com.runmit.sweedee.model.VOPrice;
import com.runmit.sweedee.model.VOPrice.PriceInfo;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.util.DateUtil;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.gridpasswordview.GridPasswordView;
import com.runmit.sweedee.view.gridpasswordview.GridPasswordView.OnPasswordChangedListener;

import android.R.integer;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.Parcelable;
import android.provider.ContactsContract.Contacts.Data;
import android.text.Html;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

public class PayStyleActivity extends Activity {

	private XL_Log log = new XL_Log(PayStyleActivity.class);
	
	private static final int PAY_ALLOW_TOTALCOUNT=5;
	
	private static final String PRE_PAY_ALLOWLEFTCOUNT="pre_pay_allowleftcount";
	
	private static final String PRE_PAY_LASTERRORTIME="pre_pay_lasterrortime";
	
	private static final long PAY_UNLOCK_TIMESPAND=10*60*1000;

	private TextView textView_myMoney;

	private UserAccount mUserAccount;

	private VOPrice mVOPrice;

	private ProgressDialog mProgressDialog;

	private List<PayStyle> mPayStylesList = null;
	
	private PayStyle mBalancePay;
	
	private RelativeLayout wallet_pay_layout;
	
	private TextView textView_virtual_money;
	
	private RelativeLayout virtualmoney_pay_layout;
	
	private long mUserId;
	
	private OnAccountChangeListener mAccountChangeListener = new OnAccountChangeListener() {
		
		@Override
		public void onAccountChange(int rtnCode, List<UserAccount> mAccounts) {
			setAccount();
		}
	};
	
	private void setAccount(){
		mUserAccount = WalletManager.getInstance().getReadAccount();
		mUserId=UserManager.getInstance().getUserId();
		
		if(mUserAccount != null){
			int price=(int) (mUserAccount.amount/100);
			textView_myMoney.setText(""+price+mUserAccount.symbol);
		}
		UserAccount mVirtual = WalletManager.getInstance().getVirtualAccount();
		if(mVirtual != null){
			int amount = (int) mVirtual.amount;
			textView_virtual_money.setText(Integer.toString(amount));
		}
	}

	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
//			case AccountEventCode.EVENT_GET_MOVIE_PALY_STYLE:
//				log.debug("EVENT_GET_MOVIE_PALY_STYLE msg.arg1=" + msg.arg1);
//				Util.dismissDialog(mProgressDialog);
//				if (msg.arg1 == 0 || msg.arg1 == 200) {
//					mPayStylesList = (List<PayStyle>) msg.obj;
//					log.debug("mPayStylesList=" + mPayStylesList.size());
//					for(int i =0,size=mPayStylesList.size();i<size;i++){
//						if(mPayStylesList.get(i).id == PayOrder.PAYMENT_WALLET){//钱包支付
//							mBalancePay = mPayStylesList.get(i);
//						}
//					}
//					mPayStylesList.remove(mBalancePay);
//					
//					if(mBalancePay == null){//钱包支付不可用
//						wallet_pay_layout.setVisibility(View.GONE);
//					}
//				} else {
//					Util.showToast(PayStyleActivity.this, getString(R.string.pay_get_may_method_fail), Toast.LENGTH_LONG);
//				}
//				break;
			case AccountEventCode.EVENT_GET_MOVIE_PALY_ORDER:
				Util.dismissDialog(mProgressDialog);				
				log.debug("EVENT_GET_MOVIE_PALY_ORDER msg.arg1="+msg.arg1);
				PayOrder mPayOrder = null;
				if(msg.arg1==0 || msg.arg1 == 200){
					mPayOrder=(PayOrder) msg.obj;
					log.debug("mPayOrder="+mPayOrder);
					if(mPayOrder.paymentId==PayOrder.PAYMENT_WALLET || mPayOrder.paymentId==PayOrder.PAYMENT_VIRTUAL_WALLET){//钱包支付
						showPayResultDialog(true,getString(R.string.pay_result_dialog_title), getString(R.string.pay_success_dialog_message));
					}
				}else{
					Util.showToast(PayStyleActivity.this, getString(R.string.pay_get_order_fail)+msg.arg1, Toast.LENGTH_LONG);
				}
				ReportActionSender.getInstance().pay(msg.arg1, mPayOrder != null ? mPayOrder.paymentId : 0, mPayOrder != null ? mPayOrder.currencyId : 0, mPayOrder != null ? mPayOrder.amount:0, mPayOrder != null ? mPayOrder.productId:0, mPayOrder != null ? mPayOrder.orderId:0, 1, mPayOrder != null ? mPayOrder.channelId:0);
				break;
			default:
				break;
			}
		};
	};
	
	protected void onResume() {
		super.onResume();
        MobclickAgent.onResume(this);
	}

    @Override
    protected void onPause() {
        super.onPause();
        MobclickAgent.onPause(this);
    }
	
	private void showPayResultDialog(final boolean success,String title,String message){
		AlertDialog.Builder builder = new AlertDialog.Builder(this,R.style.AlertDialog);
		builder.setTitle(title);
		builder.setMessage(message);
		builder.setInverseBackgroundForced(true);
		builder.setNegativeButton(Util.setColourText(this, R.string.ok, Util.DialogTextColor.BLUE), new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
			
			}
		});
		AlertDialog mAlertDialog = builder.create();
		mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
			
			@Override
			public void onDismiss(DialogInterface dialog) {
				if(success){
					//发送支付成功广播
					TaskManagerFragment.sendOrderPaidBroadcast(PayStyleActivity.this, mVOPrice.productId);
					finish();
				}
			}
		});
		mAlertDialog.show();
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_paystyle);
		getActionBar().setDisplayHomeAsUpEnabled(true);
		getActionBar().setDisplayShowHomeEnabled(false);
		mProgressDialog = new ProgressDialog(this,R.style.ProgressDialogDark);

		textView_myMoney = (TextView) findViewById(R.id.textView_myMoney);
		textView_virtual_money = (TextView) findViewById(R.id.textView_virtual_money);
		
		mVOPrice = (VOPrice) getIntent().getSerializableExtra(VOPrice.class.getSimpleName());
		mPayStylesList = getIntent().getParcelableArrayListExtra(PayStyle.class.getSimpleName());
		log.debug("mVOPrice="+mVOPrice+",mPayStylesList="+mPayStylesList);
		
		if (mVOPrice == null || mVOPrice.prices == null || mVOPrice.prices.size() == 0) {
			Util.showToast(this, getString(R.string.pay_price_null), Toast.LENGTH_LONG);
			finish();
			return;
		}

		if(mPayStylesList == null || mPayStylesList.size() == 0){
			Util.showToast(this, getString(R.string.pay_price_null), Toast.LENGTH_LONG);
			finish();
			return;
		}
		
		virtualmoney_pay_layout = (RelativeLayout) findViewById(R.id.virtualmoney_pay_layout);
		virtualmoney_pay_layout.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				showVirtualPayDialog();
			}
		});
		
		if(mVOPrice != null){
			final PriceInfo mPriceInfo = mVOPrice.getVirtualPriceInfo();
			if (mPriceInfo == null) {
				virtualmoney_pay_layout.setVisibility(View.GONE);
				findViewById(R.id.cut_line_view).setVisibility(View.GONE);
			}
		}
		
		findViewById(R.id.cash_pay_layout).setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View v) {
				Intent intent = new Intent(PayStyleActivity.this, PayMovieActivity.class);
				intent.putExtra(VOPrice.class.getSimpleName(), mVOPrice);
				mPayStylesList.remove(mBalancePay);
				intent.putParcelableArrayListExtra(PayStyle.class.getSimpleName(), (ArrayList<? extends Parcelable>) mPayStylesList);
				startActivity(intent);
				finish();
			}
		});

		wallet_pay_layout = (RelativeLayout) findViewById(R.id.wallet_pay_layout);
		wallet_pay_layout.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View v) {
				if (mUserAccount == null) {
					Util.showToast(PayStyleActivity.this, getString(R.string.get_wallet_fail_and_not_allow_pay), Toast.LENGTH_LONG);
					return;
				}
				
				int mPayLeftCount=SharePrefence.getInstance(PayStyleActivity.this).getInt(PRE_PAY_ALLOWLEFTCOUNT+"_"+mUserId, PAY_ALLOW_TOTALCOUNT);
				if(mPayLeftCount<1){			
					DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");	
					Date mLastErrorDate;
					try {
						 mLastErrorDate=formatter.parse(SharePrefence.getInstance(PayStyleActivity.this).getString(PRE_PAY_LASTERRORTIME+"_"+mUserId,DateUtil.getNowDateString()));
						 long timeSpan= new Date().getTime()-mLastErrorDate.getTime();
						 log.debug("timespan:"+timeSpan);
						 if(timeSpan>PAY_UNLOCK_TIMESPAND){
							 SharePrefence.getInstance(PayStyleActivity.this).remove(PRE_PAY_ALLOWLEFTCOUNT+"_"+mUserId);
							 SharePrefence.getInstance(PayStyleActivity.this).remove(PRE_PAY_LASTERRORTIME+"_"+mUserId);
						 }else{
						  showPasswordLockDialog();
						  return;
						 }
						
					} catch (ParseException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}

				}
				//mUserAccount.amount=10000;	
				if(TextUtils.isEmpty(PayPasswordManager.getPayPassword())){
					showSetPayPasswordDialog();	
				}else{	
				if (mUserAccount.amount >= Long.parseLong(mVOPrice.getDefaultRealPriceInfo().realPrice)) {
					showPayDialog();
				} else {
					showMoneyPoolDialog();
				}
			 }	
			}
		});
//		PriceInfo mPriceInfo = mVOPrice.prices.get(0);
//		AccountServiceManager.getInstance().reqMoviePayStyle(mPriceInfo.price, mPriceInfo.currencyId, mHandler);
		WalletManager.getInstance().addAccountChangeListener(mAccountChangeListener);
		setAccount();
		initPayStyle();
	}
	
	/**
	 * 显示虚拟货币支付的对话框
	 */
	private void showVirtualPayDialog() {
		final PriceInfo mPriceInfo = mVOPrice.getVirtualPriceInfo();
		final UserAccount mVirtualAccount = WalletManager.getInstance().getVirtualAccount();
		log.debug("mPriceInfo=" + mPriceInfo + ",VirtualAccount=" + mVirtualAccount);
		if (mPriceInfo == null) {
			Util.showToast(PayStyleActivity.this, getString(R.string.pay_price_null), Toast.LENGTH_SHORT);
			return;
		}
		if(mVirtualAccount == null){
			Util.showToast(PayStyleActivity.this, getString(R.string.sorry_get_wallet_is_null), Toast.LENGTH_SHORT);
			return;
		}
		AlertDialog.Builder mBuilder = new AlertDialog.Builder(PayStyleActivity.this,R.style.AlertDialog);
		mBuilder.setTitle(Util.setColourText(PayStyleActivity.this, R.string.pay_result_dialog_title, Util.DialogTextColor.BLUE));
		View mContentView = getLayoutInflater().inflate(R.layout.view_virtual_pay_content, null, false);
		TextView textView_need_pay_msg = (TextView) mContentView.findViewById(R.id.textView_need_pay_msg);
		TextView textView_remain_pay_toast = (TextView) mContentView.findViewById(R.id.textView_remain_pay_toast);
		String text = MessageFormat.format(getString(R.string.pay_movie_need_virtual_money_title), mPriceInfo.realPrice);
		log.debug("text="+text);
		int start = text.indexOf(mPriceInfo.realPrice);
		int size = mPriceInfo.realPrice.length();
		  
		SpannableStringBuilder style=new SpannableStringBuilder(text);     
		style.setSpan(new ForegroundColorSpan(getResources().getColor(R.color.red_btn_normal)),start,start + size,Spannable.SPAN_EXCLUSIVE_INCLUSIVE);      
		textView_need_pay_msg.setText(style);  
		           
		textView_remain_pay_toast.setText(MessageFormat.format(getString(R.string.pay_movie_need_virtual_money_msg), mVirtualAccount.amount));
		mBuilder.setView(mContentView);
		
		mBuilder.setPositiveButton(R.string.cancel, new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
				// TODO Auto-generated method stub

			}
		}).setNegativeButton(Util.setColourText(PayStyleActivity.this, R.string.ok, Util.DialogTextColor.BLUE), new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
				if (mVirtualAccount.amount >= Integer.parseInt(mPriceInfo.realPrice)) {
					Util.showDialog(mProgressDialog, getString(R.string.pay_create_order_ing));
					AccountServiceManager.getInstance().getMoviePayOrder(PayStyleActivity.this, mVOPrice.productName, UserManager.getInstance().getUserId(), mVOPrice.productId, mVOPrice.productType,
							PayOrder.PAYMENT_VIRTUAL_WALLET, mPriceInfo.price, mPriceInfo.currencyId, mPriceInfo.channelId, mHandler);
				} else {
					showVirtualMoneyNotEnough();
				}
			}
		});
		mBuilder.create().show();
	};
	
	/**
	 * 显示账户余额不足
	 */
	private void showVirtualMoneyNotEnough() {
		AlertDialog.Builder mBuilder = new AlertDialog.Builder(PayStyleActivity.this,R.style.AlertDialog);
		mBuilder.setTitle(getString(R.string.pay_result_dialog_title));
		String moneyNotEnough = getString(R.string.pay_movie_need_virtual_money_not_enough)+"\n"+MessageFormat.format(getString(R.string.pay_movie_need_virtual_money_msg), WalletManager.getInstance().getVirtualAccount().amount);
		mBuilder.setMessage(moneyNotEnough);
		mBuilder.setPositiveButton(R.string.ok, new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
			}
		});
		mBuilder.create().show();
	}

	private void initPayStyle(){
		log.debug("mPayStylesList=" + mPayStylesList.size());
		for(int i =0,size=mPayStylesList.size();i<size;i++){
			if(mPayStylesList.get(i).id == PayOrder.PAYMENT_WALLET){//钱包支付
				mBalancePay = mPayStylesList.get(i);
			}
		}
		if(mBalancePay == null){//钱包支付不可用
			wallet_pay_layout.setVisibility(View.GONE);
		}
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		WalletManager.getInstance().removeAccountChangeListener(mAccountChangeListener);
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case android.R.id.home:
			finish();
			break;
		default:
			break;
		}
		return super.onOptionsItemSelected(item);
	}

	private void showMoneyPoolDialog() {
		AlertDialog.Builder builder = new AlertDialog.Builder(this, R.style.AlertDialog);
		builder.setTitle(getString(R.string.wallet_pay_money_pool));
		builder.setMessage(MessageFormat.format(getString(R.string.wallet_pay_dialog_message), mUserAccount.amount/100));
		builder.setInverseBackgroundForced(true);
		builder.setPositiveButton(R.string.cancel, new DialogInterface.OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {

			}
		});
		builder.setNegativeButton(Util.setColourText(this, R.string.recharge, Util.DialogTextColor.BLUE), new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				Intent intent = new Intent(PayStyleActivity.this, RechargeActivity.class);
				intent.putExtra(UserAccount.class.getSimpleName(), mUserAccount);
				startActivity(intent);
			}
		});
		AlertDialog mAlertDialog = builder.create();
		mAlertDialog.show();

	}
	
	private void showPasswordErrorDialog(int leftCount) {
		AlertDialog.Builder builder = new AlertDialog.Builder(this, R.style.AlertDialog);
		String mDialogMessage = String.format(getString(R.string.pay_password_error_retry),leftCount);  
		builder.setMessage(mDialogMessage);
		builder.setInverseBackgroundForced(true);
		builder.setPositiveButton(R.string.cancel, new DialogInterface.OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {

			}
		});
		builder.setNegativeButton(R.string.forget_pay_password, new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				startActivity(new Intent(PayStyleActivity.this, ForgetPayPasswordActivity.class));
			}
		});
		AlertDialog mAlertDialog = builder.create();
		mAlertDialog.show();

	}
	
	private void showPasswordLockDialog() {
		AlertDialog.Builder builder = new AlertDialog.Builder(this, R.style.AlertDialog);
		builder.setMessage(MessageFormat.format(getString(R.string.pay_password_error_lock), mUserAccount.amount/100));
		builder.setInverseBackgroundForced(true);
		builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {

			}
		});
	
		AlertDialog mAlertDialog = builder.create();
		mAlertDialog.show();

	}
	
	private void showSetPayPasswordDialog() {
		AlertDialog.Builder builder = new AlertDialog.Builder(this, R.style.AlertDialog);
		builder.setTitle(R.string.welcome_to_user_sweedee_wallet);
		SetPasswordView mSetPasswordView = new SetPasswordView(PayStyleActivity.this,R.layout.dialog_setting_password);
		builder.setView(mSetPasswordView);
		builder.setInverseBackgroundForced(true);
		final AlertDialog mAlertDialog = builder.create();
		mSetPasswordView.setOnDialogDismissListener(new OnDialogDismissListener() {
			
			@Override
			public void onDismiss() {
				mAlertDialog.dismiss();
			}
		});
		mAlertDialog.show();
	}

	private void showPayDialog() {
		
			AlertDialog.Builder builder = new AlertDialog.Builder(this, R.style.AlertDialog);
			final PriceInfo mPriceInfo = mVOPrice.getDefaultRealPriceInfo();
			String realPrice = mPriceInfo.realPrice;
	        float price = Float.parseFloat(realPrice) / 100;
			builder.setTitle(R.string.please_enter_pay_password);
			final View mContentView = LayoutInflater.from(this).inflate(R.layout.enter_pay_password_dialog_view, null);
			TextView textView_pay_to_bug_product_name = (TextView) mContentView.findViewById(R.id.textView_pay_to_bug_product_name);
			textView_pay_to_bug_product_name.setText(MessageFormat.format(getString(R.string.pay_to_bug_product_name), mVOPrice.productName));
			
			TextView mtextView_need_money = (TextView) mContentView.findViewById(R.id.textView_need_money);
			mtextView_need_money.setText(mPriceInfo.symbol + price);
			
			final GridPasswordView enter_pay_password_gpv = (GridPasswordView) mContentView.findViewById(R.id.enter_pay_password_gpv);
			builder.setView(mContentView);
			builder.setInverseBackgroundForced(true);
			final AlertDialog mAlertDialog = builder.create();
			enter_pay_password_gpv.setOnPasswordChangedListener(new OnPasswordChangedListener() {
				
				@Override
				public void onMaxLength(String psw) {
									
					if(PayPasswordManager.checkPassword(psw)){
						mAlertDialog.dismiss();
						final PayStyle mPayStyle = mBalancePay;
						log.debug("mPayStyle=" + mPayStyle);
						if(mPriceInfo != null){
							Util.showDialog(mProgressDialog, getString(R.string.pay_create_order_ing));
							AccountServiceManager.getInstance().getMoviePayOrder(PayStyleActivity.this, mVOPrice.productName,UserManager.getInstance().getUserId(), mVOPrice.productId, mVOPrice.productType, mPayStyle.id,
									mPriceInfo.price, mPriceInfo.currencyId, mPriceInfo.channelId, mHandler);
						}
						 SharePrefence.getInstance(PayStyleActivity.this).remove(PRE_PAY_ALLOWLEFTCOUNT+"_"+mUserId);
						 SharePrefence.getInstance(PayStyleActivity.this).remove(PRE_PAY_LASTERRORTIME+"_"+mUserId);
					}else{	
						final int mPayLeftCount=SharePrefence.getInstance(PayStyleActivity.this).getInt(PRE_PAY_ALLOWLEFTCOUNT+"_"+mUserId, PAY_ALLOW_TOTALCOUNT);
						String mLastErrorTime=DateUtil.getNowDateString();
						SharePrefence.getInstance(PayStyleActivity.this).putInt(PRE_PAY_ALLOWLEFTCOUNT+"_"+mUserId, mPayLeftCount-1);
						SharePrefence.getInstance(PayStyleActivity.this).putString(PRE_PAY_LASTERRORTIME+"_"+mUserId,mLastErrorTime);
						//Util.showToast(PayStyleActivity.this, getString(R.string.pay_password_error)+"count:"+(mPayLeftCount-1)+"date:"+mLastErrorTime, Toast.LENGTH_LONG);		
						mHandler.postDelayed(new Runnable() {				
							@Override
							public void run() {
								enter_pay_password_gpv.clearPassword();
								mAlertDialog.dismiss();
								if(mPayLeftCount-1<1){
								showPasswordLockDialog();
								}else{
								showPasswordErrorDialog(mPayLeftCount-1);		
								}			
							}
						}, 100);
					}
				}
				
				@Override
				public void onChanged(String psw) {
					
				}
			});
			mAlertDialog.show();
			enter_pay_password_gpv.showIM();

	}
}
