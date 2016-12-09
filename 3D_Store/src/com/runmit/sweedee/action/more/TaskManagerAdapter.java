package com.runmit.sweedee.action.more;

import java.util.List;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.text.Html;
import android.text.TextUtils;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ImageView.ScaleType;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.RoundedBitmapDisplayer;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadInfo;
import com.runmit.sweedee.download.VideoDownloadInfo;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.util.Util;

public class TaskManagerAdapter extends BaseExpandableListAdapter {
	private ImageLoader mImageLoader;
	private DisplayImageOptions mOptions;

	private boolean isMultiSelectMode = false;
	
	@SuppressLint("UseSparseArrays")
	protected SparseArray<Integer> mvPositionType = new SparseArray<Integer>();
	protected SparseArray<Object> mvPositionObject = new SparseArray<Object>();
	@SuppressLint("UseSparseArrays")
	protected SparseArray<Integer> maPositionType = new SparseArray<Integer>();
	protected SparseArray<Object> maPositionObject = new SparseArray<Object>();

	private int groupLength = 0;
	@SuppressLint("UseSparseArrays") 
	protected SparseArray<Integer> mType = new SparseArray<Integer>();

	private Activity mFragmentActivity;
	
	public TaskManagerAdapter(Activity act) {
		super();
		mImageLoader = ImageLoader.getInstance();
		mFragmentActivity = act;
	}
	
	public void setMultiSelectMode(boolean b) {
		isMultiSelectMode = b;
	}
	
	public void clearData() {
		groupLength = 0;
		mvPositionType.clear();
		mvPositionObject.clear();
		maPositionType.clear();
		maPositionObject.clear();
		mType.clear();
	}
	      
    //重写ExpandableListAdapter中的各个方法  
    @Override  
    public int getGroupCount() {
        return groupLength;        	
    }  

    @Override  
    public Object getGroup(int groupPosition) {
    	String title = "";
    	if(groupPosition == 0){
    		if(mvPositionType.size() > 0){
    			title = mFragmentActivity.getResources().getString(R.string.search_result_type_movie);
    		} else {
    			title = mFragmentActivity.getResources().getString(R.string.search_result_type_app);
    			if(!StoreApplication.isSnailPhone) title = mFragmentActivity.getResources().getString(R.string.search_result_type_game) + "&" + title;
    		}
    	} else {
    		title = mFragmentActivity.getResources().getString(R.string.search_result_type_app);
    		if(!StoreApplication.isSnailPhone) title = mFragmentActivity.getResources().getString(R.string.search_result_type_game) + "&" + title;
    	}  
    	return title;
    }  

    @Override  
    public long getGroupId(int groupPosition) {
        return groupPosition;  
    }  

    @Override  
    public int getChildrenCount(int groupPosition) {  
    	if(groupPosition == 0){
    		if(mvPositionType.size() > 0){
    			return  mvPositionType.size();
    		} else {
    			return maPositionType.size();
    		}
    	} else {
    		return maPositionType.size();
    	}
    }  

    @Override  
    public Object getChild(int groupPosition, int childPosition) {  
    	if(groupPosition == 0){
    		if(mvPositionObject.size() > 0){
    			return  mvPositionObject.get(childPosition);
    		} else if(maPositionObject.size() > 0) {
    			return maPositionObject.get(childPosition);
    		}
    	} else {
    		if(maPositionObject.size() > 0)
    		return maPositionObject.get(childPosition);
    	}
    	return null;
    }  

    @Override  
    public long getChildId(int groupPosition, int childPosition) {
        return childPosition;  
    }  

    @Override  
    public boolean hasStableIds() {
        return true;  
    }  

    @Override  
    public View getGroupView(int groupPosition, boolean isExpanded, View convertView, ViewGroup parent) {  
        TitleViewHolder holder;
		if(convertView == null){
        	convertView = LayoutInflater.from(mFragmentActivity).inflate(R.layout.item_search_title, null);
        	holder = new TitleViewHolder();
        	holder.title = (TextView) convertView.findViewById(R.id.title_textview);
        	convertView.setTag(holder);
        }else {
        	holder = (TitleViewHolder) convertView.getTag();
        }
		
		String t = getGroup(groupPosition).toString();
		holder.title.setText(t);

        return convertView;  
    }  

