package com.runmit.sweedee.action.game;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.RoundedBitmapDisplayer;
import com.runmit.sweedee.R;
import com.runmit.sweedee.download.DownloadCreater;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadInfo;
import com.runmit.sweedee.download.DownloadManager;
import com.runmit.sweedee.download.DownloadManager.OnDownloadDataChangeListener;
import com.runmit.sweedee.download.OnStateChangeListener;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.util.ApkInstaller;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

public class ApkListViewAdapter extends BaseAdapter {

	private XL_Log log = new XL_Log(ApkListViewAdapter.class);

	private List<AppItemInfo> mListInfo;

	private ImageLoader mImageLoader;

	private DisplayImageOptions mOptions;

	private Activity mContext;

	private LayoutInflater mInflater;
	
	private DownloadManager mDownloadManager;
	
	private static final long FORMAT_SIZE = 1024 * 1024;
	
    private OnDownloadDataChangeListener mOnDownloadChangeListener = new OnDownloadDataChangeListener() {
		
		@Override
		public void onDownloadDataChange(List<DownloadInfo> mDownloadInfos) {
			for (AppItemInfo itemInfo : mListInfo) {
				itemInfo.mDownloadInfo = null;//先置空所有
			}
			for(DownloadInfo info : mDownloadInfos){
				if(info.downloadType == DownloadInfo.DOWNLOAD_TYPE_VIDEO) continue;
				bindDownloadInfo((AppDownloadInfo) info);
			}
			notifyDataSetChanged();
		}

		@Override
		public void onAppInstallChange(AppDownloadInfo mDownloadInfo,String packagename,boolean isInstall) {
			for (AppItemInfo itemInfo : mListInfo) {
				if(itemInfo.packageName.equals(packagename)){
					itemInfo.mDownloadInfo = mDownloadInfo;
					itemInfo.installState = DownloadManager.getInstallState(itemInfo);
					notifyDataSetChanged();
					return;
				}
			}
		}
	};
	
	private OnStateChangeListener mOnStateChangeListener = new OnStateChangeListener() {
		
		@Override
		public void onDownloadStateChange(DownloadInfo mDownloadInfo, int new_state) {
			log.debug("new_state="+new_state+",mDownloadInfo="+mDownloadInfo);
			bindDownloadInfo((AppDownloadInfo) mDownloadInfo);
			notifyDataSetChanged();
		}
	};
	
	private void bindDownloadInfo(AppDownloadInfo info){
		for (AppItemInfo itemInfo : mListInfo) {
			if(itemInfo.packageName.equals(info.pkgName)){
				itemInfo.mDownloadInfo = info;
				itemInfo.installState = DownloadManager.getInstallState(itemInfo);
				return;
			}
		}
	}

