package com.runmit.sweedee.action.search;

import java.util.ArrayList;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.text.Html;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.TextView;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.runmit.sweedee.DetailActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.action.game.AppDetailActivity;
import com.runmit.sweedee.constant.ServiceArg;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.manager.PlayerEntranceMgr;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.CmsItemable;
import com.runmit.sweedee.model.CmsVedioBaseInfo;
import com.runmit.sweedee.util.XL_Log;


public abstract class BaseSearchViewHolder extends RecyclerView.ViewHolder {
	protected ImageLoader mImageLoader;
	protected DisplayImageOptions mOptions;
	protected BaseSearchAdapter mSearchAdapter;
	protected Context mContext;
	
	protected XL_Log log = new XL_Log(BaseSearchViewHolder.class);
	
	public BaseSearchViewHolder(View view, BaseSearchAdapter searchAdapter, Context ctx) {
		super(view);
		mContext = ctx;
        mImageLoader = ImageLoader.getInstance();
        mSearchAdapter = searchAdapter;
	}
	public abstract void onBindViewHolder(Object mItem);
	
	
	private static void fillTexts(String lKeyWord, ArrayList<String> contents, TextView tv) {
		int len = 0;
    	if (null != contents && (len = contents.size()) > 0) {
    		
			String SLASH = "/";
			String content = "";
			StringBuilder sb = new StringBuilder();//append(contents.get(0));
			for (int i = 0; i < len; i++) {
				content = contents.get(i);
				if(!TextUtils.isEmpty(lKeyWord)){
					content = content.replace(lKeyWord, "<font color='red'>" + lKeyWord + "</font>");
	    		}
				sb.append(i == 0 ? "" : SLASH).append(content);
			}
			tv.setText(Html.fromHtml(sb.toString()));
		}else{
			tv.setText(StoreApplication.INSTANCE.getString(R.string.none_current));
		}
    	
		tv.setVisibility(View.VISIBLE);
	}
	
	private static void fillText(String lKeyWord, String content, TextView tv, boolean showMatch, int sub) {
    	if (null != content && content.length() > 0) {
    		if(!TextUtils.isEmpty(lKeyWord) && showMatch){
    			content = content.replace(lKeyWord, "<font color='red'>" + lKeyWord + "</font>");
    		}
    		if (sub == 0) {
    			tv.setText(Html.fromHtml(content));
			}else {
				tv.setText(Html.fromHtml(content.subSequence(0, sub).toString()));
			}
    	}else {
    		tv.setText(StoreApplication.INSTANCE.getString(R.string.none_current));
		}
		tv.setVisibility(View.VISIBLE);
    }
	
	private static void fillText(String lKeyWord, String content, TextView tv, int sub) {
    	if (null != content && content.length() > 0) {
    		if(!TextUtils.isEmpty(lKeyWord)){
    			content = content.replace(lKeyWord, "<font color='red'>" + lKeyWord + "</font>");
    		}
    		if (sub == 0) {
    			tv.setText(Html.fromHtml(content));
			}else {
				tv.setText(Html.fromHtml(content.subSequence(0, sub).toString()));
			}
    	}else {
    		tv.setText(StoreApplication.INSTANCE.getString(R.string.none_current));
		}
		tv.setVisibility(View.VISIBLE);
    }

	public static class TitleTextViewHolder extends BaseSearchViewHolder {
		private TextView mTextView;
		public TitleTextViewHolder(View view) {
			super(view, null, null);
			mTextView = (TextView) view.findViewById(R.id.title_textview);
		}

		@Override
		public void onBindViewHolder(Object mItem) {
			mTextView.setText((CharSequence) mItem);
		}
	}

	public static class VideoViewHolder extends BaseSearchViewHolder {
		public ImageView ivPoster;	// 海报
	    public TextView title;		// 片名
	    public TextView directer;	// 导演 
	    public TextView actor;		// 主演
	    public TextView type;		// 类型
	    public TextView time;		// 日期
	    public TextView item_grade;	// 评分
	    public View cut_line_layout; // 下划线
	    
