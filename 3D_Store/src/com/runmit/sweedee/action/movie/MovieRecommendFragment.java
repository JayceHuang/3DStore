//package com.runmit.sweedee.action.movie;
//
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.action.home.HeaderFragment;
//import com.runmit.sweedee.manager.CmsServiceManager;
//import com.runmit.sweedee.manager.InitDataLoader;
//import com.runmit.sweedee.manager.InitDataLoader.LoadEvent;
//import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
//
//import android.os.Bundle;
//import android.os.Handler;
//import android.os.Message;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.view.ViewGroup;
//
///**
// * 电影-推荐页面
// */
//public class MovieRecommendFragment extends HeaderFragment{
//	
//	private boolean isLoadData = false;
//	
//	private Handler mhandler = new Handler() {
//		public void handleMessage(Message msg) {
//			switch (msg.what) {
//				case CmsEventCode.EVENT_GET_HOME:
//					onLoadFinshed(msg.arg1, false, msg.obj);
//					break;
//			}
//		}
//	};
//	
//	@Override
//	public void onViewCreated(View view, Bundle savedInstanceState) {
//		super.onViewCreated(view, savedInstanceState);
//		if(!isLoadData){
//			isLoadData = true;
//			CmsServiceManager.getInstance().getModuleData(CmsServiceManager.MODULE_MOVIE, mhandler);
//		}
//	}
//}
//
////public class MovieRecommendFragment extends SlidHeaderFragment implements AdapterView.OnItemClickListener ,StoreApplication.OnNetWorkChangeListener{
////
////    private XL_Log log = new XL_Log(MovieRecommendFragment.class);
////    private CmsModuleInfo mCmsHomeDataInfo;
////    private GuideGallery mGuideGallery;
////    private ImageLoader mImageLoader;
////    private DisplayImageOptions mOptions;
////    private ListView mListView;
////    private List<CmsModuleInfo.CmsComponent> mListComponents = new ArrayList<CmsModuleInfo.CmsComponent>();
////
////    public MovieRecommendFragment() {
////
////    }
////
////    private Handler mhandler = new Handler() {
////        public void handleMessage(Message msg) {
////            switch (msg.what) {
////                case CmsEventCode.EVENT_GET_HOME:
////                    log.debug("CmsEventCode.EVENT_GET_HOME ret = " + msg.arg1);
////                    int stateCode = msg.arg1;
////                    if (msg.obj != null && stateCode == 0) {
////                        mCmsHomeDataInfo = (CmsModuleInfo) msg.obj;
////                        log.debug(" mCmsHomeDataInfo = " + mCmsHomeDataInfo + " stateCode = " + stateCode + " arg2 = " + msg.arg2);
////                        init(mCmsHomeDataInfo);
////                        mEmptyView.setStatus(EmptyView.Status.Gone);
////                    } else {
////                        mEmptyView.setStatus(EmptyView.Status.Empty);
////                    }
////                    break;
////            }
////        }
////    };
////
////    private void init(CmsModuleInfo cmsModuleInfo) {
////        if (cmsModuleInfo == null || cmsModuleInfo.data == null || cmsModuleInfo.data.isEmpty()) {
////            return;//数据为空
////        }
////        //第一个展示位给轮播图        
////        CmsModuleInfo.CmsComponent mTopComponet = cmsModuleInfo.data.get(0);
////        mGuideGallery.setDataSource(mTopComponet);
////        //第二个位展示ListView,listView用GridView填充
////        int length = cmsModuleInfo.data.size();
////        if (length > 1) {
////            mListComponents.clear();
////            mListComponents.addAll(cmsModuleInfo.data.subList(1, length));
////            if (mListAdapter == null) {
////                mListAdapter = new ListAdapter();
////                mListView.setFocusable(false);
////                mListView.setDivider(null);
////                mListView.setAdapter(mListAdapter);
////            } else {
////                mListAdapter.notifyDataSetChanged();
////            }
////        }
////    }
////    
////    public void onViewPagerResume() {
////        super.onViewPagerResume();
////        if (mGuideGallery != null) {
////            mGuideGallery.onResume();
////        }
////    }
////
////    @Override
////    public void onPause() {
////        super.onPause();
////    }
////
////    @Override
////    public void onViewPagerStop() {
////        super.onViewPagerStop();
////        if(mGuideGallery!=null){
////        	mGuideGallery.onStop();
////        }
////    }
////
////    private ListAdapter mListAdapter;
////
////    class ListAdapter extends BaseAdapter {
////
////        @Override
////        public int getCount() {
////            return mListComponents.size();
////        }
////
////        @Override
////        public CmsModuleInfo.CmsComponent getItem(int position) {
////            return mListComponents.get(position);
////        }
////
////        @Override
////        public long getItemId(int position) {
////            return position;
////        }
////
////        @Override
////        public View getView(int position, View convertView, ViewGroup parent) {
////            CViewHolder viewHolder = null;
////            if (convertView == null) {
////                convertView = mInflater.inflate(R.layout.item_movie_recommend_listview, null);
////                viewHolder = new CViewHolder();
////                viewHolder.tvTitle = (TextView) convertView.findViewById(R.id.title_textview);
////                viewHolder.gvContent = (GridView) convertView.findViewById(R.id.gridview);
////                viewHolder.gvContent.setFocusable(false);
////                viewHolder.gvContent.setOnItemClickListener(MovieRecommendFragment.this);
////                viewHolder.gvContent.setSelector(R.drawable.gridview_selector);
////                convertView.setTag(viewHolder);
////            } else {
////                viewHolder = (CViewHolder) convertView.getTag();
////            }
////            CmsModuleInfo.CmsComponent ccp = getItem(position);
////            viewHolder.tvTitle.setText(ccp.title);
////            viewHolder.gvContent.setAdapter(new MyAdapter(mFragmentActivity, ccp));
////            return convertView;
////        }
////
////        class CViewHolder {
////            public TextView tvTitle;
////            public GridView gvContent;
////        }
////    }
////
////
////    private EmptyView mEmptyView;
////    private View rootView;
////
////
////    @Override
////    public View onCreateHeaderView(LayoutInflater inflater, ViewGroup container) {
////        log.debug(" recommend onCreateHeaderView ");
////        View headView = inflater.inflate(R.layout.frg_movie_recommend_head, container, false);
////        headView.setBackgroundResource(R.color.transparent);
////        mGuideGallery = (GuideGallery) headView.findViewById(android.R.id.background);
////        return headView;
////    }
////
////    protected LayoutInflater mInflater;
////
////    @Override
////    public View onCreateContentView(LayoutInflater inflater, ViewGroup container) {
////        log.debug(" recommend onCreateContentView ");
////        rootView = inflater.inflate(R.layout.frg_movie_recommend_scroll_view, container, false);
////
////        mListView = (ListView) rootView.findViewById(R.id.component_listview);
////        mInflater = LayoutInflater.from(mFragmentActivity);
////        mEmptyView = (EmptyView) rootView.findViewById(R.id.empty_view);
////        mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPic(R.drawable.ic_failed);
////        mEmptyView.setStatus(EmptyView.Status.Loading);
////        mEmptyView.setEmptyTop(mFragmentActivity, 80);
////        mImageLoader = ImageLoader.getInstance();
////        mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_movie_image).build();
////        initData();
////        StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
////        return rootView;
////    }
////
////    private void initData() {
////        CmsServiceManager.getInstance().getModuleData(CmsServiceManager.MODULE_MOVIE, mhandler);
////    }
////
////    boolean isChange = false;
////
////    public void onChange(boolean isNetWorkAviliable, StoreApplication.OnNetWorkChangeListener.NetType mNetType) {
////        if (isNetWorkAviliable && !isChange) {
////            initData();
////            isChange = true;
////        } else {
////            isChange = false;
////        }
////    }
////
////    @Override
////    public void onAttach(Activity activity) {
////        super.onAttach(activity);
////    }
////
////    @Override
////    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
////        CmsItemable item = (CmsItemable) parent.getAdapter().getItem(position);
////        DetailActivity.start(mFragmentActivity, item.getId(),view);
////    }
////
////
////    private class MyAdapter extends BaseAdapter {
////
////        protected ArrayList<CmsModuleInfo.CmsItem> items;
////
////        public MyAdapter(Context context, CmsModuleInfo.CmsComponent ccomponent) {
////            this.items = ccomponent.contents;
////        }
////
////        public int getCount() {
////            return items.size();
////        }
////
////        @Override
////        public CmsItemable getItem(int position) {
////            return items.get(position).item;
////        }
////
////        public View getView(final int position, View convertView, ViewGroup parent) {
////            ViewHolder viewHolder = null;
////            if (convertView == null) {
////                convertView = mInflater.inflate(R.layout.item_detail_gridview, null);
////                viewHolder = new ViewHolder();
////                viewHolder.item_logo = (ImageView) convertView.findViewById(R.id.iv_item_logo);
////                viewHolder.item_grade = (TextView) convertView.findViewById(R.id.tv_commend_grade);
////                viewHolder.item_name = (TextView) convertView.findViewById(R.id.tv_item_name);
////                viewHolder.item_highlight = (TextView) convertView.findViewById(R.id.tv_highlight);
////                convertView.setTag(viewHolder);
////            } else {
////                viewHolder = (ViewHolder) convertView.getTag();
////            }
////            CmsItemable item = getItem(position);            
////            if (item != null) {
////                viewHolder.item_name.setText(item.getTitle());
////                viewHolder.item_grade.setText(item.getScore());
////                String hlight = item.getHighlight() ;
////                if(hlight != null){
////                	viewHolder.item_highlight.setText(hlight);	
////                }
////                String imgUrl = TDImageWrap.getPosterImgUrl(item, null);
////                mImageLoader.displayImage(imgUrl, viewHolder.item_logo, mOptions);
////            }
////            return convertView;
////        }
////
////        class ViewHolder {
////            public ImageView item_logo;
////            public TextView item_grade;
////            public TextView item_name;
////            public TextView item_highlight;
////        }
////
////        @Override
////        public long getItemId(int position) {
////            return position;
////        }
////
////    }
////}
