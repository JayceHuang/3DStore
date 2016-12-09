package com.runmit.sweedee.action.movie;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;

import android.content.Context;
import android.content.Intent;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.FragmentActivity;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.PopupWindow.OnDismissListener;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.runmit.sweedee.DetailActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.constant.ServiceArg;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.PlayerEntranceMgr;
import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
import com.runmit.sweedee.model.CmsVideoCatalog;
import com.runmit.sweedee.model.CmsVideoCatalog.Catalog;
import com.runmit.sweedee.model.SortHotWordInfo;
import com.runmit.sweedee.model.SortHotWordInfo.ConditionInfo;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.CategoryView;
import com.runmit.sweedee.view.CategoryView.OnClickCategoryListener;
import com.runmit.sweedee.view.EmptyView;
import com.runmit.sweedee.view.EmptyView.Status;
import com.runmit.sweedee.view.MyListView;
import com.runmit.sweedee.view.MyListView.FootStatus;
import com.runmit.sweedee.view.MyListView.OnLoadMoreListenr;
import com.runmit.sweedee.view.MyListView.OnRefreshListener;

public class MovieCatalogActivity extends FragmentActivity {
	
	private XL_Log log = new XL_Log(MovieCatalogActivity.class);
    private static final String ALBUMID = "albumid";
    private static final int PAGESIZE = 20;
    private static final int SORTERTEXTSIZE = 16;
    private static final int FILTERTEXTSIZE = 14;
    private SortHotWordInfo mSortHotWordInfo;
    private GridView mGridView;
    private MyListView mListView;
	private CatalogAdapter mAdapter;
	private LinkedList<String> mListItems;
	private CmsVideoCatalog mCmsVideoCatalog;
	private PopupWindow mPopupWindow;
	private TextView mFilterButton;
	private LinearLayout mSortContainer;
	private List<String> mLinkList;
	private String mCurrentSortURL;
	private HashMap<String,String> mCurrentFilterURL;
	private int mCurrentPageIndex=0;
	private EmptyView mEmptyView;
	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case CmsEventCode.EVENT_GET_ALBUMS:
				if (msg.arg1== 0) {
					CmsVideoCatalog cmsVideoCatalog = (CmsVideoCatalog) msg.obj;	
					LoadData(cmsVideoCatalog.start!=0,cmsVideoCatalog);		
					log.debug("totalcount:"+cmsVideoCatalog.total);
				} else {	
					mCmsVideoCatalog=null;
					mAdapter.notifyDataSetChanged();
					mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPicVisibility(View.VISIBLE);
					mEmptyView.setStatus(Status.Empty);
				}
				break;
			}
			mListView.onRefreshComplete();
		}
	};
	@Override
	protected void onCreate(Bundle arg0) {
		// TODO Auto-generated method stub
		super.onCreate(arg0);
		setContentView(R.layout.activity_moviecatalog);
	    getActionBar().setDisplayHomeAsUpEnabled(true);
	    mSortHotWordInfo=(SortHotWordInfo) getIntent().getSerializableExtra(SortHotWordInfo.class.getSimpleName());
	    mListView = (MyListView) findViewById(R.id.pull_refresh_List);
	    setTitle(mSortHotWordInfo.title);
	    mListView.setBackgroundResource(R.color.white);
	   // mListView.setPadding( getResources().getDimensionPixelSize(R.dimen.movie_refreshgridview_marginleft), getResources().getDimensionPixelSize(R.dimen.movie_refreshgridview_margintop), getResources().getDimensionPixelSize(R.dimen.movie_refreshgridview_marginleft), 0);	
	    mListView.setonRefreshListener(new OnRefreshListener() {
			@Override
			public void onRefresh() {
				CmsServiceManager.getInstance().getMovieBySorterAndrFilter(appendFilterURL(0,PAGESIZE),mHandler);	
			}
		});

	    mListView.setOnLoadMoreListenr(new OnLoadMoreListenr() {

			@Override
			public void onLoadData() {
			CmsServiceManager.getInstance().getMovieBySorterAndrFilter(appendFilterURL(mCmsVideoCatalog.list.size(),PAGESIZE),mHandler);	
			}
		});
	    mEmptyView = new EmptyView(this);
	    mEmptyView.setEmptyTip(R.string.loading).setEmptyPicVisibility(View.GONE);
	    mEmptyView.setGravity(Gravity.CENTER);
		((ViewGroup)mListView.getParent()).addView(mEmptyView);  
		mListView.setEmptyView(mEmptyView);
		mListView.setOverScrollMode(View.OVER_SCROLL_NEVER);
		mAdapter = new CatalogAdapter();	
		mListView.setAdapter(mAdapter);
		mFilterButton=(TextView)findViewById(R.id.button_catalog_filter);
		mFilterButton.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				mPopupWindow.showAsDropDown(mSortContainer,0,getResources().getDimensionPixelOffset(R.dimen.movie_catalog_popwindow_offset));  
				mFilterButton.setCompoundDrawablesWithIntrinsicBounds(null,null,getResources().getDrawable(R.drawable.ic_filter_up),null);
			}} );
		mSortContainer=(LinearLayout) findViewById(R.id.moviecatalog_ll_sortcontainer);	
		initFilterState();
		initSortFilter();
		initPopuptWindow();
	
	}
	
	private void LoadData(final Boolean isLoadMore,final CmsVideoCatalog mData){
		mHandler.post(new Runnable() {
			@Override
			public void run() {		
				
				if(isLoadMore){
					if(mCmsVideoCatalog!=null){
						if(mData==null){
							mListView.setFootStatus(FootStatus.Error);
							return;
						}	
					    mCmsVideoCatalog.list.addAll(mData.list);
					}else{
						mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPicVisibility(View.VISIBLE);
						mEmptyView.setStatus(Status.Empty);
						return;
				    }
					}else{
					   mCmsVideoCatalog=mData;
					}
				    if(mCmsVideoCatalog.list.size()==0){
				       mEmptyView.setEmptyTip(R.string.moviecatalog_no_data_info).setEmptyPicVisibility(View.VISIBLE);
					   mEmptyView.setStatus(Status.Empty);
				    }
				    mAdapter.notifyDataSetChanged();
				    if(!isLoadMore){
					 mListView.setSelection(0);	
				    }
					if (mCmsVideoCatalog.list.size() < mCmsVideoCatalog.total) {
						mListView.setFootStatus(FootStatus.Common);
					} else {
						mListView.setFootStatus(FootStatus.Gone);
					}				
				
				}
			});
	}
	
	protected void initFilterState(){		
		mCurrentSortURL=mSortHotWordInfo.links.get(0).link;
		log.debug("currenturl"+mCurrentSortURL);
		mCurrentFilterURL=new HashMap();
	}
	
	protected void initSortFilter(){
		CategoryView categoryView = new CategoryView(this);
	    categoryView.setClickable(true);
	    categoryView.backgroundResID=R.color.toggle_moviecatalog_sorter_selector;
	    categoryView.isAddAll=false;
	    categoryView.textSize=SORTERTEXTSIZE;
	    categoryView.radioMargin=19;
	    categoryView.setLayoutParams(new ViewGroup.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
	    mLinkList = new ArrayList<String>();
		for(int i=0;i<mSortHotWordInfo.links.size();i++){
			mLinkList.add(mSortHotWordInfo.links.get(i).label);
		 }
		categoryView.add(mLinkList);
		categoryView.setOnClickCategoryListener(new OnClickCategoryListener(){

			@Override
			public void click(RadioGroup group, int checkedId) {
				// TODO Auto-generated method stub	
			RadioButton radioButton=(RadioButton)group.findViewById(checkedId);
			log.debug("radioindex:"+radioButton.getTag().toString());
			mCurrentSortURL=mSortHotWordInfo.links.get(Integer.parseInt(radioButton.getTag().toString())).link;
			log.debug("currentsorturl:"+mCurrentSortURL);
			CmsServiceManager.getInstance().getMovieBySorterAndrFilter(appendFilterURL(0,PAGESIZE),mHandler);
			}});
		categoryView.setDefaultChecked();
		mSortContainer.addView(categoryView);
	}
	
	private OnClickCategoryListener mOnClickCategoryListener=new OnClickCategoryListener(){
		@Override
		public void click(RadioGroup group, int checkedId) {
			// TODO Auto-generated method stub	
		int filterIndex=Integer.parseInt(group.getTag().toString());
		RadioButton radioButton=(RadioButton)group.findViewById(checkedId);
		int conditionIndex=Integer.parseInt(radioButton.getTag().toString());
		mCurrentFilterURL.put(filterIndex+"", mSortHotWordInfo.filters.get(filterIndex).conditions.get(conditionIndex).link);		
		CmsServiceManager.getInstance().getMovieBySorterAndrFilter(appendFilterURL(0,PAGESIZE),mHandler);
		}};
		
	 protected void initPopuptWindow() {  
	      ViewGroup popupWindow_view = (ViewGroup) getLayoutInflater().inflate(R.layout.activity_popupwindow_moviecatalog, null,false);  
	      popupWindow_view.setLayoutParams(new ViewGroup.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
	      for(int i=0;i<mSortHotWordInfo.filters.size();i++){
	    	  ConditionInfo conditionInfo=mSortHotWordInfo.filters.get(i);
	    	  CategoryView categoryView = new CategoryView(this);
	    	  categoryView.setBackgroundResource(R.color.white);
	    	  categoryView.backgroundResID=R.color.toggle_moviecatalog_filter_selector;
		      categoryView.setClickable(true);
		      categoryView.isAddAll=false;
		      categoryView.textSize=FILTERTEXTSIZE;
		      categoryView.radioMargin=18;
		      categoryView.setLayoutParams(new ViewGroup.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
		      List<String> mList = new ArrayList<String>();
		      for(int j=0;j<conditionInfo.conditions.size();j++){
		    	  mList.add(conditionInfo.conditions.get(j).label);
		      }
		      categoryView.label=mSortHotWordInfo.filters.get(i).label;
		      categoryView.labelMargin=12;
		      categoryView.add(mList);
		      categoryView.setTag(i);
			  categoryView.setOnClickCategoryListener(mOnClickCategoryListener);
			  categoryView.setDivierVisible(View.VISIBLE);
		      popupWindow_view.addView(categoryView);   	  
	      }
	      mPopupWindow = new PopupWindow(popupWindow_view,LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT, true);  
	      mPopupWindow.setBackgroundDrawable(new BitmapDrawable());
	      mPopupWindow.setOutsideTouchable(true); 
	      mPopupWindow.setOnDismissListener(new OnDismissListener(){

			@Override
			public void onDismiss() {
				// TODO Auto-generated method stub
			    mFilterButton.setCompoundDrawablesWithIntrinsicBounds(null,null,getResources().getDrawable(R.drawable.ic_filter_down),null);
			}
	    	  
	      });

	    }  
	 
	 @Override
	 public boolean onOptionsItemSelected(MenuItem item) {
	        switch (item.getItemId()) {
	            case android.R.id.home:
	                finish();
	                break;
	            default:
	                break;
	        }
	        return super.onOptionsItemSelected(item);
	    }
	 
	 
	  private String appendFilterURL(int pageIndex,int pageCount){
           String filterURL=mCurrentSortURL;
           for(String key :mCurrentFilterURL.keySet()){
           filterURL+="&"+mCurrentFilterURL.get(key);
           }
           filterURL+="&start="+pageIndex+"&rows="+pageCount;
           return filterURL;
	   }
	
	class CatalogAdapter extends BaseAdapter {
		private LayoutInflater mInflater;
		private ImageLoader mImageLoader;
		DisplayImageOptions mOptions;
		public CatalogAdapter() {
			super();
			mInflater = LayoutInflater.from(MovieCatalogActivity.this);
			mImageLoader = ImageLoader.getInstance();
			mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.defalut_movie_small_item).build();
		}

		@Override
		public int getCount() {
			int count;
			if(mCmsVideoCatalog==null){
				count=0;
			}else{
				if(mCmsVideoCatalog.list.size()%2==0){
					count=mCmsVideoCatalog.list.size()/2;
				}else{
					count=mCmsVideoCatalog.list.size()/2+1;
				}
			}
			log.debug("cmsvideocatalog:"+count);
			return count;
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(final int position, View convertView, ViewGroup parent) {
			log.debug("positionindex:"+position*2);
			final ViewHolder holder;
			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.item_movie_catalog, parent, false);
				holder = new ViewHolder();
				holder.img_logo = (ImageView) convertView.findViewById(R.id.item_moviecatalog_img_logo);
				holder.tv_title = (TextView) convertView.findViewById(R.id.item_moviecatalog_tv_title);
				holder.time_title = (TextView) convertView.findViewById(R.id.item_moviecatalog_time_title);
				holder.img_logo1 = (ImageView) convertView.findViewById(R.id.item_moviecatalog_img_logo1);
				holder.tv_title1 = (TextView) convertView.findViewById(R.id.item_moviecatalog_tv_title1);
				holder.time_title1 = (TextView) convertView.findViewById(R.id.item_moviecatalog_time_title1);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}		
			holder.tv_title.setText("");	
			holder.tv_title1.setText("");	
			
			final Catalog leftCatalog=mCmsVideoCatalog.list.get(position*2);
	      
			View parentView=(View) holder.img_logo.getParent();
			String title =leftCatalog.title ;
            if(title != null){
            	holder.tv_title.setText(title);	
            }  
            if(leftCatalog != null && leftCatalog.films != null && leftCatalog.films.size() > 0){
            	holder.time_title.setText(cuvertTime(leftCatalog.films.get(0).duration));
            }
            String imgUrl = TDImageWrap.getShortVideoPoster(leftCatalog);	
            mImageLoader.displayImage(imgUrl,holder.img_logo,mOptions);
			parentView.setOnClickListener(new OnClickListener() {
				
				@Override
				public void onClick(View v) {
					// TODO Auto-generated method stub
				     if(leftCatalog.light){
				    	 Catalog mCmsCatalog=leftCatalog;
				    	 String imgUrl = TDImageWrap.getShortVideoPoster(mCmsCatalog);
						 String fileName = mCmsCatalog.getTitle();
						 int albumId = mCmsCatalog.getId();
						 int vedioId = mCmsCatalog.getVideoId();
						 log.debug("albumId="+albumId+",vedioId="+vedioId);
						 int mode = 0;
						 if(mCmsCatalog != null && mCmsCatalog.films != null && mCmsCatalog.films.size() > 0){
						 mode = mCmsCatalog.films.get(0).mode;
						 }
					     new PlayerEntranceMgr(MovieCatalogActivity.this).startgetPlayUrl(null, mode, imgUrl, null, fileName, albumId, vedioId, ServiceArg.URL_TYPE_PLAY, false);						 
				     }else{
					     Intent intent = new Intent(MovieCatalogActivity.this, DetailActivity.class);
					     int albumId=leftCatalog.albumId;
				         intent.putExtra(ALBUMID, albumId);
				         MovieCatalogActivity.this.startActivity(intent);
				     }
				}
			});
			
			parentView=(View) holder.img_logo1.getParent();	
			if((position*2+1)<=mCmsVideoCatalog.list.size()-1){		
				
				final Catalog rightCatalog=mCmsVideoCatalog.list.get(position*2+1);
				if(parentView.getVisibility()==View.INVISIBLE){
				  parentView.setVisibility(View.VISIBLE); 
				}
				title =rightCatalog.title ;
	            if(title != null){
	            	holder.tv_title1.setText(title);	
	            }
	            if(rightCatalog != null && rightCatalog.films != null && rightCatalog.films.size() > 0){
	            	holder.time_title1.setText(cuvertTime(rightCatalog.films.get(0).duration));
	            }
	            imgUrl = TDImageWrap.getShortVideoPoster(rightCatalog);	
	            mImageLoader.displayImage(imgUrl,holder.img_logo1,mOptions);	
				parentView.setOnClickListener(new OnClickListener() {
						
						@Override
						public void onClick(View v) {
							// TODO Auto-generated method stub
							// TODO Auto-generated method stub
							   if(rightCatalog.light){
							    	 Catalog mCmsCatalog=rightCatalog;
							    	 String imgUrl = TDImageWrap.getShortVideoPoster(mCmsCatalog);
									 String fileName = mCmsCatalog.getTitle();
									 int albumId = mCmsCatalog.getId();
									 int vedioId = mCmsCatalog.getVideoId();
									 log.debug("albumId="+albumId+",vedioId="+vedioId);
									 int mode = 0;
									 if(mCmsCatalog != null && mCmsCatalog.films != null && mCmsCatalog.films.size() > 0){
									 mode = mCmsCatalog.films.get(0).mode;
									 }
								     new PlayerEntranceMgr(MovieCatalogActivity.this).startgetPlayUrl(null, mode, imgUrl, null, fileName, albumId, vedioId, ServiceArg.URL_TYPE_PLAY, false);						 
							     }else{
								     Intent intent = new Intent(MovieCatalogActivity.this, DetailActivity.class);
								     int albumId=rightCatalog.albumId;
							         intent.putExtra(ALBUMID, albumId);
							         MovieCatalogActivity.this.startActivity(intent);
							     }
						}
					});
	
			}else{
			    parentView.setVisibility(View.INVISIBLE); 
			}
          
			return convertView;
		}

		@Override
		public Object getItem(int position) {
			// TODO Auto-generated method stub
			return null;
		}
		
		class ViewHolder {
			ImageView img_logo;
			TextView  tv_title;
			TextView  time_title;
			ImageView img_logo1;
			TextView  tv_title1;
			TextView  time_title1;
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
