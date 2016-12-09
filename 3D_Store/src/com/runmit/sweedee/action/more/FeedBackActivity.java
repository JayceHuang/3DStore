package com.runmit.sweedee.action.more;

import java.io.UnsupportedEncodingException;
import java.util.Locale;

import org.apache.http.Header;
import org.apache.http.HttpEntity;
import org.apache.http.entity.StringEntity;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.telephony.TelephonyManager;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;

import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.TextHttpResponseHandler;
import com.runmit.sweedee.R;
import com.runmit.sweedee.report.sdk.DeviceUuidFactory;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.update.Config;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.Util;

public class FeedBackActivity extends NewBaseRotateActivity {
	
	private static final String FEEDBACK_POST_URL = "http://clotho.d3dstore.com/comment/newcomment";// "http://192.168.20.114:8080/clotho/comment/newcomment";
	private static final int FEED_BACK_RESPONSE_ERROR = 10000;
	private static final int FEED_BACK_RESPONSE_SUCC = 10001;
	
	private EditText editText1;
	private EditText etContact;
	private ProgressDialog mDialog;
	private Button submit;
	private InputMethodManager imInputMethodManager;

	private String UDID;
	private int product_id = Constant.PRODUCT_ID;

	private String content = "";
	private String contact = "";
	private Context mContext;
	
