package com.runmit.sweedee.action.game;

import java.util.List;

import android.app.ProgressDialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.StoreApplication.OnNetWorkChangeListener;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadManager;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.AppPageInfo;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.EmptyView;
import com.runmit.sweedee.view.MyListView;
import com.runmit.sweedee.view.MyListView.FootStatus;
import com.runmit.sweedee.view.MyListView.OnLoadMoreListenr;
import com.runmit.sweedee.view.MyListView.OnRefreshListener;

/**
 * 游戏全部页面
 */
public class AllGameFragment extends BaseFragment implements OnNetWorkChangeListener {

	private XL_Log log = new XL_Log(AllGameFragment.class);

	private View mViewRoot = null;
	private MyListView mPullToRefreshListView = null;
	private ApkListViewAdapter mListViewAdapter = null;
	private ProgressDialog mProgressDialog = null;
	private EmptyView mEmptyView = null;

	private AppPageInfo mAppPageInfo;

	private static final int ONE_PAGE_SIZE = 20;

	public android.widget.AbsListView getCurrentListView() {
		return mPullToRefreshListView;
	}

	AdapterView.OnItemClickListener itemClickHandler = new AdapterView.OnItemClickListener() {
		@Override
		public void onItemClick(AdapterView<?> adapterView, View view, int pos, long l) {
			AppItemInfo item = mListViewAdapter.getItem(pos - 1);
			AppDetailActivity.start(mFragmentActivity, view, item);
		}
	};

	protected Handler mhandler = new Handler() {
		public void handleMessage(Message msg) {
			Util.dismissDialog(mProgressDialog);
			switch (msg.what) {
			case CmsEventCode.EVENT_GET_APP_LIST:
				if (msg.arg1 == 0) {
					mAppPageInfo = (AppPageInfo) msg.obj;
					final List<AppItemInfo> mListInfo = mAppPageInfo.list;
					new Thread(new Runnable() {

						@Override
						public void run() {
							for (AppItemInfo itemInfo : mListInfo) {
								AppDownloadInfo mDownloadInfo = DownloadManager.getInstance().bindDownloadInfo(itemInfo);
								itemInfo.mDownloadInfo = mDownloadInfo;
							}
							refreshList(mAppPageInfo.start != 0, mListInfo);
						}
					}).start();
					mEmptyView.setStatus(EmptyView.Status.Gone);
				} else {
					mEmptyView.setStatus(EmptyView.Status.Empty);// 错误提示
				}
				mPullToRefreshListView.onRefreshComplete();
				break;
			default:
				break;
			}
		}
	};

	private void refreshList(final boolean isAppend, final List<AppItemInfo> mListInfo) {
		mhandler.post(new Runnable() {

			@Override
			public void run() {
				mListViewAdapter.refreshList(isAppend, mListInfo);
				if (mListViewAdapter.getCount() < mAppPageInfo.total) {
					mPullToRefreshListView.setFootStatus(FootStatus.Common);
				} else {
					mPullToRefreshListView.setFootStatus(FootStatus.Gone);
				}
			}
		});
	}

	@Override
	public void onDestroyView() {
		super.onDestroyView();
        ViewGroup parent = (ViewGroup) mViewRoot.getParent();
        if (parent != null) {
            parent.removeView(mViewRoot);
        }
	}
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		if (mViewRoot != null) {
			return mViewRoot;
		}
		mViewRoot = inflater.inflate(R.layout.fragment_model, container, false);
		mPullToRefreshListView = (MyListView) mViewRoot.findViewById(R.id.model_list_view);
		mPullToRefreshListView.setonRefreshListener(new OnRefreshListener() {

			@Override
			public void onRefresh() {
				loadData(false);
			}
		});

		mPullToRefreshListView.setOnLoadMoreListenr(new OnLoadMoreListenr() {

			@Override
			public void onLoadData() {
				if (mAppPageInfo != null) {
					if (mAppPageInfo.total > mListViewAdapter.getCount()) {
						CmsServiceManager.getInstance().getAppList(false, getAppType(), mListViewAdapter.getCount(), ONE_PAGE_SIZE, mhandler);
					}
				}
			}
		});

		mListViewAdapter = new ApkListViewAdapter(mFragmentActivity);
		mPullToRefreshListView.setAdapter(mListViewAdapter);
		mPullToRefreshListView.setOnItemClickListener(itemClickHandler);
		mEmptyView = (EmptyView) mViewRoot.findViewById(R.id.empty_view);
		mEmptyView.setEmptyTip(R.string.no_network_movie).setEmptyPic(R.drawable.ic_failed);
		mEmptyView.setStatus(EmptyView.Status.Loading);
		mEmptyView.setEmptyTop(mFragmentActivity, 80);
		mPullToRefreshListView.setEmptyView(mEmptyView);

		StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
		loadData(true);
		return mViewRoot;
	}

	public void onResume() {
		super.onResume();
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		mListViewAdapter.onDestroy();
	}

	@Override
	public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
		if (isNetWorkAviliable && (mListViewAdapter.getCount() == 0)) {
			loadData(true);
		}
	}

	protected void loadData(boolean usingCache) {
		CmsServiceManager.getInstance().getAppList(usingCache, getAppType(), 0, ONE_PAGE_SIZE, mhandler);
	}

	protected String getAppType() {
		return AppItemInfo.STR_GAME;
	}
}