    @Override  
    public View getChildView(int groupPosition, int childPosition, boolean isLastChild, View convertView, ViewGroup parent) {
        DownloadInfo info = (DownloadInfo) getChild(groupPosition, childPosition);
        if(info == null) return null;
        if(info.downloadType == DownloadInfo.DOWNLOAD_TYPE_VIDEO) {
			mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_movie_image).build();
            return getVideoView(convertView, info, childPosition, isLastChild);
        } else {
        	float density = mFragmentActivity.getResources().getDisplayMetrics().density;
        	mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.game_detail_icon_h324).setDisplayer(new RoundedBitmapDisplayer((int) (30 * density))).build();
        	return getAppView(convertView, info, childPosition, isLastChild);
        }
    }  

    private View getAppView(View convertView, DownloadInfo info, int position, boolean isLastChild) {
    	TaskViewHolder holder = null;
    	if(convertView != null) {
    		holder = (TaskViewHolder) convertView.getTag();
    	}
    	if (convertView == null || holder == null || holder.type != info.downloadType) {
			LayoutInflater inflater = (LayoutInflater) mFragmentActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			holder = new TaskViewHolder();
			convertView = inflater.inflate(R.layout.item_taskmanager_video,null);
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
			holder.type = info.downloadType;
			convertView.setTag(holder);
		}
    	
		holder.tv_rate.setVisibility(ViewGroup.VISIBLE);
		holder.pb_progress.setVisibility(ViewGroup.VISIBLE);
		holder.tv_tips.setVisibility(ViewGroup.INVISIBLE);
		holder.vv_statuslayout.setVisibility(ViewGroup.VISIBLE);
		holder.tv_state.setBackgroundColor(mFragmentActivity.getResources().getColor(R.color.taskmanager_state_btn_other));
		
		holder.line_start.setVisibility(position == 0 ? View.VISIBLE : View.INVISIBLE);
		holder.line_mid.setVisibility(isLastChild ? View.INVISIBLE : View.VISIBLE);
		holder.line_end.setVisibility(isLastChild ? View.VISIBLE : View.INVISIBLE);
		
		final AppDownloadInfo mAppInfo = (AppDownloadInfo) info;
		holder.pb_progress.setProgress(mAppInfo.getDownloadProgress());
		holder.tv_title.setText(mAppInfo.mTitle);
		holder.tv_state.setText(mAppInfo.getState() + "");
				
		final AppItemInfo apInfo = new AppItemInfo();
		apInfo.id = Integer.parseInt(mAppInfo.mAppkey);
		String imageUri = TDImageWrap.getAppIcon(apInfo);
		mImageLoader.displayImage(imageUri, holder.iv_logo, mOptions);

		if (info.isSelected) {
			holder.iv_check.setVisibility(View.VISIBLE);
		} else {
			holder.iv_check.setVisibility(View.INVISIBLE);
		}

		holder.tv_rate.setText(Util.convertFileSize(mAppInfo.downloadSpeed*1024, 2, false) + "/s");
		String totalSize = Util.convertFileSize(mAppInfo.mFileSize, 1,true);
		String currentSize = Util.convertFileSize(mAppInfo.getDownPos(), 1, true);
		String size = totalSize;
		if (mAppInfo.mFileSize <= 0) {
			size = "";
		} else {
			size = currentSize + "/" + totalSize;
		}
		holder.tv_filesize.setText(size);

		if (mAppInfo.installState == DownloadInfo.STATE_UPDATE) {
			holder.tv_state.setText(R.string.download_app_install);
			holder.vv_statuslayout.setVisibility(ViewGroup.INVISIBLE);
			holder.pb_progress.setVisibility(ViewGroup.INVISIBLE);
			holder.tv_tips.setVisibility(ViewGroup.VISIBLE);
			holder.tv_tips.setText(R.string.taskmanager_tip_install);
		} else if (mAppInfo.installState == AppDownloadInfo.STATE_INSTALL) {
			holder.tv_state.setText(R.string.download_app_open);
			holder.vv_statuslayout.setVisibility(ViewGroup.INVISIBLE);
			holder.pb_progress.setVisibility(ViewGroup.INVISIBLE);
			holder.tv_tips.setVisibility(ViewGroup.VISIBLE);
			holder.tv_tips.setText(R.string.taskmanager_tip_openapp);
		} else {
			int state = mAppInfo.getState();
			if (state == AppDownloadInfo.STATE_RUNNING) {
				holder.tv_state.setText(R.string.download_app_paused);
				holder.tv_state.setBackgroundColor(mFragmentActivity.getResources().getColor(R.color.taskmanager_state_btn_pause));
			} else if (state == AppDownloadInfo.STATE_PAUSE) {
				holder.tv_rate.setVisibility(ViewGroup.INVISIBLE);
				holder.tv_state.setText(R.string.download_app_resume);
			} else if (state == AppDownloadInfo.STATE_FAILED) {
				holder.tv_state.setText(R.string.download_app_retry);
				holder.vv_statuslayout.setVisibility(ViewGroup.INVISIBLE);
				holder.pb_progress.setVisibility(ViewGroup.INVISIBLE);
				holder.tv_tips.setVisibility(ViewGroup.VISIBLE);
				holder.tv_tips.setText(Html.fromHtml(mFragmentActivity.getResources().getString(R.string.taskmanager_tip_retry)));
			} else if (state == AppDownloadInfo.STATE_FINISH) {
				holder.tv_state.setText(R.string.download_app_install);
				holder.vv_statuslayout.setVisibility(ViewGroup.INVISIBLE);
				holder.pb_progress.setVisibility(ViewGroup.INVISIBLE);
				holder.tv_tips.setVisibility(ViewGroup.VISIBLE);
				holder.tv_tips.setText(R.string.taskmanager_tip_install);
			} else if (state == AppDownloadInfo.STATE_WAIT) {
				holder.tv_state.setText(R.string.download_app_paused);
				holder.vv_statuslayout.setVisibility(ViewGroup.INVISIBLE);
				holder.pb_progress.setVisibility(ViewGroup.INVISIBLE);
				holder.tv_tips.setVisibility(ViewGroup.VISIBLE);
				holder.tv_tips.setText(R.string.taskmanager_tip_wait);
				holder.tv_state.setBackgroundColor(mFragmentActivity.getResources().getColor(R.color.taskmanager_state_btn_pause));
			}

		}
		final View clickView = convertView;
		holder.tv_state.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View button) {
				if(mListener != null && !isMultiSelectMode){
					mListener.onClick(clickView, mAppInfo);
				}
			}
		});

		return convertView;
	}

	private View getVideoView(View convertView, final DownloadInfo info, int position, boolean isLastChild) {
		TaskViewHolder holder = null;
		
		if(convertView != null) {
			holder = (TaskViewHolder) convertView.getTag();
		}
		
		if (convertView == null || holder == null || holder.type != info.downloadType) {
			LayoutInflater inflater = (LayoutInflater) mFragmentActivity .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			holder = new TaskViewHolder();
			convertView = inflater.inflate(R.layout.item_taskmanager_video, null);
			holder.tv_title = (TextView) convertView .findViewById(R.id.item_taskmanager_title);
			holder.tv_state = (TextView) convertView .findViewById(R.id.item_taskmanager_state);
			holder.iv_check = (ImageView) convertView .findViewById(R.id.item_taskmanager_check);
			holder.pb_progress = (ProgressBar) convertView .findViewById(R.id.item_taskmanager_progress);
			holder.tv_filesize = (TextView) convertView .findViewById(R.id.item_taskmanager_filesize);
			holder.tv_rate = (TextView) convertView .findViewById(R.id.item_taskmanager_rate);
			holder.iv_logo = (ImageView) convertView .findViewById(R.id.item_taskmanager_app_logo);
			holder.tv_tips = (TextView) convertView .findViewById(R.id.item_taskmanager_tips);
			holder.vv_statuslayout = (View) convertView .findViewById(R.id.item_taskmanager_statuslayout);
			holder.line_start = (View) convertView .findViewById(R.id.lineStart);
			holder.line_mid = (View) convertView.findViewById(R.id.lineMid);
			holder.line_end = (View) convertView.findViewById(R.id.lineEnd);
			holder.type = info.downloadType;
			convertView.setTag(holder);
		}
		
		holder.iv_logo.setScaleType(ScaleType.FIT_XY);
		holder.tv_rate.setVisibility(ViewGroup.VISIBLE);
		holder.pb_progress.setVisibility(ViewGroup.VISIBLE);
		holder.tv_tips.setVisibility(ViewGroup.INVISIBLE);
		holder.vv_statuslayout.setVisibility(ViewGroup.VISIBLE);
		holder.tv_title.setText(info.mTitle + "");
		holder.tv_state.setBackgroundColor(mFragmentActivity.getResources().getColor(R.color.taskmanager_state_btn_other));
		
		holder.line_start.setVisibility(position == 0 ? View.VISIBLE : View.INVISIBLE);
		holder.line_mid.setVisibility(isLastChild ? View.INVISIBLE : View.VISIBLE);
		holder.line_end.setVisibility(isLastChild ? View.VISIBLE : View.INVISIBLE);
		
		double percent = info.getDownloadProgress();
		holder.pb_progress.setMax(100);
		holder.pb_progress.setProgress((int) percent);
		if (info.isSelected) {
			holder.iv_check.setVisibility(View.VISIBLE);
		} else {
			holder.iv_check.setVisibility(View.INVISIBLE);
		}
		final VideoDownloadInfo taskInfo = (VideoDownloadInfo) info;

		if (TextUtils.isEmpty(taskInfo.poster)) {
			holder.iv_logo.setImageResource(R.drawable.default_movie_image);// 默认图
		} else {
			// 设置图片
			String imgUrl = taskInfo.poster;
			mImageLoader.displayImage(imgUrl, holder.iv_logo, mOptions);
		}
			holder.tv_rate.setText(Util.convertFileSize(taskInfo.downloadSpeed*1024, 2, false) + "/s");
			String totalSize = Util.convertFileSize(taskInfo.mFileSize, 1, true);
			String currentSize = Util.convertFileSize(taskInfo.getDownPos(),1, true);
			String size = totalSize;
			if (taskInfo.mFileSize <= 0) {
				size = "";
			} else {
				size = currentSize + "/" + totalSize;
			}
			holder.tv_filesize.setText(size);
			if (taskInfo.isRunning()) {
				holder.tv_state.setText(R.string.download_app_paused);
				holder.tv_state.setBackgroundColor(mFragmentActivity.getResources().getColor(R.color.taskmanager_state_btn_pause));
			} else if (taskInfo.getState() == DownloadInfo.STATE_PAUSE) {
				holder.tv_state.setText(R.string.download_app_resume);
				holder.tv_rate.setVisibility(View.INVISIBLE);
			} else if (taskInfo.getState() == DownloadInfo.STATE_FINISH) {
				holder.tv_state.setText(R.string.download_app_open);
				holder.vv_statuslayout.setVisibility(View.INVISIBLE);
				holder.tv_tips.setVisibility(View.VISIBLE);
				holder.tv_tips.setText(R.string.taskmanager_tip_openvideo);
				holder.pb_progress.setVisibility(View.INVISIBLE);
			} else if (taskInfo.getState() == DownloadInfo.STATE_FAILED) {
				holder.tv_state.setText(R.string.download_app_retry);
				holder.vv_statuslayout.setVisibility(View.INVISIBLE);
				holder.tv_tips.setVisibility(View.VISIBLE);
				holder.tv_tips.setText(Html.fromHtml(mFragmentActivity.getResources().getString(R.string.taskmanager_tip_retry)));
				holder.pb_progress.setVisibility(View.INVISIBLE);
			} else if (taskInfo.isWaiting()) {
				holder.tv_state.setText(R.string.download_app_paused);
				holder.vv_statuslayout.setVisibility(View.INVISIBLE);
				holder.tv_tips.setVisibility(View.VISIBLE);
				holder.tv_tips.setText(R.string.taskmanager_tip_wait);
				holder.pb_progress.setVisibility(View.INVISIBLE);
				holder.tv_state.setBackgroundColor(mFragmentActivity.getResources().getColor(R.color.taskmanager_state_btn_pause));
			}
			final View clickView = convertView;
			holder.tv_state.setOnClickListener(new View.OnClickListener() {

				@Override
				public void onClick(View v) {
					if(mListener != null && !isMultiSelectMode) {
						mListener.onClick(clickView, info);
					}
				}
			});
			return convertView;
	}


	@Override  
    public boolean isChildSelectable(int groupPosition, int childPosition) {
        return true;  
    }
    
    class TitleViewHolder {
    	public TextView title;		// 片名
    }
    
    class TaskViewHolder {
    	public int type;
		public TextView tv_title;
		public TextView tv_state;
		public ImageView iv_check;
		public ProgressBar pb_progress;
		public TextView tv_filesize;
		public TextView tv_rate;
		public ImageView iv_logo;
		public TextView tv_tips;
		public View vv_statuslayout;
		public View line_end;
		public View line_start;
		public View line_mid;
	}

	public void setData(List<DownloadInfo> mDownloadInfos) {
		DownloadInfo dwnInfo;
		int videoCount = 0, appCount = 0;
		for(int i = 0, len = mDownloadInfos.size(); i < len; i++) {
			dwnInfo = mDownloadInfos.get(i);
			if(dwnInfo instanceof VideoDownloadInfo) {
				if(dwnInfo.isFinish())continue;
				mvPositionType.put(videoCount, dwnInfo.downloadType);
				mvPositionObject.put(videoCount++, dwnInfo);
			} else if(dwnInfo instanceof AppDownloadInfo) {
				maPositionType.put(appCount, dwnInfo.downloadType);
				maPositionObject.put(appCount++, dwnInfo);
			}
		}
		
		if(mvPositionType.size() > 0) groupLength ++;
		if(maPositionType.size() > 0) groupLength ++;
	}
	
	private OnClickListener mListener;
	public void setOnClickListener(OnClickListener listener) {
		mListener = listener;
	}
	
	public interface OnClickListener {
		void onClick(View clickView, DownloadInfo info);
	}	
}
