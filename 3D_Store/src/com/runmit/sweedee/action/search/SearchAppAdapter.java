package com.runmit.sweedee.action.search;

import android.content.Context;

import com.runmit.sweedee.action.search.SearchAppFragment.App_Type;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.VOSearch;

public class SearchAppAdapter extends BaseSearchAdapter {
	
	public SearchAppAdapter(Context context) {
		super(context);
	}

	public void initPositionData(VOSearch searchResult, String keyword, App_Type appType) {
		super.initPositionData(searchResult, keyword);
		int i, len;
		if (appType == App_Type.GAME && searchResult.GAME != null && searchResult.GAME.list.size() > 0) {
			len = searchResult.GAME.list.size();
			for (i = 0; i < len; i++) {
				AppItemInfo info = searchResult.GAME.list.get(i);
				mPositionType.put(mItemCount, TYPE_GAME);
				mPositionObject.put(mItemCount, new Object[]{info, i==len-1});
				mItemCount++;
			}
		}
		if (appType == App_Type.APP && searchResult.APP != null && searchResult.APP.list.size() > 0) {
			len = searchResult.APP.list.size();
			for (i = 0; i < len; i++) {
				AppItemInfo info = searchResult.APP.list.get(i);
				mPositionType.put(mItemCount, TYPE_APP);
				mPositionObject.put(mItemCount, new Object[]{info, i==len-1});
				mItemCount++;
			}
		}
		mLastKeyword = keyword;
	}
}
