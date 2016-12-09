package com.runmit.sweedee.action.home;

import java.util.ArrayList;
import java.util.List;

import org.lucasr.twowayview.widget.StaggeredGridLayoutManager;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.FadeInBitmapDisplayer;
import com.nostra13.universalimageloader.core.display.RoundedBitmapDisplayer;
import com.runmit.sweedee.DetailActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.game.AppDetailActivity;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.CmsItemable;
import com.runmit.sweedee.model.CmsModuleInfo;
import com.runmit.sweedee.model.CmsModuleInfo.CmsComponent;
import com.runmit.sweedee.model.CmsModuleInfo.CmsItem;
import com.runmit.sweedee.util.Util;

import android.annotation.SuppressLint;
import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.AbsoluteSizeSpan;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

public class HomeRecycleAdapter extends RecyclerView.Adapter<HomeRecycleAdapter.HomeViewHolder>{

	private Context mContext;

	protected static final int TYPE_TITLE = 0;

	protected static final int TYPE_VIDEO = 1;

	protected static final int TYPE_GAME = 2;

	protected static final int TYPE_APP = 3;

	protected static final int TYPE_HEADER = 4;

	protected final LayoutInflater mLayoutInflater;

	private final View mFakeHeader;

	protected int mItemCount = 0;
	
	/**
	 * 存放当前位置的viewType
	 */
	@SuppressLint("UseSparseArrays")
	protected SparseArray<Integer> mPositionType = new SparseArray<Integer>();

	/**
	 * 存放当前位置对应的object，用内存换cpu
	 */
	protected SparseArray<Object> mPositionObject = new SparseArray<Object>();

	public enum GridType {
		ONE, TWO
	}

	public HomeRecycleAdapter(Context context, List<CmsComponent> mList, View header) {
		mLayoutInflater = LayoutInflater.from(context);
		this.mFakeHeader = header;
		this.mContext = context;
		initPositionData(mList);

	}

	protected void initPositionData(List<CmsComponent> cmsComponents) {
		// 初始化head
		mPositionType.put(mItemCount, TYPE_HEADER);
		mPositionObject.put(mItemCount, null);
		mItemCount = 1;

		for (int i = 0, size = cmsComponents.size(); i < size; i++) {
			ArrayList<CmsItem> contents = cmsComponents.get(i).contents;
			mPositionType.put(mItemCount, TYPE_TITLE);
			mPositionObject.put(mItemCount, cmsComponents.get(i));
			mItemCount += 1;
			for (CmsItem mCmsItem : contents) {
				if (mCmsItem.type.value.equals(CmsItem.TYPE_ALBUM)) {
					mPositionType.put(mItemCount, TYPE_VIDEO);
				} else if (mCmsItem.type.value.equals(CmsItem.TYPE_GAME)) {
					mPositionType.put(mItemCount, TYPE_GAME);
				} else {
					mPositionType.put(mItemCount, TYPE_APP);
				}
				mPositionObject.put(mItemCount, mCmsItem);
				mItemCount += 1;
			}
		}
	}

