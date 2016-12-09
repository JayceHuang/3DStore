package com.runmit.sweedee.action.space;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.nhaarman.listviewanimations.AnimDeleteAdapter.AnimDeleteListener;
import com.nhaarman.listviewanimations.appearance.simple.SwingBottomInAnimationAdapter;
import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.DetailActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.StoreApplication.OnNetWorkChangeListener;
import com.runmit.sweedee.download.AppDownloadInfo;
import com.runmit.sweedee.download.DownloadInfo;
import com.runmit.sweedee.download.DownloadManager;
import com.runmit.sweedee.download.VideoDownloadInfo;
import com.runmit.sweedee.download.DownloadManager.OnDownloadDataChangeListener;
import com.runmit.sweedee.manager.PlayerEntranceMgr;
import com.runmit.sweedee.model.TaskInfo;
import com.runmit.sweedee.util.StaticHandler;
import com.runmit.sweedee.util.StaticHandler.MessageListener;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.EmptyView;
import com.runmit.sweedee.view.EmptyView.Status;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class VideoCacheFragment extends BaseFragment implements MessageListener, OnItemLongClickListener, OnItemClickListener, OnNetWorkChangeListener{
	
	private XL_Log log = new XL_Log(VideoCacheFragment.class);

	public final static int DELM_DOWNLOAD_TASK_DELETE = 10000;
	
	private ListView listView;

	private ProgressDialog progressDialog;

	private VideoCacheListAdapter mAdapter;

	private boolean isMultiSelectMode;

	private ViewGroup loadingLayout;

	private DownloadManager mDownloadManager;

	private ArrayList<VideoDownloadInfo> mLocalTaskList = new ArrayList<VideoDownloadInfo>();

	private StaticHandler mHandler;

	private EmptyView mEmptyView;
	private Menu mMenu;
	private MenuItem editMenuIt;
    private AnimatorSet mAnimatorSet = new AnimatorSet();
	private View mRootView;
    private ViewGroup operate_layout;
    private TextView mBtnSelectAll;
    private TextView mBtnDelete;
    private boolean pageEnable = true;
    private View latstView;     // 打开缓存任务时的view;
    
    private int deletingSize = 0;

    @Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setHasOptionsMenu(true);
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		mRootView = inflater.inflate(R.layout.frg_videocache, container, false);
		mDownloadManager = DownloadManager.getInstance();
		mHandler = new StaticHandler(this);
		StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
		initView();
		return mRootView;
	}

	@Override
	public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
		mMenu = menu;
		inflater.inflate(R.menu.local_space_menu, menu);
		editMenuIt = menu.findItem(R.id.local_space_edit);
        // 透过viewpager 切换回来时会触发一次
//		if(mLocalTaskList != null){
//			log.debug("runnit list size:"+mLocalTaskList.size());
//		}
//		if(mLocalTaskList == null || mLocalTaskList.size()==0){
//	        hiddenEditMenu();			
//		}
		editMenuIt.setTitle(R.string.cancel);
		hiddenEditMenu();
