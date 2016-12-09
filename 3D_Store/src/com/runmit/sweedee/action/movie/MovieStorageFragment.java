package com.runmit.sweedee.action.movie;

import java.util.ArrayList;
import java.util.Locale;

import android.content.Intent;
import android.os.Bundle;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.AbsoluteSizeSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.DetailActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.constant.ServiceArg;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.manager.PlayerEntranceMgr;
import com.runmit.sweedee.model.CmsItemable;
import com.runmit.sweedee.model.CmsModuleInfo.CmsItem;
import com.runmit.sweedee.model.CmsVedioBaseInfo;
import com.runmit.sweedee.model.SortHotWordInfo;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.EmptyView;

/**
 * 电影模块-影库页面
 */
public class MovieStorageFragment extends BaseFragment implements AdapterView.OnItemClickListener {
	XL_Log log = new XL_Log(MovieStorageFragment.class);
	private ListView mContentListView;
	private View rootView;
	private EmptyView mEmptyView;

	private MoivieAdapter myAdapter;

	private ArrayList<CmsItem> mCmsItems;

	public MovieStorageFragment(ArrayList<CmsItem> cmsItems) {
		this.mCmsItems = new ArrayList<CmsItem>();
		for(CmsItem mItem : cmsItems){
			if(mItem.item != null){
				mCmsItems.add(mItem);
			}
		}
	}

	@Override
	public AbsListView getCurrentListView() {
		return mContentListView;
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		if (rootView != null) {
			return onlyOneOnCreateView(rootView);
		}
		rootView = inflater.inflate(R.layout.frg_movie_storage, container, false);
		mContentListView = (ListView) rootView.findViewById(R.id.gv_movie_storage);
		mEmptyView = (EmptyView) rootView.findViewById(R.id.rl_empty_tip);
		mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPic(R.drawable.ic_failed);
		mEmptyView.setStatus(EmptyView.Status.Loading);
		mEmptyView.setEmptyTop(mFragmentActivity, 110);
		mContentListView.setOnItemClickListener(this);

		if (mCmsItems != null && mCmsItems.size() > 0) {
			myAdapter = new MoivieAdapter(mCmsItems);
			mContentListView.setAdapter(myAdapter);
			mEmptyView.setStatus(EmptyView.Status.Gone);
		} else {
			mEmptyView.setStatus(EmptyView.Status.Empty);
		}

		return rootView;
	}

	@Override
	public void onItemClick(AdapterView<?> parent, final View view, int position, long id) {
		CmsItem mCmsItem = myAdapter.getItem(position);
		CmsItemable mCmsItemable = myAdapter.getItem(position).item;
		if(mCmsItem.type.value.equals(CmsItem.HOTWORD)){
			SortHotWordInfo mHotWordInfo = (SortHotWordInfo) mCmsItem.item;
			Intent intent = new Intent(mFragmentActivity,MovieCatalogActivity.class);
			intent.putExtra(SortHotWordInfo.class.getSimpleName(), mHotWordInfo);
			startActivity(intent);
		}else if(mCmsItem.type.value.equals(CmsItem.TYPE_ALBUM) || mCmsItem.type.value.equals(CmsItem.ALBUM_DCSH)){
			DetailActivity.start(mFragmentActivity, view, mCmsItemable.getId());
		}else{
			String imgUrl = TDImageWrap.getShortVideoPoster(mCmsItemable);
			String fileName = mCmsItemable.getTitle();
			int albumId = mCmsItemable.getId();
			int vedioId = mCmsItemable.getVideoId();
			log.debug("albumId="+albumId+",vedioId="+vedioId);
			int mode = 0;
			CmsVedioBaseInfo mCmsVedioBaseInfo = (CmsVedioBaseInfo) mCmsItemable;
			if(mCmsVedioBaseInfo != null && mCmsVedioBaseInfo.films != null && mCmsVedioBaseInfo.films.size() > 0){
				mode = mCmsVedioBaseInfo.films.get(0).mode;
			}
			new PlayerEntranceMgr(mFragmentActivity).startgetPlayUrl(null, mode, imgUrl, null, fileName, albumId, vedioId, ServiceArg.URL_TYPE_PLAY, false);
		}
	}

