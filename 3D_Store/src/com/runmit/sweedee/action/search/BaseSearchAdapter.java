package com.runmit.sweedee.action.search;

import org.lucasr.twowayview.widget.StaggeredGridLayoutManager;

import com.runmit.sweedee.R;
import com.runmit.sweedee.model.VOSearch;
import com.runmit.sweedee.util.XL_Log;

import android.annotation.SuppressLint;
import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

public class BaseSearchAdapter extends RecyclerView.Adapter<BaseSearchViewHolder> {

	protected int mItemCount = 0;

	protected static final int TYPE_TITLE = 0;
	protected static final int TYPE_VIDEO = 1;
	protected static final int TYPE_SHORT_VIDEO = 2;
	protected static final int TYPE_GAME = 3;
	protected static final int TYPE_APP = 4;

	protected LayoutInflater mLayoutInflater;
	protected String mLastKeyword;
	

	/**
	 * 存放当前位置的viewType
	 */
	@SuppressLint("UseSparseArrays")
	protected SparseArray<Integer> mPositionType = new SparseArray<Integer>();

	/**
	 * 存放当前位置对应的object，用内存换cpu
	 */
	protected SparseArray<Object> mPositionObject = new SparseArray<Object>();

	protected Context mContext;

	protected XL_Log log = new XL_Log(BaseSearchAdapter.class);
	
	public BaseSearchAdapter(Context context) {
		mLayoutInflater = LayoutInflater.from(context);
		this.mContext = context;
		mPositionType = new SparseArray<Integer>();
		mPositionObject = new SparseArray<Object>();
		mItemCount = 0;
	}
	
	public void initPositionData(VOSearch searchResult, String keyword){
		mPositionType = new SparseArray<Integer>();
		mPositionObject = new SparseArray<Object>();
		mItemCount = 0;
	}
	

	@Override
	public int getItemCount() {
		return mItemCount;
	}
	
	public Object getItem(int pisition) {
		return mPositionObject.get(pisition);
	}
	
	@Override
	public int getItemViewType(int position) {
//		log.debug("position:" + position + "  mPositionType:" + mPositionType);
		return mPositionType.get(position);
	}

	@Override
	public void onBindViewHolder(BaseSearchViewHolder holder, int pisition) {
		holder.onBindViewHolder(getItem(pisition));
	}
	
	@Override
	public BaseSearchViewHolder onCreateViewHolder(ViewGroup parent, int vtype) {
		switch (vtype) {
		case TYPE_TITLE:{
			View titleView = mLayoutInflater.inflate(R.layout.item_search_title, parent, false);
			final StaggeredGridLayoutManager.LayoutParams lp = (StaggeredGridLayoutManager.LayoutParams) titleView.getLayoutParams();
			lp.span = 2;
			titleView.setLayoutParams(lp);
			return new BaseSearchViewHolder.TitleTextViewHolder(titleView);
		}
		case TYPE_VIDEO:{
			View videoView = mLayoutInflater.inflate(R.layout.search_resource_list_item_child, parent, false);
			final StaggeredGridLayoutManager.LayoutParams lp = (StaggeredGridLayoutManager.LayoutParams) videoView.getLayoutParams();
			lp.span = 2;
			videoView.setLayoutParams(lp);
			return new BaseSearchViewHolder.VideoViewHolder(videoView, this, mContext);
		}
		case TYPE_SHORT_VIDEO:{
			View videoView = mLayoutInflater.inflate(R.layout.search_resource_grid_item_child, parent, false);
			final StaggeredGridLayoutManager.LayoutParams lp = (StaggeredGridLayoutManager.LayoutParams) videoView.getLayoutParams();
			lp.span = 1;
			videoView.setLayoutParams(lp);
			return new BaseSearchViewHolder.ShortVideoViewHolder(videoView, this, mContext);
		}
		case TYPE_GAME:
		case TYPE_APP:{
			View v = mLayoutInflater.inflate(R.layout.search_resource_list_item_child_app, parent, false);
			final StaggeredGridLayoutManager.LayoutParams lp = (StaggeredGridLayoutManager.LayoutParams) v.getLayoutParams();
			lp.span = 2;
			v.setLayoutParams(lp);
			return new BaseSearchViewHolder.AppViewHolder(v, this, mContext);
		}
		default:
			break;
		}
		return null;
	}

}
