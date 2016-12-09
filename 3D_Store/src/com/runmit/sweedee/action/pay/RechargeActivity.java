package com.runmit.sweedee.action.pay;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.graphics.Paint;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.alipay.sdk.app.PayTask;
import com.loopj.android.http.RequestHandle;
import com.umeng.analytics.MobclickAgent;
import com.runmit.sweedee.R;
import com.runmit.sweedee.manager.AccountServiceManager;
import com.runmit.sweedee.manager.WalletManager;
import com.runmit.sweedee.manager.data.EventCode.AccountEventCode;
import com.runmit.sweedee.model.PayOrder;
import com.runmit.sweedee.model.PayStyle;
import com.runmit.sweedee.model.PayStyleList;
import com.runmit.sweedee.model.Quota;
import com.runmit.sweedee.model.QuotaList;
import com.runmit.sweedee.model.UserAccount;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.StaticHandler;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
//import com.skymobi.pay.app.PayApplication;
//import com.skymobi.pay.sdk.SkyPayServer;
import com.unionpay.UPPayAssistEx;
import com.unionpay.uppay.PayActivity;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class RechargeActivity extends Activity implements StaticHandler.MessageListener{
	
	private XL_Log log = new XL_Log(RechargeActivity.class);

	private LinearLayout pay_stype_layout;

	private Button submit_order;

	private ProgressDialog mProgressDialog;
	
	private List<PayStyle> mPayStylesList=new ArrayList<PayStyle>();
	
	private UserAccount mUserAccount;
	
	private RequestHandle mRequestHandle;
	
	private int check_positon=0;
	
	private static final int RQF_PAY = 1;
	
	private List<View> payStyleViewList;
	
	private int mCurrentAmount = 0;
	
	private List<Quota> mQuotas;
	
	private int currentSelectQuota = 3;
	
	private LinearLayout layout_money1,layout_money2,layout_money3,layout_money4;
	
	private TextView textView_money1,textView_money2,textView_money3,textView_money4;
	


	@Override
	public void handleMessage(Message msg) {
		switch (msg.what) {
		case AccountEventCode.EVENT_GET_RECHARGE_QUOTA://获取充值固定金额
			Util.dismissDialog(mProgressDialog);
			if(msg.arg1 == 0){
				QuotaList obj = (QuotaList) msg.obj;
				mQuotas = obj.data;
				currentSelectQuota = Math.min(currentSelectQuota, mQuotas.size() - 1);
				setRechargeQuoda();
			}else{
				Util.showToast(RechargeActivity.this, getString(R.string.pay_get_recharge_quota), Toast.LENGTH_LONG);
			}
			break;
		case AccountEventCode.EVENT_GET_MOVIE_PALY_STYLE:
			log.debug("EVENT_GET_MOVIE_PALY_STYLE msg.arg1="+msg.arg1);
			Util.dismissDialog(mProgressDialog);
			if(msg.arg1==0 || msg.arg1==200){
				mPayStylesList=((PayStyleList) msg.obj).data;
				log.debug("mPayStylesList="+mPayStylesList.size());
				for(int i =0,size=mPayStylesList.size();i<size;i++){
					if(mPayStylesList.get(i).id == PayOrder.PAYMENT_WALLET){//钱包支付
						mPayStylesList.remove(i);
						break;
					}
				}
				initPayStyleView();
			}else{
				if(mPayStylesList != null){
					mPayStylesList.clear();
				}
				initPayStyleView();
				Util.showToast(RechargeActivity.this, getString(R.string.pay_get_may_method_fail), Toast.LENGTH_LONG);
			}
			break;
		case AccountEventCode.EVENT_GET_MOVIE_PALY_ORDER:
			Util.dismissDialog(mProgressDialog);				
			log.debug("EVENT_GET_MOVIE_PALY_ORDER msg.arg1="+msg.arg1);
			PayOrder mPayOrder = null;
			if(msg.arg1==0 || msg.arg1 == 200){
				mPayOrder=(PayOrder) msg.obj;
				log.debug("mPayOrder="+mPayOrder);
				if(mPayOrder.paymentId==PayOrder.PAYMENT_ALIPAY){//支付宝钱包
					startAliPay(mPayOrder);
				}else if(mPayOrder.paymentId==PayOrder.PAYMENT_UNIONPAY){//银联支付
					/**
			    	 * 参数说明： 参数说明：
			    	 * activity —— 用于启动支付控件的活对象 用于启动支付控件的活对象
			    	 * spId —— 保留使用，这里输入null
			    	 * sysProvider sysProvider —— 保留使用，这里输入 保留使用，这里输入 保留使用，这里输入 保留使用，这里输入 null null
			    	 * orderInfo —— 订单信息为交易流水号，即TN
			    	 * mode —— 银联后台环境标识，“ 00 ”将在银联正式环境发起交易 ”
			    	 * 返回值：
			    	 * UPPayAssist UPPayAssist Ex.PLUGIN_VALID —— 该终端已经安装控件 ，并启动该终端已经安装控件并启动
			    	 * UPPayAssist UPPayAssistEx.PLUGIN_NOT_FOUND —— 手机终端尚未安装支付控件，需要先 手机终端尚未安装支付控件，
			    	 */
			        UPPayAssistEx.startPayByJAR(RechargeActivity.this,PayActivity.class, null, null,mPayOrder.tn,"00");
				}else if(mPayOrder.paymentId==PayOrder.PAYMENT_SKYSMS){
//					PayApplication mPayApplication = new PayApplication();
//					mPayApplication.applicationOnCreat(getApplicationContext());
//					
//					SkyPayServer mSkyPayServer = SkyPayServer.getInstance();
//					SkyPayHandle mPayHandler = new SkyPayHandle();
//					int initRet = mSkyPayServer.init(mPayHandler);
//					if (SkyPayServer.PAY_RETURN_SUCCESS == initRet) {
//						//斯凯付费实例初始化成功！
//					} else {
//						//服务正处于付费状态或	传入参数为空
//					}
//					//开始计费
//					int payRet = mSkyPayServer.startActivityAndPay(RechargeActivity.this, mPayOrder.skymobipayString);
//
//			    	if (SkyPayServer.PAY_RETURN_SUCCESS == payRet) {
////						Toast.makeText(PayMovieActivity.this, "接口斯凯付费调用成功", Toast.LENGTH_LONG).show();//初始化成功
//			    	} else {
//			    		//未初始化 \ 传入参数有误 \ 服务正处于付费状态
//						Toast.makeText(RechargeActivity.this, getString(R.string.pay_fail) + payRet, Toast.LENGTH_LONG).show();
//			    	}
				}else if(mPayOrder.paymentId==PayOrder.PAYMENT_PAYPAL){//paypal支付
					Intent intent = new Intent(RechargeActivity.this, PayPalActivity.class);
					intent.putExtra(PayOrder.class.getSimpleName(), mPayOrder);
					intent.putExtra("isRecharge", true);
					startActivity(intent);
					finish();
				}else if(mPayOrder.paymentId == PayOrder.PAYMENT_WEIXIN){
					//TODO 微信支付
				}
			}else{
				Util.showToast(RechargeActivity.this, getString(R.string.pay_get_order_fail)+msg.arg1, Toast.LENGTH_LONG);
			}
			ReportActionSender.getInstance().pay(msg.arg1, mPayOrder != null ? mPayOrder.paymentId : 0, mPayOrder != null ? mPayOrder.currencyId : 0, mPayOrder != null ? mPayOrder.amount:0, mPayOrder != null ? mPayOrder.productId:0, mPayOrder != null ? mPayOrder.orderId:0, 1, mPayOrder != null ? mPayOrder.channelId:0);
			break;
		case RQF_PAY:
			boolean success = false;
			String showPayMessage = "";
			PayResult payResult = new PayResult((String) msg.obj);
			// 支付宝返回此次支付结果及加签，建议对支付宝签名信息拿签约时支付宝提供的公钥做验签
			String resultInfo = payResult.getResult();
			String resultStatus = payResult.getResultStatus();
			log.debug("resultStatus="+resultStatus);
			if(!TextUtils.equals(resultStatus, "6001")){//非用户取消的支付错误
				if (TextUtils.equals(resultStatus, "9000")) {// 判断resultStatus 为“9000”则代表支付成功，具体状态码代表含义可参考接口文档
					success = true;
					showPayMessage = getString(R.string.recharge_result_dialog_message);
				} else{
					// 判断resultStatus 为非“9000”则代表可能支付失败
					// “8000”代表支付结果因为支付渠道原因或者系统原因还在等待支付结果确认，最终交易是否成功以服务端异步通知为准（小概率状态）
					if (TextUtils.equals(resultStatus, "8000")) {
						showPayMessage = getString(R.string.pay_waiting_dialog_message_and_rtncode)+resultStatus;
					} else {
						// 其他值就可以判断为支付失败，包括用户主动取消支付，或者系统返回的错误
						showPayMessage = getString(R.string.pay_fail_dialog_message_and_rtncode)+resultStatus;
					}
				}
				showPayResultDialog(success,getString(R.string.recharge_result_dialog_title), showPayMessage);
			}
			break;
		default:
			break;
		}
	}

	
	private StaticHandler mHandler;
	
	private void setRechargeQuoda() {
		// TODO 填充充值金额界面
		layout_money1.setVisibility(mQuotas.size() > 0 ? View.VISIBLE : View.GONE);
		layout_money2.setVisibility(mQuotas.size() > 1 ? View.VISIBLE : View.GONE);
		layout_money3.setVisibility(mQuotas.size() > 2 ? View.VISIBLE : View.GONE);
		layout_money4.setVisibility(mQuotas.size() > 3 ? View.VISIBLE : View.GONE);
		
		final View.OnClickListener mClickListener = new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				switch (v.getId()) {
				case R.id.layout_money1:
					currentSelectQuota = 0;
					refreshRechargeQuoda();
					break;
				case R.id.layout_money2:
					currentSelectQuota = 1;
					refreshRechargeQuoda();
					break;
				case R.id.layout_money3:
					currentSelectQuota = 2;
					refreshRechargeQuoda();
					break;
				case R.id.layout_money4:
					currentSelectQuota = 3;
					refreshRechargeQuoda();
					break;
				default:
					break;
				}
			}
		};
		layout_money1.setOnClickListener(mClickListener);
		layout_money2.setOnClickListener(mClickListener);
		layout_money3.setOnClickListener(mClickListener);
		layout_money4.setOnClickListener(mClickListener);
		
		try {
			textView_money1.setText(mQuotas.get(0).toString());
			textView_money2.setText(mQuotas.get(1).toString());
			textView_money3.setText(mQuotas.get(2).toString());
			textView_money4.setText(mQuotas.get(3).toString());
		} catch (ArrayIndexOutOfBoundsException e) {
			e.printStackTrace();
		} catch (IndexOutOfBoundsException e) {
			e.printStackTrace();
		}
		refreshRechargeQuoda();
	}
	
	private void refreshRechargeQuoda(){
		layout_money1.setBackgroundResource(currentSelectQuota == 0 ? R.drawable.list_button_focus:R.color.transparent);
		layout_money2.setBackgroundResource(currentSelectQuota == 1 ? R.drawable.list_button_focus:R.color.transparent);
		layout_money3.setBackgroundResource(currentSelectQuota == 2 ? R.drawable.list_button_focus:R.color.transparent);
		layout_money4.setBackgroundResource(currentSelectQuota == 3 ? R.drawable.list_button_focus:R.color.transparent);
		
		int colorSelect = getResources().getColor(R.color.pay_money_select);
		int colorCommon = getResources().getColor(R.color.title_color);
		textView_money1.setTextColor(currentSelectQuota == 0 ? colorSelect:colorCommon);
		textView_money2.setTextColor(currentSelectQuota == 1 ? colorSelect:colorCommon);
		textView_money3.setTextColor(currentSelectQuota == 2 ? colorSelect:colorCommon);
		textView_money4.setTextColor(currentSelectQuota == 3 ? colorSelect:colorCommon);

		Util.showDialog(mProgressDialog, getString(R.string.pay_get_pay_method_ing));
		mCurrentAmount = mQuotas.get(currentSelectQuota).amount;
		String amount = Integer.toString(mQuotas.get(currentSelectQuota).amount);
		
		//重新請求獲取支付方式
		mRequestHandle = AccountServiceManager.getInstance().reqMoviePayStyle(amount,mQuotas.get(currentSelectQuota).currencyId, mHandler);
	}

	private void startAliPay(final PayOrder mPayOrder){
		new Thread(new Runnable() {
			
			@Override
			public void run() {
				log.debug("alipayString="+mPayOrder.alipayString);
				PayTask alipay = new PayTask(RechargeActivity.this);// 构造PayTask 对象
				String result = alipay.pay(mPayOrder.alipayString);// 调用支付接口，获取支付结果
				log.debug("result="+result+",alipayString="+mPayOrder.alipayString);
				
				if(!TextUtils.isEmpty(result)){
					Message ali_msg = new Message();
					ali_msg.what = RQF_PAY;
					ali_msg.obj = result;
					mHandler.sendMessage(ali_msg);
				}else{
					//TODO
				}
			}
		}).start();
		
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_recharge_pay);
		getActionBar().setDisplayHomeAsUpEnabled(true);
		mHandler = new StaticHandler(this);
		mUserAccount = (UserAccount) getIntent().getSerializableExtra(UserAccount.class.getSimpleName());
		if(mUserAccount == null){
			finish();
			return;
		}
		
		pay_stype_layout = (LinearLayout) findViewById(R.id.pay_stype_layout);
		submit_order = (Button) findViewById(R.id.submit_order);

		layout_money1 = (LinearLayout) findViewById(R.id.layout_money1);
		layout_money2 = (LinearLayout) findViewById(R.id.layout_money2);
		layout_money3 = (LinearLayout) findViewById(R.id.layout_money3);
		layout_money4 = (LinearLayout) findViewById(R.id.layout_money4);
		
		textView_money1 = (TextView) findViewById(R.id.textView_money1);
		textView_money2 = (TextView) findViewById(R.id.textView_money2);
		textView_money3 = (TextView) findViewById(R.id.textView_money3);
		textView_money4 = (TextView) findViewById(R.id.textView_money4);
		
		
		mProgressDialog = new ProgressDialog(this,R.style.ProgressDialogDark);
		mProgressDialog.setOnCancelListener(new OnCancelListener() {
			
			@Override
			public void onCancel(DialogInterface dialog) {
				if(mRequestHandle!=null && !mRequestHandle.isCancelled()){
					mRequestHandle.cancel(false);
				}
			}
		});
		
		submit_order.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				log.debug("setOnClickListener check_positon="+check_positon);
				if(mPayStylesList!=null && mPayStylesList.size() > check_positon){
					PayStyle mPayStyle=mPayStylesList.get(check_positon);
					log.debug("mPayStyle="+mPayStyle);
					Util.showDialog(mProgressDialog, getString(R.string.pay_create_order_ing));
					AccountServiceManager.getInstance().rechargeOrder(RechargeActivity.this, mPayStyle.id, mUserAccount.accountType, mCurrentAmount, mUserAccount.currencyId, mHandler);
                    MobclickAgent.onEvent(RechargeActivity.this, Constant.PurchaseClick);
				}
				
			}
		});
		Util.showDialog(mProgressDialog, getString(R.string.loading));
		AccountServiceManager.getInstance().getRechargeQuota(mUserAccount.currencyId, mHandler);
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
    
	private void initPayStyleView(){
		LayoutInflater mLayoutInflater=getLayoutInflater();
		payStyleViewList=new ArrayList<View>();
		pay_stype_layout.removeAllViews();
		if(mPayStylesList == null){
			return;
		}
		for (Iterator<PayStyle> iter = mPayStylesList.iterator(); iter.hasNext();) {
			PayStyle mPayStyle = iter.next();
			if(mPayStyle.id == PayOrder.PAYMENT_UNIONPAY || mPayStyle.id == PayOrder.PAYMENT_PAYPAL || mPayStyle.id == PayOrder.PAYMENT_ALIPAY){
				
			}else{
				iter.remove();
			}
		}
		for(int i=0,size=mPayStylesList.size();i<size;i++){
			View convertView=mLayoutInflater.inflate(R.layout.pay_style_item, null);
			ImageView checkBox_select=(ImageView) convertView.findViewById(R.id.checkBox_select);
			ImageView imageView_paystyle_bg=(ImageView) convertView.findViewById(R.id.imageView_paystyle_bg);
			TextView textView_payName = (TextView) convertView.findViewById(R.id.textView_payName);
			
			PayStyle mPayStyle=mPayStylesList.get(i);
			if(check_positon==i){
				checkBox_select.setImageResource(R.drawable.radio_down);
			}else{
				checkBox_select.setImageResource(R.drawable.radio_normal);
			}
			textView_payName.setText(mPayStyle.payment);
			switch (mPayStyle.id) {
			case PayOrder.PAYMENT_ALIPAY://阿里
				imageView_paystyle_bg.setImageResource(R.drawable.ic_pay_treasure);
				break;
//			case PayOrder.PAYMENT_SKYSMS://短信
//				imageView_paystyle_bg.setImageResource(R.drawable.ic_mobile_payment);
//				break;
			case PayOrder.PAYMENT_UNIONPAY://银联
				imageView_paystyle_bg.setImageResource(R.drawable.ic_upcash);
				break;
			case PayOrder.PAYMENT_PAYPAL:
				imageView_paystyle_bg.setImageResource(R.drawable.ic_paypal_treasure);
				break;
//			case PayOrder.PAYMENT_WEIXIN:
//				imageView_paystyle_bg.setImageResource(R.drawable.ic_weixin_payment);
//				break;
			default:
				continue;
			}
			convertView.setTag(i);
			convertView.setOnClickListener(new View.OnClickListener() {
				
				@Override
				public void onClick(View v) {
					check_positon=(Integer) v.getTag();
					log.debug("check_positon="+check_positon);
					notifyOnSelect();
				}
			});
			payStyleViewList.add(convertView);
			pay_stype_layout.addView(convertView);
			
			if(i!=size-1){
				LinearLayout mLine=new LinearLayout(RechargeActivity.this);
				mLine.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.WRAP_CONTENT));
				mLine.setBackgroundResource(R.drawable.line_black);;
				pay_stype_layout.addView(mLine);
			}
		}
	}
	
	private void notifyOnSelect(){
		if(payStyleViewList!=null){
			for(int i=0,size=payStyleViewList.size();i<size;i++){
				ImageView checkBox_select=(ImageView) payStyleViewList.get(i).findViewById(R.id.checkBox_select);
				if(check_positon==i){
					checkBox_select.setImageResource(R.drawable.radio_down);
				}else{
					checkBox_select.setImageResource(R.drawable.radio_normal);
				}
			}
		}
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		/*************************************************
		 * 
		 * 步骤3：处理银联手机支付控件返回的支付结果
		 * 
		 ************************************************/
		if (data == null) {
			return;
		}
		
		/*
		 * 支付控件返回字符串:success、fail、cancel 分别代表支付成功，支付失败，支付取消
		 */
		String str = data.getExtras().getString("pay_result");
		if (!str.equalsIgnoreCase("cancel")) {
			boolean success = false;
			String msg = "";
			if (str.equalsIgnoreCase("success")) {
				success = true;
				msg = getString(R.string.recharge_result_dialog_message);
			} else if (str.equalsIgnoreCase("fail")) {
				msg = getString(R.string.pay_fail_dialog_message);
			}
			showPayResultDialog(success,getString(R.string.recharge_result_dialog_title), msg);
		}
	}
	
	private void showPayResultDialog(final boolean success,String title,String message){
		if(success){//充值成功后请求刷新用户账户
			WalletManager.getInstance().reqGetAccount();
			WalletManager.sendWalletChangeBroadcast(this);
		}
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
					finish();
				}
			}
		});
		try {
			mAlertDialog.show();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

    @Override
    protected void onResume() {
        super.onResume();
        MobclickAgent.onResume(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        MobclickAgent.onPause(this);
    }
	
//	private class SkyPayHandle extends Handler {
//		
//		private XL_Log log = new XL_Log(SkyPayHandle.class);
//
//		public static final String STRING_MSG_CODE = "msg_code";
//		public static final String STRING_ERROR_CODE = "error_code";
//		public static final String STRING_PAY_STATUS = "pay_status";
//		public static final String STRING_PAY_PRICE = "pay_price";
//		public static final String STRING_CHARGE_STATUS = "3rdpay_status";
//
//		@Override
//		public void handleMessage(Message msg) {
//			super.handleMessage(msg);
//			if (msg.what == SkyPayServer.MSG_WHAT_TO_APP) {
//				String retInfo = (String) msg.obj;
//				log.debug("retInfo="+retInfo);
//				Map<String, String> map = new HashMap<String, String>();
//
//				String[] keyValues = retInfo.split("&|=");
//				for (int i = 0; i < keyValues.length; i = i + 2) {
//					map.put(keyValues[i], keyValues[i + 1]);
//				}
//
//				int msgCode = Integer.parseInt(map.get(STRING_MSG_CODE));
//				log.debug("msgCode="+msgCode+",status="+map.get(STRING_PAY_STATUS));
//				// 解析付费状态和已付费价格
//				// 使用其中一种方式请删掉另外一种
//				if (msgCode == 100) {
//					// 短信付费返回
//					if (map.get(STRING_PAY_STATUS) != null) {
//						int payStatus = Integer.parseInt(map.get(STRING_PAY_STATUS));
//						int payPrice = Integer.parseInt(map.get(STRING_PAY_PRICE));
//						int errcrCode = 0;
//						if (map.get(STRING_ERROR_CODE) != null) {
//							errcrCode = Integer.parseInt(map.get(STRING_ERROR_CODE));
//						}
//						log.debug("payStatus="+payStatus+",payPrice="+payPrice+",errcrCode="+errcrCode);
//						switch (payStatus) {
//						case 102:
//							String message = getString(R.string.recharge_result_dialog_message);
//							showPayResultDialog(true,getString(R.string.recharge_result_dialog_title), message);
//							break;
//						case 101:
//							showPayResultDialog(false,getString(R.string.recharge_result_dialog_title), getString(R.string.pay_fail_dialog_message_and_rtncode) + errcrCode);
//							break;
//						}
//					}
//
//				} else {
//					// 解析错误码
//					int errcrCode = Integer.parseInt(map.get(STRING_ERROR_CODE));
//					if(errcrCode != 503 && errcrCode != 506){
//						showPayResultDialog(false,getString(R.string.recharge_result_dialog_title), getString(R.string.pay_fail_dialog_message_and_rtncode) + errcrCode);
//					}
//				}
//			}
//		}
//	}
}
