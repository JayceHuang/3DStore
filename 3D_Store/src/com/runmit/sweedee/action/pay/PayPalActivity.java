package com.runmit.sweedee.action.pay;

import com.runmit.sweedee.R;
import com.runmit.sweedee.manager.AccountServiceManager;
import com.runmit.sweedee.manager.WalletManager;
import com.runmit.sweedee.model.PayOrder;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.view.MenuItem;
import android.view.View;
import android.webkit.DownloadListener;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.LinearLayout;

public class PayPalActivity extends Activity {

	private XL_Log log = new XL_Log(PayPalActivity.class);

	private PayOrder mPayOrder;

	private WebView mainWebView;

	private LinearLayout loading_view;
	
	private boolean isRecharge = false;

	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			log.debug("msg.arg1=" + msg.arg1);
			boolean success = false;
			String message;
			if (msg.arg1 == 0) {
				success = true;
				message = getString(R.string.pay_success_dialog_message);
			} else if (msg.arg1 == 3019) {//order is paying,TODO show paying toast
				success = true;
				message = getString(R.string.pay_success_dialog_message);
			} else {
				message = getString(R.string.pay_fail_dialog_message) + msg.arg1;
			}
			showPayResultDialog(success, getString(R.string.pay_result_dialog_title), message);
		};
	};

	private void showPayResultDialog(final boolean success, String title, String message) {
		if (success) {
			if(isRecharge){
				WalletManager.getInstance().reqGetAccount();
				WalletManager.sendWalletChangeBroadcast(this);
			}else{
				AccountServiceManager.getInstance().reportPaySuccess(getApplicationContext(), mPayOrder.orderId, mHandler);
			}
		}
		AlertDialog.Builder builder = new AlertDialog.Builder(this, R.style.AlertDialog);
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
				finish();
			}
		});
		mAlertDialog.show();
	}

	public static final String BACKURL_USER_GIVEUP = "http://www.runmit.com/backupurl/user/giveup";

	public static final String PRODUCTURL_CALLBACL = "http://www.runmit.com/producturl/user/callback";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_paypal);
		getActionBar().setDisplayHomeAsUpEnabled(true);
		getActionBar().setDisplayShowHomeEnabled(false);

		mPayOrder = (PayOrder) getIntent().getSerializableExtra(PayOrder.class.getSimpleName());
		isRecharge = getIntent().getBooleanExtra("isRecharge", false);
		
		loading_view = (LinearLayout) findViewById(R.id.loading_view);

		mainWebView = (WebView) findViewById(R.id.content_webView);
		WebSettings ws;
		ws = mainWebView.getSettings();
		ws.setJavaScriptEnabled(true);
		ws.setSupportZoom(true);
		ws.setCacheMode(WebSettings.LOAD_NO_CACHE);
		ws.setUseWideViewPort(true);
		ws.setBuiltInZoomControls(true);
		ws.setLoadWithOverviewMode(true);
		ws.setSaveFormData(false);
		ws.setJavaScriptCanOpenWindowsAutomatically(true);
		ws.setCacheMode(WebSettings.LOAD_NO_CACHE);

		mainWebView.setWebChromeClient(new WebChromeClient() {

			@Override
			public void onProgressChanged(WebView view, int newProgress) {
				super.onProgressChanged(view, newProgress);
				log.debug("newProgress=" + newProgress);
				if (newProgress > 50) {
					loading_view.setVisibility(View.GONE);
				}
			}
		});

		mainWebView.setDownloadListener(new DownloadListener() {

			@Override
			public void onDownloadStart(String url, String userAgent, String contentDisposition, String mimetype, long contentLength) {
				Uri uri = Uri.parse(url);
				Intent intent = new Intent(Intent.ACTION_VIEW, uri);
				startActivity(intent);
			}
		});

		mainWebView.setWebViewClient(new WebViewClient() {

			@Override
			public boolean shouldOverrideUrlLoading(WebView view, String url) {
				log.debug("shouldOverrideUrlLoading=" + url + ",token=" + mPayOrder.paypalToken);
				if (url.contains(BACKURL_USER_GIVEUP)) {
					finish();
				} else if (url.contains(PRODUCTURL_CALLBACL)) {
					AccountServiceManager.getInstance().paypalCheckout(mPayOrder.paypalToken, mHandler);
				} else {
					view.loadUrl(url);
				}
				return true;
			}

			@Override
			public void onPageFinished(WebView view, String url) {
				super.onPageFinished(view, url);
			}
		});

		mainWebView.loadUrl(mPayOrder.paypalString);
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
}