		public VideoViewHolder(View view, BaseSearchAdapter adp, Context ctx) {
			super(view, adp, ctx);
	        mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_movie_image).build();
	        
			ivPoster = (ImageView) view.findViewById(R.id.iv_search_result_list_item_poster);
	        title = (TextView) view.findViewById(R.id.tv_search_result_list_item_title);
	        directer = (TextView) view.findViewById(R.id.tv_search_result_list_item_directer);
	        actor = (TextView) view.findViewById(R.id.tv_search_result_list_item_actor);
	        type = (TextView) view.findViewById(R.id.tv_search_result_list_item_type);
	        time = (TextView) view.findViewById(R.id.tv_search_result_list_item_time);
	        item_grade = (TextView) view.findViewById(R.id.search_result_grade);
	        cut_line_layout = view.findViewById(R.id.cut_line_layout);
	        view.setBackgroundResource(R.drawable.seletor_setting_item);
		}

		@Override
		public void onBindViewHolder(Object obj) {
			CmsVedioBaseInfo item = null;
			boolean isEnd = false;
			try {
				Object objs[] = (Object[])obj;
				item = (CmsVedioBaseInfo) objs[0];
				isEnd = (Boolean) objs[1];
			} catch(Exception e) {
				item = (CmsVedioBaseInfo) obj;
			}
			if(item == null)return;
			String mLastKeyword = mSearchAdapter.mLastKeyword;
		 	ivPoster.setVisibility(View.VISIBLE);
	    	String imgUrl = TDImageWrap.getVideoPoster(item);
	        mImageLoader.displayImage(imgUrl, ivPoster, mOptions);
	        // 填充文字
	    	fillText(mLastKeyword, item.title, title, 0);
	    	fillTexts(mLastKeyword, item.directors, directer);
	    	fillTexts(mLastKeyword, item.actors, actor);
			fillTexts(mLastKeyword, item.genres, type);
			fillText(mLastKeyword, item.releaseDate, time, 4);
			
			String score = item.getScore();
			Spannable WordtoSpan = new SpannableString(score);
			WordtoSpan.setSpan(new AbsoluteSizeSpan(14, true), 0, 1, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
			WordtoSpan.setSpan(new AbsoluteSizeSpan(10, true), 1, 3, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
			item_grade.setText(WordtoSpan);		
			
			cut_line_layout.setVisibility(isEnd ? View.INVISIBLE : View.VISIBLE);
			
			final int id = item.getId();
			this.itemView.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View paramView) {
		            ImageView img = ivPoster;
					DetailActivity.start(mContext, img, id);
				}
			});
		}
	}

	public static class ShortVideoViewHolder extends BaseSearchViewHolder {
		public ImageView ivPoster;	// 海报
	    public TextView title;		// 片名
	    public TextView time;		// 日期
		public ShortVideoViewHolder(View view, BaseSearchAdapter adp, Context ctx) {
			super(view, adp, ctx);
			mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.defalut_movie_small_item).build();
			ivPoster = (ImageView) view.findViewById(R.id.iv_search_result_list_item_poster);
	        title = (TextView) view.findViewById(R.id.tv_search_result_list_item_title);
	        time = (TextView) view.findViewById(R.id.tv_search_result_list_item_time);
		}

		@Override
		public void onBindViewHolder(Object mItem) {
			final CmsVedioBaseInfo item = (CmsVedioBaseInfo) mItem;
			String title = item.title;
			// holder.title.setText(title);
			String mLastKeyword = mSearchAdapter.mLastKeyword;
			fillText(mLastKeyword, title, this.title, 0);
			
			int time = item.getDuration();
			
			this.time.setText(formatTime(time));
			
			String imgUrl = TDImageWrap.getShortVideoPoster(item);
	        mImageLoader.displayImage(imgUrl, ivPoster, mOptions);
	        
	        this.itemView.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View paramView) {
		            playShotVideo(item);
				}
			});
	        
		}
		private void playShotVideo(CmsVedioBaseInfo info) {
			String imgUrl = TDImageWrap.getShortVideoPoster(info);
			String fileName = info.getTitle();
			int albumId = info.getId();
			int vedioId = info.getVideoId();
			log.debug("albumId=" + albumId + ",vedioId=" + vedioId);
			int mode = 0;
			CmsVedioBaseInfo mCmsVedioBaseInfo = info;
			if (mCmsVedioBaseInfo != null && mCmsVedioBaseInfo.films != null
					&& mCmsVedioBaseInfo.films.size() > 0) {
				mode = mCmsVedioBaseInfo.films.get(0).mode;
			}
			new PlayerEntranceMgr(mContext).startgetPlayUrl(null, mode, imgUrl, null, fileName, albumId, vedioId, ServiceArg.URL_TYPE_PLAY, false);
		}
		
		private String formatTime(int time){
			StringBuilder str = new StringBuilder();
			int h = time / 3600000;
			int m = (time - h*3600000) / 60000;
			int s = (time - h*3600000 - m * 60000) / 1000;
			
			if(m < 10){
				str.append("0");
			}
			str.append(m);
			str.append(":");
			
			if(s < 10){
				str.append("0");
			}
			str.append(s);
			return str.toString();
		}
		
		
	}

	public static class AppViewHolder extends BaseSearchViewHolder {
		public ImageView ivPoster;	// 海报
	    public TextView title;		// 片名
	    public TextView size_tv;	//
	    public TextView type;		// 类型
	    public TextView downloads_tv;		//
	    public View cut_line_layout;    // 下滑线
		public AppViewHolder(View view, BaseSearchAdapter adp, Context ctx) {
			super(view, adp, ctx);
			mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.game_detail_icon_h324).build();
	        ivPoster = (ImageView) view.findViewById(R.id.iv_search_result_list_item_poster);
	        title = (TextView) view.findViewById(R.id.tv_search_result_list_item_title);
	        size_tv = (TextView) view.findViewById(R.id.tv_search_result_list_item_size);
	        type = (TextView) view.findViewById(R.id.tv_search_result_list_item_type);
	        downloads_tv = (TextView) view.findViewById(R.id.tv_search_result_list_item_downloads);
	        cut_line_layout = (View) view.findViewById(R.id.cut_line_layout);
		}

		@Override
		public void onBindViewHolder(Object obj) {
			AppItemInfo item = null;
			boolean isEnd = false;
			try {
				Object objs[] = (Object[])obj;
				item = (AppItemInfo) objs[0];
				isEnd = (Boolean) objs[1];
			} catch(Exception e) {
				item = (AppItemInfo) obj;
			}
	        
			if(item == null)return;
			
			String score = item.getScore();
			if (null == score || "0".equals(score)) {
				score = "5.0";
			}
			if (score.length() == 1) {
				score += ".0";
			}
			
	    	// 设置海报
	    	ivPoster.setVisibility(View.VISIBLE);
	    	String imgUrl = TDImageWrap.getAppIcon(item);
	        mImageLoader.displayImage(imgUrl, ivPoster, mOptions);
	        // 填充文字
	        
	        String mLastKeyword = mSearchAdapter.mLastKeyword;
	        fillText(mLastKeyword, item.title, title, true, 0);
	        fillText(mLastKeyword, (item.fileSize / 1024 /1024)+"M", size_tv, false, 0);
	        fillText(mLastKeyword, item.installationTimes + StoreApplication.INSTANCE.getString(R.string.installNums), downloads_tv, false, 0);
	        ArrayList<String> lists = new ArrayList<String>();
	        for (AppItemInfo.AppGenre app : item.genres)
	            lists.add(app.title);
	        fillTexts(mLastKeyword, lists, type);
	        
	        cut_line_layout.setVisibility(isEnd ? View.INVISIBLE : View.VISIBLE);
	        
	        final AppItemInfo aInfo = item;
	        this.itemView.setOnClickListener(new OnClickListener() {				
				@Override
				public void onClick(View paramView) {
		            AppDetailActivity.start(mContext, itemView, aInfo);
				}
			});
		}
	}
}
