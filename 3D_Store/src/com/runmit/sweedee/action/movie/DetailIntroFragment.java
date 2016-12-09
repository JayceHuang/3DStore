package com.runmit.sweedee.action.movie;

import java.util.ArrayList;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
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
import com.runmit.sweedee.model.CmsVedioBaseInfo;
import com.runmit.sweedee.model.CmsVedioDetailInfo;
import com.runmit.sweedee.model.CmsVedioDetailInfo.Vedio;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.DirectListView;
import com.runmit.sweedee.view.EmptyView;


/**
 * 详情页-简介
 */
public class DetailIntroFragment extends BaseFragment implements AdapterView.OnItemClickListener {

    XL_Log log = new XL_Log(DetailIntroFragment.class);
    private TextView tv_movie_recommend;
    private LinearLayout movie_intro;
    private EmptyView mEmptyView;

    public DetailIntroFragment() {
    }

    private CmsVedioDetailInfo cmsVedioDetailInfo;//详情页的data对象
    private View rootView;
    private DirectListView mDirectListView;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        //让view只加载一次,避免重复加载造成的资源浪费
        if (rootView != null) {
            return onlyOneOnCreateView(rootView);
        }
        rootView = inflater.inflate(R.layout.frg_detail_intro, container, false);
        mDirectListView = (DirectListView) rootView.findViewById(R.id.detail_intro_films);
        tv_movie_recommend = (TextView) rootView.findViewById(R.id.tv_movie_recommend);
        movie_intro = (LinearLayout) rootView.findViewById(R.id.lly_movie_intro);
        mEmptyView = (EmptyView) rootView.findViewById(R.id.rl_empty_tip);
        mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPic(R.drawable.ic_failed);
        mEmptyView.setEmptyTop(mFragmentActivity, 70);
        if (cmsVedioDetailInfo == null) {
            mEmptyView.setStatus(EmptyView.Status.Loading);
        }
        initData();
        return rootView;
    }

    private void initData() {
        DetailActivity detailActivity = (DetailActivity) mFragmentActivity;
        detailActivity.addIntroListener(new DetailActivity.IntroDataCallBack() {
            @Override
            public void onIntroSuccess(int stateCode, CmsVedioDetailInfo mDetailInfo) {
                cmsVedioDetailInfo = mDetailInfo;
                if (stateCode == 0) {
                    if (mDetailInfo.summary != null) {
                        mEmptyView.setStatus(EmptyView.Status.Gone);
                        tv_movie_recommend.setText("        " + mDetailInfo.summary);
                    }
                    if (mDetailInfo.trailers.size() != 0 && mDetailInfo.trailers != null) {
                        mEmptyView.setStatus(EmptyView.Status.Gone);
                        movie_intro.setVisibility(View.VISIBLE);
                        mDirectListView.setAdapter(new MyAdapter());
                        mDirectListView.setOnItemClickListener(DetailIntroFragment.this);
                    }
                }
            }

            @Override
            public void onIntroFailed() {
                mEmptyView.setStatus(EmptyView.Status.Empty);
            }
        });
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        String poster = TDImageWrap.getShortVideoPoster(cmsVedioDetailInfo);
        Vedio mVedio = cmsVedioDetailInfo.trailers.get(position);
        new PlayerEntranceMgr(mFragmentActivity).startgetPlayUrl(null,mVedio.mode,poster, mVedio.subtitles, getString(R.string.detail_movie_prevue), mVedio.albumId, mVedio.videoId, ServiceArg.URL_TYPE_PLAY, false);
    }


    class MyAdapter extends BaseAdapter {
        private ImageLoader mImageLoader;
        private LayoutInflater mInflater;
        private DisplayImageOptions mOptions;

        public MyAdapter() {
            super();
            mInflater = LayoutInflater.from(mFragmentActivity);
            mImageLoader = ImageLoader.getInstance();
            mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_trailer_image_480x270).build();
        }

        @Override
        public int getCount() {
            return cmsVedioDetailInfo.trailers.size();
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public long getItemId(int position) {
            return 0;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder holder;
            if (convertView == null) {
                holder = new ViewHolder();
                convertView = mInflater.inflate(R.layout.movie_films_item, null);
                holder.films_bg = (ImageView) convertView.findViewById(R.id.iv_prevue_bg);
                holder.films_play = (ImageView) convertView.findViewById(R.id.iv_prevue_play);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }
            Vedio vedio = cmsVedioDetailInfo.trailers.get(position);
            if (vedio != null) {
                String imgUrl = TDImageWrap.getSnapshotPoster(vedio.videoId);
                log.debug("DetailIntroFragment imgUrl = " + imgUrl);
                if (imgUrl != null && imgUrl != "") {
                    holder.films_play.setVisibility(View.VISIBLE);
                    mImageLoader.displayImage(imgUrl, holder.films_bg, mOptions);
                }
            }
            return convertView;
        }

        class ViewHolder {
            ImageView films_bg;
            ImageView films_play;
        }
    }
}
