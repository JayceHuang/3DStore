package com.runmit.sweedee.manager;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.action.login.LoginActivity;
import com.runmit.sweedee.action.more.SettingItemActivity;
import com.runmit.sweedee.action.pay.PayStyleActivity;
import com.runmit.sweedee.constant.ServiceArg;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadCreater;
import com.runmit.sweedee.download.DownloadInfo;
import com.runmit.sweedee.download.DownloadManager;
import com.runmit.sweedee.download.DownloadSQLiteHelper;
import com.runmit.sweedee.download.VideoDownloadInfo;
import com.runmit.sweedee.manager.data.EventCode.AccountEventCode;
import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;
import com.runmit.sweedee.model.CmsVedioDetailInfo.Vedio;
import com.runmit.sweedee.model.PayOrder;
import com.runmit.sweedee.model.PayStyle;
import com.runmit.sweedee.model.PayStyleList;
import com.runmit.sweedee.model.UserAccount;
import com.runmit.sweedee.model.VOPrice;
import com.runmit.sweedee.model.VOPrice.PriceInfo;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.player.PlayerActivity;
import com.runmit.sweedee.player.PlayerConstant;
import com.runmit.sweedee.player.RealPlayUrlInfo;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.EnvironmentTool;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.TDString;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * 管理是否直接进入播放器的界面
 *
 * @author admin
 */
@SuppressLint("DefaultLocale")
public class PlayerEntranceMgr {

	private Context mContext;

	private XL_Log log = new XL_Log(PlayerEntranceMgr.class);

	private String mFileName;

	private int albumId;

	private int videoId;

	private ArrayList<SubTitleInfo> subtitles;

	private ProgressDialog mProgressDialog;

	private int loadUrlType;

	private String poster;

	private VOPrice mVOPrice;

	private boolean needLogin = true;
	
	private int mode;
	
	private static final String DOWNLOAD_SUFFIX = ".rmf";
	
//	exports.moduleName = {
//		    CMS:'CMS',
//		    VRS:'VRS',
//		    PAY:'PAY',
//		    UC:'UC',
//		    GSLB:'GSLB'
//		};
	public static final String GSLB_CKECK_ERROR_MODULE_PAY = "PAY";
	public static final String GSLB_CKECK_ERROR_MODULE_UC = "UC";
	
	private int mTaskId;	// 当前过期task的id
	private boolean isRestartDownload = false; // 是否超时续传的类型
	
	public PlayerEntranceMgr(Context context) {
		mContext = context;
	}

