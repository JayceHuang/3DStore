//package com.runmit.sweedee;
//
//import android.os.Bundle;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.view.ViewGroup;
//import android.widget.LinearLayout;
//
//
//public abstract class HomeHeadFragment extends BaseFragment {
//
//	LinearLayout mLinearLayout;
//	
//	@Override
//	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
//		if (mLinearLayout != null) {
//            return onlyOneOnCreateView(mLinearLayout);
//        }else{
//        	LinearLayout mLinearLayout = new LinearLayout(getActivity());
//    		mLinearLayout.setOrientation(LinearLayout.VERTICAL);
//    		View mHeadView = inflater.inflate(R.layout.transparent_head_layout, mLinearLayout, false);
//    		mHeadView.findViewById(R.id.transparent_actionbar).setVisibility(View.GONE);
//    		
//    		mLinearLayout.addView(mHeadView);
//    		mLinearLayout.addView(onCreateContentView(inflater, container, savedInstanceState));
//    		
//    		return mLinearLayout;
//        }
//	}
//	
//	protected abstract View onCreateContentView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState);
//}
