package com.runmit.sweedee.action.movie;

import java.util.List;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.AbsoluteSizeSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.DetailActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.data.EventCode;
import com.runmit.sweedee.model.CmsItemable;
import com.runmit.sweedee.model.CmsVedioBaseInfo;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.EmptyView;


/**
 * 详情页-相关
 */
public class DetailAboutFragment extends BaseFragment implements AdapterView.OnItemClickListener , StoreApplication.OnNetWorkChangeListener {

    private XL_Log log = new XL_Log(DetailAboutFragment.class);
    private GridView mGridView;
    private EmptyView mEmptyView;
    private View rootView;

    public DetailAboutFragment() {
    	
    }
    List<CmsVedioBaseInfo> cmsVedioBaseInfos;
    private Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
                case EventCode.CmsEventCode.EVENT_GET_RECOMMEND:
                    int stateCode = msg.arg1;
                    if (stateCode == 0 && msg.obj != null) {
                        if(cmsVedioBaseInfos==null){
                            cmsVedioBaseInfos = (List<CmsVedioBaseInfo>) msg.obj;
                            log.debug("cmsVedioBaseInfos = " + cmsVedioBaseInfos + ",stateCode = " + msg.arg1);
                            if(cmsVedioBaseInfos.equals("[]")){
                                mEmptyView.setStatus(EmptyView.Status.Empty);
                            }
                            MyAdapter myAdapter = new MyAdapter(cmsVedioBaseInfos);
                            mGridView.setAdapter(myAdapter);
                            mEmptyView.setStatus(EmptyView.Status.Gone);
                        }
                    } else {
                        mEmptyView.setStatus(EmptyView.Status.Empty);
                    }
                    break;
            }
        }
    };


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        //让view只加载一次,避免重复加载造成的资源浪费
        if (rootView != null) {
            return onlyOneOnCreateView(rootView);
        }
        rootView = inflater.inflate(R.layout.frg_detail_about, container, false);
        mGridView = (GridView) rootView.findViewById(R.id.gv_detail_about);
        mEmptyView = (EmptyView) rootView.findViewById(R.id.rl_empty_tip);
        mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPic(R.drawable.ic_failed);
        mEmptyView.setStatus(EmptyView.Status.Loading);
        mEmptyView.setEmptyTop(mFragmentActivity, 70);
        mGridView.setOnItemClickListener(this);
        initData();
        StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
        return rootView;
    }

    private void initData() {
        int albumid = getArguments().getInt("albumid");
        if (albumid == -1) {
            Util.showToast(mFragmentActivity, getString(R.string.no_albumid), Toast.LENGTH_SHORT);
            mEmptyView.setStatus(EmptyView.Status.Empty);
        } else {
            CmsServiceManager.getInstance().getMovieRecommend(albumid, 0, 20, mHandler);
        }
    }

    boolean flag = false;

    @Override
    public void onChange(boolean isNetWorkAviliable, StoreApplication.OnNetWorkChangeListener.NetType mNetType) {
        if (isNetWorkAviliable && !flag) {
            initData();
            flag = true;
        } else {
            flag = false;
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, final View view, int position, long id) {
        CmsItemable item = (CmsItemable) parent.getAdapter().getItem(position);
        DetailActivity.start(mFragmentActivity, view, item.getId());
       
        //清理详情页的任务栈  
        //这里延时的理由是 当从详情页启动详情页是，如果马上关闭之前的详情页，会造成动画显示问题
        mHandler.postDelayed(new Runnable() {			
			@Override
			public void run() {
				DetailActivity detailActivity = (DetailActivity) mFragmentActivity;
				detailActivity.realFinish();
			}
		}, 1000);
        
    }

    class MyAdapter extends BaseAdapter {
        private List<CmsVedioBaseInfo> cmsVedioBaseInfos;
        private ImageLoader mImageLoader;
        private LayoutInflater mInflater;
        private DisplayImageOptions mOptions;

        public MyAdapter(List<CmsVedioBaseInfo> c) {
            super();
            cmsVedioBaseInfos = c;
            mImageLoader = ImageLoader.getInstance();
            mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_movie_image_white).build();
            mInflater = LayoutInflater.from(mFragmentActivity);
        }

        @Override
        public int getCount() {
            return cmsVedioBaseInfos.size();
        }

        @Override
        public CmsVedioBaseInfo getItem(int position) {
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
                convertView = mInflater.inflate(R.layout.item_detail_gridview, parent,false);
                holder = new ViewHolder();
                holder.tv_grade = (TextView) convertView.findViewById(R.id.tv_commend_grade);
                holder.tv_name = (TextView) convertView.findViewById(R.id.tv_item_name);
                holder.bg_logo = (ImageView) convertView.findViewById(R.id.iv_item_logo);
                holder.item_highlight = (TextView) convertView.findViewById(R.id.tv_highlight);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }
            CmsVedioBaseInfo cvbi = getItem(position);            
            if (cvbi != null) {
                holder.tv_name.setText(cvbi.title);
                
                String score = cvbi.getScore();
				Spannable WordtoSpan = new SpannableString(score);
				WordtoSpan.setSpan(new AbsoluteSizeSpan(14, true), 0, 1, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				WordtoSpan.setSpan(new AbsoluteSizeSpan(10, true), 1, 3, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				holder.tv_grade.setText(WordtoSpan);
				
                String hlight = cvbi.getHighlight() ;
                if(hlight != null){
                	holder.item_highlight.setText(hlight);	
                }                  
                String imgUrl = TDImageWrap.getDetailRecommendPoster(cvbi);
                mImageLoader.displayImage(imgUrl, holder.bg_logo, mOptions);
            }
            return convertView;
        }

        class ViewHolder {
            TextView tv_name;
            TextView tv_grade;
            ImageView bg_logo;
            TextView item_highlight;
        }
    }
}