	private Handler mHandler = new Handler() {
		public void handleMessage(Message msg) {
			log.debug("handleMessage msg.what=" + msg.what + ",arg1=" + msg.arg1);
			switch (msg.what) {
			case CmsEventCode.EVENT_GET_VEDIO_GSLB_URL:
			case CmsEventCode.EVENT_GET_VEDIO_CDN_URL: {
				if (msg.arg1 == 0) {//返回为0，正常点播流程
					Util.dismissDialog(mProgressDialog);
					RealPlayUrlInfo rInfo = (RealPlayUrlInfo) msg.obj;
					log.debug("rInfo=" + rInfo + ",loadUrlType=" + loadUrlType);
					
					if (loadUrlType == ServiceArg.URL_TYPE_PLAY) {
						Intent intent = new Intent(mContext, PlayerActivity.class);
						intent.putExtra(PlayerConstant.INTENT_PLAY_ALBUMID, albumId);
						intent.putExtra(PlayerConstant.INTENT_PLAY_VIDEOID, videoId);
						intent.putExtra(PlayerConstant.INTENT_PLAY_MODE, PlayerConstant.ONLINE_PLAY_MODE);
						intent.putExtra(PlayerConstant.PLAY_URL_LISTINFO, rInfo);
						intent.putExtra(PlayerConstant.INTENT_PLAY_FILE_NAME, mFileName);
						intent.putExtra(PlayerConstant.INTENT_PLAY_SUBTITLE, subtitles);
						intent.putExtra(PlayerConstant.INTENT_PLAY_POSTER, poster);
						intent.putExtra(PlayerConstant.INTENT_NEED_LOGIN, needLogin);
						intent.putExtra(PlayerConstant.INTENT_VIDEO_SOURCE_MODE, mode);
						mContext.startActivity(intent);
					} else {
						int downloadResolution = SharePrefence.getInstance(mContext).getInt(SettingItemActivity.DOWNLOAD_SOLUTION, Constant.NORMAL_VOD_URL);
						String mDownloadUrl = null;
						String fileName = null;
						// 默认下载为1080p 且 存在1080p的链接
						if (downloadResolution == Constant.HIGH_VOD_URL && rInfo.has1080Redition()) {
							mDownloadUrl = rInfo.getP1080List().get(0);
							fileName = mFileName + "_1080P"+DOWNLOAD_SUFFIX;
						}
						// 默认设置720p, 或是是不存在1080p链接
						else if (rInfo.has720Redition()) {
							mDownloadUrl = rInfo.getP720List().get(0);
							fileName = mFileName + "_720P"+DOWNLOAD_SUFFIX;
						}
						// 默认设置720p, 但是不存在720p链接
						else {
							mDownloadUrl = rInfo.getP1080List().get(0);
							fileName = mFileName + "_1080P"+DOWNLOAD_SUFFIX;
						}
						fileName = fileName.replace(" ", "_");
						String cookie = "";
						log.debug("loadUrlType=" + loadUrlType + " ,isRestartDownload="+isRestartDownload + " ,mTaskId="+mTaskId + " ,Url="+mDownloadUrl);
						if(isRestartDownload){
							DownloadInfo mDownloadInfo = DownloadManager.getInstance().getDownloadInfoById(mTaskId);
							mDownloadInfo.downloadUrl = mDownloadUrl;
							DownloadManager.getInstance().updateInfo(mDownloadInfo);
							return;
						}
						createRealDownload(downloadResolution, mDownloadUrl, fileName, cookie);
					}
				} else if (msg.arg1 == 607) {//服务器返回未购买
					JSONObject jobj;
					int status = 0;
					String module = null;
					Util.dismissDialog(mProgressDialog);
					try {
						jobj = new JSONObject((String)msg.obj);
						module = jobj.getString("module");
	        			if(module.equals(GSLB_CKECK_ERROR_MODULE_PAY)){
	        				JSONObject data = jobj.getJSONObject("data");
	            			status = data.getInt("status");
	        			}
					} catch (JSONException e) {
						e.printStackTrace();
					}
					log.debug("module="+module + ",status="+status +",mVOPrice="+mVOPrice);
					if(GSLB_CKECK_ERROR_MODULE_UC.equals(module)){
						Util.showToast(mContext, mContext.getString(R.string.login_state_unavailable), Toast.LENGTH_LONG);
						UserManager.getInstance().userLogout(false);
						gotoLogin();
					}else if (status == 4) {//购买状态为正在购买，则提示用户
						Util.dismissDialog(mProgressDialog);
						Util.showToast(mContext, mContext.getString(R.string.pay_waiting), Toast.LENGTH_LONG);
					} else {// 服务器返回未支付，请求获取可用支付方式
						if (mVOPrice != null) {//判断价格是否为空
							if(!checkPriceIsZero()){
								PriceInfo mRealPriceInfo = mVOPrice.getDefaultRealPriceInfo();
								log.debug("mRealPriceInfo=" + mRealPriceInfo);
								if (mRealPriceInfo != null) {//真实价格是否为空
									AccountServiceManager.getInstance().reqMoviePayStyle(mRealPriceInfo.price, mRealPriceInfo.currencyId, mHandler);
								} else {
									final PriceInfo mVirtualPriceInfo = mVOPrice.getVirtualPriceInfo();
									log.debug("mVirtualPriceInfo=" + mVirtualPriceInfo);
									if(mVirtualPriceInfo != null){//如果虚拟价格不为空
										showPayDialog();//则走支付流程
									}else{//此处是虚拟价格和真实价格都为空，但是mVOPrice不为空，应该不会走到此处。简单提示
										Util.showToast(mContext, mContext.getString(R.string.pay_price_null), Toast.LENGTH_LONG);
									}
								}
							}
						} else {//价格为空拉取价格
							AccountServiceManager.getInstance().getProductPrice(albumId,false, 3, mHandler);
						}
					}
				} else {
					Util.dismissDialog(mProgressDialog);
					handleError(msg);
				}
			}
				break;
			case AccountEventCode.EVENT_GET_MOVIE_PALY_STYLE: {//拉取可用支付方式
				log.debug("EVENT_GET_MOVIE_PALY_STYLE msg.arg1=" + msg.arg1);
				Util.dismissDialog(mProgressDialog);
				if (msg.arg1 == 0) {//拉取失败
					ArrayList<PayStyle> mPayStylesList = (ArrayList<PayStyle>) ((PayStyleList) msg.obj).data;
					log.debug("mPayStylesList="+mPayStylesList);
					if (checkHasPayStyle(mPayStylesList)) {//有可用支付方式
						Intent intent = new Intent(mContext, PayStyleActivity.class);
						intent.putExtra(VOPrice.class.getSimpleName(), mVOPrice);
						intent.putParcelableArrayListExtra(PayStyle.class.getSimpleName(), mPayStylesList);
						mContext.startActivity(intent);
					} else {
						showPayDialog();//显示虚拟币支付
					}
				} else {
					showPayDialog();
				}
			}
				break;
			case AccountEventCode.EVENT_GET_MOVIE_PALY_ORDER: {
				Util.dismissDialog(mProgressDialog);
				log.debug("EVENT_GET_MOVIE_PALY_ORDER msg.arg1=" + msg.arg1);
				PayOrder mPayOrder = null;
				if (msg.arg1 == 0) {
					mPayOrder = (PayOrder) msg.obj;//在这个界面的支付我们都默认钱包支付或者虚拟币支付
					log.debug("mPayOrder=" + mPayOrder);
					gotoPlayer();
				} else {
					Util.showToast(mContext, mContext.getString(R.string.pay_get_order_fail) + msg.arg1, Toast.LENGTH_LONG);
				}
				ReportActionSender.getInstance().pay(msg.arg1, mPayOrder != null ? mPayOrder.paymentId : 0, mPayOrder != null ? mPayOrder.currencyId : 0, mPayOrder != null ? mPayOrder.amount:0, mPayOrder != null ? mPayOrder.productId:0, mPayOrder != null ? mPayOrder.orderId:0, 1, mPayOrder != null ? mPayOrder.channelId:0);
			}
				break;
			case AccountEventCode.EVENT_GET_PRICE://获取价格
				int priceStateCode = msg.arg1;
				if (priceStateCode == 0 ) {
					mVOPrice = (VOPrice) msg.obj;
					if(!checkPriceIsZero()){
						PriceInfo mDefault = mVOPrice.getDefaultRealPriceInfo();
						if(mDefault != null){
                    		AccountServiceManager.getInstance().reqMoviePayStyle(mDefault.price, mDefault.currencyId, mHandler);
                    	}else{//此处应该不会走到此处
                    		Util.dismissDialog(mProgressDialog);
                    		Util.showToast(mContext, mContext.getString(R.string.pay_price_null), Toast.LENGTH_LONG);
                    	}
					}
				}else{
					Util.dismissDialog(mProgressDialog);
					Util.showToast(mContext, mContext.getString(R.string.play_get_playurl_failed), Toast.LENGTH_LONG);
				}
				break;
			default:
				break;
			}
		}
	};