//        changeEditBtn(isMultiSelectMode);
		super.onCreateOptionsMenu(menu, inflater);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.local_space_edit:
            mAdapter.setMultiSelectMode(false);
            changeSelectMode();
			break;
		default:
			break;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	public void onSaveInstanceState(Bundle outState) {
		log.debug("onSaveInstanceState");
		super.onSaveInstanceState(outState);
	}

	private void hiddenEditMenu() {
		if (null != mMenu) {
			for (int i = 0; i < mMenu.size(); i++) {
				mMenu.getItem(i).setVisible(false);
				mMenu.getItem(i).setEnabled(false);
			}
		}
	}

	private void showEditMenu() {
		if (null != mMenu) {
			for (int i = 0; i < mMenu.size(); i++) {
				mMenu.getItem(i).setVisible(true);
				mMenu.getItem(i).setEnabled(true);
			}
		}
	}

	@Override
	public void onStart() {
		super.onStart();
	}

	@Override
	public void onResume() {
		super.onResume();
		initTasks();
	}

	private void initTasks() {
		final List<DownloadInfo> tempList = mDownloadManager.getDownloadInfoList();
		if(!pageEnable || mAdapter == null || tempList == null)return;
		initRunningTask(tempList);
	}

	private void clearAllSelectedState() {
		for (DownloadInfo task : mLocalTaskList) {
			task.isSelected = false;
		}
	}

	@Override
	public void onPause() {
		super.onPause();
		clearAllSelectedState();
	}

	@Override
	public void onStop() {
		super.onStop();
	}

	@Override
	public void onDestroy() {
		log.debug("fragment onDestroy");
		super.onDestroy();
	}

	@Override
	public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
		log.debug("is avaliable:" + isNetWorkAviliable + "  type:" + mNetType);
	}

	@Override
	public void handleMessage(final Message msg) {
		switch (msg.what) {
		case DELM_DOWNLOAD_TASK_DELETE:
			log.debug("DELM_DOWNLOAD_TASK_DELETE msg.arg1=" + msg.arg1);
			mAdapter.animScaleDelete(new AnimDeleteListener() {
				@Override
				public void onAnimDeleteEnd() {
					quitMultiSelectMode();
					changeSelectMode();
					updateDeletedView();
				}
			});
			break;
		default:
			break;
		}
	}

	@Override
	public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
		if (position < mLocalTaskList.size()) {
			if (!isMultiSelectMode) {
				if (mLocalTaskList != null && mLocalTaskList.size() > 0) {
					final DownloadInfo task = mAdapter.getItem(position);
					task.isSelected = true;
				}
				mAdapter.setMultiSelectMode(true);
				changeSelectMode();
			}
			return true;
		}
		return false;
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
		if (position < 0 || mAdapter == null) {
			return;
		}
		final VideoDownloadInfo task = mAdapter.getItem(position);
		if (isMultiSelectMode) {
			task.isSelected = !task.isSelected;
            updateOperateMenu();
			mAdapter.notifyDataSetChanged();
		} else {
			if (task.getState() == TaskInfo.LOCAL_STATE.SUCCESS) {// 缓存完成
                latstView = view;
				openDownloadedFile(task);
			}
		}
	}

	private void initView() {
		log.debug("initView");
		// 手机空间
		progressDialog = new ProgressDialog(getActivity(), R.style.ProgressDialogDark);

		// 缓存列表
		listView = (ListView) mRootView.findViewById(R.id.local_fragment_lv);
		mEmptyView = (EmptyView) mRootView.findViewById(R.id.rl_empty_tip);
		mEmptyView.setEmptyTip(R.string.no_download_record_msg).setEmptyPic(R.drawable.ic_empty_box);
		listView.setOnItemLongClickListener(this);
		listView.setOnItemClickListener(this);
		setListAdapter();

		// loading ...
		loadingLayout = (ViewGroup) mRootView.findViewById(R.id.loading_layout);
		toggleLoading(false);
		mEmptyView.setStatus(Status.Loading);

        operate_layout = (ViewGroup)mRootView.findViewById(R.id.operate_layout);
        mBtnSelectAll = (TextView) operate_layout.findViewById(R.id.tv_operate_all);
        mBtnDelete = (TextView) operate_layout.findViewById(R.id.tv_operate_delete);
        mBtnSelectAll.setOnClickListener(OperateClickHandler);
        mBtnDelete.setOnClickListener(OperateClickHandler);
	}

	// ====================================================================================
	
	private void setListAdapter() {
		if (mAdapter == null) {
			mAdapter = new VideoCacheListAdapter(getActivity(), mLocalTaskList);
			SwingBottomInAnimationAdapter swingBottomInAnimationAdapter = new SwingBottomInAnimationAdapter(mAdapter);
			swingBottomInAnimationAdapter.setAbsListView(listView);
			swingBottomInAnimationAdapter.getViewAnimator().setInitialDelayMillis(300);
			listView.setAdapter(swingBottomInAnimationAdapter);
		} else {
			mAdapter.notifyDataSetChanged();
		}
	}

	// 删除缓存任务时的确认框
	private void showDeleteMultipleTasksDialog(final List<DownloadInfo> list) {
		if (list == null || list.size() == 0) {
			return;
		}
		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity(), R.style.AlertDialog);
		builder.setTitle(Util.setColourText(getActivity(), R.string.delete_tasks_confirmation, Util.DialogTextColor.BLUE));
		final View checkView = LayoutInflater.from(getActivity()).inflate(R.layout.alert_dialog_checkbox, null);
		builder.setView(checkView);

		builder.setNegativeButton(Util.setColourText(getActivity(), R.string.ok, Util.DialogTextColor.BLUE), new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				Util.showDialog(progressDialog, getString(R.string.deleting_task));
				mDownloadManager.addOnDownloadDataChangeListener(mOnDownloadChangeListener);
				deletingSize = list.size();
				for (DownloadInfo info : list) {
					mDownloadManager.deleteTask(info, true);
				}
			}
		});

		builder.setPositiveButton(R.string.cancel, new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
			}
		});
		AlertDialog dialog = builder.create();
		dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_APPLICATION_PANEL);
		dialog.show();
	}

	/**
	 * 切换loading显隐状态
	 */
	private void toggleLoading(boolean isShow) {
		if (isShow) {
			loadingLayout.setVisibility(View.VISIBLE);
			return;
		}
		loadingLayout.setVisibility(View.GONE);
	}

	private void dismissDialog() {
		mHandler.post(new Runnable() {

			@Override
			public void run() {
				Util.dismissDialog(progressDialog);
			}
		});
	}

	// 任务操作
	// =====================================================================================
	// task状态分类添加到列表中
	private void initRunningTask(List<DownloadInfo> list) {
		mLocalTaskList.clear();
		
		for (DownloadInfo info : list) {
			if(info.downloadType == DownloadInfo.DOWNLOAD_TYPE_VIDEO && info.getState() == DownloadInfo.STATE_FINISH)
			mLocalTaskList.add((VideoDownloadInfo) info);
		}
		if (mLocalTaskList.size() <= 0) {
			mEmptyView.setStatus(Status.Empty);
			mAdapter.notifyDataSetChanged();
			return;
		}
		mEmptyView.setStatus(Status.Gone);
		if (mAdapter != null) {
			mAdapter.notifyDataSetChanged();
		}
		log.debug("init Running Task list size is:" + mLocalTaskList.size());
	}

	/**
	 * 删除gridview内的数据，更新显示
	 *
	 */
	private void updateDeletedView() {
		        
        mLocalTaskList.clear();
		List<DownloadInfo> lists = mDownloadManager.getDownloadInfoList(true);
		for (DownloadInfo info : lists) {
			if(info.downloadType == DownloadInfo.DOWNLOAD_TYPE_VIDEO && info.getState() == DownloadInfo.STATE_FINISH)
				mLocalTaskList.add((VideoDownloadInfo) info);
		}
		Log.d("mzh", "updateDeletedView mLocalTaskList size:" + mLocalTaskList.size());
		hiddenEditMenu();
		// 遍历完毕后更新状态
		if (mLocalTaskList.size() == 0) {
			mEmptyView.setStatus(Status.Empty);
			listView.setVisibility(View.GONE);
		} else {
			mAdapter.notifyDataSetChanged();
		}
	}
	
	private OnDownloadDataChangeListener mOnDownloadChangeListener = new OnDownloadDataChangeListener() {
		@Override
		public void onDownloadDataChange(List<DownloadInfo> mDownloads) {
			if(deletingSize > 0) {
				deletingSize--;
				return;
			}			
			mHandler.obtainMessage(DELM_DOWNLOAD_TASK_DELETE).sendToTarget();
			dismissDialog();
			mDownloadManager.removeOnDownloadDataChangeListener(mOnDownloadChangeListener);
		}
		
		@Override
		public void onAppInstallChange(AppDownloadInfo mDownloadInfo, String packagename, boolean isInstall) {
			
		}
	};

	private List<DownloadInfo> selectedTasks = new ArrayList<DownloadInfo>();

	private List<DownloadInfo> getSelectedLxTasks() {
		selectedTasks.clear();
		for (DownloadInfo task : mLocalTaskList) {
			if (task.isSelected) {
				selectedTasks.add(task);
			}
		}
		return selectedTasks;
	}

	/**
	 * @param task
	 *            要播放的视频信息
	 */
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
	
	
	private void showTaskOverDueDialog(final VideoDownloadInfo mTaskInfo) {
		if (mTaskInfo == null) {
			return;
		}
		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity(), R.style.AlertDialog);
		builder.setTitle(Util.setColourText(getActivity(), R.string.confirm_title, Util.DialogTextColor.BLUE));
		builder.setMessage(R.string.task_over_due_toast_msg);
		builder.setNegativeButton(Util.setColourText(getActivity(), R.string.re_download, Util.DialogTextColor.BLUE), new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				deletingSize = 1;
				mDownloadManager.addOnDownloadDataChangeListener(mOnDownloadChangeListener);
				mDownloadManager.deleteTask(mTaskInfo, true);
				DetailActivity.start(getActivity(), latstView, mTaskInfo.albumnId);
			}
		});

		builder.setPositiveButton(R.string.cancel, new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				
			}
		});
		AlertDialog dialog = builder.create();
		dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_APPLICATION_PANEL);
		dialog.show();
	}
	
	private boolean isTaskOverDue(DownloadInfo mTaskInfo){
		if (mTaskInfo == null || TextUtils.isEmpty(mTaskInfo.finishtime)) {
			return false;
		}
		long currentTime = System.currentTimeMillis()/1000;
		long duration = currentTime - Long.parseLong(mTaskInfo.finishtime);
		long day = duration / (24*60*60);
		return day >= 3;
	}

    /**
     * 显示操作栏
     */
	private void showOperateMenu() {
        updateOperateMenu();
        showEditMenu();
        setOperateViewVisibity(true, true);
	}

    /**
     * 更新按钮状态
     */
    private void updateOperateMenu(){
        List<DownloadInfo> tasks = getSelectedLxTasks();
        int select_num = tasks.size();
        String del_str = getString(R.string.operate_delete);
        if (select_num == 0) {
            mBtnDelete.setText(del_str);
        } else {
            mBtnDelete.setText(del_str+"(" + select_num + ")");
        }
    }

    /**
     * 退出操作栏
     */
    private void quitMultiSelectMode() {
    	quitMultiSelectMode(true);
    }
	private void quitMultiSelectMode(boolean withAnimate) {
		for (DownloadInfo task : mLocalTaskList) {
			task.isSelected = false;
		}
        setOperateViewVisibity(false, withAnimate);
		hiddenEditMenu();
		mAdapter.setMultiSelectMode(false);
		mAdapter.notifyDataSetChanged();
	}

    /**
     * 操作栏的动画控制
     * @param visiable 显示状态
     */
    private void setOperateViewVisibity(final boolean visiable, boolean withAnimate) {
        mAnimatorSet.setDuration(200);
        mAnimatorSet.removeAllListeners();
        mAnimatorSet.addListener(new AnimatorListener() {
			@Override
			public void onAnimationStart(Animator paramAnimator) {}
			
			@Override
			public void onAnimationRepeat(Animator paramAnimator) {}
			
			@Override
			public void onAnimationEnd(Animator paramAnimator) {
				if (!visiable) operate_layout.setVisibility(View.GONE);
			}
			
			@Override
			public void onAnimationCancel(Animator paramAnimator) {}
        });
        
        if (visiable) {
            mAnimatorSet.playTogether(
                    ObjectAnimator.ofFloat(operate_layout, "alpha", 0, 1),
                    ObjectAnimator.ofFloat(operate_layout, "translationY", operate_layout.getHeight() / 2, 0));
        } else {
            mAnimatorSet.playTogether(
                    ObjectAnimator.ofFloat(operate_layout, "alpha", 1, 0),
                    ObjectAnimator.ofFloat(operate_layout, "height", 0, operate_layout.getHeight() / 2));
        }
        if(withAnimate){
        	operate_layout.setVisibility(View.VISIBLE);
        	mAnimatorSet.start();
        } else {
        	operate_layout.setVisibility(View.GONE);
        }
    }

	// 单多选状态切换
	private void changeSelectMode() {
		isMultiSelectMode = !isMultiSelectMode;
		if (isMultiSelectMode) {
			showOperateMenu();
		} else {
			quitMultiSelectMode();
		}
//		changeEditBtn(isMultiSelectMode);
		mAdapter.notifyDataSetChanged();
	}

//	private void changeEditBtn(boolean isEdit) {
//		int text_id = isEdit ? R.string.cancel : R.string.edit;
//		String text = getString(text_id);
//		editMenuIt.setTitle(text);
//	}

	public boolean onBackPressed() {
		if (isMultiSelectMode) {
			changeSelectMode();
            return true;
		}
        return false;
	}

    /**
     * 编辑按钮的响应事件处理
     */
    private View.OnClickListener OperateClickHandler  = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            switch (view.getId()){
                case R.id.tv_operate_all:
                    for(DownloadInfo info : mLocalTaskList){
                        info.isSelected = true;
                    }
                    updateOperateMenu();
                    mAdapter.notifyDataSetChanged();
                    break;
                case R.id.tv_operate_delete:
                    showDeleteMultipleTasksDialog(selectedTasks);
                    break;
                default:
                    break;
            }
        }
    };

    public void setPageEnable(boolean pageEnable) {
        this.pageEnable = pageEnable;
        isMultiSelectMode = false;
        quitMultiSelectMode(false);
        initTasks();
    }
}