	@Override
	public HomeViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
		switch (viewType) {
		case TYPE_TITLE: {
			View titleView = mLayoutInflater.inflate(R.layout.view_game_cut_title, parent, false);
			final StaggeredGridLayoutManager.LayoutParams lp = (StaggeredGridLayoutManager.LayoutParams) titleView.getLayoutParams();
			lp.span = 12;
			titleView.setLayoutParams(lp);
			return new TitleTextViewHolder(titleView);
		}
		case TYPE_VIDEO: {
			View videoView = mLayoutInflater.inflate(R.layout.item_homemovie_gridview, parent, false);
			final StaggeredGridLayoutManager.LayoutParams lp = (StaggeredGridLayoutManager.LayoutParams) videoView.getLayoutParams();
			lp.span = 6;
			videoView.setLayoutParams(lp);
			return new VideoItemViewHolder(videoView);
		}
		case TYPE_GAME: {
			View gameView = mLayoutInflater.inflate(R.layout.item_hotgame_gridview_one, parent, false);
			final StaggeredGridLayoutManager.LayoutParams lp = (StaggeredGridLayoutManager.LayoutParams) gameView.getLayoutParams();
			lp.span = 4;
			gameView.setLayoutParams(lp);
			return new GameItemViewHolder(gameView, GridType.ONE);
		}
		case TYPE_APP: {
			View gameView = mLayoutInflater.inflate(R.layout.item_hotgame_gridview_two, parent, false);
			final StaggeredGridLayoutManager.LayoutParams lp = (StaggeredGridLayoutManager.LayoutParams) gameView.getLayoutParams();
			lp.span = 3;
			gameView.setLayoutParams(lp);
			return new GameItemViewHolder(gameView, GridType.TWO);
		}
		case TYPE_HEADER:
		default: {
			final StaggeredGridLayoutManager.LayoutParams lp = (StaggeredGridLayoutManager.LayoutParams) mFakeHeader.getLayoutParams();
			lp.span = 12;
			mFakeHeader.setLayoutParams(lp);
			return new HomeViewHolder(mFakeHeader) {

				@Override
				public void onBindViewHolder(Object mItem) {

				}
			};
		}
		}
	}

	@Override
	public int getItemCount() {
		return mItemCount;
	}

	@Override
	public int getItemViewType(int position) {
		return mPositionType.get(position);
	}

	public Object getItem(int pisition) {
		return mPositionObject.get(pisition);
	}

	@Override
	public void onBindViewHolder(HomeViewHolder holder, int pisition) {
		holder.onBindViewHolder(getItem(pisition));
	}

	public abstract class HomeViewHolder extends RecyclerView.ViewHolder {

		public HomeViewHolder(View itemView) {
			super(itemView);
		}

		public abstract void onBindViewHolder(Object mItem);
	}

	private class TitleTextViewHolder extends HomeViewHolder {

		private TextView mTextView;

		private TitleTextViewHolder(View view) {
			super(view);
			mTextView = (TextView) view.findViewById(R.id.title_textview);
		}

		@Override
		public void onBindViewHolder(Object mItem) {
			CmsComponent mCmsComponent = (CmsComponent) mItem;
			mTextView.setText(mCmsComponent.title);
		}
	}

	private class GameItemViewHolder extends HomeViewHolder {
		
		private ImageView item_logo;
		private TextView item_name;
		private TextView hotgame_install_count;

		private DisplayImageOptions mOptions;
		private GridType mGridType; // 用于判断是一行3列还是4列
		private ImageLoader mImageLoader = ImageLoader.getInstance();

		private GameItemViewHolder(View view, GridType gridType) {
			super(view);
			this.mGridType = gridType;
			this.item_logo = (ImageView) view.findViewById(R.id.iv_newgame_item_logo);
			this.item_name = (TextView) view.findViewById(R.id.tv_newgame_item_name);
			this.hotgame_install_count = (TextView) view.findViewById(R.id.tv_hotgame_install_count);

			float density = view.getResources().getDisplayMetrics().density;
			if (mGridType.equals(GridType.ONE)) {
				mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_game_horizontal).setDisplayer(new RoundedBitmapDisplayer((int) (3 * density))).build();
			} else {
				mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.black_game_detail_icon).setDisplayer(new RoundedBitmapDisplayer((int) (8 * density))).build();
			}
		}

		public void onBindViewHolder(Object mItem) {
			CmsModuleInfo.CmsItem ci = (CmsItem) mItem;
			if (ci != null && ci.item != null) {
				final CmsItemable item = ci.item;

				this.item_name.setText(item.getTitle());
				this.hotgame_install_count.setText(Util.addComma(item.getInstallCount(), mContext));
				String imgUrl = mGridType == GridType.ONE ? TDImageWrap.getAppPoster(item) : TDImageWrap.getAppIcon(item);
				mImageLoader.displayImage(imgUrl, this.item_logo, mOptions);
				
				this.itemView.setOnClickListener(new View.OnClickListener() {
					
					@Override
					public void onClick(View v) {
						AppDetailActivity.start(mContext, itemView, (AppItemInfo) item);
					}
				});
			}
		}
	}

	private class VideoItemViewHolder extends HomeViewHolder {
		
		private ImageView item_logo;
		private TextView item_grade;
		private TextView item_name;
		private TextView item_highlight;

		private ImageLoader mImageLoader = ImageLoader.getInstance();
		private DisplayImageOptions mOptions;

		private VideoItemViewHolder(View view) {
			super(view);
			this.item_logo = (ImageView) view.findViewById(R.id.iv_item_logo);
			this.item_grade = (TextView) view.findViewById(R.id.tv_commend_grade);
			this.item_name = (TextView) view.findViewById(R.id.tv_item_name);
			this.item_highlight = (TextView) view.findViewById(R.id.tv_highlight);
			mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.defalut_movie_small_item).setDisplayer(new FadeInBitmapDisplayer(300)).build();
		}

		public void onBindViewHolder(Object mItem) {
			CmsModuleInfo.CmsItem ci = (CmsItem) mItem;
			if (ci != null && ci.item != null) {
				final CmsItemable item = ci.item;
				this.item_name.setText(item.getTitle());
				if (item.getScore().length() >= 3) {
					Spannable WordtoSpan = new SpannableString(item.getScore());
					WordtoSpan.setSpan(new AbsoluteSizeSpan(14, true), 0, 1, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
					WordtoSpan.setSpan(new AbsoluteSizeSpan(10, true), 1, 3, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
					this.item_grade.setText(WordtoSpan);
				}

				String hlight = item.getHighlight();
				if (hlight != null) {
					this.item_highlight.setText(hlight);
				}
				String imgUrl = TDImageWrap.getHomeVideoPoster(item);
				mImageLoader.displayImage(imgUrl, this.item_logo, mOptions);
				
				this.itemView.setOnClickListener(new View.OnClickListener() {
					
					@Override
					public void onClick(View v) {
						DetailActivity.start(mContext, itemView, item.getId());
					}
				});
			}
		}
	}
}
