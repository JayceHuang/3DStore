package com.runmit.sweedee.action.more;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.CheckBox;
import android.widget.ExpandableListView;
import android.widget.ExpandableListView.OnChildClickListener;
import android.widget.ExpandableListView.OnGroupClickListener;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.DetailActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.StoreApplication.OnNetWorkChangeListener;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadInfo;
import com.runmit.sweedee.download.DownloadManager;
import com.runmit.sweedee.download.DownloadManager.OnDownloadDataChangeListener;
import com.runmit.sweedee.download.OnStateChangeListener;
import com.runmit.sweedee.download.VideoDownloadInfo;
import com.runmit.sweedee.manager.PlayerEntranceMgr;
import com.runmit.sweedee.util.ApkInstaller;
import com.runmit.sweedee.util.EnvironmentTool;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.view.EmptyView;

public class TaskManagerFragment extends BaseFragment implements OnNetWorkChangeListener, com.runmit.sweedee.action.more.TaskManagerAdapter.OnClickListener {
	private XL_Log log = new XL_Log(TaskManagerFragment.class);
	public static final String INTENT_ACTION_USER_ORDER_PAID = "com.runmit.sweedee.USER_ORDER_PAID";
	
	private View mViewRoot;
	private List<DownloadInfo> mSelectedTaskInfos = new ArrayList<DownloadInfo>();
	private ViewGroup operate_layout;
	private AnimatorSet mAnimatorSet = new AnimatorSet();
	private DownloadManager mDownloadManager;
	private MenuItem editMenuIt;
	private TextView mBtnSelectAll;
	private TextView mBtnDelete;
	private ProgressBar local_space_progressbar;
	private TextView local_fragment_empty_space_size;
	private EmptyView mEmptyView;
	private boolean isMultiSelectMode = false;
	private boolean fragmentAttaching = false;

	private OrderPaidReceiver mMyReceiver;
	private List<DownloadInfo> mDownloadInfos;
	
	private VideoDownloadInfo lastClickedVideoTask;	// 超时的任务;
	private ExpandableListView mExpandListView;
	private TaskManagerAdapter adapter; 
		
	@Override
	public void onActivityCreated(Bundle savedInstanceState) {
		super.onActivityCreated(savedInstanceState);
		setHasOptionsMenu(true);
	}

	public View onCreateView(LayoutInflater inflater, ViewGroup container,Bundle savedInstanceState) {
		mViewRoot = inflater.inflate(R.layout.frg_taskmanager, container, false);
		mDownloadManager = DownloadManager.getInstance();
		initView();
		List<DownloadInfo> list = mDownloadManager.getDownloadInfoList();
		initTaskData(list);
		mDownloadManager.addOnDownloadDataChangeListener(mOnDownloadChangeListener);
		registerReceiver();
		StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
		return mViewRoot;
	}
	
	@Override
	public void onAttach(Activity activity) {
		super.onAttach(activity);
		this.fragmentAttaching = true;
	}
	
	@Override
	public void onDetach() {
		super.onDetach();
		this.fragmentAttaching = false;
	}

	private void initView() {
		// 手机空间
		// progressDialog = new ProgressDialog(getActivity(),
		// R.style.ProgressDialogDark);
		local_space_progressbar = (ProgressBar) mViewRoot.findViewById(R.id.local_space_progressbar);
		local_fragment_empty_space_size = (TextView) mViewRoot.findViewById(R.id.local_fragment_empty_space_size);
		local_fragment_empty_space_size.setVisibility(View.VISIBLE);
		operate_layout = (ViewGroup) mViewRoot.findViewById(R.id.taskmanager_operate_layout);
		mBtnSelectAll = (TextView) operate_layout.findViewById(R.id.tv_operate_all);
		mBtnDelete = (TextView) operate_layout.findViewById(R.id.tv_operate_delete);
		mBtnSelectAll.setOnClickListener(OperateClickHandler);
		mBtnDelete.setOnClickListener(OperateClickHandler);

		// 空间大小View的小动画
		View spaceLayout = mViewRoot.findViewById(R.id.taskmanager_space_layout);
		Animation anim = AnimationUtils.loadAnimation(mFragmentActivity,R.anim.slide_bottom_to_top);
		spaceLayout.setAnimation(anim);
		mExpandListView = (ExpandableListView) mViewRoot.findViewById(R.id.explist);
		mExpandListView.setDivider(null);
		adapter = new TaskManagerAdapter(mFragmentActivity);
		mExpandListView.setAdapter(adapter);
		mExpandListView.setGroupIndicator(null);
		mExpandListView.setOnGroupClickListener(new OnGroupClickListener() {
			@Override
			public boolean onGroupClick(ExpandableListView paramExpandableListView, View paramView, int paramInt, long paramLong) {
				return true;
			}
		});
		mExpandListView.setOnChildClickListener(mOnItemClickListener);
		mExpandListView.setOnItemLongClickListener(mOnItemLongClickListener);
		
		adapter.setOnClickListener(this);
		for(int len = adapter.getGroupCount()-1; len >= 0; len--){
			mExpandListView.expandGroup(len);
		}
		mEmptyView = (EmptyView) mViewRoot.findViewById(R.id.taskmanager_empty_tip);
		mEmptyView.setEmptyTip(R.string.taskmanager_emptytip).setEmptyPic(R.drawable.ic_empty_box);
		mEmptyView.setStatus(EmptyView.Status.Empty);
	}
	

