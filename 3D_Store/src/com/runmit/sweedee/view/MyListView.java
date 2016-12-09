package com.runmit.sweedee.view;

import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.graphics.Canvas;
import android.os.Build;
import android.util.AttributeSet;
import android.util.Log;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.View.OnClickListener;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.LinearInterpolator;
import android.view.animation.RotateAnimation;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.AbsListView.OnScrollListener;

import com.runmit.sweedee.R;
import com.runmit.sweedee.action.movie.DetailAboutFragment;
import com.runmit.sweedee.util.XL_Log;

/**
 * 下拉刷新listview
 */
public class MyListView extends ListView {
	private XL_Log log = new XL_Log(MyListView.class);
    private static final float PULL_RESISTANCE = 1.7f;
    private static final int BOUNCE_ANIMATION_DURATION = 700;
    private static final int BOUNCE_ANIMATION_DELAY = 100;
    private static final float BOUNCE_OVERSHOOT_TENSION = 1.4f;
    private static final int ROTATE_ARROW_ANIMATION_DURATION = 250;
    private static final int PULLREFRESH_MAX_OFFSET = 56;
    

    public static enum State {
        PULL_TO_REFRESH, RELEASE_TO_REFRESH, REFRESHING
    }

    /**
     * Interface to implement when you want to get notified of 'pull to refresh'
     * events. Call setOnRefreshListener(..) to activate an OnRefreshListener.
     */
    public interface OnRefreshListener {

        /**
         * Method to be called when a refresh is requested
         */
        public void onRefresh();
    }

    private static int measuredHeaderHeight;

