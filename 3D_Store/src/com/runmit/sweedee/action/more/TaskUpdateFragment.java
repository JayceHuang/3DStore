package com.runmit.sweedee.action.more;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.os.Bundle;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.RoundedBitmapDisplayer;
import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.download.DownloadCreater;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadInfo;
import com.runmit.sweedee.download.DownloadManager;
import com.runmit.sweedee.download.OnStateChangeListener;
import com.runmit.sweedee.download.DownloadManager.OnDownloadDataChangeListener;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.util.ApkInstaller;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.view.EmptyView;

public class TaskUpdateFragment extends BaseFragment {
	private View mViewRoot;
	private ListView mTaskUpdateListView;
	private EmptyView mEmptyView;
	private DownloadManager mDownloadManager;
	private ArrayList<AppItemInfo> mAppInfoList;
	private TaskUpdateAdapter mTaskUpdateAdapter;

	public View onCreateView(LayoutInflater inflater, ViewGroup container,Bundle savedInstanceState) {
		mViewRoot = inflater.inflate(R.layout.frg_taskupdate, container, false);
		mDownloadManager = DownloadManager.getInstance();
		initView();
		mDownloadManager.addOnDownloadDataChangeListener(mOnDownloadChangeListener);
		return mViewRoot;
	}

	private void initView() {
		mEmptyView = (EmptyView) mViewRoot.findViewById(R.id.taskupdate_empty_tip);
		mEmptyView.setEmptyTip(StoreApplication.isSnailPhone ? R.string.taskupdate_emptytip_snail : R.string.taskupdate_emptytip).setEmptyPic(R.drawable.ic_empty_box);
		mEmptyView.setStatus(EmptyView.Status.Empty);
		mTaskUpdateListView = (ListView) mViewRoot.findViewById(R.id.frg_taskupdate_listview);
		mTaskUpdateListView.setOnItemClickListener(new OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {
			}
		});
		mAppInfoList = StoreApplication.INSTANCE.mAppInfoList;
		if(mAppInfoList != null && mAppInfoList.size()>0) {
			bindDownloadInfo();
			mEmptyView.setVisibility(View.GONE);
		}else {
			mEmptyView.setVisibility(View.VISIBLE);
		}
		mTaskUpdateAdapter = new TaskUpdateAdapter();
		mTaskUpdateListView.setAdapter(mTaskUpdateAdapter);
	}

	private OnDownloadDataChangeListener mOnDownloadChangeListener = new OnDownloadDataChangeListener() {
		@Override
		public void onDownloadDataChange(List<DownloadInfo> mDownloads) {
			if (mAppInfoList != null && mAppInfoList.size() > 0) {
				updateAppInfo(mDownloads);
			}
			mTaskUpdateAdapter.notifyDataSetChanged();
		}
		@Override
		public void onAppInstallChange(AppDownloadInfo mDownloadInfo,
				String packagename, boolean isInstall) {
			// TODO 此处在主线程刷新数据有问题，后续优化
			if(mAppInfoList!=null){
				ArrayList<AppItemInfo> deleteAppInfo=new ArrayList<AppItemInfo>();
				for (AppItemInfo appItemInfo : mAppInfoList) {
					if(appItemInfo.packageName.equals(packagename)){
						deleteAppInfo.add(appItemInfo);
					}
				}
				mAppInfoList.removeAll(deleteAppInfo);
				if(mAppInfoList.size()>0) {
					mEmptyView.setVisibility(View.GONE);
				}else {
					mEmptyView.setVisibility(View.VISIBLE);
				}
				mTaskUpdateAdapter.notifyDataSetChanged();
			}
		}
	};

	@Override
	public void onStart() {
		super.onStart();
	}

	private void bindDownloadInfo() {
		for (AppItemInfo appItemInfo : mAppInfoList) {
			AppDownloadInfo downloadInfo = DownloadManager.getInstance().bindDownloadInfo(appItemInfo);
			appItemInfo.mDownloadInfo = downloadInfo;
		}
	}

	private void updateAppInfo(List<DownloadInfo> mDownloads) {
		for (AppItemInfo appInfo : mAppInfoList) {
			boolean isInDownloads=false;
			for (DownloadInfo downloadInfo : mDownloads) {
				if (appInfo.packageName.equals(downloadInfo.getBeanKey())){
					appInfo.mDownloadInfo = (AppDownloadInfo) downloadInfo;
					isInDownloads=true;
					break;
				}
			}		
			if(!isInDownloads){
			appInfo.mDownloadInfo =DownloadManager.getInstance().bindDownloadInfo(appInfo);	
			}
		}
		mTaskUpdateAdapter.notifyDataSetChanged();
	}

	public boolean onBackPressed() {
		return false;
	}

	class TaskUpdateAdapter extends BaseAdapter {
		private DisplayImageOptions mOptions;
		private ImageLoader mImageLoader;
		public TaskUpdateAdapter() {
			super();
			mImageLoader = ImageLoader.getInstance();
			float density = mFragmentActivity.getResources().getDisplayMetrics().density;
			mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.game_detail_icon_h324).setDisplayer(new RoundedBitmapDisplayer((int) (30 * density))).build();
		}
		
		@Override
		public int getCount() {
			return mAppInfoList == null ? 0 : mAppInfoList.size();
		}
		
		@Override
		public Object getItem(int position) {
			return null;
		}
		
		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			AppTaskHolder holder = new AppTaskHolder();
			if (convertView == null) {
				LayoutInflater inflater = (LayoutInflater) mFragmentActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
				convertView = inflater.inflate(R.layout.item_taskmanager_update,null);
				TextView tv_title = (TextView) convertView.findViewById(R.id.item_taskmanager_title);
				TextView tv_state = (TextView) convertView.findViewById(R.id.item_taskmanager_state);
				ImageView iv_check = (ImageView) convertView.findViewById(R.id.item_taskmanager_check);
				ProgressBar pb_progress = (ProgressBar) convertView.findViewById(R.id.item_taskmanager_progress);
				TextView tv_filesize = (TextView) convertView.findViewById(R.id.item_taskmanager_filesize);
				TextView tv_rate = (TextView) convertView.findViewById(R.id.item_taskmanager_rate);
				ImageView iv_logo = (ImageView) convertView.findViewById(R.id.item_taskmanager_app_logo);
				TextView tv_tips = (TextView) convertView.findViewById(R.id.item_taskmanager_tips);
				View vv_statuslayout = (View) convertView.findViewById(R.id.item_taskmanager_statuslayout);
				View line_start = (View) convertView.findViewById(R.id.lineStart);
				View line_mid = (View) convertView.findViewById(R.id.lineMid);
				View line_end = (View) convertView.findViewById(R.id.lineEnd);
				holder.tv_title = tv_title;
				holder.tv_state = tv_state;
				holder.iv_check = iv_check;
				holder.pb_progress = pb_progress;
				holder.tv_filesize = tv_filesize;
				holder.tv_rate = tv_rate;
				holder.iv_logo = iv_logo;
				holder.tv_tips = tv_tips;
				holder.vv_statuslayout = vv_statuslayout;
				holder.line_start=line_start;
				holder.line_mid=line_mid;
				holder.line_end=line_end;
				convertView.setTag(holder);
			} else {
				holder = (AppTaskHolder) convertView.getTag();
			}
			holder.tv_rate.setVisibility(ViewGroup.VISIBLE);
			holder.pb_progress.setVisibility(ViewGroup.VISIBLE);
			holder.tv_tips.setVisibility(ViewGroup.VISIBLE);
			holder.vv_statuslayout.setVisibility(ViewGroup.INVISIBLE);
			holder.tv_state.setBackgroundColor(mFragmentActivity.getResources().getColor(R.color.taskmanager_state_btn_other));
			
			holder.line_start.setVisibility(position == 0 ? View.VISIBLE : View.INVISIBLE);
			holder.line_mid.setVisibility(position == getCount() - 1 ? View.INVISIBLE : View.VISIBLE);
			holder.line_end.setVisibility(position == getCount() - 1 ? View.VISIBLE : View.INVISIBLE);

			final AppItemInfo mAppInfo = TaskUpdateFragment.this.mAppInfoList.get(position);
			String imageUri = TDImageWrap.getAppIcon(mAppInfo);
			mImageLoader.displayImage(imageUri, holder.iv_logo, mOptions);
			holder.tv_title.setText(mAppInfo.title);

			AppDownloadInfo mDownloadInfo = mAppInfo.mDownloadInfo;
			if (mDownloadInfo != null) {
				holder.pb_progress.setProgress(mDownloadInfo.getDownloadProgress());
				holder.tv_rate.setText(Util.convertFileSize(mDownloadInfo.downloadSpeed*1024, 2, false) + "/s");
				String totalSize = Util.convertFileSize(mDownloadInfo.mFileSize, 1, true);
				String currentSize = Util.convertFileSize(mDownloadInfo.getDownPos(), 1, true);
				String size = totalSize;
				if (mDownloadInfo.mFileSize <= 0) {
					size = "";
				} else {
					size = currentSize + "/" + totalSize;
				}
				holder.tv_filesize.setText(size);
			}
			if (mAppInfo.installState == AppDownloadInfo.STATE_UPDATE&& (mDownloadInfo == null || (mDownloadInfo != null&& mDownloadInfo.getState() != AppDownloadInfo.STATE_RUNNING && mDownloadInfo.getState() != AppDownloadInfo.STATE_PAUSE))) {

				String versionName =String.format(getString(R.string.taskmanager_version_text),mAppInfo.versionName);
				if (mDownloadInfo == null) {
					holder.tv_state.setText(R.string.download_app_update);
					holder.pb_progress.setVisibility(ViewGroup.INVISIBLE);
					holder.tv_tips.setText(versionName+ "，"+Util.convertFileSize( mAppInfo.fileSize, 1, true));
				} else if (mDownloadInfo.getState() == AppDownloadInfo.STATE_FINISH) {
					holder.tv_state.setText(R.string.download_app_install);
					holder.tv_tips.setText(R.string.taskmanager_tip_install);
					holder.pb_progress.setVisibility(ViewGroup.INVISIBLE);
				} else {
					holder.tv_state.setText(R.string.download_app_update);
					holder.pb_progress.setVisibility(ViewGroup.INVISIBLE);
					holder.tv_tips.setText(versionName+ "，"+Util.convertFileSize(mAppInfo.fileSize, 1, true));
				}
			} else if (mAppInfo.installState == AppDownloadInfo.STATE_INSTALL) {
				holder.tv_state.setText(R.string.download_app_open);
			} else {
				int state = mDownloadInfo.getState();
				if (state == AppDownloadInfo.STATE_RUNNING) {
					holder.tv_state.setText(R.string.download_app_paused);
					holder.tv_tips.setVisibility(ViewGroup.INVISIBLE);
					holder.pb_progress.setVisibility(ViewGroup.VISIBLE);
					holder.vv_statuslayout.setVisibility(ViewGroup.VISIBLE);
					holder.tv_state.setBackgroundColor(mFragmentActivity.getResources().getColor(R.color.taskmanager_state_btn_pause));
				} else if (state == AppDownloadInfo.STATE_PAUSE) {
					holder.tv_tips.setVisibility(ViewGroup.INVISIBLE);
					holder.vv_statuslayout.setVisibility(ViewGroup.VISIBLE);
					holder.tv_rate.setVisibility(ViewGroup.INVISIBLE);
					holder.tv_state.setText(R.string.download_app_resume);
				} else if (state == AppDownloadInfo.STATE_FAILED) {
					holder.tv_state.setText(R.string.download_app_retry);
					holder.tv_tips.setText(Html.fromHtml(getResources().getString(R.string.taskmanager_tip_retry)));		
				} else if (state == AppDownloadInfo.STATE_FINISH) {
					holder.tv_state.setText(R.string.download_app_install);
					holder.tv_tips.setText(R.string.taskmanager_tip_install);
				} else if (state == AppDownloadInfo.STATE_WAIT) {
					holder.tv_state.setText(R.string.download_app_paused);
					holder.tv_tips.setText(R.string.taskmanager_tip_wait);
					holder.tv_state.setBackgroundColor(mFragmentActivity.getResources().getColor(R.color.taskmanager_state_btn_pause));
				}
			}

			holder.tv_state.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View button) {
					AppDownloadInfo mDownloadInfo = mAppInfo.mDownloadInfo;
					if (mAppInfo.installState == AppDownloadInfo.STATE_UPDATE&& (mDownloadInfo == null || (mDownloadInfo != null&& mDownloadInfo.getState() != AppDownloadInfo.STATE_RUNNING && mDownloadInfo.getState() != AppDownloadInfo.STATE_PAUSE))) {
						new DownloadCreater(mFragmentActivity).startDownload(mAppInfo,mOnStateChangeListener);
					} else if (mAppInfo.installState == AppDownloadInfo.STATE_INSTALL) {
						ApkInstaller.lanchAppByPackageName(mFragmentActivity,mAppInfo.appKey, mAppInfo.appId,mAppInfo.packageName, mAppInfo.type, 4);
					} else {
						if (mDownloadInfo == null) {
							new DownloadCreater(mFragmentActivity).startDownload(mAppInfo,mOnStateChangeListener);
						} else {
							int state = mDownloadInfo.getState();
							if (state == AppDownloadInfo.STATE_FINISH) {
								ApkInstaller.installApk(mFragmentActivity,mDownloadInfo);
							} else if (state == AppDownloadInfo.STATE_RUNNING) {
								DownloadManager.getInstance().pauseDownloadTask(mDownloadInfo);
							} else if (state == AppDownloadInfo.STATE_FAILED|| state == AppDownloadInfo.STATE_PAUSE) {
								if (!Util.isNetworkAvailable()) {
									Toast.makeText(mFragmentActivity,mFragmentActivity.getString(R.string.no_network_toast),Toast.LENGTH_SHORT).show();
									return;
								}
								DownloadManager.getInstance().startDownload(mDownloadInfo, mOnStateChangeListener);
							} else if (state == AppDownloadInfo.STATE_WAIT) {
								DownloadManager.getInstance().pauseDownloadTask(mDownloadInfo);
							}
						}
					}
				}
			});

			return convertView;
		}

		class AppTaskHolder {
			public TextView tv_title;
			public TextView tv_state;
			public ImageView iv_check;
			public ProgressBar pb_progress;
			public TextView tv_filesize;
			public TextView tv_rate;
			public TextView tv_tips;
			public View vv_statuslayout;
			public ImageView iv_logo;
			public View line_end;
			public View line_start;
			public View line_mid;
		}

		private OnStateChangeListener mOnStateChangeListener = new OnStateChangeListener() {

			@Override
			public void onDownloadStateChange(DownloadInfo mDownloadInfo, int new_state) {
				bindDownloadInfo((AppDownloadInfo) mDownloadInfo);
				notifyDataSetChanged();
			}
		};
		
		private void bindDownloadInfo(AppDownloadInfo info) {
			for (AppItemInfo itemInfo : mAppInfoList) {
				if (itemInfo.packageName.equals(info.pkgName)) {
					itemInfo.mDownloadInfo = info;
					itemInfo.installState = DownloadManager.getInstallState(itemInfo);
					return;
				}
			}
		}

	}

}