	public ApkListViewAdapter(Activity context) {
		mListInfo = new ArrayList<AppItemInfo>();
		mContext = context;
		mInflater = LayoutInflater.from(mContext);
		mImageLoader = ImageLoader.getInstance();
		mDownloadManager = DownloadManager.getInstance();
		mDownloadManager.addOnDownloadDataChangeListener(mOnDownloadChangeListener);
		float density = context.getResources().getDisplayMetrics().density;
		mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.game_detail_icon_h324).setDisplayer(new RoundedBitmapDisplayer((int) (30*density))).build();
	}
	
    public void onDestroy() {
    	mDownloadManager.removeOnDownloadDataChangeListener(mOnDownloadChangeListener);
    }

	public void refreshList(boolean isApend, List<AppItemInfo> listInfo) {
		if (!isApend) {
			mListInfo.clear();
		}
		mListInfo.addAll(listInfo);
		notifyDataSetChanged();
	}
	
	public List<AppItemInfo> getAllAppList(){
		return mListInfo;
	}

	public int getCount() {
		return mListInfo.size();
	}

	public long getItemId(int position) {
		return position;
	}

	@Override
	public AppItemInfo getItem(int position) {
		return mListInfo.get(position);
	}

	private ViewHolder holder;

	public View getView(int position, View convertView, ViewGroup parent) {
		if (convertView == null) {
			convertView = mInflater.inflate(R.layout.item_home_game_layout, parent, false);
			holder = new ViewHolder();
			holder.title = (TextView) convertView.findViewById(R.id.textview_appTitle);
			holder.sizeText = (TextView) convertView.findViewById(R.id.textview_appSize);
			holder.genresText = (TextView) convertView.findViewById(R.id.textview_appMessage);
			holder.imageView = (ImageView) convertView.findViewById(R.id.img_icon);
			holder.relativeLayout = convertView.findViewById(R.id.relativeLayout);
			holder.downloadButton = (TextView) convertView.findViewById(R.id.btn_download_tv);
			holder.progressBar = (ProgressBar) convertView.findViewById(R.id.download_progress_bar);
			convertView.setTag(holder);
		} else {
			holder = (ViewHolder) convertView.getTag();
		}

		final AppItemInfo mAppInfo = getItem(position);
		initView(holder, mAppInfo);
		return convertView;
	}

	private void initView(final ViewHolder holder, final AppItemInfo mAppInfo) {
		holder.title.setText(mAppInfo.getTitle());
		// 包大小 安装人数
		String sizeString = "";
		sizeString += mAppInfo.fileSize/FORMAT_SIZE +"M";
		sizeString += "   ";
		sizeString += Util.addComma(mAppInfo.installationTimes, mContext);
		holder.sizeText.setText(sizeString);

		String genresString = "";
		List<AppItemInfo.AppGenre> genresArray = mAppInfo.genres;
		boolean firstString = true;
		for (AppItemInfo.AppGenre genre : genresArray) {
			if (firstString) {
				firstString = false;
			} else {
				genresString += "/";
			}
			genresString += genre.title;
		}
		holder.genresText.setText(genresString);
		String imageUri = TDImageWrap.getAppIcon(mAppInfo);
		mImageLoader.displayImage(imageUri, holder.imageView, mOptions);

		AppDownloadInfo mDownloadInfo = mAppInfo.mDownloadInfo;
		if (mAppInfo.installState == AppDownloadInfo.STATE_UPDATE && (mDownloadInfo == null || (mDownloadInfo != null && mDownloadInfo.getState() != AppDownloadInfo.STATE_RUNNING && mDownloadInfo.getState() != AppDownloadInfo.STATE_PAUSE  && mDownloadInfo.getState() != DownloadInfo.STATE_FINISH))) {
			buttonUpdate();
		} else if (mAppInfo.installState == AppDownloadInfo.STATE_INSTALL) {
			buttonOpen();
		} else {
			if (mDownloadInfo == null) {
				buttonDownload();
			} else {
				int state = mDownloadInfo.getState();
				if (state == AppDownloadInfo.STATE_RUNNING) {
					buttonDownloading();
					int prog = mDownloadInfo.getDownloadProgress();
					holder.progressBar.setProgress(prog);
				} else if (state == AppDownloadInfo.STATE_PAUSE) {
					buttonPaused();
					int prog = mDownloadInfo.getDownloadProgress();
					holder.progressBar.setProgress(prog);
				} else if (state == AppDownloadInfo.STATE_FAILED) {
					buttonFailed();
				} else if (state == AppDownloadInfo.STATE_FINISH) {
					buttonInstall();
				} else if (state == AppDownloadInfo.STATE_WAIT) {
					buttonWait();
				}
			}
		}

		holder.relativeLayout.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View button) {
				AppDownloadInfo mDownloadInfo = mAppInfo.mDownloadInfo;
				log.debug("mDownloadInfo="+mDownloadInfo);
				if (mAppInfo.installState == AppDownloadInfo.STATE_UPDATE && (mDownloadInfo == null || (mDownloadInfo != null && mDownloadInfo.getState() != AppDownloadInfo.STATE_RUNNING && mDownloadInfo.getState() != AppDownloadInfo.STATE_PAUSE))) {
					new DownloadCreater(mContext).startDownload(mAppInfo,mOnStateChangeListener);
				} else if (mAppInfo.installState == AppDownloadInfo.STATE_INSTALL) {
					ApkInstaller.lanchAppByPackageName(mContext, mAppInfo.appKey, mAppInfo.appId, mAppInfo.packageName, mAppInfo.type, 4);
				} else {
					if (mDownloadInfo == null) {
						new DownloadCreater(mContext).startDownload(mAppInfo,mOnStateChangeListener);
					} else {
						int state = mDownloadInfo.getState();
						if (state == AppDownloadInfo.STATE_FINISH) {
							ApkInstaller.installApk(mContext, mDownloadInfo);
						} else if (state == AppDownloadInfo.STATE_RUNNING) {
							DownloadManager.getInstance().pauseDownloadTask(mDownloadInfo);
						} else if (state == AppDownloadInfo.STATE_FAILED || state == AppDownloadInfo.STATE_PAUSE) {
							if (!Util.isNetworkAvailable()) {
								Toast.makeText(mContext, mContext.getString(R.string.no_network_toast), Toast.LENGTH_SHORT).show();
								return;
							}
							DownloadManager.getInstance().startDownload(mDownloadInfo, mOnStateChangeListener);
						}else if (state == AppDownloadInfo.STATE_WAIT){
							DownloadManager.getInstance().pauseDownloadTask(mDownloadInfo);
						}
					}
				}
			}
		});
	}

	private void buttonDownload() {
		holder.downloadButton.setTextColor(mContext.getResources().getColor(R.color.white));
		holder.downloadButton.setText(R.string.download_app_init);
		holder.progressBar.setVisibility(View.GONE);
		holder.relativeLayout.setBackground(mContext.getResources().getDrawable(R.drawable.progress_bg_green_selector));
	}

	private void buttonDownloading() {
		holder.downloadButton.setTextColor(mContext.getResources().getColor(R.color.black));
		holder.downloadButton.setText(R.string.download_app_running);
		holder.progressBar.setVisibility(View.VISIBLE);
		holder.relativeLayout.setBackground(mContext.getResources().getDrawable(R.drawable.progress_bg_green_selector));

	}

	private void buttonPaused() {
		holder.relativeLayout.setBackground(mContext.getResources().getDrawable(R.drawable.progress_bg_green_selector));
		holder.downloadButton.setTextColor(mContext.getResources().getColor(R.color.black));
		holder.downloadButton.setText(R.string.download_app_paused);
		holder.progressBar.setVisibility(View.VISIBLE);
	}

	private void buttonOpen() {
		holder.downloadButton.setTextColor(mContext.getResources().getColor(R.color.white));
		holder.downloadButton.setText(R.string.download_app_open);
		holder.relativeLayout.setBackground(mContext.getResources().getDrawable(R.drawable.progress_bg_blue_selector));
		holder.progressBar.setVisibility(View.GONE);
	}

	private void buttonUpdate() {
		holder.downloadButton.setTextColor(mContext.getResources().getColor(R.color.white));
		holder.downloadButton.setText(R.string.download_app_update);
		holder.relativeLayout.setBackground(mContext.getResources().getDrawable(R.drawable.progress_bg_blue_selector));
		holder.progressBar.setVisibility(View.GONE);
	}

	private void buttonFailed() {
		holder.relativeLayout.setBackground(mContext.getResources().getDrawable(R.drawable.progress_bg_blue_selector));
		holder.downloadButton.setTextColor(mContext.getResources().getColor(R.color.white));
		holder.downloadButton.setText(R.string.download_app_failed);
		holder.progressBar.setVisibility(View.GONE);
	}

	private void buttonInstall() {
		holder.relativeLayout.setBackground(mContext.getResources().getDrawable(R.drawable.progress_bg_blue_selector));
		holder.downloadButton.setTextColor(mContext.getResources().getColor(R.color.white));
		holder.downloadButton.setText(R.string.download_app_install);
		holder.progressBar.setVisibility(View.GONE);
	}
	
	private void buttonWait() {
		holder.relativeLayout.setBackground(mContext.getResources().getDrawable(R.drawable.progress_bg_green_selector));
		holder.downloadButton.setTextColor(mContext.getResources().getColor(R.color.white));
		holder.downloadButton.setText(R.string.download_app_wait_down);
		holder.progressBar.setVisibility(View.GONE);
	}

	class ViewHolder {
		TextView title;
		TextView sizeText;
		TextView genresText;
		ImageView imageView;
		View relativeLayout;
		TextView downloadButton;
		ProgressBar progressBar;
	}
}