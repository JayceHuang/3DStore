/**
 *
 */
package com.runmit.sweedee.action.myvideo;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
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
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.nhaarman.listviewanimations.AnimDeleteAdapter;
import com.nhaarman.listviewanimations.AnimDeleteAdapter.AnimDeleteListener;
import com.nhaarman.listviewanimations.appearance.simple.SwingBottomInAnimationAdapter;
import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.login.LoginActivity;
import com.runmit.sweedee.action.more.NewBaseRotateActivity;
import com.runmit.sweedee.constant.ServiceArg;
import com.runmit.sweedee.datadao.PlayRecord;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.manager.PlayerEntranceMgr;
import com.runmit.sweedee.player.PlayRecordHelper;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.sdk.user.member.task.UserManager.LoginState;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.view.EmptyView;
import com.runmit.sweedee.view.EmptyView.Status;

/**
 * @author Sven.Zhan
 *         <p/>
 *         播放记录界面
 */
public class PlayRecordFragment extends BaseFragment implements View.OnClickListener, OnItemLongClickListener, OnItemClickListener,UserManager.LoginStateChange{

    String TAG = PlayRecordFragment.class.getSimpleName();

    ListView mListView;

    EmptyView mEmptyView;

    ViewGroup mOperateView;

    TextView mBtnSelectAll;

    TextView mBtnDelete;

    RecordAdapter mAdapter;

    List<PlayRecord> mList = new ArrayList<PlayRecord>();

    List<PlayRecord> deleteList = new ArrayList<PlayRecord>();

    private boolean isEditMode = false;

    private int selectNum = 0;

    private Menu mMenu;

    private AnimatorSet mAnimatorSet = new AnimatorSet();

    private boolean hasStarted = false;
    
