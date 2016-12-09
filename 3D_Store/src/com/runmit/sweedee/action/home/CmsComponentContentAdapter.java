package com.runmit.sweedee.action.home;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.TextView;

import com.runmit.sweedee.DetailActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.game.AppDetailActivity;
import com.runmit.sweedee.action.game.GameGridAdapter;
import com.runmit.sweedee.action.game.GameGridAdapter.GridType;
import com.runmit.sweedee.manager.GetSysAppInstallListManager.AppInfo;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.CmsItemable;
import com.runmit.sweedee.model.CmsModuleInfo.CmsComponent;
import com.runmit.sweedee.model.CmsModuleInfo.CmsItem;

public class CmsComponentContentAdapter extends BaseAdapter {

	private enum ColumeType {
		Video, AppGridFour, AppGridSix;
	}

	private Context mContext;

	private List<CmsComponent> cmsComponents;

	private LayoutInflater mLayoutInflater;

	private SparseArray<View> mHashMap = new SparseArray<View>();

	public CmsComponentContentAdapter(Context context, List<CmsComponent> mList) {
		mContext = context;
		mLayoutInflater = LayoutInflater.from(mContext);
		cmsComponents = mList;
		mHashMap.clear();
	}

	@Override
	public int getCount() {
		return cmsComponents.size();
	}

	@Override
	public int getViewTypeCount() {
		return Math.max(cmsComponents.size(), 1);// 至少为1
	}

	@Override
	public CmsComponent getItem(int position) {
		return cmsComponents.get(position);
	}

	@Override
	public long getItemId(int position) {
		return position;
	}

	/**
	 * 检测Colume类型，因为首页的GuideGallery不好检测，所以默认第0项就是轮播，将错就错
	 * 
	 * @param mItem
	 * @return
	 */
	private ColumeType getColumeType(CmsComponent mItem) {
		ArrayList<CmsItem> mList = mItem.contents;
		boolean isContainMovie = false;
		boolean isContainGame = false;
		for (CmsItem item : mList) {
			if (item.type.value.equals(CmsItem.TYPE_ALBUM)) {
				isContainMovie = true;
			} else if (item.type.value.equals(CmsItem.TYPE_GAME)) {
				isContainGame = true;
			}
		}
		if (isContainMovie) {
			return ColumeType.Video;
		} else if (isContainGame) {
			return ColumeType.AppGridSix;
		}
		return ColumeType.AppGridFour;
	}

	@Override
	public View getView(final int position, View convertView, ViewGroup parent) {
		CmsComponent mItem = getItem(position);
		ColumeType mColumeType = getColumeType(mItem);
		switch (mColumeType) {
		case Video:
			View mMovieGridView = mHashMap.get(position);
			if (mMovieGridView == null) {
				mMovieGridView = mLayoutInflater.inflate(R.layout.view_home_video_grid, parent, false);
				mHashMap.put(position, mMovieGridView);
				TextView mCutTitleVideo = (TextView) mMovieGridView.findViewById(R.id.title_textview);
				mCutTitleVideo.setText(mItem.title);
				GridView mVideoGridView = (GridView) mMovieGridView.findViewById(R.id.gridview);
				mVideoGridView.setFocusable(false);
				mVideoGridView.setAdapter(new VideoItemAdapter(mContext, mItem));
				mVideoGridView.setOnItemClickListener(new AdapterView.OnItemClickListener() {

					@Override
					public void onItemClick(AdapterView<?> parent, final View view, int position, long id) {
						CmsItemable item = (CmsItemable) parent.getAdapter().getItem(position);
						DetailActivity.start(mContext, view, item.getId());
					}
				});
			}
			return mMovieGridView;
		case AppGridFour:
			View appFourView = mHashMap.get(position);
			if (appFourView == null) {
				appFourView = mLayoutInflater.inflate(R.layout.item_game_listview_two, parent, false);
				mHashMap.put(position, appFourView);

				TextView mAppTitle = (TextView) appFourView.findViewById(R.id.title_textview);
				mAppTitle.setText(mItem.title);
				GridView mAppGridView = (GridView) appFourView.findViewById(R.id.gridview);
				mAppGridView.setFocusable(false);
				mAppGridView.setAdapter(new GameGridAdapter(mContext, mItem, GridType.TWO));
				mAppGridView.setOnItemClickListener(new AdapterView.OnItemClickListener() {

					@Override
					public void onItemClick(AdapterView<?> parent, final View view, int position, long id) {
						AppItemInfo item = (AppItemInfo) parent.getAdapter().getItem(position);
						AppDetailActivity.start(mContext, view, item);
					}
				});
			}
			return appFourView;
		case AppGridSix:
			View mGameGridSix = mHashMap.get(position);
			if (mGameGridSix == null) {
				mGameGridSix = mLayoutInflater.inflate(R.layout.item_game_listview_one, parent, false);
				mHashMap.put(position, mGameGridSix);

				TextView mCutTitle = (TextView) mGameGridSix.findViewById(R.id.title_textview);
				mCutTitle.setText(mItem.title);

				GridView mGameGridView = (GridView) mGameGridSix.findViewById(R.id.gridview);
				mGameGridView.setFocusable(false);
				mGameGridView.setAdapter(new GameGridAdapter(mContext, mItem, GridType.ONE));
				mGameGridView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
					@Override
					public void onItemClick(AdapterView<?> parent, final View view, int position, long id) {
						AppItemInfo item = (AppItemInfo) parent.getAdapter().getItem(position);
						AppDetailActivity.start(mContext, view, item);
					}
				});
			}
			return mGameGridSix;
		default:
			throw new RuntimeException();
		}
	}
}
