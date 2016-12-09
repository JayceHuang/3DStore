package com.runmit.sweedee.action.search;
import android.text.TextUtils;
import com.runmit.sweedee.R;
import com.runmit.sweedee.model.VOSearch;

public class SearchAppFragment extends BaseSearchFragemnt {
	
	enum App_Type {
		GAME,
		APP
	}
	
	public App_Type appType;
	
	public SearchAppFragment(App_Type type) {
		layout_id = R.layout.frg_search_app;
		appType = type;
	}
	
	@Override
	public void setSearchResult(VOSearch searchResult, String lastKeywords) {
		this.searchResult = searchResult;
		this.keyword = lastKeywords;
		if(adapter == null) return;
		
		((SearchAppAdapter)adapter).initPositionData(searchResult, lastKeywords, appType);
		adapter.notifyDataSetChanged();
		updateView();
	}
	
	
	@Override
	protected void initViews() {
		super.initViews();
		adapter = new SearchAppAdapter(mFragmentActivity);
		mRecycleView.setAdapter(adapter);
		if(!TextUtils.isEmpty(keyword) && searchResult != null) {
			setSearchResult(searchResult, keyword);
		}
	}
	
	@Override
	protected boolean updateView() {
		if(!super.updateView())return false;
		
		boolean emptyResult = (adapter.getItemCount() == 0);
		
		showEmpty(emptyResult);
		
		return true;
	}
}