	private OnDownloadDataChangeListener mOnDownloadChangeListener = new OnDownloadDataChangeListener() {
		@Override
		public void onDownloadDataChange(List<DownloadInfo> mDownloads) {
			log.debug("TaskManagerFragment mDownloads="+mDownloads);
			initTaskData(mDownloads);
			updateSpace();
		}
		
		@Override
		public void onAppInstallChange(AppDownloadInfo mDownloadInfo, String packagename, boolean isInstall) {
			
		}
	};

	private OnChildClickListener mOnItemClickListener = new OnChildClickListener() {
		@Override
		public boolean onChildClick(ExpandableListView parent, View v, int groupPosition, int childPosition, long id) {
			if (isMultiSelectMode) {
				DownloadInfo info = (DownloadInfo) adapter.getChild(groupPosition, childPosition);
				info.isSelected = !info.isSelected;
				for(DownloadInfo di : mDownloadInfos) {
					if(di.downloadId == info.downloadId) {
						di.isSelected = info.isSelected;
						break;
					}
				}
				setSelectedCount();
			}
			return false;
		}
	};

	private OnItemLongClickListener mOnItemLongClickListener = new OnItemLongClickListener() {
		@Override
		public boolean onItemLongClick(AdapterView<?> parent, View view,int position, long id) {
			changeSelectMode();
			return false;
		}
	};
	
	@Override
	public void onStart() {
		super.onStart();
		updateSpace();
	}

	@Override
	public void onResume() {
		super.onResume();
	}

	@Override
	public void onPause() {
		super.onPause();
	}