	private Handler mHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			if (null == mContext) {
				return;
			}
			if (msg.what == FEED_BACK_RESPONSE_SUCC) {
				Util.showToast(mContext, mContext.getString(R.string.submit_success), 2000);
				editText1.setText("");
				etContact.setText("");
				Util.dismissDialog(mDialog);
				etContact.requestFocus();
				imInputMethodManager.hideSoftInputFromWindow(etContact.getWindowToken(), InputMethodManager.RESULT_UNCHANGED_SHOWN);
				finish();
			} else if (msg.what == FEED_BACK_RESPONSE_ERROR) {
				Util.dismissDialog(mDialog);
				Util.showToast(mContext, mContext.getString(R.string.submit_failed), 2000);
			}
		}

	};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_setting_feedback);
		mContext = this;
		getActionBar().setDisplayHomeAsUpEnabled(true);
		getActionBar().setDisplayShowHomeEnabled(false);

		editText1 = (EditText) findViewById(R.id.editText1);
		etContact = (EditText) findViewById(R.id.etContact);
		submit = (Button) findViewById(R.id.btnSubmit);
		submit.setOnClickListener(onclickListener);
		mDialog = new ProgressDialog(this, R.style.ProgressDialogDark);
		UDID = new DeviceUuidFactory(mContext).getDeviceUuid();
		imInputMethodManager = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
	}
	
	@Override
	protected void onAnimationFinishIn(){
		editText1.requestFocus();
		imInputMethodManager.showSoftInput(editText1, InputMethodManager.SHOW_FORCED); 
    }
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case android.R.id.home:
			editText1.requestFocus();
			imInputMethodManager.hideSoftInputFromWindow(editText1.getWindowToken(), 0);
			finish();
			break;
		default:
			break;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	protected void onPause() {
		super.onPause();
		log.debug("onPause");
		editText1.requestFocus();
		imInputMethodManager.hideSoftInputFromWindow(editText1.getWindowToken(), 0);
	}
		
	@Override
	protected void onDestroy() {
		super.onDestroy();
		mContext = null;
	}

	private View.OnClickListener onclickListener = new View.OnClickListener() {

		@Override
		public void onClick(View v) {
			if (v.getId() == R.id.btnSubmit) {
				if (null == editText1.getText() || "".equals(editText1.getText().toString().trim())) {
					Util.showToast(FeedBackActivity.this, mContext.getString(R.string.submit_empty_tip), 1000);
					return;
				} else if (editText1.getText().length() > 300) {
					Util.showToast(FeedBackActivity.this, mContext.getString(R.string.submit_max_check_tip), 1000);
					return;
				}

				if (Util.isNetworkAvailable(FeedBackActivity.this)) {
					content = editText1.getText().toString().trim();
					contact = etContact.getText().toString().trim();
					postSuggestContent();
					imInputMethodManager.hideSoftInputFromWindow(editText1.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
				} else {
					Util.showToast(FeedBackActivity.this, mContext.getString(R.string.has_no_using_network), 1000);
				}
			} else {
				imInputMethodManager.hideSoftInputFromWindow(editText1.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
				finish();
			}
		}
	};

	private void postSuggestContent() {
		Util.showDialog(mDialog, mContext.getString(R.string.feedback_submitting));
		JSONObject params = new JSONObject();
		try {

			HttpEntity entity = null;
			try {
				genParams(params);
				entity = new StringEntity(params.toString(), "UTF-8");
			} catch (UnsupportedEncodingException e) {
				e.printStackTrace();
			}
			new AsyncHttpClient().post(FeedBackActivity.this, FEEDBACK_POST_URL, entity, "application/json", new TextHttpResponseHandler() {
				@Override
				public void onSuccess(int statusCode, Header[] headers, String responseString) {
					JSONObject jsonObject;
					try {
						jsonObject = new JSONObject(responseString);
						int rtn = jsonObject.getInt("rtn");
						if (rtn != 0) {
							mHandler.sendEmptyMessage(FEED_BACK_RESPONSE_ERROR);
						}else{
							mHandler.sendEmptyMessage(FEED_BACK_RESPONSE_SUCC);
						}
					} catch (JSONException e) {
						e.printStackTrace();
					}
				}

				@Override
				public void onFailure(int statusCode, Header[] headers, String responseString, Throwable throwable) {
					mHandler.sendEmptyMessage(FEED_BACK_RESPONSE_ERROR);
				}
			});
		} catch (JSONException e1) {
			e1.printStackTrace();
		}
	}

	private void genParams(JSONObject params) throws JSONException {
		params.put("hwid", Util.getPeerId()); // SuperD硬件设备ID，由芯片以系统api的方式提供给客户端，对于没有连接SuperD的设备不用上报
		params.put("udid", UDID); // 终端设备的唯一标识，用来唯一标识一个终端设备，是用户追踪的依据。md5（mac+imei）
		params.put("wifimac", Util.getLocalWifiMacAddress(mContext)); // 设备WiFI
																		// mac地址
		params.put("wirelesssmac", ""); // 设备无线mac地址
		params.put("wiremac", ""); // 设备有线mac地址
		params.put("os", "1"); // 系统类型：1-Android；2-Ios
		params.put("osver", Build.VERSION.RELEASE); // 系统版本
		params.put("device", "1"); // 设备类型：1-phone；2-pad；3-phone on box
		params.put("area", Locale.getDefault().getCountry()); // 设备系统中设定的地区信息，取值范围？ISO标准的两字母的国家代码,如CN、US
		params.put("language", Locale.getDefault().getLanguage()); // 设备系统中设定的语言信息，取值范围？ISO标准的两字母语言代码，如en、zh
		String imei = ((TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE)).getDeviceId();
		params.put("imei", imei != null ? imei : "unknow"); // Android设备上报imei码
		params.put("idfv", ""); // IOS上报该字段，IOS中的供应商标识，Android上报为空字符串
		params.put("appkey", mContext.getPackageName()); // 应用唯一标识，IOS为bundleID；Android为包名
		params.put("appver", Config.getVerName(mContext)); // app version
															// 应用版本，例如：1.1.1.2
		params.put("uid", "" + UserManager.getInstance().getUserId()); // 登录用户唯一标识，如果是匿名用户不用上传
		params.put("devicebrand", Build.BRAND);
		params.put("devicedevice", Build.DEVICE); // Android设备DeviceInfo中的DEVICE
		params.put("devicemodel", Build.MODEL);
		params.put("devicehardware", Build.HARDWARE);
		params.put("deviceid", Build.ID);
		params.put("deviceserial", Build.SERIAL);
		params.put("ro", Constant.SCREEN_WIDTH + "_" + Constant.SCREEN_HEIGHT);// 设备分辨率，例如：1024_768
		params.put("channel", "" + product_id); // 分发渠道标识，由客户端维护和提供完整的信息
		params.put("dts", "" + System.currentTimeMillis()); // 设备本地时间（毫秒）

		// 联系方式
		params.put("contact", contact);
		// 内容
		params.put("content", content);
	}
}