	private void createRealDownload(final int resolution, final String urlStr, final String fileName, final String cookie) {
		new Thread(new Runnable() {
			
			@Override
			public void run() {
				long filesize = 0;
				try {
					long t1 = System.nanoTime();
					URL url = new URL(urlStr);
					HttpURLConnection conn = (HttpURLConnection) url.openConnection();
					conn.setConnectTimeout(5 * 1000);
					conn.setRequestMethod("GET");
					conn.setRequestProperty("Charset", "UTF-8");
					conn.setRequestProperty("Connection", "Keep-Alive");
					conn.setAllowUserInteraction(true);

					int responseCode = conn.getResponseCode();
					if (responseCode < 300) {
						String s = conn.getHeaderField("Content-Length");
						try{
							filesize = Long.parseLong(s);
							log.debug("create Real Download parse content len header:" + filesize + "  Content-Length:" + s);
						}catch(Exception e) {
							filesize = conn.getContentLength();
						}
					}

					t1 = System.nanoTime() - t1;
					log.debug("create Real Download filesize:" + filesize + " t1:" + t1);
					if(filesize <= 0) {
						Util.showToast(StoreApplication.INSTANCE, StoreApplication.INSTANCE.getString(R.string.download_failed), Toast.LENGTH_SHORT);
					} else {
						VideoDownloadInfo downloadInfo = new VideoDownloadInfo();
						downloadInfo.downloadUrl = urlStr;
						downloadInfo.albumnId = albumId;
						downloadInfo.mode = mode;
						downloadInfo.mSubTitleInfos = subtitles;
						downloadInfo.videoId = videoId;
						downloadInfo.mFileSize = filesize;
						downloadInfo.mTitle = fileName;
						downloadInfo.poster = poster;
						downloadInfo.downloadType = DownloadInfo.DOWNLOAD_TYPE_VIDEO;
	
						downloadInfo.starttime = Long.toString(System.currentTimeMillis());
						downloadInfo.setDownloadState(AppDownloadInfo.STATE_WAIT);
						downloadInfo.mPath = EnvironmentTool.getFileDownloadCacheDir() + fileName;
	
						DownloadSQLiteHelper downloadSQLiteHelper = new DownloadSQLiteHelper(StoreApplication.INSTANCE);
						downloadSQLiteHelper.insertData(downloadInfo);
	
						DownloadManager.getInstance().startDownload(downloadInfo, null);
						ReportActionSender.getInstance().download(videoId, fileName, resolution);
						Util.showToast(StoreApplication.INSTANCE, StoreApplication.INSTANCE.getString(R.string.download_tast_success), Toast.LENGTH_SHORT);

					}
					// cost too much here
					t1 = System.nanoTime();
					conn.disconnect();
					
					t1 = System.nanoTime() - t1;
					log.debug("spend time:" + t1);
					
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}).start();
	}

	/**
	 * 判断价格是否为0
	 * @return
	 */
	private boolean checkPriceIsZero(){
		PriceInfo mDefault = mVOPrice.getDefaultRealPriceInfo();
        PriceInfo mVirtual = mVOPrice.getVirtualPriceInfo();
        log.debug("mDefault="+mDefault+",mVirtual="+mVirtual+",mVOPrice="+mVOPrice);
        if(mDefault != null && Integer.parseInt(mDefault.realPrice)==0){//真实价格不为空,并且为0,支付
        	AccountServiceManager.getInstance().getMoviePayOrder(mContext, mVOPrice.productName, UserManager.getInstance().getUserId(), mVOPrice.productId, mVOPrice.productType,
					PayOrder.PAYMENT_VIRTUAL_WALLET, mDefault.price, mDefault.currencyId, mDefault.channelId, mHandler);
        	return true;
        }else if(mVirtual != null && Integer.parseInt(mVirtual.realPrice)==0){//虚拟价格不为空，并且为0
        	AccountServiceManager.getInstance().getMoviePayOrder(mContext, mVOPrice.productName, UserManager.getInstance().getUserId(), mVOPrice.productId, mVOPrice.productType,
					PayOrder.PAYMENT_VIRTUAL_WALLET, mVirtual.price, mVirtual.currencyId, mVirtual.channelId, mHandler);
        	return true;
        }
		return false;
	}
	
	/**
	 * 判断是否有可用支付方式
	 * @return
	 */
	private boolean checkHasPayStyle(ArrayList<PayStyle> mPayStylesList){
		if(mPayStylesList == null || mPayStylesList.size() == 0){
			return false;
		}else{
			for(PayStyle mPayStyle : mPayStylesList){
				if(mPayStyle.id == PayOrder.PAYMENT_ALIPAY || mPayStyle.id == PayOrder.PAYMENT_SKYSMS || mPayStyle.id == PayOrder.PAYMENT_UNIONPAY || mPayStyle.id == PayOrder.PAYMENT_PAYPAL){
					return true;
				}
			}
		}
		return false;
	}

	/** 处理非0逻辑 */
	private void handleError(Message msg) {
		switch (msg.arg1) {
		case ExceptionCode.SocketException:
			Util.showToast(mContext, TDString.getStr(R.string.play_neterror_and_trylater) + "(" + msg.arg1 + ")", Toast.LENGTH_LONG);
			break;
		case ExceptionCode.SocketTimeoutException:
			Util.showToast(mContext, TDString.getStr(R.string.play_nettimeout_and_trylater) + "(" + msg.arg1 + ")", Toast.LENGTH_LONG);
			break;
		default:
			Util.showToast(mContext, TDString.getStr(R.string.play_get_playurl_failed) + "(" + msg.arg1 + ")", Toast.LENGTH_LONG);
			break;
		}
	}

	/**
	 * 显示虚拟币支付结果
	 * 
	 * @param success
	 * @param title
	 * @param message
	 */
	private void showPayResultDialog(final boolean success, String title, String message) {
		AlertDialog.Builder builder = new AlertDialog.Builder(mContext, R.style.AlertDialog);
		builder.setTitle(title);
		builder.setMessage(message);
		builder.setInverseBackgroundForced(true);
		builder.setNegativeButton(Util.setColourText(mContext, R.string.ok, Util.DialogTextColor.BLUE), new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {

			}
		});
		AlertDialog mAlertDialog = builder.create();
		mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {

			@Override
			public void onDismiss(DialogInterface dialog) {
				if (success) {
					// TODO 自动播放
				}
			}
		});
		mAlertDialog.show();
	}

