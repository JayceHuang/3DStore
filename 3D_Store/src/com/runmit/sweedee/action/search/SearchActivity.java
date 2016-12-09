package com.runmit.sweedee.action.search;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.app.ActionBar;
import android.app.ActionBar.LayoutParams;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnCancelListener;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Message;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.Toast;

import com.umeng.analytics.MobclickAgent;
import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.TdStoreMainView;
import com.runmit.sweedee.action.search.DefaultEffectFaces.DefaultEffectFacesParams;
import com.runmit.sweedee.action.search.DefaultEffectFaces.DefaultEffectFacesParams.EasyingType;
import com.runmit.sweedee.action.search.DefaultEffectFaces.DefaultEffectFacesParams.EffectType;
import com.runmit.sweedee.action.search.SearchAppFragment.App_Type;
import com.runmit.sweedee.datadao.DataBaseManager;
import com.runmit.sweedee.datadao.SearchHisRecord;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
import com.runmit.sweedee.model.SearchRecommendList;
import com.runmit.sweedee.model.VOSearch;
import com.runmit.sweedee.model.VOSearchKey;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.util.StaticHandler;
import com.runmit.sweedee.util.StaticHandler.MessageListener;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.PagerSlidingTabStrip;

public class SearchActivity extends FragmentActivity implements OnClickListener, MessageListener, OnEditorActionListener, TextWatcher,
		View.OnTouchListener, OnItemClickListener, DialogInterface.OnDismissListener, SearchHotkeyLayout.IOnTextItemClickListener {

	/*
	// static 
	*/
	private static final String ANIM_ENABLED = "search_ani_enable"; //动画标志
	private static final String TAG = "SearchActivity";				//TAG
	private static final int MAX_RECORD_SIZE = 4;					// 搜索记录条数
	
	/*
	// 顶栏控件
	*/
	private EditText mEtInputBox;		// 搜索输入框
	private ImageView mIvInputBoxclear; // 清除输入内容的按钮 
	private TextView mTopBarBackBtn;	// 取消按钮
	private TextView mTvInputboxQuery;	// 搜索及
	private ImageView mInputboxBg;		// 输入框背景
	
	/*
	// 搜索结果-列表
	*/
	private RelativeLayout mRLEmptyResultContainner;		// 搜索结果为空
	private ViewPager mResultViewPager;						// 展示搜索结果的viewpager
	private PagerSlidingTabStrip mResultTabs;				// viewpager SlidingTab 控件
	private LinearLayout mSearchResultContainer;					// 搜索结果的总容器

    private boolean hasResult;								// 标记是否搜索到东西
    private long searchStartTime;							// 搜索开始时间，用于上报
    
	/*
	// 热搜词
	*/
	private ArrayList<VOSearchKey> mKeywordList;			// 热搜词数据列表
    private LinearLayout mHKMovieLayout;					// 热搜词视频部分的layout容器
    private LinearLayout mHKGameLayout;						// 热搜词游戏部分的layout容器
    private LinearLayout mHKAppLayout;						// 热搜词应用部分的layout容器
	private RelativeLayout mRecommendContainer;				// 热搜词layout容器
	
	/*
	// 搜索记录
	*/
	private GridView mSearchHisGridView;					// 搜索记录gridView容器
	private TextView mClearHisBtn;							// 清除搜索记录按钮
	private SearchHistoryAdapter hisAdapter;				// 搜索记录gridView adapter
	private RelativeLayout mSearchHisContainer;				// 搜索记录的容器 

	/*
	// 页面进入效果
	*/
	private View actView;									// activity view
	private FlipViewGroup effectView;						// 折纸效果的view
	private boolean isAnimating;							// 是否在展开动画
	
	private XL_Log mLog = new XL_Log(SearchActivity.class);	// xl_log
	private InputMethodManager mIMM;						// 输入框
	private StaticHandler mHandler;							// handler
	private String mLastKeyword = null;						// 搜索词
	private ProgressDialog mDialogInit;						// Loading 对话框
	private int lastPage;									// 默认定位搜索结果到首页对应的tab
	
	/*
	// 状态机
	*/
	private static final int STATE_INIT = 0;
	private static final int STATE_INITTED = STATE_INIT +1;
	private static final int STATE_LOADING = STATE_INITTED +1;
	private static final int STATE_RESULT = STATE_LOADING +1;
	private static final int STATE_LOADING_ERROR = STATE_RESULT +1;
	
	private int mStatus;
	private int mStateBeforeLoading;
		
    /**
     * 带效果启动activity
     * @param activity
     * @param userinfo 传入intent的hashmap info
     */
    public static void start(Activity activity, HashMap<String, Integer> userinfo) {
        Intent intent = new Intent(activity, SearchActivity.class);
        intent.putExtra(ANIM_ENABLED, true);
        if(userinfo != null){
        	for (Map.Entry<String, Integer> entry : userinfo.entrySet()) {  
        	    //Log.d(TAG, "key= " + entry.getKey() + " and value= " + entry.getValue());
        	    intent.putExtra(entry.getKey(), entry.getValue());
        	}  
        }
        Activity3DEffectTool.startActivity(activity, intent);
    }
    
    /**
     *  响应异步事件
     */
 	@Override
 	public void handleMessage(Message msg) {
 		switch (msg.what) {
 		case CmsEventCode.EVENT_GET_HOT_SEARCHKEY:
 			handleRecommendData(msg);
 			break;
 		case CmsEventCode.EVENT_GET_SEARCH:
 			handleSearchResultData(msg);
 			break;
 		case FlipViewGroup.MSG_ANIMATOIN_FIN:
 			isAnimating = false;
 			if(mStatus >= STATE_INITTED)return;
 			setCurrentState(STATE_INITTED);
 			Activity3DEffectTool.isLaunching = false;
 		default:
 			break;
 		}
 	}
 	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mHandler = new StaticHandler(SearchActivity.this);
		lastPage = getIntent().getIntExtra(TdStoreMainView.CURRENT_TAB_KEY, 0);
		setSelfContentView();
		initViewsAndEvent();
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		Activity3DEffectTool.isLaunching = false;
	}
	
	/**
	 * 设置contentview
	 */
	private void setSelfContentView() {
		isAnimating = getIntent().getBooleanExtra(ANIM_ENABLED, false);
		actView = View.inflate(this, R.layout.activity_search, null);
		if (isAnimating) {
			effectView = new FlipViewGroup(this, actView, false, mHandler, new DefaultEffectFacesParams(EffectType.Folding, EasyingType.Easying));
			setContentView(effectView);
		}else {
			setContentView(actView);
		}
	}
	
	/**
	 * 初始化actionbar
	 */
	private void initActionBar() {
		ActionBar actionBar = getActionBar();
		if (null != actionBar) {
			actionBar.setDisplayShowHomeEnabled(false); 
			actionBar.setDisplayShowCustomEnabled(true);
			actionBar.setDisplayShowTitleEnabled(false);
			actionBar.setDisplayUseLogoEnabled(false);
			LayoutInflater inflator = (LayoutInflater) this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			View v = inflator.inflate(R.layout.search_resource_input_box, null);
			ActionBar.LayoutParams layout = new ActionBar.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
			actionBar.setCustomView(v, layout);
		}
	}

	/**
	 * 初始化ui, 事件
	 */
	private void initViewsAndEvent() {

		initActionBar();
		
		// 顶部输入框部分
		mEtInputBox = (EditText) findViewById(R.id.search_resource_input_box_keyword_et);
		mIvInputBoxclear = (ImageView) findViewById(R.id.search_resource_input_box_clear_iv);
		mTopBarBackBtn = (TextView) findViewById(R.id.search_resource_input_box_back_btn);
		mTvInputboxQuery = (TextView) findViewById(R.id.search_resource_input_box_search_btn);
		mInputboxBg = (ImageView) findViewById(R.id.input_box_bg_rl);
		// 虚拟键盘 对象
		mIMM = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
		// 创建Loading
		mDialogInit = new ProgressDialog(this, R.style.ProgressDialogDark);
		// 搜索记录
		mSearchHisGridView = (GridView) findViewById(R.id.search_history_body_gv);
		mClearHisBtn = (TextView) findViewById(R.id.search_history_clear);
		//热搜词
        mHKMovieLayout = (LinearLayout) findViewById(R.id.hotkey_movie);
        mHKGameLayout = (LinearLayout) findViewById(R.id.hotkey_game);
        mHKAppLayout = (LinearLayout) findViewById(R.id.hotkey_app);
		mRecommendContainer = (RelativeLayout) findViewById(R.id.search_recommend_containner_rl);
		// 搜索结果
		mSearchResultContainer = (LinearLayout)findViewById(R.id.searchResult);
		mResultTabs = (PagerSlidingTabStrip) findViewById(R.id.search_result_tabs);
		mSearchHisContainer = (RelativeLayout) findViewById(R.id.search_history_containner_rl);
		// 没搜索结果 mLvList显示的内容
		mRLEmptyResultContainner = (RelativeLayout) findViewById(R.id.search_empty_result_containner_rl);
		initViewPager();
		// add event listener
		mEtInputBox.setOnEditorActionListener(this);
		mEtInputBox.addTextChangedListener(this);
		mEtInputBox.setOnTouchListener(this);
		mIvInputBoxclear.setOnClickListener(this);
		mTopBarBackBtn.setOnClickListener(this);
		mTvInputboxQuery.setOnClickListener(this);
		mSearchHisGridView.setOnItemClickListener(this);
		mClearHisBtn.setOnClickListener(this);
		mDialogInit.setOnDismissListener(this);
		
		mFragmentsList.add(new SearchComplexFragment());
		mFragmentsList.add(new SearchVideoFragment());
		if(!StoreApplication.isSnailPhone) {
			mFragmentsList.add(new SearchAppFragment(App_Type.GAME));
		}
		mFragmentsList.add(new SearchAppFragment(App_Type.APP));
		 		
        mResultViewPager.setAdapter(new UIFragmentPageAdapter(getSupportFragmentManager()));
        		
		mResultTabs.setViewPager(mResultViewPager);
		mResultTabs.setDividerColorResource(R.color.background_green);
		mResultTabs.setIndicatorColorResource(R.color.thin_blue);
		mResultTabs.setSelectedTextColorResource(R.color.thin_blue);
		mResultTabs.setDividerColor(Color.TRANSPARENT);
		DisplayMetrics dm = getResources().getDisplayMetrics();
		mResultTabs.setUnderlineHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, dm));
		mResultTabs.setIndicatorHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 2, dm));
		mResultTabs.setTextSize((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 16, dm));
		mResultTabs.setTextColor(getResources().getColor(R.color.game_tabs_text_color));
		
 		mResultViewPager.setCurrentItem(lastPage);
 		
		// 准备请求数据
		enterInit();
	}

	private void initViewPager() {
		mResultViewPager = new ViewPager(this) {
			@Override
			public boolean onTouchEvent(MotionEvent event) {
				try{
					return super.onTouchEvent(event);
				} catch(IllegalArgumentException ex) {}  
				return false; 
			}
		};
		mResultViewPager.setId(R.id.search_tab_vp);
		LinearLayout.LayoutParams listLayoutParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
		mSearchResultContainer.addView(mResultViewPager, listLayoutParams);
	}

	/**
	 * 进入加载初始数据状态
	 */
	private void enterInit() {
		setCurrentState(STATE_INIT);
		genLocalHistoryList();
		CmsServiceManager.getInstance().getHotSearchKey(mHandler);
	}
	
	@Override
    public void onTextItemClick(String string) {
        setEditText(string);
        enterLoading(mStatus, string);
    }
	
	/**
	 * 修改搜索页面状态
	 * @param status
	 * @param isUpdateUI
	 */
	private void setCurrentState(int status){
		setCurrentState(status, true);
	}
	
	private void setCurrentState(int status, boolean isUpdateUI){
		mStatus = status;
		
		if(isUpdateUI)
		switch(mStatus){
		case STATE_INIT:
			// 隐藏所有推荐榜单的UI
			toggleRecommendLayer(false);
			// 隐藏 所有与搜索相关的UI
			toggleResultContainer(false);
			mLog.debug("state init.");
 			break;
		case STATE_INITTED:
			Util.dismissDialog(mDialogInit);
			toggleResultContainer(false);
			toggleHistoryLayer(true);
			toggleRecommendLayer(true);
 			mEtInputBox.requestFocus();
 			showOrHideIme(true);
 			mLog.debug("state inited.");
			break;
		case STATE_LOADING:
			showOrHideIme(false);
			// 进入一个新的keyword生命周期
			Util.showDialog(mDialogInit, getString(R.string.searching), false);
			mLog.debug("state loading.");
			break;
		case STATE_LOADING_ERROR:
			mLog.debug("enterError() LOADING_ERROR");
			Util.dismissDialog(mDialogInit);
			String msg = getString(R.string.search_resource_query_failed);
			showLoadingErrorDlg(msg);
			mLog.debug("state error.");
			break;
		case STATE_RESULT:
			// 设置空显示
			toggleHistoryLayer(false);
			toggleResultContainer(true);
			toggleRecommendLayer(false);
			switchCancleNSearchBtn(true);
			Util.dismissDialog(mDialogInit);
			mLog.debug("state result.");
			break;
		default:
			break;
		}
	}

	/**
	 * 开始搜索
	 * 
	 * @param lastStatus 上次状态
	 * @param keyword 搜索词
	 */
	private void enterLoading(int lastStatus, String keyword) {
		if(isAnimating)return; // 动画结束前不去响应搜索
		if (mStatus != STATE_INITTED && mStatus != STATE_RESULT) {
			return;
		}
		if (keyword == null || keyword.length() == 0) {
			String msg = getString(R.string.search_resource_info_input_is_null);
			toast(msg);
			return;
		}
		if (keyword.length() > 50) {
			String msg = getString(R.string.search_resource_info_input_excceed_max_num);
			toast(msg);
			return;
		}
		if (!Util.isNetworkAvailable(SearchActivity.this)) {
			String msg = getString(R.string.search_resource_info_no_network);
			toast(msg);
			return;
		}
		
		// 保存现场-状态机相关
		mStateBeforeLoading = lastStatus;
		setCurrentState(STATE_LOADING);
		mLog.debug("enterLoading() LOADING");
		
		mLastKeyword = keyword;
		//搜索输入"微"appinfo有内容，wifi staging
		CmsServiceManager.getInstance().getSearch(mLastKeyword, 0, 100, mHandler);
        searchStartTime = System.currentTimeMillis();
	}
	
	/*
	// 错误提示框
	*/
	private AlertDialog.Builder mLoadingErrorDialog = null;
	private void showLoadingErrorDlg(String msg) {
		if (null == mLoadingErrorDialog) {
			mLoadingErrorDialog = new AlertDialog.Builder(this,R.style.AlertDialog);
			mLoadingErrorDialog.setTitle(Util.setColourText(this,R.string.fetch_data_error, Util.DialogTextColor.BLUE));
			mLoadingErrorDialog.setMessage(msg);
			mLoadingErrorDialog.setOnCancelListener(new OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					try {
						dialog.dismiss();
						setCurrentState(mStateBeforeLoading);
					} catch (Exception e) {
						e.printStackTrace();
					} finally {
						mLoadingErrorDialog = null;
					}
				}
			});

			mLoadingErrorDialog.setPositiveButton(getString(R.string.cancel),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        try {
                            dialog.dismiss();
                            setCurrentState(mStateBeforeLoading);
                        } catch (Exception e) {
                            e.printStackTrace();
                        } finally {
                            mLoadingErrorDialog = null;
                        }
                    }
                });

			mLoadingErrorDialog.setNegativeButton(Util.setColourText(this, R.string.redo, Util.DialogTextColor.BLUE),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        try {
                            dialog.dismiss();
                            // 恢复现场
                            mLog.debug("showLoadingErrorDlg status:" + mStatus + " return upper state " + mStateBeforeLoading);
                            setCurrentState(mStateBeforeLoading, false);
                            enterLoading(mStatus, mLastKeyword);
                        } catch (Exception e) {
                            e.printStackTrace();
                        } finally {
                            mLoadingErrorDialog = null;
                        }
                    }
                });
			mLoadingErrorDialog.create();
		}
		if (!isFinishing()) {
			mLoadingErrorDialog.show();
		}
	}

	@Override
	public void onDismiss(DialogInterface dialog) {
		switch (mStatus) {
		case STATE_INIT:
			mEtInputBox.postDelayed(new Runnable() {
				@Override
				public void run() {
					finishActivity();
				}
			}, 50);
			break;
		case STATE_LOADING:
			setCurrentState(mStateBeforeLoading);
			break;
		default:
			break;
		}
	}

	// 响应物理键Back
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			handleBack();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	private void handleBack() {
		switch (mStatus) {
		case STATE_INITTED:
			finishActivity();
			break;
		case STATE_RESULT:
			setCurrentState(STATE_INITTED);
			break;
		default:
			finishActivity();
			break;
		}
	}
	
	private void finishActivity(){
		showOrHideIme(false);
		mLog.debug("current state:"+mStatus);
		this.finish();
	}
	
	// 响应点击事件
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.search_resource_input_box_clear_iv:
			clearInputBox();
			break;
		case R.id.search_resource_input_box_back_btn:
			setCurrentState(STATE_INITTED, false);
			handleBack();
			break;
		case R.id.search_resource_input_box_search_btn:
			enterLoading(mStatus, getEditTextContent());
			break;
		case R.id.search_history_clear:
			clearAllSearchHistory();
			break;
		default:
			break;
		}
	}

	// 响应列表点击事件
	@Override
	public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
		switch (v.getId()) {
		case R.id.search_history_list_item_id:
			String keywordFromHistory = hisAdapter.getItem(position);
			setEditText(keywordFromHistory);
			enterLoading(mStatus, keywordFromHistory);
			break;
		default:
			break;
		}

	}

	// EditText的响应函数
	public void afterTextChanged(Editable s) {}

	public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

	public void onTextChanged(CharSequence s, int start, int before, int count) {
		if (TextUtils.isEmpty(s)) {
			showOrHideClearBtn(false);
			switchCancleNSearchBtn(true);
		} else {
			showOrHideClearBtn(true);
			switchCancleNSearchBtn(false);
		}
	}

	// IME的响应函数
	@Override
	public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
		if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
			String key = getEditTextContent();
			enterLoading(mStatus, key);
			return true;
		}
		if (actionId == EditorInfo.IME_ACTION_SEARCH || actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_GO || actionId == EditorInfo.IME_ACTION_NEXT) {
			String key = getEditTextContent();
			enterLoading(mStatus, key);
			return true;
		}
		return false;
	}

	// 响应点击事件
	@Override
	public boolean onTouch(View view, MotionEvent event) {
		if(R.id.search_resource_input_box_keyword_et == view.getId()) {
			showOrHideIme(true);
			mEtInputBox.requestFocus();
			return true;
		}
		return false;
	}

	private void handleSearchResultData(Message msg) {
		if(mStatus >= STATE_RESULT)return;
		int stateCode = msg.arg1;
		if (stateCode != 0) {
			if (mStatus != STATE_LOADING) {
				return;
			}
			setCurrentState(STATE_LOADING_ERROR);
			return;
		}
        VOSearch searchResult = (VOSearch) msg.obj;

        hasResult = searchResult.ALBUM != null && searchResult.ALBUM.list.size() > 0 ||
        			searchResult.LIGHTALBUM != null && searchResult.LIGHTALBUM.list.size() > 0 ||
        			searchResult.GAME != null && searchResult.GAME.list.size() > 0 ||
        			searchResult.APP != null && searchResult.APP.list.size() > 0;
        			
        mLog.debug("handleSearchResultData:" + searchResult.ALBUM.list.size());
        
        for(BaseSearchFragemnt frag : mFragmentsList){
        	frag.setSearchResult(searchResult, mLastKeyword);
        }
        
		addSearchHistory(mLastKeyword);
		setCurrentState(STATE_RESULT);
		
		// report
		long duration = System.currentTimeMillis() - searchStartTime;
		ReportActionSender.getInstance().search(mLastKeyword, duration, hasResult);
	}
	

	/**
	 * 动画结束前，处理推荐返回
	 * @param msg msg.obj为List(String) 对象
	 */
	private void handleRecommendData(Message msg) {
		if(mStatus >= STATE_INITTED) return;
		SearchRecommendList recommendData = (SearchRecommendList) msg.obj;
        mKeywordList = new ArrayList<VOSearchKey>();
        if (recommendData == null) return;

        mKeywordList.addAll(recommendData.list);
		if (mKeywordList.size() > 0) {
            SearchHotkeyLayout layer1 = new SearchHotkeyLayout(this);
            layer1.setDataList(this, mKeywordList.get(0).list);
            mHKMovieLayout.addView(layer1);
            if(!StoreApplication.isSnailPhone){
	            SearchHotkeyLayout layer2 = new SearchHotkeyLayout(this);
	            layer2.setDataList(this, mKeywordList.get(2).list);
	            mHKGameLayout.addView(layer2);
	            findViewById(R.id.static_game_tab).setVisibility(View.VISIBLE);
	            mHKGameLayout.setVisibility(View.VISIBLE);
	            layer2.setOnTextItemClickListener(this);
            }
            SearchHotkeyLayout layer3 = new SearchHotkeyLayout(this);
            layer3.setDataList(this, mKeywordList.get(1).list);
            mHKAppLayout.addView(layer3);
            layer1.setOnTextItemClickListener(this);
            layer3.setOnTextItemClickListener(this);
		} else {
			mLog.debug("handle Recommend Data error:" + msg);
		}		
		toggleRecommendLayer(mKeywordList.size()>0);
	}
    
	/**
	 * 初始化本地搜索记录
	 */
	private void genLocalHistoryList() {
		ArrayList<SearchHisRecord> searchHistoryList = (ArrayList<SearchHisRecord>) DataBaseManager.getInstance().getSearchHisDao().loadAll();
		hisAdapter = new SearchHistoryAdapter(this);
		hisAdapter.clearList(false);
		
		for (int i = searchHistoryList.size() - 1, j = 0; i >= 0 && j < MAX_RECORD_SIZE; i--, j++) {
			hisAdapter.addToList(searchHistoryList.get(i).getWords());
		}

		toggleHistoryLayer(true);

		mSearchHisGridView.setAdapter(hisAdapter);
	}

	// /////////////////////////////////////////////////////////////
	// 其它辅助方法
	private void addSearchHistory(String words) {
		long time = System.currentTimeMillis();

		// 更新列表
		ArrayList<String> li = hisAdapter.wordList;
		ArrayList<String> tmplist = new ArrayList<String>();
		tmplist.add(words);
		if (!li.contains(words)) {
			int len = Math.min(MAX_RECORD_SIZE-1, li.size());
			tmplist.addAll(li.subList(0, len));
		} else {
			hisAdapter.wordList.remove(words);
			tmplist.addAll(li);
		}
		hisAdapter.clearList(false);
		hisAdapter.addLists(tmplist);
		hisAdapter.notifyDataSetChanged();
		toggleHistoryLayer(false);

		// 插入数据库
		SearchHisRecord tmpRec = new SearchHisRecord(words, time);
		long i = DataBaseManager.getInstance().getSearchHisDao().insertOrReplace(tmpRec);
		mLog.debug("add success:" + words + "  i:" + i);
	}

	private void clearAllSearchHistory() {
		SearchHistoryAdapter ad = (SearchHistoryAdapter) mSearchHisGridView.getAdapter();
		if (null != ad) {
			ad.clearList(true);
		}
		DataBaseManager.getInstance().getSearchHisDao().deleteAll();
		toggleHistoryLayer(false);
		showOrHideIme(false);
		mLog.debug("delete all search history.");
	}

	private void toast(String msg) {
		Util.showToast(this, msg, Toast.LENGTH_SHORT);
	}

	private void showOrHideClearBtn(boolean isShow) {
		if (isShow) {
			mIvInputBoxclear.setVisibility(View.VISIBLE);
			mInputboxBg.setImageResource(R.drawable.top_seach_input_box_down);
		} else {
			mIvInputBoxclear.setVisibility(View.INVISIBLE);
			mInputboxBg.setImageResource(R.drawable.top_seach_input_box_normal);
		}
	}

	private void switchCancleNSearchBtn(boolean showCancleBtn) {
		if (showCancleBtn) {
			mTopBarBackBtn.setVisibility(View.VISIBLE);
			mTvInputboxQuery.setVisibility(View.GONE);
		} else {
			mTopBarBackBtn.setVisibility(View.GONE);
			mTvInputboxQuery.setVisibility(View.VISIBLE);
		}
	}

	/**
	 * 切换搜索结果 模块显隐状态
	 * @param isShow 是否显示
	 */
	private void toggleResultContainer(boolean isShow) {
		if (isShow) {
            if(hasResult){
            	mSearchResultContainer.setVisibility(View.VISIBLE);
        		mRLEmptyResultContainner.setVisibility(View.GONE);
            }else {
                mRLEmptyResultContainner.setVisibility(View.VISIBLE);
            }
			return;
		}
		mSearchResultContainer.setVisibility(View.INVISIBLE);
		mRLEmptyResultContainner.setVisibility(View.GONE);
	}
	
	/**
	 * 
	 * 切换热搜词模块显隐状态
	 * @param force			默认状态
	 */
	private void toggleRecommendLayer(boolean force) {
		boolean isShow = force && mKeywordList != null && mKeywordList.size() > 0;

		if (isShow) {
			mRecommendContainer.setVisibility(View.VISIBLE);
			return;
		}
		mRecommendContainer.setVisibility(View.GONE);
	}

	/**
	 * 切换本地数据模块显隐状态
	 * 
	 * @param flag
	 */
	private void toggleHistoryLayer(boolean flag) {
		if (flag && hisAdapter != null && hisAdapter.getCount() > 0) {
			mSearchHisContainer.setVisibility(View.VISIBLE);
			return;
		}
		mSearchHisContainer.setVisibility(View.GONE);
	}

	/**
	 * 切换虚拟键盘显隐状态
	 * @param isShow 是否显示
	 */
	private void showOrHideIme(boolean isShow) {
		if (isShow) {
			mIMM.showSoftInput(mEtInputBox,InputMethodManager.SHOW_FORCED); 
		} else {
			mIMM.hideSoftInputFromWindow(mEtInputBox.getWindowToken(), 0);
		}
	}

	private String getEditTextContent() {
		if (null == mEtInputBox.getText()) {
			return null;
		}
		String keyword = mEtInputBox.getText().toString();

		if (TextUtils.isEmpty(keyword)) {
			return null;
		}

		return keyword.trim();
	}

	private void setEditText(String keyword) {
		mEtInputBox.setText(keyword);
		mEtInputBox.setSelection(mEtInputBox.getText().length());
	}

	private void clearInputBox() {
		mEtInputBox.setText("");
	}

    @Override
    protected void onResume() {
        super.onResume();
        MobclickAgent.onResume(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        MobclickAgent.onPause(this);
        showOrHideIme(false);
    }
    
    private List<BaseSearchFragemnt> mFragmentsList = new ArrayList<BaseSearchFragemnt>();
	// /////////////////////////////////////////////////////////////////////////////////////////////
	// 本地搜索记录的Adapter
	private class SearchHistoryAdapter extends BaseAdapter {
		private ArrayList<String> wordList = new ArrayList<String>();
		private LayoutInflater mInflater;

		public SearchHistoryAdapter(Activity aty) {
			mInflater = aty.getLayoutInflater();
		}

		public void addToList(String item) {
			wordList.add(item);
		}

		public void addLists(ArrayList<String> ls) {
			wordList.addAll(ls);
			this.notifyDataSetChanged();
		}

		public void clearList(boolean updateView) {
			wordList.clear();
			if (updateView) {
				this.notifyDataSetChanged();
			}
		}

		@Override
		public int getCount() {
			return wordList.size();
		}

		@Override
		public String getItem(int position) {
			return wordList.get(position);
		}

		@Override
		public long getItemId(int position) {
			return (long) position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			ListItemViewHolder holder = null;
			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.search_history_list_item, null);

				holder = new ListItemViewHolder();
				holder.wordTv = (TextView) convertView.findViewById(R.id.search_history_list_item_text_tv);

				convertView.setTag(holder);
			} else {
				holder = (ListItemViewHolder) convertView.getTag();
			}

			holder.wordTv.setText(wordList.get(position));

			return convertView;
		}

		private class ListItemViewHolder {
			public TextView wordTv;
		}

	}

	/*
	// viewpager fragment adapter
	*/
	private class UIFragmentPageAdapter extends FragmentPagerAdapter{
		private String[] mHomeTasTitles = getResources().getStringArray(R.array.search_tabs);
		public UIFragmentPageAdapter(FragmentManager fm) {
			super(fm);
			if(StoreApplication.isSnailPhone){
				mHomeTasTitles = new String[] {mHomeTasTitles[0], mHomeTasTitles[1], mHomeTasTitles[3], mHomeTasTitles[4]};
			}
		}
		
		@Override
		public CharSequence getPageTitle(int position) {
			return mHomeTasTitles[position];
		}

		@Override
		public BaseFragment getItem(int position) {
			return mFragmentsList.get(position);
		}

		@Override
		public int getCount() {
			return mFragmentsList.size();
		}
		
	}
}