	class MoivieAdapter extends BaseAdapter {
		private ArrayList<CmsItem> cmsVedioBaseInfos;
		private ImageLoader mImageLoader;
		private LayoutInflater mInflater;
		private DisplayImageOptions mOptions;

		public MoivieAdapter(ArrayList<CmsItem> c) {
			super();
			cmsVedioBaseInfos = c;
			mImageLoader = ImageLoader.getInstance();
			mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_home_movie).build();
			mInflater = LayoutInflater.from(mFragmentActivity);
		}

		@Override
		public int getCount() {
			return cmsVedioBaseInfos.size();
		}

		@Override
		public CmsItem getItem(int position) {
			return cmsVedioBaseInfos.get(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			ViewHolder holder;
			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.item_movie_type, parent, false);
				holder = new ViewHolder();
//				holder.tv_grade = (TextView) convertView.findViewById(R.id.tv_commend_grade);
				holder.tv_name = (TextView) convertView.findViewById(R.id.tv_item_name);
				holder.bg_logo = (ImageView) convertView.findViewById(R.id.iv_item_logo);
				holder.tv_commend_grade = (TextView) convertView.findViewById(R.id.tv_commend_grade);
				holder.layout_score = (FrameLayout) convertView.findViewById(R.id.layout_score);
				holder.lines = (ImageView) convertView.findViewById(R.id.lines);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}
			CmsItem cvbi = getItem(position);
			if (cvbi != null) {
				holder.tv_name.setText(cvbi.item.getTitle());
				if(cvbi.type.value.equals(CmsItem.TYPE_ALBUM) || cvbi.type.value.equals(CmsItem.ALBUM_DCSH)){
					String score = cvbi.item.getScore();
					Spannable WordtoSpan = new SpannableString(score);
					WordtoSpan.setSpan(new AbsoluteSizeSpan(14, true), 0, 1, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
					WordtoSpan.setSpan(new AbsoluteSizeSpan(10, true), 1, 3, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
					holder.tv_commend_grade.setText(WordtoSpan);
					holder.layout_score.setVisibility(View.VISIBLE);
				}else{
					holder.layout_score.setVisibility(View.INVISIBLE);
				}
				
				String imgUrl;
				if(cvbi.type.value.equals(CmsItem.HOTWORD)){
					SortHotWordInfo mHotWordInfo = (SortHotWordInfo) cvbi.item;
					imgUrl = mHotWordInfo.poster;
				}else{
					imgUrl = TDImageWrap.getShortVideoPoster(cvbi.item);// 设置图片
				}
				log.debug("imgUrl="+imgUrl);
				mImageLoader.displayImage(imgUrl, holder.bg_logo, mOptions);
			}
			
			if(cvbi.type.value.equals(CmsItem.HOTWORD)){
				holder.lines.setVisibility(View.VISIBLE);
			}else {
				holder.lines.setVisibility(View.GONE);
			}
			return convertView;
		}

		class ViewHolder {
			TextView tv_name;
			TextView tv_commend_grade;
//			TextView tv_grade;
			ImageView bg_logo;
			ImageView lines;
			FrameLayout layout_score;
		}
		
		private String cuvertTime(int time){
			int mCurSec = time/1000;
	        int hour = mCurSec / 60 / 60;
	        int leftSec = mCurSec % (60 * 60);
	        int min = leftSec / 60;
	        int sec = leftSec % 60;
	        String strHour = String.format(Locale.getDefault(),"%d", hour);
	        String strMin = String.format(Locale.getDefault(),"%d", min);
	        String strSec = String.format(Locale.getDefault(),"%d", sec);

	        if (hour < 10) {
	            strHour = "0" + hour;
	        }
	        if (min < 10) {
	            strMin = "0" + min;
	        }
	        if (sec < 10) {
	            strSec = "0" + sec;
	        }
	        String formattedTime = strHour + ":" + strMin + ":" + strSec;
	        return formattedTime;
		}
	}
}
