package com.runmit.sweedee.action.search;

import android.content.Context;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.CmsVedioBaseInfo;
import com.runmit.sweedee.model.VOSearch;

public class SearchComplexAdapter extends BaseSearchAdapter {
	
	public SearchComplexAdapter(Context context) {
		super(context);
	}

	public void initPositionData(VOSearch searchResult, String keyword) {
		super.initPositionData(searchResult, keyword);
		int i, len;
		if (searchResult.ALBUM != null && searchResult.ALBUM.list.size() > 0) {
			len = Math.min(5, searchResult.ALBUM.list.size());
			mPositionType.put(mItemCount, TYPE_TITLE);
			mPositionObject.put(mItemCount++, mContext.getString(R.string.search_result_type_movie));
			for (i = 0; i < len; i++) {
				CmsVedioBaseInfo info = searchResult.ALBUM.list.get(i);
				mPositionType.put(mItemCount, TYPE_VIDEO);
				mPositionObject.put(mItemCount, new Object[]{info, i==len-1});
				mItemCount++;
			}
		}
		
		if (searchResult.LIGHTALBUM != null && searchResult.LIGHTALBUM.list.size() > 0) {
			if(mItemCount == 0) {
				mPositionType.put(mItemCount, TYPE_TITLE);
				mPositionObject.put(mItemCount++, mContext.getString(R.string.search_result_type_movie));
			}
			len = Math.min(6, searchResult.LIGHTALBUM.list.size());
			for (i = 0; i < len; i++) {
				CmsVedioBaseInfo info = searchResult.LIGHTALBUM.list.get(i);
				mPositionType.put(mItemCount, TYPE_SHORT_VIDEO);
				mPositionObject.put(mItemCount, info);
				mItemCount++;
			}
		}
		
		if(!StoreApplication.isSnailPhone)
		if (searchResult.GAME != null && searchResult.GAME.list.size() > 0) {
			len = Math.min(5, searchResult.GAME.list.size());
			mPositionType.put(mItemCount, TYPE_TITLE);
			mPositionObject.put(mItemCount++, mContext.getString(R.string.search_result_type_game));
			for (i = 0; i < len; i++) {
				AppItemInfo info = searchResult.GAME.list.get(i);
				mPositionType.put(mItemCount, TYPE_GAME);
				mPositionObject.put(mItemCount, new Object[]{info, i==len-1});
				mItemCount++;
			}
		}
		
		if (searchResult.APP != null && searchResult.APP.list.size() > 0) {
			len = Math.min(5, searchResult.APP.list.size());
			mPositionType.put(mItemCount, TYPE_TITLE);
			mPositionObject.put(mItemCount++, mContext.getString(R.string.search_result_type_app));
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
