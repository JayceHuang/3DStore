package com.runmit.sweedee.action.search;

import android.util.Log;
import com.runmit.sweedee.R;
import com.runmit.sweedee.model.VOSearch;

public class SearchComplexFragment extends BaseSearchFragemnt {
	
	public SearchComplexFragment() {
		layout_id = R.layout.frg_search_complex;
	}
	
	protected void initViews() {
		adapter = new SearchComplexAdapter(mFragmentActivity);
		super.initViews();
	}
	
	@Override
	protected boolean updateView() {
		if(!super.updateView()) return false;
		
		boolean emptyResult = (adapter.getItemCount() == 0);
		
		showEmpty(emptyResult);
        
        return true;
	}
	

	@Override
	public void setSearchResult(VOSearch searchResult, String lastKeywords) {
		this.searchResult = searchResult;
		this.keyword = lastKeywords;
		if(adapter == null) return;
		
		adapter.initPositionData(searchResult, lastKeywords);
		adapter.notifyDataSetChanged();
		updateView();
	}
}
