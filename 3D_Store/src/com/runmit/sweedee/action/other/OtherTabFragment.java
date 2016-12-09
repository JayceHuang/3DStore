//package com.runmit.sweedee.action.other;
//
//import android.graphics.Bitmap;
//import android.graphics.BitmapFactory;
//import android.graphics.drawable.BitmapDrawable;
//import android.os.Bundle;
//import android.os.Handler;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.view.ViewGroup;
//
//import com.runmit.sweedee.BaseFragment;
//import com.runmit.sweedee.HomeHeadFragment;
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.action.game.GameFragment;
//import com.runmit.sweedee.util.XL_Log;
//
//public class OtherTabFragment extends HomeHeadFragment {
//
//	private XL_Log log = new XL_Log(GameFragment.class);
//
//	private View mViewRoot; 
//	
//	private Handler mHandler = new Handler();
//	
//	@Override
//	public View onCreateContentView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
//		log.debug("onCreateView=" + this);
//		if(mViewRoot!=null){
//			ViewGroup parent = (ViewGroup) mViewRoot.getParent();
//	        if (parent != null) {
//	            parent.removeView(mViewRoot);
//	        }
//		}else{
//			mViewRoot = inflater.inflate(R.layout.fragment_other, container,false);
//			loadImage();
//		}
//		return mViewRoot;
//	}
//
//	private void loadImage(){
//		new Thread(new Runnable() {
//			
//			@Override
//			public void run() {
//				final Bitmap mBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.background_other);
//				mHandler.post(new Runnable() {
//					
//					@Override
//					public void run() {
//						mViewRoot.setBackground(new BitmapDrawable(getResources(), mBitmap));
//					}
//				});
//			}
//		}).start();
//	}
//	
//	@Override
//	public void onCreate(Bundle savedInstanceState) {
//		log.debug("onCreate");
//		super.onCreate(savedInstanceState);
//	}
//
//	public void onResume() {
//		super.onResume();
//	}
//
//	@Override
//	public void onPause() {
//		super.onPause();
//	}
//
//	@Override
//	public void onStop() {
//		super.onStop();
//	}
//
//	@Override
//	public void onDestroy() {
//		super.onDestroy();
//	}
//}
