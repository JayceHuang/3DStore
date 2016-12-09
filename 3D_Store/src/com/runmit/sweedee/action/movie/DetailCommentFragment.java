package com.runmit.sweedee.action.movie;

import android.os.Bundle;
import android.os.Handler;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.data.EventCode;
import com.runmit.sweedee.model.CmsVedioComment;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.EmptyView;

import java.util.List;

/**
 * 详情页-评论
 */
public class DetailCommentFragment extends BaseFragment implements  StoreApplication.OnNetWorkChangeListener {
    XL_Log log = new XL_Log(DetailCommentFragment.class);
    private RecyclerView recyclerView;
    private EmptyView mEmptyView;
    List<CmsVedioComment> cmsVedioComment;
    public DetailCommentFragment() {
        log.debug("检测创建 DetailCommentFragment Create");
    }

    private Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
                case EventCode.CmsEventCode.EVENT_GET_MOVIE_COMMENTS:
                    int stateCode = msg.arg1;
                    if (stateCode == 0 && msg.obj != null) {
                        if(cmsVedioComment==null){
                            cmsVedioComment = (List<CmsVedioComment>) msg.obj;
                            if(cmsVedioComment.equals("[]")){
                                mEmptyView.setStatus(EmptyView.Status.Empty);
                            }
                            MyAdapter adapter = new MyAdapter(cmsVedioComment);
                            recyclerView.setAdapter(adapter);
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
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onResume() {
        super.onResume();
        log.debug(" DetailCommentFragment onResume ");
    }

    @Override
    public void onPause() {
        super.onPause();
        log.debug(" DetailCommentFragment onPause ");
    }

    @Override
    public void onStop() {
        super.onStop();
        log.debug(" DetailCommentFragment onStop ");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        log.debug(" DetailCommentFragment onDestroy ");
    }

    View rootView;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        //让view只加载一次,避免重复加载造成的资源浪费
        if (rootView != null) {
            return onlyOneOnCreateView(rootView);
        }
        rootView = inflater.inflate(R.layout.frg_detail_comment, container, false);
        mEmptyView = (EmptyView) rootView.findViewById(R.id.rl_empty_tip);
        mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPic(R.drawable.ic_failed);
        mEmptyView.setStatus(EmptyView.Status.Loading);
        mEmptyView.setEmptyTop(mFragmentActivity, 70);
        recyclerView = (RecyclerView) rootView.findViewById(R.id.comm_recyclerView);
        recyclerView.setHasFixedSize(true);
        // 创建一个线性布局管理器
        LinearLayoutManager layoutManager = new LinearLayoutManager(mFragmentActivity);
        // 设置布局管理器
        recyclerView.setLayoutManager(layoutManager);
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
            CmsServiceManager.getInstance().getMovieComments(albumid, 0, 20, mHandler);
        }
    }

    boolean flag = false;

    @Override
    public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
        if (isNetWorkAviliable && !flag) {
            initData();
            flag = true;
        } else {
            flag = false;
        }
    }

    class MyAdapter extends RecyclerView.Adapter<MyAdapter.ViewHolder> {
        private List<CmsVedioComment> cmsVedioComment;

        // 数据集
        public MyAdapter(List<CmsVedioComment> c) {
            super();
            cmsVedioComment = c;
        }

        @Override
        public ViewHolder onCreateViewHolder(ViewGroup viewGroup, int i) {

            View view = LayoutInflater.from(mFragmentActivity).inflate(R.layout.frg_comment_item, viewGroup, false);

            ViewHolder holder = new ViewHolder(view);

            return holder;
        }

        @Override
        public void onBindViewHolder(ViewHolder viewHolder, int i) {
            // 绑定数据到ViewHolder上
            viewHolder.comment_content.setText(cmsVedioComment.get(i).title);
            viewHolder.comment_time.setText(cmsVedioComment.get(i).commentDate);
        }

        @Override
        public int getItemCount() {
            return cmsVedioComment.size();
        }

        public class ViewHolder extends RecyclerView.ViewHolder {
            public TextView comment_name;
            public TextView comment_time;
            public TextView comment_content;

            public ViewHolder(View itemView) {
                super(itemView);
                comment_name = (TextView) itemView.findViewById(R.id.tv_comment_name);
                comment_time = (TextView) itemView.findViewById(R.id.tv_comment_time);
                comment_content = (TextView) itemView.findViewById(R.id.tv_comment_content);
            }
        }
    }
}