    private View mRootView;
    
    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
    	super.onActivityCreated(savedInstanceState);
    	setHasOptionsMenu(true);
    }
    
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    	mRootView = inflater.inflate(R.layout.activity_playrcord, container, false);
    	mListView = (ListView) mRootView.findViewById(R.id.frg_playrecord_lv);
        mEmptyView = (EmptyView) mRootView.findViewById(R.id.rl_empty_tip);
        mOperateView = (ViewGroup) mRootView.findViewById(R.id.operate_layout);
        mBtnSelectAll = (TextView) mOperateView.findViewById(R.id.tv_operate_all);
        mBtnDelete = (TextView) mOperateView.findViewById(R.id.tv_operate_delete);

        mAdapter = new RecordAdapter(getActivity());
        SwingBottomInAnimationAdapter swingBottomInAnimationAdapter = new SwingBottomInAnimationAdapter(mAdapter);
        swingBottomInAnimationAdapter.setAbsListView(mListView);
        swingBottomInAnimationAdapter.getViewAnimator().setInitialDelayMillis(300);
        mListView.setAdapter(swingBottomInAnimationAdapter);
        mListView.setOnItemLongClickListener(this);
        
        //空界面信息
        mEmptyView.setEmptyTip(R.string.no_play_record_tip).setEmptyPic(R.drawable.ic_watch_history);
        mListView.setOnItemClickListener(this);
        mBtnSelectAll.setOnClickListener(this);
        mBtnDelete.setOnClickListener(this);
        
        UserManager.getInstance().addLoginStateChangeListener(this);
        if(UserManager.getInstance().isLogined()){
        	loadData();
        }else{//未登录
        	mEmptyView.setStatus(Status.Empty);
        	mEmptyView.setEmptyPic(R.drawable.ic_paypal_history);
        	int start = 2;
        	int end = 4;
        	String str = getString(R.string.need_login_to_get_play_record);
        	if(Locale.getDefault().equals(Locale.CHINA) || Locale.getDefault().equals(Locale.TAIWAN)){
        		
        	}else{
        		start = str.indexOf("Login");
        		end = start + "Login".length();
        	}
        	SpannableString ss = new SpannableString(str);  
            ss.setSpan(new ClickableSpan(){
     			@Override
     			public void onClick(View widget) {
     				NewBaseRotateActivity.bitmap = null;
     				startActivity(new Intent(getActivity(), LoginActivity.class));
     			}}, start, end,Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);  
             TextView tvTextView = mEmptyView.gettvEmptyTip();
             tvTextView.setText(ss);  
             tvTextView.setMovementMethod(LinkMovementMethod.getInstance());  
        }
    	return mRootView;
    };
    
    public class RecordAdapter extends AnimDeleteAdapter {

        ImageLoader mImageLoader;

        DisplayImageOptions mOptions;

        LayoutInflater mInflater;

        public RecordAdapter(Context ctx) {
            mImageLoader = ImageLoader.getInstance();
            mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.defalut_movie_small_item).build();
            mInflater = LayoutInflater.from(ctx);
        }

        @Override
        public int getCount() {
            return mList.size();
        }

        @Override
        public PlayRecord getItem(int position) {
            return mList.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        protected View initConverView(int position, View convertView, ViewGroup parent) {
            ViewHolder holder = null;
            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.play_record_item, null);
                holder = new ViewHolder();
                holder.mIvPoster = (ImageView) convertView.findViewById(R.id.iv_poster);
                holder.mIvSelect = (ImageView) convertView.findViewById(R.id.iv_select);
                holder.mTvName = (TextView) convertView.findViewById(R.id.tv_name);
                holder.mTime = (TextView) convertView.findViewById(R.id.time);
                holder.mTvInfo = (TextView) convertView.findViewById(R.id.tv_info);
                holder.line_start = convertView.findViewById(R.id.lineStart);
                holder.line = convertView.findViewById(R.id.lineMid);
                holder.line_end = convertView.findViewById(R.id.lineEnd);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }
            return convertView;
        }

        @Override
        protected void showHolder(int position, AninViewHolder animHolder) {
            ViewHolder holder = (ViewHolder) animHolder;
            PlayRecord record = getItem(position);
            holder.mTvName.setText(record.getName());
            holder.mIvSelect.setVisibility(record.isSelected ? View.VISIBLE : View.INVISIBLE);
            holder.isSelected = record.isSelected;
            //设置图片
            String imgUrl = record.getPicurl();
            mImageLoader.displayImage(imgUrl, holder.mIvPoster, mOptions);
            
            SimpleDateFormat format=new SimpleDateFormat("yyyy-MM-dd   HH:mm");
            String time = format.format(record.getCreateTime());
            holder.mTime.setText(time);
            
            //设置续播信息
            float pro = (float) record.getPlayPos() / record.getDuration();
            if (record.getDuration() - record.getPlayPos() < PlayRecordHelper.IGNORE_LEFT_TIME) {
                holder.mTvInfo.setText(R.string.re_play);
            } else {
                if (record.getDuration() == 0 || record.getPlayPos() == 0) {
                    holder.mTvInfo.setText(R.string.continue_play_tip);
                } else {
                	SpannableString wathto = new SpannableString(getString(R.string.watch_to) + " ");
                	SpannableString percent = new SpannableString((int) (pro * 100) + "%");
                	percent.setSpan(new ForegroundColorSpan(getResources().getColor(R.color.home_cut_title_tab_color)), 0, percent.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                	SpannableStringBuilder builder = new SpannableStringBuilder(wathto); 
                	builder.append(percent);
                    holder.mTvInfo.setText(builder);
                }
            }
            
            // 线条
            holder.line_start.setVisibility(position == 0 ? View.VISIBLE : View.INVISIBLE);
            holder.line.setVisibility(position == getCount() - 1 ? View.INVISIBLE : View.VISIBLE);
            holder.line_end.setVisibility(position == getCount() - 1 ? View.VISIBLE : View.INVISIBLE);
        }

        class ViewHolder extends AninViewHolder {

            public ImageView mIvPoster;

            public ImageView mIvSelect;

            public TextView mTvName;

            public TextView mTime;
            public TextView mTvInfo;
            
            public View line_end;
            
    		public View line_start;
    		
    		public View line;
        }

    }


    private void loadData() {
        new LoadTask().execute();
    }

    @Override
	public void onStart() {
        super.onStart();
        if (hasStarted && UserManager.getInstance().isLogined()) {
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    loadData();
                }
            }, 400);
        }
        hasStarted = true;
    }

    private void onLoadComplete(List<PlayRecord> result) {
        mList.clear();
        if (result != null && result.size() > 0) {
            mList.addAll(result);
        }

        if (mList.size() > 0) {
            mEmptyView.setStatus(Status.Gone);
            mListView.setVisibility(View.VISIBLE);
//            showEditMenu();
        } else {
//            hiddenEditMenu();
            mEmptyView.setStatus(Status.Empty);
            mEmptyView.setEmptyTip(R.string.no_play_record_tip).setEmptyPic(R.drawable.ic_watch_history);
            
            mListView.setVisibility(View.GONE);
        }
        for (PlayRecord record : mList) {
            record.isSelected = false;
        }
        mAdapter.notifyDataSetChanged();
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
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.tv_operate_all:
                selectAll(true);
                break;
            case R.id.tv_operate_delete:
            	showDeleteMultipleTasksDialog();
                break;
        }
    }
    
    // 删除记录时的确认框
 	private void showDeleteMultipleTasksDialog() {
 		boolean deleteReady = false;
 		for (PlayRecord record : mList) {
            if (record.isSelected) {
            	deleteReady = true;
            	break;
            }
        }
 		if(!deleteReady)return;
 		
 		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity(), R.style.AlertDialog);
 		builder.setTitle(Util.setColourText(getActivity(), R.string.delete_tasks_confirmation, Util.DialogTextColor.BLUE));
 		final View checkView = LayoutInflater.from(getActivity()).inflate(R.layout.alert_dialog_checkbox, null);
 		builder.setView(checkView);

 		builder.setNegativeButton(Util.setColourText(getActivity(), R.string.ok, Util.DialogTextColor.BLUE), new DialogInterface.OnClickListener() {
 			@Override
 			public void onClick(DialogInterface dialog, int which) {
 				dialog.dismiss();
 				mAdapter.animScaleDelete(new AnimDeleteListener() {
                    @Override
                    public void onAnimDeleteEnd() {
                        new DeleteTask().execute();
                    }
                });
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

    private void enterEditMode() {
        selectNum = 0;
        isEditMode = true;
        setOperateViewVisibity(true, true);
//        freshEditMenu();
        showEditMenu();
    }

    private void quitEditMode(boolean refresh, boolean withAnimate) {
        selectNum = 0;
        isEditMode = false;
        setOperateViewVisibity(false, withAnimate);
//        freshEditMenu();
        hiddenEditMenu();
        if (refresh) {
            selectAll(false);
        }
    }

    private void setOperateViewVisibity(final boolean visiable, boolean withAnimate) {
        mAnimatorSet.setDuration(400);
        mAnimatorSet.removeAllListeners();
        mAnimatorSet.addListener(new AnimatorListener() {
			@Override
			public void onAnimationStart(Animator paramAnimator) {}
			
			@Override
			public void onAnimationRepeat(Animator paramAnimator) {}
			
			@Override
			public void onAnimationEnd(Animator paramAnimator) {
				if (!visiable) mOperateView.setVisibility(View.GONE);
			}
			
			@Override
			public void onAnimationCancel(Animator paramAnimator) {}
        });
        
        if (visiable) {
            mAnimatorSet.playTogether(
                    ObjectAnimator.ofFloat(mOperateView, "alpha", 0, 1),
                    ObjectAnimator.ofFloat(mOperateView, "translationY", mOperateView.getHeight() / 2, 0));

        } else {
            mAnimatorSet.playTogether(
                    ObjectAnimator.ofFloat(mOperateView, "alpha", 1, 0),
                    ObjectAnimator.ofFloat(mOperateView, "translationY", 0, mOperateView.getHeight() / 2));
        }
        
        if(withAnimate){
        	mOperateView.setVisibility(View.VISIBLE);
        	mAnimatorSet.start();            
        }else {
        	mOperateView.setVisibility(View.GONE);
        }
    }

    public void selectAll(boolean isSelectAll) {
        for (PlayRecord record : mList) {
            record.isSelected = isSelectAll;
        }
        selectNum = isSelectAll ? mList.size() : 0;
        setDeleteText();
        mAdapter.notifyDataSetChanged();
    }

    private void setDeleteText() {
        String base = getString(R.string.operate_delete);
        if (selectNum > 0) {
            mBtnDelete.setText(base + "(" + selectNum + ")");
        } else {
            mBtnDelete.setText(base);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        PlayRecord record = mAdapter.getItem(position);
        if (isEditMode) {
            record.isSelected = !record.isSelected;
            if (record.isSelected) {
                selectNum++;
            } else {
                selectNum--;
            }
            setDeleteText();
            mAdapter.notifyDataSetChanged();
        } else {
            new PlayerEntranceMgr(getActivity()).startgetPlayUrl(null,record.getMode(), record.getPicurl(), record.getSubTitleList(),
                    record.getName(), (int) record.getAlbumid(), record.getVedioid(), ServiceArg.URL_TYPE_PLAY, true);
        }
    }
    
    @Override
	public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
		if (!isEditMode) {
			PlayRecord record = mAdapter.getItem(position);
			record.isSelected = !record.isSelected;
			selectNum = 1;
	        isEditMode = true;
	        setOperateViewVisibity(true, true);
//	        freshEditMenu();
	        showEditMenu();
	        setDeleteText();
	        mAdapter.notifyDataSetChanged();
		}
		return true;
	}

    class LoadTask extends AsyncTask<Void, Void, List<PlayRecord>> {

        @Override
        protected List<PlayRecord> doInBackground(Void... params) {
            long uid = UserManager.getInstance().getUserId();
            List<PlayRecord> list = PlayRecordHelper.loadRecords(uid);
            return list;
        }

        @Override
        protected void onPostExecute(List<PlayRecord> result) {
            onLoadComplete(result);
        }
    }

    class DeleteTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... params) {
            List<PlayRecord> deleteList = new ArrayList<PlayRecord>();
            for (PlayRecord record : mList) {
                if (record.isSelected) {
                    deleteList.add(record);
                }
            }
            PlayRecordHelper.deleteRecords(deleteList);
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            quitEditMode(false, true);
            new LoadTask().execute();
        }
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        mMenu = menu;
        inflater.inflate(R.menu.multiedit, menu);
        mEditMenuItem = menu.findItem(R.id.edit);
        mEditMenuItem.setTitle(R.string.cancel);
        hiddenEditMenu();
//        if (mList.isEmpty()) {
//            hiddenEditMenu();
//        }
//        freshEditMenu();
    }

    MenuItem mEditMenuItem = null;

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.edit:
                if (!mAnimatorSet.isRunning()) {
                    mEditMenuItem = item;
                    if (isEditMode) {
                        quitEditMode(true, true);
                    } else {
                        enterEditMode();
                    }
                }
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

//    private void freshEditMenu() {
//        if (mEditMenuItem != null) {
//            mEditMenuItem.setTitle(isEditMode ? R.string.cancel : R.string.edit);
//        }
//    }

    public boolean onBackPressed() {
        if (isEditMode) {
            quitEditMode(true, true);
            return true;
        }
        return false;
    }
    
    public void setPageEnable(boolean pageEnable) {
    	if(!pageEnable)
        quitEditMode(true, false);
    }
    
    @Override
    public void onDestroy() {
    	super.onDestroy();
    	UserManager.getInstance().removeLoginStateChangeListener(this);
    }

	@Override
	public void onChange(LoginState lastState, LoginState currentState, Object userdata) {
		if(currentState == LoginState.LOGINED){
			//登录成功
			loadData();
		}else{
			mEmptyView.setStatus(Status.Empty);
			
			mEmptyView.setEmptyPic(R.drawable.ic_paypal_history);
       	 	SpannableString ss = new SpannableString(getString(R.string.need_login_to_get_play_record));  
            ss.setSpan(new ClickableSpan(){
    			@Override
    			public void onClick(View widget) {
    				NewBaseRotateActivity.bitmap = null;
    				startActivity(new Intent(getActivity(), LoginActivity.class));
    			}}, 2, 4,Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);  
            TextView tvTextView = mEmptyView.gettvEmptyTip();
            tvTextView.setText(ss);  
            tvTextView.setMovementMethod(LinkMovementMethod.getInstance());  
		}
	}
}