	@Override
	public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
		inflater.inflate(R.menu.local_space_menu, menu);
		editMenuIt = menu.findItem(R.id.local_space_edit);
		if (isMultiSelectMode) {
			editMenuIt.setVisible(true);
		} else {
			editMenuIt.setVisible(false);
		}
		editMenuIt.setTitle(R.string.cancel);
		super.onCreateOptionsMenu(menu, inflater);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.local_space_edit:
			changeSelectMode();
			break;
		default:
			break;
		}
		return super.onOptionsItemSelected(item);
	}
	
	public void initTaskData(List<DownloadInfo> lists) {
		adapter.clearData();
		if(lists == null || lists.size() <= 0) {
			adapter.notifyDataSetChanged();
			mEmptyView.setStatus(EmptyView.Status.Empty);
			return;
		}
		mDownloadInfos = restoreTaskCheck(lists);
		adapter.setData(mDownloadInfos);
		adapter.notifyDataSetChanged();
		if(adapter.getGroupCount() == 0){
			mEmptyView.setStatus(EmptyView.Status.Empty);
			return;
		}
		
		for(int len = adapter.getGroupCount()-1; len >= 0; len--){
			mExpandListView.expandGroup(len);
		}
		mEmptyView.setStatus(EmptyView.Status.Gone);
	}

	private List<DownloadInfo> restoreTaskCheck(List<DownloadInfo> taskInfos) {
		for (DownloadInfo taskInfo : taskInfos) {
			if(mDownloadInfos != null)
			for (DownloadInfo mtaskInfo : mDownloadInfos) {
				if (taskInfo.downloadId == mtaskInfo.downloadId) {
					taskInfo.isSelected = mtaskInfo.isSelected;
				}
			}
		}
		return taskInfos;
	}

	private View.OnClickListener OperateClickHandler = new View.OnClickListener() {
		@Override
		public void onClick(View view) {
			switch (view.getId()) {
			case R.id.tv_operate_all:
				setAllSelectState(true);
				setSelectedCount();
				break;
			case R.id.tv_operate_delete:
				showDeleteMultipleTasksDialog(mSelectedTaskInfos);
				break;
			default:
				break;
			}
		}
	};

	private void showDeleteMultipleTasksDialog(final List<DownloadInfo> taskList) {

		AlertDialog.Builder builder = new AlertDialog.Builder(mFragmentActivity,R.style.AlertDialog);
		builder.setTitle(Util.setColourText(mFragmentActivity,R.string.delete_tasks_confirmation, Util.DialogTextColor.BLUE));
		final View checkView = LayoutInflater.from(mFragmentActivity).inflate(R.layout.alert_dialog_taskmanager, null);
		final CheckBox cb_deleteFile = (CheckBox) checkView.findViewById(R.id.isdeletelocalfile);
		log.debug("isDeleteLocalFile:" + cb_deleteFile.isChecked());
		builder.setView(checkView);

		builder.setNegativeButton(Util.setColourText(mFragmentActivity,R.string.ok, Util.DialogTextColor.BLUE),new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				if (taskList != null && taskList.size() > 0) {
					for (DownloadInfo delInfo : taskList) {
						mDownloadManager.deleteTask(delInfo,cb_deleteFile.isChecked());
					}
				}
				changeSelectMode();
			}
		});
		builder.setPositiveButton(R.string.cancel,new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
			}
		});
		AlertDialog dialog = builder.create();
		dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_APPLICATION_PANEL);
		dialog.show();
	}

	private void updateSpace() {
		if(!fragmentAttaching) return;
		// 获取到缓存界面设置的目录并显示空间大小,如果没有设置过，则为默认的目录
		String path = EnvironmentTool.getFileDownloadCacheDir();
		long avail_size = Util.getAvailableSizeforDownloadPath(path);
		long max_space = Util.getTotalSizeforDownloadPath(path);// 拿到缓存目录的大小并进行计算
		local_fragment_empty_space_size.setText(String.format(getString(R.string.local_space_size),Util.getMBSize(avail_size), Util.getMBSize(max_space)));
		if (max_space > 0) {
			long used_space = max_space - avail_size;
			local_space_progressbar.setProgress((int) (used_space * local_space_progressbar.getMax() / max_space));
		} else {
			local_space_progressbar.setProgress(0);
		}
	}
	
	private void setAllSelectState(boolean isSelected) {
		for(int i = 0, len = adapter.getGroupCount(); i < len; i++){
			for(int j = 0, size = adapter.getChildrenCount(i); j < size; j++){
				DownloadInfo itemInfo = (DownloadInfo) adapter.getChild(i, j);
				itemInfo.isSelected = isSelected;
			}
		} 
		adapter.notifyDataSetChanged();
	}

	private void setSelectedCount() {
		int count = 0;
		mSelectedTaskInfos.clear();
		for (DownloadInfo itemInfo : mDownloadInfos) {
			if (itemInfo.isSelected) {
				mSelectedTaskInfos.add(itemInfo);
				count++;
			}
		}
		setDeleteText(count);
	}

	private void setDeleteText(int selectNum) {
		String base = getString(R.string.operate_delete);
		if (selectNum > 0) {
			mBtnDelete.setText(base + "(" + selectNum + ")");
		} else {
			mBtnDelete.setText(base);
		}
		adapter.notifyDataSetChanged();
	}

	private void showOperateMenu() {
		setOperateViewVisibity(true);
	}

	private void setOperateViewVisibity(boolean visiable) {
		mAnimatorSet.setDuration(200);
		if (visiable) {
			operate_layout.setVisibility(View.VISIBLE);
			mAnimatorSet.playTogether(ObjectAnimator.ofFloat(operate_layout,"alpha", 0, 1), ObjectAnimator.ofFloat(operate_layout,"translationY", operate_layout.getHeight() / 2, 0));
		} else {
			mAnimatorSet.playTogether(ObjectAnimator.ofFloat(operate_layout,"alpha", 1, 0), ObjectAnimator.ofFloat(operate_layout,"translationY", 0, operate_layout.getHeight() / 2));
		}
		mAnimatorSet.start();
	}

	private void changeSelectMode() {
		isMultiSelectMode = !isMultiSelectMode;
		if (isMultiSelectMode) {
			showOperateMenu();
			editMenuIt.setVisible(true);
		} else {
			setAllSelectState(false);
			setOperateViewVisibity(false);
			editMenuIt.setVisible(false);
		}
		adapter.setMultiSelectMode(isMultiSelectMode);
	}

	public boolean onBackPressed() {
		if (isMultiSelectMode) {
			changeSelectMode();
			return true;
		}
		return false;
	}
	
	private void openDownloadedFile(VideoDownloadInfo task) {
		if (Util.isSDCardExist()) {
			String filePath = task.mPath;
			if (TextUtils.isEmpty(filePath)) {// 首先判断是否有记录保存
				Util.showToast(StoreApplication.INSTANCE, getString(R.string.file_no_exist), Toast.LENGTH_LONG);
				return;
			}
			File file = new File(filePath);
			if (!file.exists()) {// 再判断该文件是否存在
				Util.showToast(StoreApplication.INSTANCE, getString(R.string.file_no_exist), Toast.LENGTH_LONG);
				return;
			}
			if(isTaskOverDue(task)){
				showTaskOverDueDialog(task); 
			}else{
				String dirPath = filePath.substring(0, filePath.lastIndexOf(File.separator)) + File.separator;
				new PlayerEntranceMgr(getActivity()).localPlay(getActivity(), dirPath, task.mTitle, task.mFileSize, task.mSubTitleInfos,task.mode);
			}
			
		} else {
			Util.showToast(StoreApplication.INSTANCE, getString(R.string.no_found_sdcard), Toast.LENGTH_LONG);
			return;
		}
	}
    
    private void showTaskOverDueDialog(final DownloadInfo mTaskInfo) {
		if (mTaskInfo == null) {
			return;
		}
		AlertDialog.Builder builder = new AlertDialog.Builder(mFragmentActivity, R.style.AlertDialog);
		builder.setTitle(Util.setColourText(mFragmentActivity,R.string.confirm_title, Util.DialogTextColor.BLUE));
		builder.setMessage(R.string.task_over_due_toast_msg);
		builder.setNegativeButton(Util.setColourText(mFragmentActivity,R.string.re_download, Util.DialogTextColor.BLUE),new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				mDownloadManager.deleteTask(mTaskInfo, true);					
				DetailActivity.start(mFragmentActivity, clickView, ((VideoDownloadInfo)mTaskInfo).albumnId);
			}
		});

		builder.setPositiveButton(R.string.cancel,new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {

			}
		});
		AlertDialog dialog = builder.create();
		dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_APPLICATION_PANEL);
		dialog.show();
	}

	private boolean isTaskOverDue(DownloadInfo mTaskInfo) {
		if (mTaskInfo == null || TextUtils.isEmpty(mTaskInfo.finishtime)) {
			return false;
		}
		long currentTime = System.currentTimeMillis() / 1000;
		long duration = currentTime - Long.parseLong(mTaskInfo.finishtime);
		long day = duration / (24 * 60 * 60);
		return day >= 3;
	}
		
	@Override
	public void onDestroy() {
		super.onDestroy();
		unregisterReceiver();
		setAllSelectState(false);
		mDownloadManager.removeOnDownloadDataChangeListener(mOnDownloadChangeListener);
	}
	
	// 
	private void registerReceiver(){
    	mMyReceiver = new OrderPaidReceiver();
		IntentFilter intentFilter = new IntentFilter(INTENT_ACTION_USER_ORDER_PAID);
		mFragmentActivity.registerReceiver(mMyReceiver, intentFilter);
    }
    
    private void unregisterReceiver(){
    	mFragmentActivity.unregisterReceiver(mMyReceiver);
    }
    
	// 如果支付完成，继续下载
    public class OrderPaidReceiver extends BroadcastReceiver{
		@Override
		public void onReceive(Context context, Intent intent) {
			int aId = intent.getIntExtra("albumnId", -1);
			if(lastClickedVideoTask == null || aId != lastClickedVideoTask.albumnId){
				return;
			}
			if(intent.getAction().equals(INTENT_ACTION_USER_ORDER_PAID)){
				new PlayerEntranceMgr(mFragmentActivity).startRegetUrl(lastClickedVideoTask);
				lastClickedVideoTask = null;
			}
		}
    }
    // 发送支付完成的广播
    public static void sendOrderPaidBroadcast(Context context, int albumnId){
		Intent i = new Intent(INTENT_ACTION_USER_ORDER_PAID);
		i.putExtra("albumnId", albumnId);
		context.sendBroadcast(i);
	}

	@Override
	public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
		if(!isNetWorkAviliable){
			pauseAllRunningTask();
			//Toast.makeText(TaskManagerFragment.this.getActivity(),getString(R.string.no_network_toast),Toast.LENGTH_SHORT).show();
		}
	}
	
	private void pauseAllRunningTask() {
		if(mDownloadInfos != null && mDownloadInfos.size() > 0) {
			for (DownloadInfo downloadInfo : mDownloadInfos) {
				if (downloadInfo != null && (downloadInfo.getState() == AppDownloadInfo.STATE_RUNNING || downloadInfo.getState() == AppDownloadInfo.STATE_WAIT)) {
					mDownloadManager.pauseDownloadTask(downloadInfo);
				}
			}
		}
	}
	
	private View clickView;
	@Override
	public void onClick(View v, DownloadInfo info) {
		this.clickView = v;		
		if(info.downloadType == DownloadInfo.DOWNLOAD_TYPE_VIDEO) {
			clickVideoItem((VideoDownloadInfo) info);
		} else {
			clickAppItem((AppDownloadInfo) info);
		}		
	}
	
	private void clickVideoItem(VideoDownloadInfo info) {
		if (info.isFinish()) {
			openDownloadedFile(info);
		} else if (info.getState() == DownloadInfo.STATE_PAUSE || info.getState() == DownloadInfo.STATE_FAILED) {
			if (!Util.isNetworkAvailable()) {
				Toast.makeText(mFragmentActivity,mFragmentActivity.getString(R.string.no_network_toast),Toast.LENGTH_SHORT).show();
				return;
			}	
			String path = EnvironmentTool.getFileDownloadCacheDir();
			long avail_size = Util.getAvailableSizeforDownloadPath(path);
			if(avail_size < info.mFileSize - info.getDownPos()){
				Toast.makeText(mFragmentActivity,mFragmentActivity.getString(R.string.sdcard_no_space),Toast.LENGTH_SHORT).show();
				return;
			}
			lastClickedVideoTask = info;
			mDownloadManager.startDownload(info, mOnStateChangeListener);
		} else if (info.getState() == DownloadInfo.STATE_RUNNING || info.getState() == DownloadInfo.STATE_WAIT) {
			mDownloadManager.pauseDownloadTask(info);
		}
	}
	
	private void clickAppItem(AppDownloadInfo info) {
		int state = info.getState();
		if (state == AppDownloadInfo.STATE_FINISH) {
			ApkInstaller.installApk(mFragmentActivity, info);
		} else if (state == AppDownloadInfo.STATE_RUNNING) {
			DownloadManager.getInstance().pauseDownloadTask(info);
		} else if (state == AppDownloadInfo.STATE_FAILED || state == AppDownloadInfo.STATE_PAUSE) {
			if (!Util.isNetworkAvailable()) {
				Toast.makeText(mFragmentActivity,mFragmentActivity.getString(R.string.no_network_toast),Toast.LENGTH_SHORT).show();
				return;
			}
			DownloadManager.getInstance().startDownload(info, mOnStateChangeListener);
		} else if (state == AppDownloadInfo.STATE_WAIT) {
			DownloadManager.getInstance().pauseDownloadTask(info);
		}
	}
	
	private OnStateChangeListener mOnStateChangeListener = new OnStateChangeListener() {
		@Override
		public void onDownloadStateChange(DownloadInfo mDownloadInfo, int new_state) {
			for(DownloadInfo info : mDownloadInfos){
				if(info.getBeanKey().equals(mDownloadInfo.getBeanKey())) {
					info.setDownloadState(new_state);
					break;
				}
			}
			notifyDataSetChanged();
		}

		private void notifyDataSetChanged() {
			adapter.notifyDataSetChanged();
		}
	};

}