package com.runmit.sweedee.action.search;
import com.runmit.sweedee.R;
import com.runmit.sweedee.model.VOSearch;

public class SearchVideoFragment extends BaseSearchFragemnt {	
	
	public SearchVideoFragment() {
		layout_id = R.layout.frg_search_video;
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
	
	protected void initViews() {
		adapter = new SearchVideoAdapter(mFragmentActivity);
		super.initViews();		
	}
	
	@Override
	protected boolean updateView() {
		if(!super.updateView())return false;
		
		boolean emptyResult = adapter == null || adapter.getItemCount() == 0;
		
		showEmpty(emptyResult);
		
		return true;
	}
}