    private boolean lockScrollWhileRefreshing;
    private String pullToRefreshText;
    private String releaseToRefreshText;
    private String refreshingText;
    private String lastUpdatedText;
    private SimpleDateFormat lastUpdatedDateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault());

    private float previousY;
    private int headerPadding;
    private boolean hasResetHeader;
    private long lastUpdated = -1;
    protected State state;
    private LinearLayout headerContainer;
    private LinearLayout header;
    private ObjectAnimator mObjectAnimator;
    private ImageView image;
    private ProgressBar spinner;
    private TextView text;
    private TextView lastUpdatedTextView;
    private OnItemClickListener onItemClickListener;
    private OnItemLongClickListener onItemLongClickListener;
    private OnRefreshListener onRefreshListener;

    private float mScrollStartY;
    private final int IDLE_DISTANCE = 5;

    private ValueAnimator mScrollValueAnimator;
    
	private View mFootView;
	private TextView tv_loadingtip;
	private ProgressBar progress;
	private LinearLayout loading_layout;
	private FootStatus footStatus;
	private OnLoadMoreListenr mOnLoadMoreListenr;

    public MyListView(Context context) {
        super(context);
        init();
    }

    public MyListView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public MyListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

	public void setOnLoadMoreListenr(OnLoadMoreListenr mOnLoadMoreListenr) {
		this.mOnLoadMoreListenr = mOnLoadMoreListenr;
	}
	
    /**
     * onItemClickListener
     */
    public void setOnItemClickListener(OnItemClickListener onItemClickListener) {
        this.onItemClickListener = onItemClickListener;
    }

    @Override
    public void setOnItemLongClickListener(OnItemLongClickListener onItemLongClickListener) {
        this.onItemLongClickListener = onItemLongClickListener;
    }

    /**
     * Activate an OnRefreshListener to get notified on 'pull to refresh'
     * events.
     *
     * @param onRefreshListener The OnRefreshListener to get notified
     */
    public void setonRefreshListener(OnRefreshListener onRefreshListener) {
        this.onRefreshListener = onRefreshListener;
        isRefreshable = true;
    }

    /**
     * @return If the list is in 'Refreshing' state
     */
    public boolean isRefreshing() {
        return state == State.REFRESHING;
    }

    /**
     * Default is false. When lockScrollWhileRefreshing is set to true, the list
     * cannot scroll when in 'refreshing' mode. It's 'locked' on refreshing.
     *
     * @param lockScrollWhileRefreshing
     */
    public void setLockScrollWhileRefreshing(boolean lockScrollWhileRefreshing) {
        this.lockScrollWhileRefreshing = lockScrollWhileRefreshing;
    }

    /**
     * Default: "dd/MM HH:mm". Set the format in which the last-updated
     * date/time is shown. Meaningless if 'showLastUpdatedText == false
     * (default)'. See 'setShowLastUpdatedText'.
     *
     * @param lastUpdatedDateFormat
     */
    public void setLastUpdatedDateFormat(SimpleDateFormat lastUpdatedDateFormat) {
        this.lastUpdatedDateFormat = lastUpdatedDateFormat;
    }

    /**
     * Explicitly set the state to refreshing. This is useful when you want to
     * show the spinner and 'Refreshing' text when the refresh was not triggered
     * by 'pull to refresh', for example on start.
     */
    public void setRefreshing() {
        state = State.REFRESHING;
        scrollTo(0, 0);
        setUiRefreshing();
        setHeaderPadding(0);
    }

    public State getCurrentState() {
        return this.state;
    }

    // Nexus 5(4.4系统)点击快速滑动块的时候会回调长按事件，处理之
    private void checkFastScrollAbility() {
        if ("Nexus 5".equals(Build.MODEL)) {
            setFastScrollEnabled(false);
        }
    }

    /**
     * Set the state back to 'pull to refresh'. Call this method when refreshing
     * the data is finished.
     */
    public void onRefreshComplete() {
        state = State.PULL_TO_REFRESH;
        resetHeader();
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm");
        lastUpdatedTextView.setText(MessageFormat.format(getResources().getString(R.string.ptr_last_updated), sdf.format(new Date())));
    }

    /**
     * Change the label text on state 'Pull to Refresh'
     *
     * @param pullToRefreshText Text
     */
    public void setTextPullToRefresh(String pullToRefreshText) {
        this.pullToRefreshText = pullToRefreshText;
        if (state == State.PULL_TO_REFRESH) {
            text.setText(pullToRefreshText);
        }
    }

    /**
     * Change the label text on state 'Release to Refresh'
     *
     * @param releaseToRefreshText Text
     */
    public void setTextReleaseToRefresh(String releaseToRefreshText) {
        this.releaseToRefreshText = releaseToRefreshText;
        if (state == State.RELEASE_TO_REFRESH) {
            text.setText(releaseToRefreshText);
        }
    }

    /**
     * Change the label text on state 'Refreshing'
     *
     * @param refreshingText Text
     */
    public void setTextRefreshing(String refreshingText) {
        this.refreshingText = refreshingText;
        if (state == State.REFRESHING) {
            text.setText(refreshingText);
        }
    }

    @Override
    protected void layoutChildren() {
        try {
            super.layoutChildren();
        } catch (IllegalStateException e) {// 用此方法规避IllegalStateException崩溃
            ListAdapter adapter = getAdapter();
            if (adapter instanceof BaseAdapter) {
                ((BaseAdapter) adapter).notifyDataSetChanged();
                super.layoutChildren();
            }
        }
    }
    
	private OnScrollListener mOnScrollListener = new OnScrollListener() {

		boolean can_touch = true;

		public void onScrollStateChanged(AbsListView view, int scrollState) {
			switch (scrollState) {
			case SCROLL_STATE_TOUCH_SCROLL:
				break;
			case OnScrollListener.SCROLL_STATE_IDLE:
				int count = view.getCount();
				int last = view.getLastVisiblePosition();
				if ((last == (count - 1)) && can_touch && footStatus == FootStatus.Common) {
					mFootView.performClick();
				}
			default:
				break;
			}
		}

		public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount) {
			can_touch = (visibleItemCount != totalItemCount);
		}
	};

    private void init() {
        setVerticalFadingEdgeEnabled(false);

        headerContainer = (LinearLayout) LayoutInflater.from(getContext()).inflate(R.layout.head, null);
        header = (LinearLayout) headerContainer.findViewById(R.id.item_container);
        text = (TextView) header.findViewById(R.id.head_tipsTextView);
        lastUpdatedTextView = (TextView) header.findViewById(R.id.head_lastUpdatedTextView);
        image = (ImageView) header.findViewById(R.id.head_arrowImageView);
        spinner = (ProgressBar) header.findViewById(R.id.head_progressBar);

        pullToRefreshText = getContext().getString(R.string.pull_to_refresh_pull_label);
        releaseToRefreshText = getContext().getString(R.string.pull_to_refresh_release_label);
        refreshingText = getContext().getString(R.string.pull_to_refresh_refreshing_label);
        lastUpdatedText = getContext().getString(R.string.ptr_last_updated);

        mObjectAnimator = ObjectAnimator.ofFloat(image, "rotation", 0,180);
        mObjectAnimator.setDuration(ROTATE_ARROW_ANIMATION_DURATION);
        mObjectAnimator.setInterpolator(new LinearInterpolator());
        
        addHeaderView(headerContainer);
        setState(State.PULL_TO_REFRESH);
        isVerticalScrollBarEnabled();

        ViewTreeObserver vto = header.getViewTreeObserver();
        vto.addOnGlobalLayoutListener(new PTROnGlobalLayoutListener());

        super.setOnItemClickListener(new PTROnItemClickListener());
        super.setOnItemLongClickListener(new PTROnItemLongClickListener());
        
        setSelector(R.drawable.gridview_selector);
        checkFastScrollAbility();
        
        mFootView = LayoutInflater.from(getContext()).inflate(R.layout.list_footer, this, false);
		tv_loadingtip = (TextView) mFootView.findViewById(R.id.foot_tipsTextView);
		progress = (ProgressBar) mFootView.findViewById(R.id.foot_progressbar);
		loading_layout = (LinearLayout) mFootView.findViewById(R.id.loading_layout);
		this.setOnScrollListener(mOnScrollListener);
		mFootView.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				if (mOnLoadMoreListenr != null) {
					mOnLoadMoreListenr.onLoadData();
				}
				setFootStatus(FootStatus.Loading);
			}
		});
		addFooterView(mFootView);
    }

    private void setHeaderPadding(final int padding) {
        headerPadding = padding;
        post(new Runnable() {

            @Override
            public void run() {
                MarginLayoutParams mlp = (ViewGroup.MarginLayoutParams) header.getLayoutParams();
                mlp.setMargins(0, padding, 0, 0);
                header.setLayoutParams(mlp);
                invalidate();
            }
        });

    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (lockScrollWhileRefreshing && (state == State.REFRESHING || getAnimation() != null && !getAnimation().hasEnded())) {

            return true;
        }
        if(previousY==-1){
        	previousY = event.getY();
        }
        if (isRefreshable) {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    if (getFirstVisiblePosition() == 0) {
                        previousY = event.getY();
                    } else {
                        previousY = -1;
                    }
                    mScrollStartY = event.getY();
                    break;

                case MotionEvent.ACTION_UP:
                    if (previousY != -1 && (state == State.RELEASE_TO_REFRESH || getFirstVisiblePosition() == 0)) {
                        switch (state) {
                            case RELEASE_TO_REFRESH:
                                setState(State.REFRESHING);
                                bounceBackHeader();
                                break;
                            case PULL_TO_REFRESH:
                                resetHeader();
                                break;
                            default:
                                break;
                        }
                    }
                    break;

                case MotionEvent.ACTION_MOVE:
                    if (previousY != -1 && getFirstVisiblePosition() == 0 && Math.abs(mScrollStartY - event.getY()) > IDLE_DISTANCE) {
                    	
                        float y = event.getY();
                        float diff = y - previousY;
                        if (diff > 0) {
                            diff /= PULL_RESISTANCE;
                        }
                        previousY = y;

                        int newHeaderPadding = Math.max(Math.round(headerPadding + diff), -header.getHeight());
                            newHeaderPadding=Math.min(newHeaderPadding,PULLREFRESH_MAX_OFFSET);
                        if (newHeaderPadding != headerPadding && state != State.REFRESHING) {
                            setHeaderPadding(newHeaderPadding);

                            if (state == State.PULL_TO_REFRESH && headerPadding > 0) {
                                setState(State.RELEASE_TO_REFRESH);
                                image.setPivotX(image.getWidth()/2);
                                image.setPivotY(image.getHeight()/2);
                                mObjectAnimator.start();
                            } else if (state == State.RELEASE_TO_REFRESH && headerPadding < 0) {
                                setState(State.PULL_TO_REFRESH);
                                mObjectAnimator.reverse();
                            }
                        }
                    }
                    break;
            }
        }
        return super.onTouchEvent(event);
    }

    private void bounceBackHeader() {
        MarginLayoutParams mlp = (ViewGroup.MarginLayoutParams) header.getLayoutParams();
        final int originalHeight = mlp.topMargin;
        final int drawchildheight = 0;
        mScrollValueAnimator = ValueAnimator.ofInt(originalHeight, drawchildheight).setDuration(BOUNCE_ANIMATION_DURATION);
        mScrollValueAnimator.setInterpolator(new DecelerateInterpolator(BOUNCE_OVERSHOOT_TENSION));
        mScrollValueAnimator.addListener(new AnimatorListener() {

            @Override
            public void onAnimationStart(Animator arg0) {
            }

            @Override
            public void onAnimationRepeat(Animator arg0) {
            }

            @Override
            public void onAnimationEnd(Animator arg0) {
                setHeaderPadding(drawchildheight);
            }

            @Override
            public void onAnimationCancel(Animator arg0) {
                setHeaderPadding(drawchildheight);
            }
        });

        mScrollValueAnimator.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator valueAnimator) {
                int height = (Integer) valueAnimator.getAnimatedValue();
                setHeaderPadding(height);
            }
        });
        mScrollValueAnimator.start();
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        try {
            super.dispatchDraw(canvas);
        } catch (NullPointerException e) {
            e.printStackTrace();
        } catch (InflateException e) {
            e.printStackTrace();
        }

    }

    private void resetHeader() {
        if (mScrollValueAnimator != null) {
            mScrollValueAnimator.cancel();
            mScrollValueAnimator = null;
        }
        if (getFirstVisiblePosition() >= 0) {
            setHeaderPadding(-header.getHeight());
            setState(State.PULL_TO_REFRESH);
            return;
        }
    }

    private void setUiRefreshing() {
        spinner.setVisibility(View.VISIBLE);
        image.clearAnimation();
        image.setVisibility(View.INVISIBLE);
        text.setText(refreshingText);
    }

    private void setState(State state) {
        this.state = state;
        switch (state) {
            case PULL_TO_REFRESH:
                spinner.setVisibility(View.INVISIBLE);
                image.setVisibility(View.VISIBLE);
                text.setText(pullToRefreshText);
                if (lastUpdated != -1) {
                    SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm");
                    lastUpdatedTextView.setText(MessageFormat.format(getResources().getString(R.string.ptr_last_updated), sdf.format(new Date())));
                }
                break;
            case RELEASE_TO_REFRESH:
                spinner.setVisibility(View.INVISIBLE);
                image.setVisibility(View.VISIBLE);
                text.setText(releaseToRefreshText);
                break;

            case REFRESHING:
                setUiRefreshing();

                lastUpdated = System.currentTimeMillis();
                if (onRefreshListener == null) {
                    setState(State.PULL_TO_REFRESH);
                } else {
                    onRefreshListener.onRefresh();
                }

                break;
        }
    }

    @Override
    protected void onScrollChanged(int l, int t, int oldl, int oldt) {
        super.onScrollChanged(l, t, oldl, oldt);

        if (!hasResetHeader) {
            if (measuredHeaderHeight > 0 && state != State.REFRESHING) {
                setHeaderPadding(-measuredHeaderHeight);
            }

            hasResetHeader = true;
        }
    }

    private class PTROnGlobalLayoutListener implements OnGlobalLayoutListener {

        @Override
        public void onGlobalLayout() {
            int initialHeaderHeight = header.getHeight();

            if (initialHeaderHeight > 0) {
                measuredHeaderHeight = initialHeaderHeight;

                if (measuredHeaderHeight > 0 && state != State.REFRESHING) {
                    setHeaderPadding(-measuredHeaderHeight);
                    requestLayout();
                }
            }
            getViewTreeObserver().removeGlobalOnLayoutListener(this);
        }
    }

    private class PTROnItemClickListener implements OnItemClickListener {

        @Override
        public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
            hasResetHeader = false;
            if (onItemClickListener != null && state == State.PULL_TO_REFRESH) {
                onItemClickListener.onItemClick(adapterView, view, position, id);
            }
        }
    }

    private class PTROnItemLongClickListener implements OnItemLongClickListener {

        @Override
        public boolean onItemLongClick(AdapterView<?> adapterView, View view, int position, long id) {
            hasResetHeader = false;
            if (onItemLongClickListener != null && state == State.PULL_TO_REFRESH) {
                return onItemLongClickListener.onItemLongClick(adapterView, view, position, id);
            }
            return false;
        }
    }

    /**
     * 
     */
    public void manuallyTriggerRefresh() {
        state = State.REFRESHING;
        scrollTo(0, 0);
        setUiRefreshing();
        setHeaderPadding(0);
        setState(State.REFRESHING);
    }

    public void setRefreshable(boolean refreshable) {
        isRefreshable = refreshable;
    }

    private boolean isRefreshable;

    public void setAdapter(ListAdapter adapter) {
        lastUpdated = System.currentTimeMillis();
        lastUpdatedTextView.setVisibility(View.VISIBLE);
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm");
        lastUpdatedTextView.setText(MessageFormat.format(getResources().getString(R.string.ptr_last_updated), sdf.format(new Date())));
        super.setAdapter(adapter);
    }
    
	public void setFootStatus(FootStatus status) {
		if (footStatus == status) {
			return;
		}
		footStatus = status;
		switch (status) {
		case Loading:
			loading_layout.setVisibility(View.VISIBLE);
			progress.setVisibility(View.VISIBLE);
			tv_loadingtip.setText(R.string.loading);
			break;
		case Error:
			loading_layout.setVisibility(View.VISIBLE);
			progress.setVisibility(View.GONE);
			tv_loadingtip.setText(R.string.click_to_retry);
			break;
		case Gone:
			loading_layout.setVisibility(View.GONE);
			break;
		case Common:
			loading_layout.setVisibility(View.VISIBLE);
			progress.setVisibility(View.GONE);
			tv_loadingtip.setText(R.string.loading_more);
			break;
		default:
			break;
		}
	}

	public enum FootStatus {
		Loading, // 加载状态
		Error, // 出错状态
		Gone, // 不可见
		Common// 普通状态
	}

	public interface OnLoadMoreListenr {
		void onLoadData();
	}
}
