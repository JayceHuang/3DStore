package com.runmit.sweedee.action.search;

import org.lucasr.twowayview.TwoWayLayoutManager.Orientation;
import org.lucasr.twowayview.widget.TwoWayView;

import android.os.Bundle;
import android.support.v7.widget.RecyclerView.RecycledViewPool;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.model.VOSearch;
import com.runmit.sweedee.util.XL_Log;

public class BaseSearchFragemnt extends BaseFragment {
	XL_Log log = new XL_Log(BaseSearchFragemnt.class);
	protected View rootView;
	protected static SearchActivity context;
	
	protected int layout_id;
	
	protected BaseSearchAdapter adapter;
	protected TwoWayView mRecycleView;
	
	protected VOSearch searchResult;
	protected String keyword;
	
	protected View search_empty_complext_result_rl;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		context = (SearchActivity) this.getActivity();
		if (layout_id == 0){
			throw new Error("layout_id not init!!");
		}
	}
	
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		rootView = inflater.inflate(layout_id, container, false);
		initViews();
		
		return rootView;
	}
	
	public void setSearchResult(VOSearch searchResult, String lastKeywords){}
	
	protected void initViews() {
		search_empty_complext_result_rl = rootView.findViewById(R.id.search_empty_complext_result_rl);
		mRecycleView = (TwoWayView) rootView.findViewById(R.id.searchRecycleView);
		mRecycleView.setHasFixedSize(true);
		mRecycleView.setLongClickable(true);
		mRecycleView.setLayoutManager(new org.lucasr.twowayview.widget.StaggeredGridLayoutManager(Orientation.VERTICAL, 2, 3));
		mRecycleView.setAdapter(adapter);
		
		if(!TextUtils.isEmpty(this.keyword) && this.searchResult != null) {
			setSearchResult(this.searchResult, this.keyword);
		}
	}

	protected void showEmpty(boolean emptyResult){
		if(emptyResult){
			search_empty_complext_result_rl.setVisibility(View.VISIBLE);
		}else{
			search_empty_complext_result_rl.setVisibility(View.GONE);
		}
	}
	
	protected RecycledViewPool getRecycledViewPool () {
		
		return mRecycleView.getRecycledViewPool();
	}
	
	protected boolean updateView() {
		if(rootView == null){return false;}
		
		return true;
	}
}