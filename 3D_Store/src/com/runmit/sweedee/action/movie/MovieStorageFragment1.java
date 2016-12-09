//package com.runmit.sweedee.action.movie;
//
//import android.content.Intent;
//import android.graphics.Color;
//import android.graphics.drawable.ColorDrawable;
//import android.os.Bundle;
//import android.os.Handler;
//import android.view.Gravity;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.view.ViewGroup;
//import android.view.WindowManager;
//import android.widget.AdapterView;
//import android.widget.BaseAdapter;
//import android.widget.Button;
//import android.widget.CompoundButton;
//import android.widget.GridView;
//import android.widget.ImageView;
//import android.widget.LinearLayout;
//import android.widget.PopupWindow;
//import android.widget.RadioButton;
//import android.widget.RadioGroup;
//import android.widget.RelativeLayout;
//import android.widget.TextView;
//
//import com.nostra13.universalimageloader.core.DisplayImageOptions;
//import com.nostra13.universalimageloader.core.ImageLoader;
//import com.runmit.sweedee.BaseFragment;
//import com.runmit.sweedee.DetailActivity;
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
//import com.runmit.sweedee.manager.CmsServiceManager;
//import com.runmit.sweedee.manager.data.EventCode;
//import com.runmit.sweedee.model.CmsVedioBaseInfo;
//import com.runmit.sweedee.model.VedioFilterType;
//import com.runmit.sweedee.util.XL_Log;
//import com.runmit.sweedee.view.EmptyView;
//
//import java.util.ArrayList;
//import java.util.List;
//
///**
// * 筛选页的备份，以后影片多了恢复该页面
// */
//public class MovieStorageFragment1 extends BaseFragment implements View.OnClickListener, AdapterView.OnItemClickListener {
//    XL_Log log = new XL_Log(MovieStorageFragment1.class);
//    private GridView mGridView;
//    private RelativeLayout rely_movie_choose_title;
//    private TextView movie_library_choose;
//    private RelativeLayout empty_view;
//    private PopupWindow mPopupWindow;
//    private RadioGroup rg_movie_choose_location;
//    private RadioGroup rg_movie_choose_rank;
//    private RadioGroup rg_movie_choose_type;
//    private RadioGroup rg_movie_choose_time;
//    private TextView tv_movie_choose_location;
//    private TextView tv_movie_choose_rank;
//    private TextView tv_movie_choose_type;
//    private TextView tv_movie_choose_time;
//    private View rootView;
//    private Button bt_movie_choose_ok;
//    private Button bt_movie_choose_cancle;
//
//    private Handler mHandler = new Handler() {
//        public void handleMessage(android.os.Message msg) {
//
//            switch (msg.what) {
//                case EventCode.CmsEventCode.EVENT_GET_RECOMMEND:
//                    List<CmsVedioBaseInfo> cmsVedioBaseInfos = (List<CmsVedioBaseInfo>) msg.obj;
//                    log.debug("cmsVedioBaseInfos = " + cmsVedioBaseInfos + ",stateCode = " + msg.arg1);
//                    int stateCode = msg.arg1;
//                    if (stateCode == 0 && cmsVedioBaseInfos != null) {
//                        mGridView.setAdapter(new MyAdapter(cmsVedioBaseInfos));
//                        mEmptyView.setStatus(EmptyView.Status.Gone);
//                    }else  {
//                        mEmptyView.setStatus(EmptyView.Status.Empty);
//                      
//                    }
//                    break;
//                case EventCode.CmsEventCode.EVENT_GET_FILTRATE_CONFIG:
//                    ArrayList<VedioFilterType> mVedioFilterType = (ArrayList<VedioFilterType>) msg.obj;
//                    log.debug("mVedioFilterType = " + mVedioFilterType + ",stateCode = " + msg.arg1 + " -- " + msg.arg2);
//                    int filterStateCode = msg.arg1;
//                    if (mVedioFilterType != null) {
//                        log.debug(" mVedioFilterType.size = " + mVedioFilterType.size() + "");
//                        //根据数组长度挨个传递对象进行加载,待改良
//                        dynamicAdd(tv_movie_choose_time, rg_movie_choose_time, mVedioFilterType.get(mVedioFilterType.size() - 1),null);
//                        dynamicAdd(tv_movie_choose_type, rg_movie_choose_type, mVedioFilterType.get(mVedioFilterType.size() - 2), Type.GENRES);
//                        dynamicAdd(tv_movie_choose_location, rg_movie_choose_location, mVedioFilterType.get(mVedioFilterType.size() - 3), Type.REGION);
//                        dynamicAdd(tv_movie_choose_rank, rg_movie_choose_rank, mVedioFilterType.get(mVedioFilterType.size() - 4),null);
//                    }
//                    break;
//            }
//        }
//    };
//
//    public enum Type {
//        //类型,地区
//        GENRES, REGION, LG, CT,
//    }
//
//    //动态加载按钮配置选项
//    public static void dynamicAdd(TextView title, RadioGroup rgItems, VedioFilterType mVdeioType, final Type mType) {
//        title.setText(mVdeioType.title);//设置标题
//        List<VedioFilterType.Item> mArray = mVdeioType.items;
//        for (VedioFilterType.Item item : mArray) {
//            RadioButton radioButton = new RadioButton(rgItems.getContext());
//            radioButton.setButtonDrawable(rgItems.getContext().getResources().getDrawable(android.R.color.transparent));
//            radioButton.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
//                @Override
//                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
//                    if (isChecked) {
//                        buttonView.setTextColor(buttonView.getContext().getResources().getColor(R.color.thin_blue));
//                        switch (mType){
//                            case GENRES:
//                                break;
//                            
//                            case REGION:
//                                break;
//                        }
//                    } else {
//                        buttonView.setTextColor(buttonView.getContext().getResources().getColor(R.color.movie_choose_text_bg));
//                    }
//                }
//            });
//            radioButton.setTextColor(rgItems.getContext().getResources().getColor(R.color.movie_choose_text_bg));
//            radioButton.setTextSize(16);
//            radioButton.setPadding(0, 0, 40, 0);
//            radioButton.setText(item.title);//设置每个item元素
//            LinearLayout.LayoutParams mListLayoutParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
//            mListLayoutParams.gravity = Gravity.CENTER_VERTICAL;
//            mListLayoutParams.width = LinearLayout.LayoutParams.WRAP_CONTENT;
//            mListLayoutParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;
//            rgItems.addView(radioButton, mListLayoutParams);
//        }
//    }
//
//    @Override
//    public void onCreate(Bundle savedInstanceState) {
//        super.onCreate(savedInstanceState);
//
//    }
//   private EmptyView mEmptyView;
//    @Override
//    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
//        rootView = inflater.inflate(R.layout.frg_movie_storage, container, false);
//        CmsServiceManager.getInstance().getMovieRecommend(20, 0, 20, mHandler);
//        mGridView = (GridView) rootView.findViewById(R.id.gv_movie_storage);
//        mEmptyView = (EmptyView) rootView.findViewById(R.id.rl_empty_tip);
//        mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPic(R.drawable.ic_watch_history);
//        mEmptyView.setStatus(EmptyView.Status.Loading);
//        mGridView.setOnItemClickListener(this);
//        rely_movie_choose_title = (RelativeLayout) rootView.findViewById(R.id.rely_movie_choose_title);
//        movie_library_choose = (TextView) rootView.findViewById(R.id.tv_movie_storage_choose);
//        movie_library_choose.setOnClickListener(this);
//
//        return rootView;
//    }
//
//    @Override
//    public void onClick(View v) {
//        switch (v.getId()) {
//            //筛选界面
//            case R.id.tv_movie_storage_choose:
//                CmsServiceManager.getInstance().getMovieFilteConfig(mHandler);//拉取筛选配置
//                View view = View.inflate(mFragmentActivity, R.layout.pop_movie_choose, null);
//                bt_movie_choose_ok = (Button) view.findViewById(R.id.bt_movie_choose_ok);
//                bt_movie_choose_cancle = (Button) view.findViewById(R.id.bt_movie_choose_cancle);
//                bt_movie_choose_ok.setOnClickListener(this);
//                bt_movie_choose_cancle.setOnClickListener(this);
//
//                rg_movie_choose_rank = (RadioGroup) view.findViewById(R.id.rg_movie_choose_rank);
//                rg_movie_choose_location = (RadioGroup) view.findViewById(R.id.rg_movie_choose_location);
//                rg_movie_choose_type = (RadioGroup) view.findViewById(R.id.rg_movie_choose_type);
//                rg_movie_choose_time = (RadioGroup) view.findViewById(R.id.rg_movie_choose_time);
//                tv_movie_choose_rank = (TextView) view.findViewById(R.id.tv_movie_choose_rank);
//                tv_movie_choose_location = (TextView) view.findViewById(R.id.tv_movie_choose_location);
//                tv_movie_choose_type = (TextView) view.findViewById(R.id.tv_movie_choose_type);
//                tv_movie_choose_time = (TextView) view.findViewById(R.id.tv_movie_choose_time);
//
//                rg_movie_choose_location.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
//                    @Override
//                    public void onCheckedChanged(RadioGroup group, int checkedId) {
//                        // TODO Auto-generated method stub  
////                        RadioButton tempButton = (RadioButton)group.findViewById(checkedId); // 通过RadioGroup的findViewById方法，找到ID为checkedID的RadioButton  
//                    }
//                });
//
//                mPopupWindow = new PopupWindow(view, WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT);
//                mPopupWindow.setBackgroundDrawable(new ColorDrawable(
//                        Color.TRANSPARENT));
//                mPopupWindow.showAtLocation(rootView, Gravity.TOP, 0, 0);
//                break;
//            case R.id.bt_movie_choose_ok:
//
//
//                if (mPopupWindow.isShowing() && mPopupWindow != null) {
//                    mPopupWindow.dismiss();
//                }
//                break;
//            case R.id.bt_movie_choose_cancle:
//                if (mPopupWindow.isShowing() && mPopupWindow != null) {
//                    mPopupWindow.dismiss();
//                }
//                break;
//        }
//    }
//
//
//    @Override
//    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
//        Intent intent = new Intent(mFragmentActivity, DetailActivity.class);
//        startActivity(intent);
//    }
//
//    class MyAdapter extends BaseAdapter {
//        private List<CmsVedioBaseInfo> cmsVedioBaseInfos;
//        private ImageLoader mImageLoader;
//        private LayoutInflater mInflater;
//        private DisplayImageOptions mOptions;
//
//        public MyAdapter(List<CmsVedioBaseInfo> c) {
//            super();
//            cmsVedioBaseInfos = c;
//            mImageLoader = ImageLoader.getInstance();
//            mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_movie_image).build();
//            mInflater = LayoutInflater.from(mFragmentActivity);
//        }
//
//        @Override
//        public int getCount() {
//            return cmsVedioBaseInfos.size();
//        }
//
//        @Override
//        public Object getItem(int position) {
//            return null;
//        }
//
//        @Override
//        public long getItemId(int position) {
//            return 0;
//        }
//
//        @Override
//        public View getView(int position, View convertView, ViewGroup parent) {
//            ViewHolder holder;
//            if (convertView == null) {
//                convertView = mInflater.inflate(R.layout.item_detail_gridview, null);
//                holder = new ViewHolder();
//                holder.tv_grade = (TextView) convertView.findViewById(R.id.tv_commend_grade);
//                holder.tv_name = (TextView) convertView.findViewById(R.id.tv_item_name);
//                holder.bg_logo = (ImageView) convertView.findViewById(R.id.iv_item_logo);
//                convertView.setTag(holder);
//            } else {
//                holder = (ViewHolder) convertView.getTag();
//            }
//            if (cmsVedioBaseInfos.get(position) != null) {
//                holder.tv_name.setText(cmsVedioBaseInfos.get(position).title);
//                holder.tv_grade.setText(String.valueOf(cmsVedioBaseInfos.get(position).score));
//                //设置图片
//                String imgUrl = cmsVedioBaseInfos.get(position).getPosterUrl(CmsVedioBaseInfo.PosterType.POSTER);
//                mImageLoader.displayImage(imgUrl, holder.bg_logo, mOptions);
//            }
//            return convertView;
//        }
//
//        class ViewHolder {
//            TextView tv_name;
//            TextView tv_grade;
//            ImageView bg_logo;
//        }
//    }
//}
