package com.runmit.sweedee.view;
//package com.xunlei.cloud.view;
//
//import android.content.Context;
//import android.content.Intent;
//import android.graphics.drawable.Drawable;
//import android.os.Handler;
//import android.util.AttributeSet;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.widget.ImageView;
//import android.widget.RelativeLayout;
//import android.widget.TextView;
//
//import com.xunlei.cloud.R;
//import com.xunlei.cloud.action.space.LocalFragmentActivity;
//import com.xunlei.cloud.manager.DELocalManager;
//import com.xunlei.cloud.manager.PrivacyLockManager;
//import com.xunlei.cloud.util.XL_Log;

//public class CornerMessageView extends RelativeLayout{
//	XL_Log log = new XL_Log(CornerMessageView.class);
//	private ImageView bgImg;
//	private TextView cornerText;
//	private int count;
//	private boolean isSmartMode = true;
//	private boolean isFixMode = false;
//	
//	final LayoutInflater mInflater;
//
//	public boolean isFixMode() {
//		return isFixMode;
//	}
//
//	public void setFixMode(boolean isFixMode) {
//		this.isFixMode = isFixMode;
//	}
//
//	private Handler mHandler=new Handler();
//	MyDownloadObserver myDownloadObserver=new MyDownloadObserver();
//	
//	public CornerMessageView(Context context) {
//		super(context);
//		mInflater=LayoutInflater.from(context);
//		mInflater.inflate(R.layout.cornermessage_view, this);
//		init(context, null);
//	}
//	
//	
//	public CornerMessageView(Context context, AttributeSet attrs) {
//		super(context, attrs);
//		mInflater=LayoutInflater.from(context);
//		mInflater.inflate(R.layout.cornermessage_view, this);
//		init(context, attrs);
//	}
//
//	public CornerMessageView(Context context, AttributeSet attrs, int defStyle) {
//		super(context, attrs, defStyle);
//		mInflater=LayoutInflater.from(context);
//		mInflater.inflate(R.layout.cornermessage_view, this);
//		init(context, attrs);
//	}
//
//	private void init(final Context context, AttributeSet attrs) {
//		bgImg = (ImageView) findViewById(R.id.imageView1_icon);
//		cornerText =(TextView) findViewById(R.id.textView1_size);
//		cornerText.setVisibility(View.INVISIBLE);
//		
//		setOnClickListener(new OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				PrivacyLockManager.startLocalActivity(context, false);
////				Intent intent = new Intent();
////				intent.setClass(context, LocalFragmentActivity.class);
////				context.startActivity(intent);
//			}
//		});
//	}
//
//	public void setImageBackground(int resId){
//		bgImg.setImageResource(resId);
//	}
//	
//	public void setCornerBackground(int resId){
//		cornerText.setBackgroundResource(resId);
//	}
//	
//	public void setCornerText(int count){
//		this.count = count;
//		cornerText.setText(String.valueOf(count));
//	}
//	
//	public void setCornerVisible(boolean flag){
//		cornerText.setVisibility(flag?View.VISIBLE:View.GONE);
//	}
//	
//	public int getCurrentCount(){
//		return count;
//	}
//	
//	@Override
//	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
//		super.onMeasure(widthMeasureSpec, heightMeasureSpec);
////		ViewGroup.LayoutParams params = getLayoutParams();
////		int targetWidth = 0;
////		int count = getChildCount();
////		for(int i = 0; i < count; i++){
////			targetWidth += getChildAt(i).getMeasuredWidth();
////		}
////		params.width = targetWidth;
////		setLayoutParams(params);
//	}
//	
//	
//	class MyDownloadObserver implements DownloadObserver{
//
//		/* (non-Javadoc)
//		 * @see com.xunlei.cloud.manager.DELocalManager.DownloadObserver#onDownLoadFileStateChange(int)
//		 */
//		@Override
//		public void onDownLoadFileStateChange(final int downloadingCount) {
//			mHandler.post(new Runnable() {
//				@Override
//				public void run() {
//					setCornerText(downloadingCount);
//					if(isFixMode){
//						if(downloadingCount == 0){
//							cornerText.setVisibility(View.INVISIBLE);
//						}else{
//							cornerText.setVisibility(View.VISIBLE);
//						}
//						//setVisibility(View.VISIBLE);
//					}else{
//						if(downloadingCount == 0){
//							setVisibility(View.GONE);
//						}else{
//							setVisibility(View.VISIBLE);
//						}
//					}
//				}
//			});
//		}
//	}
//
//	public boolean isSmartMode() {
//		return isSmartMode;
//	}
//
//	public void setSmartMode(boolean isSmartMode) {
//		this.isSmartMode = isSmartMode;
//	}
//	
//	public void onResume(){
//		DELocalManager.getInstance().addObserver(myDownloadObserver);
//		DELocalManager.getInstance().notifyDownloadingEvent();
//	}
//	
//	public void onDestroy(){
//		DELocalManager.getInstance().removerObserver(myDownloadObserver);
//	}
//	
//}
