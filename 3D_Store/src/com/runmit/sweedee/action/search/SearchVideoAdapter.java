package com.runmit.sweedee.action.search;

import android.content.Context;

import com.runmit.sweedee.model.CmsVedioBaseInfo;
import com.runmit.sweedee.model.VOSearch;

public class SearchVideoAdapter extends BaseSearchAdapter {
	
	public SearchVideoAdapter(Context context) {
		super(context);
	}

	public void initPositionData(VOSearch searchResult, String keyword) {
		super.initPositionData(searchResult, keyword);
		int i, len;
		if (searchResult.ALBUM != null && searchResult.ALBUM.list.size() > 0) {
			len = searchResult.ALBUM.list.size() ;
			for (i = 0; i < len; i++) {
				CmsVedioBaseInfo info = searchResult.ALBUM.list.get(i);
				mPositionType.put(mItemCount, TYPE_VIDEO);
				mPositionObject.put(mItemCount, new Object[]{info, i==len-1});
				mItemCount ++;
			}
		}
		
		if (searchResult.LIGHTALBUM != null && searchResult.LIGHTALBUM.list.size() > 0) {
			len = searchResult.LIGHTALBUM.list.size();
			for (i = 0; i < len; i++) {
				CmsVedioBaseInfo info = searchResult.LIGHTALBUM.list.get(i);
				mPositionType.put(mItemCount, TYPE_SHORT_VIDEO);
				mPositionObject.put(mItemCount, info);
				mItemCount ++;
			}
		}
		mLastKeyword = keyword;
	}	
}