	/**
	 * 显示虚拟货币支付的对话框
	 */
	private void showPayDialog() {
		final PriceInfo mPriceInfo = mVOPrice.getVirtualPriceInfo();
		UserAccount mVirtualAccount = WalletManager.getInstance().getVirtualAccount();
		log.debug("mPriceInfo=" + mPriceInfo + ",VirtualAccount=" + mVirtualAccount);
		if (mPriceInfo == null) {
			Util.showToast(mContext, mContext.getString(R.string.pay_price_null), Toast.LENGTH_SHORT);
			return;
		}
		if(mVirtualAccount == null){
			Util.showToast(mContext, mContext.getString(R.string.sorry_get_wallet_is_null), Toast.LENGTH_SHORT);
			return;
		}
		AlertDialog.Builder mBuilder = new AlertDialog.Builder(mContext,R.style.AlertDialog);
		mBuilder.setTitle(Util.setColourText(mContext, R.string.pay_result_dialog_title, Util.DialogTextColor.BLUE));
		View mContentView = ((Activity) mContext).getLayoutInflater().inflate(R.layout.view_virtual_pay_content, null, false);
		TextView textView_need_pay_msg = (TextView) mContentView.findViewById(R.id.textView_need_pay_msg);
		TextView textView_remain_pay_toast = (TextView) mContentView.findViewById(R.id.textView_remain_pay_toast);
		String text = MessageFormat.format(mContext.getString(R.string.pay_movie_need_virtual_money_title), mPriceInfo.realPrice);
		log.debug("text="+text);
		int start = text.indexOf(mPriceInfo.realPrice);
		int size = mPriceInfo.realPrice.length();
		  
		SpannableStringBuilder style=new SpannableStringBuilder(text);     
		style.setSpan(new ForegroundColorSpan(mContext.getResources().getColor(R.color.red_btn_normal)),start,start + size,Spannable.SPAN_EXCLUSIVE_INCLUSIVE);      
		textView_need_pay_msg.setText(style);  
		           
		textView_remain_pay_toast.setText(MessageFormat.format(mContext.getString(R.string.pay_movie_need_virtual_money_msg), mVirtualAccount.amount));
		mBuilder.setView(mContentView);
		
		mBuilder.setPositiveButton(R.string.cancel, new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
				// TODO Auto-generated method stub

			}
		}).setNegativeButton(R.string.ok, new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
				if (WalletManager.getInstance().getVirtualAccount().amount >= Integer.parseInt(mVOPrice.getVirtualPriceInfo().realPrice)) {
					AccountServiceManager.getInstance().getMoviePayOrder(mContext, mVOPrice.productName, UserManager.getInstance().getUserId(), mVOPrice.productId, mVOPrice.productType,
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
		AlertDialog.Builder mBuilder = new AlertDialog.Builder(mContext,R.style.AlertDialog);
		mBuilder.setTitle(mContext.getString(R.string.pay_result_dialog_title));
		String moneyNotEnough = mContext.getString(R.string.pay_movie_need_virtual_money_not_enough)+"\n"+MessageFormat.format(mContext.getString(R.string.pay_movie_need_virtual_money_msg), WalletManager.getInstance().getVirtualAccount().amount);
		mBuilder.setMessage(moneyNotEnough);
		mBuilder.setPositiveButton(R.string.ok, new OnClickListener() {

			@Override
			public void onClick(DialogInterface dialog, int which) {
			}
		});
		mBuilder.create().show();
	}

	/**
	 * 
	 * @param voPrice
	 * @param mode com.runmit.sweedee.model.CmsVedioBaseInfo.Vedio.mode
	 * @param picurl
	 * @param subtitles
	 * @param fileName
	 * @param albumId
	 * @param vedioId
	 * @param loadUrlType ServiceArg.URL_TYPE_PLAY or ServiceArg.URL_TYPE_DOWNLOAD
	 * @param checkLogin
	 */
	public void startgetPlayUrl(VOPrice voPrice,int mode, String picurl, ArrayList<SubTitleInfo> subtitles, final String fileName, final int albumId, final int vedioId, final int loadUrlType,
			boolean checkLogin) {
		if (!preCheck(loadUrlType, checkLogin)) {
			return;
		}
		this.mode = mode;
		this.needLogin = checkLogin;
		this.mVOPrice = voPrice;
		this.subtitles = subtitles;
		this.poster = picurl;

		this.mFileName = fileName;
		this.albumId = albumId;
		this.videoId = vedioId;
		this.loadUrlType = loadUrlType;

		if (loadUrlType == ServiceArg.URL_TYPE_PLAY || !checkDownload()) {
			gotoPlayer();
		}
	}

	private boolean checkDownload() {
		DownloadInfo downloadInfo = DownloadManager.getInstance().getDownloadInfo(String.valueOf(albumId));
		if (downloadInfo != null && downloadInfo.isFinish()) {// 判断是否下载完成
			File file = new File(downloadInfo.mPath);
			log.debug("The local uri is " + downloadInfo.mPath);
			if (!file.exists()) {
				DownloadManager.getInstance().deleteTask(downloadInfo);
				downloadInfo = null;
			}
		}

		if (downloadInfo == null) {// 创建一个新的下载任务
			return false;
		}
		if(!downloadInfo.isFinish() && !downloadInfo.isRunning()){
			DownloadManager.getInstance().startDownload(downloadInfo, null);
		}
		Util.showToast(StoreApplication.INSTANCE, StoreApplication.INSTANCE.getString(R.string.download_tast_already_exist), Toast.LENGTH_SHORT);
		
		return true;
	}

	/**
	 * 重新获取播放链接
	 * @param albumId	视频albumId
	 * @param vedioId	视频vedioId
	 * @param taskid	当前任务id
	 */
	public void startRegetUrl(final VideoDownloadInfo info){
		if (!preCheck(loadUrlType, true)) {
			return;
		}
		this.loadUrlType = ServiceArg.URL_TYPE_DOWNLOAD;
		this.isRestartDownload = true;
		this.mTaskId = info.downloadId;
		
		gotoPlayer();
	}

	/**
	 * 获取点播url前的检测 1 网络可用性检测 2 3G流量保护设置检测 3.登陆检测
	 *
	 * @return 返回检测结果
	 */
	private boolean preCheck(int loadUrlType, boolean checkLogin) {
		if(!checkInstall3DPlayer()){
			return false;
		}
		if (!Util.checkOperateNetWorkAvail()) {
			return false;
		}
		if (!checkDownloadableIn3G(mContext, loadUrlType)) {
			return false;
		}
		if (UserManager.getInstance().getLoginState() == UserManager.LoginState.COMMON && checkLogin) {// 未登录
			gotoLogin();
			return false;
		}
		return true;
	}

	private void gotoLogin() {
		Intent intent = new Intent(mContext, LoginActivity.class);
		mContext.startActivity(intent);
	}

	private boolean checkDownloadableIn3G(Context ctx, int loadUrlType) {
		SharePrefence mSharePrefence = SharePrefence.getInstance(ctx);
		boolean canDoAction = loadUrlType == ServiceArg.URL_TYPE_PLAY ? mSharePrefence.getBoolean(SettingItemActivity.MOBILE_PLAY, false) : mSharePrefence.getBoolean(
				SettingItemActivity.MOBILE_DOWNLOAD, false);
		if (Util.isMobileNet(ctx) && !canDoAction) {
			Util.showDialogIn3GDownloadMode(ctx);
		} else {
			return true;
		}
		return false;
	}

	private void gotoPlayer() {
		mProgressDialog = new ProgressDialog(mContext, R.style.ProgressDialogDark);
		if (loadUrlType == ServiceArg.URL_TYPE_PLAY) {
			Util.showDialog(mProgressDialog, TDString.getStr(R.string.play_loading_url),true);
		} else {
			Util.showDialog(mProgressDialog, TDString.getStr(R.string.download_loading_url),true);
		}

		CmsServiceManager.getInstance().getGslbPlayUrl(albumId, videoId, loadUrlType, mHandler);
	}

	public void localPlay(Activity activity, String dirPath, String mFileName, long fileSize, ArrayList<SubTitleInfo> captions,int mode) {
		Intent intent = new Intent(activity, PlayerActivity.class);
		intent.putExtra(PlayerConstant.INTENT_PLAY_MODE, PlayerConstant.LOCAL_PLAY_MODE);
		intent.putExtra(PlayerConstant.INTENT_PLAY_FILE_NAME, mFileName);
		intent.putExtra(PlayerConstant.LOCAL_PLAY_PATH, dirPath);
		intent.putExtra(PlayerConstant.INTENT_PLAY_SUBTITLE, captions);
		intent.putExtra(PlayerConstant.INTENT_VIDEO_SOURCE_MODE, mode);
		activity.startActivity(intent);
	}

	/**
	 * 根据音轨设置获取相应的Vedio对象
	 * 
	 * @return
	 */
	public static Vedio getVedioByAudioSetting(List<Vedio> vedios) {
		int audioType = SharePrefence.getInstance(StoreApplication.INSTANCE).getInt(Constant.AUDIO_TRACK_SETTING, Constant.AUDIO_FIRST_LOCAL);
		if (audioType == Constant.AUDIO_FIRST_ORIN) {
			for (Vedio vedio : vedios) {
				if (vedio.isOriginal) {
					return vedio;
				}
			}
		}

		// 如果未找到原生的或者设置项不是原生音轨设置
		String lg = Locale.getDefault().getLanguage().toLowerCase();
		for (Vedio vedio : vedios) {
			if (vedio.isOriginal) {
				String vedioLg = vedio.audioLanguage == null ? "" : vedio.audioLanguage.toLowerCase();
				if (vedioLg.equals(lg)) {
					return vedio;
				}
			}
		}

		// 都未找到，数据问题，我们为了兼容，返回第一个
		return vedios.get(0);
	}
	
	
	private boolean checkInstall3DPlayer(){
		boolean isInstall = isInstallPlayer(mContext);
		if(!isInstall){//未安装
			AlertDialog.Builder mBuilder = new AlertDialog.Builder(mContext, R.style.AlertDialog);
			mBuilder.setTitle(R.string.confirm_title);
			mBuilder.setMessage(R.string.install_player_msg);
			mBuilder.setNegativeButton(R.string.download_app_install, new OnClickListener() {
				
				@Override
				public void onClick(DialogInterface dialog, int which) {
					Uri uri =Uri.parse("http://dl.d3dstore.com/moblie/android/3DPlayer_guanfang.apk");
					Intent it = new Intent(Intent.ACTION_VIEW,uri);
					mContext.startActivity(it);
					
//					String serviceString = Context.DOWNLOAD_SERVICE;  
//					DownloadManager downloadManager = (DownloadManager)mContext.getSystemService(serviceString);  
//					Uri uri = Uri.parse("http://dl.d3dstore.com/moblie/android/3DPlayer_guanfang.apk");  
//					DownloadManager.Request request = new Request(uri);  
//					downloadManager.enqueue(request);  
				}
			});
			mBuilder.setPositiveButton(R.string.cancel, new OnClickListener() {
				
				@Override
				public void onClick(DialogInterface dialog, int which) {
					// TODO Auto-generated method stub
					
				}
			});
			mBuilder.create().show();
		}
		return isInstall;
	}
	
	public static boolean isInstallPlayer(Context context) {
		try {
			PackageInfo packageInfo = context.getPackageManager().getPackageInfo("com.runmit.player", 0);
			return true;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return false;
	}

}
