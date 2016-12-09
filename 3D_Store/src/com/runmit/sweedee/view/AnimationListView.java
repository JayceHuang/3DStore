/**
 * AnimationDirectListView.java 
 * com.xunlei.cloud.view.AnimationDirectListView
 * @author: zhang_zhi
 * @date: 2013-7-15 上午9:53:17
 */
package com.runmit.sweedee.view;

import java.util.HashSet;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewPropertyAnimator;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.widget.AbsListView;
import android.widget.ListView;

import com.runmit.sweedee.R;

/**
 * {@link https://github.com/twotoasters/JazzyListView}
 * 
 * @author zhang_zhi listview的动画效果
 * 
 *         修改记录：修改者，修改日期，修改内容
 */
public class AnimationListView extends ListView {

	private final JazzyHelper mHelper;

	/**
	 * @param context
	 * @param attrs
	 * @param defStyle
	 */
	public AnimationListView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mHelper = new JazzyHelper(context, attrs);
		super.setOnScrollListener(mHelper);
	}

	/**
	 * @param context
	 * @param attrs
	 */
	public AnimationListView(Context context, AttributeSet attrs) {
		super(context, attrs);
		mHelper = new JazzyHelper(context, attrs);
		super.setOnScrollListener(mHelper);
	}

	/**
	 * @param context
	 */
	public AnimationListView(Context context) {
		super(context);
		mHelper = new JazzyHelper(context, null);
		super.setOnScrollListener(mHelper);
	}

	@Override
	public final void setOnScrollListener(OnScrollListener l) {
		mHelper.setOnScrollListener(l);
	}

	/**
	 * Sets whether new items or all items should be animated when they become
	 * visible.
	 * 
	 * @param onlyAnimateNew
	 *            True if only new items should be animated; false otherwise.
	 */
	public void setShouldOnlyAnimateNewItems(boolean onlyAnimateNew) {
		mHelper.setShouldOnlyAnimateNewItems(onlyAnimateNew);
	}

	/**
	 * If true animation will only occur when scrolling without the users finger
	 * on the screen.
	 * 
	 * @param onlyFlingEvents
	 */
	public void setShouldOnlyAnimateFling(boolean onlyFling) {
		mHelper.setShouldOnlyAnimateFling(onlyFling);
	}

	/**
	 * Stop animations after the list has reached a certain velocity. When the
	 * list slows down it will start animating again. This gives a performance
	 * boost as well as preventing the list from animating under the users
	 * finger if they suddenly stop it.
	 * 
	 * @param itemsPerSecond
	 *            , set to JazzyHelper.MAX_VELOCITY_OFF to turn off max
	 *            velocity. While 13 is a good default, it is dependent on the
	 *            size of your items.
	 */
	public void setMaxAnimationVelocity(int itemsPerSecond) {
		mHelper.setMaxAnimationVelocity(itemsPerSecond);
	}

	public class GrowEffect {
		//参照storehouse以80%的大小进行渐变
		private static final float INITIAL_SCALE_FACTOR = 0.8f;

		public void initView(View item, int position, int scrollDirection) {
			item.setPivotX(item.getWidth() / 2);
			item.setPivotY(item.getHeight() / 2);
			item.setScaleX(INITIAL_SCALE_FACTOR);
			item.setScaleY(INITIAL_SCALE_FACTOR);
		}

		public void setupAnimation(View item, int position, int scrollDirection, ViewPropertyAnimator animator) {
			animator.scaleX(1).scaleY(1);
		}
	}

	public class JazzyHelper implements OnScrollListener {

		public static final int DURATION = 600;
		public static final int OPAQUE = 255, TRANSPARENT = 0;

		private GrowEffect mTransitionEffect = new GrowEffect();
		private boolean mIsScrolling = false;
		private int mFirstVisibleItem = -1;
		private int mLastVisibleItem = -1;
		private int mPreviousFirstVisibleItem = 0;
		private long mPreviousEventTime = 0;
		private double mSpeed = 0;
		private int mMaxVelocity = 0;
		public static final int MAX_VELOCITY_OFF = 0;

		private AbsListView.OnScrollListener mAdditionalOnScrollListener;

		private boolean mOnlyAnimateNewItems;
		private boolean mOnlyAnimateOnFling;
		private boolean mIsFlingEvent;
		private final HashSet<Integer> mAlreadyAnimatedItems;

		public JazzyHelper(Context context, AttributeSet attrs) {

			mAlreadyAnimatedItems = new HashSet<Integer>();

			TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.AnimationListView);
			int maxVelocity = a.getInteger(R.styleable.AnimationListView_max_velocity, MAX_VELOCITY_OFF);
			mOnlyAnimateNewItems = a.getBoolean(R.styleable.AnimationListView_only_animate_new_items, false);
			mOnlyAnimateOnFling = a.getBoolean(R.styleable.AnimationListView_max_velocity, false);
			a.recycle();

			setMaxAnimationVelocity(maxVelocity);
		}

		public void setOnScrollListener(AbsListView.OnScrollListener l) {
			mAdditionalOnScrollListener = l;
		}

		/**
		 * @see AbsListView.OnScrollListener#onScroll
		 */
		@Override
		public final void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount) {
			boolean shouldAnimateItems = (mFirstVisibleItem != -1 && mLastVisibleItem != -1);

			int lastVisibleItem = firstVisibleItem + visibleItemCount - 1;
			if (mIsScrolling && shouldAnimateItems) {
				setVelocity(firstVisibleItem, totalItemCount);
				int indexAfterFirst = 0;
				while (firstVisibleItem + indexAfterFirst < mFirstVisibleItem) {
					View item = view.getChildAt(indexAfterFirst);
					doJazziness(item, firstVisibleItem + indexAfterFirst, -1);
					indexAfterFirst++;
				}

				int indexBeforeLast = 0;
				while (lastVisibleItem - indexBeforeLast > mLastVisibleItem) {
					View item = view.getChildAt(lastVisibleItem - firstVisibleItem - indexBeforeLast);
					doJazziness(item, lastVisibleItem - indexBeforeLast, 1);
					indexBeforeLast++;
				}
			} else if (!shouldAnimateItems) {
				for (int i = firstVisibleItem; i < visibleItemCount; i++) {
					mAlreadyAnimatedItems.add(i);
				}
			}

			mFirstVisibleItem = firstVisibleItem;
			mLastVisibleItem = lastVisibleItem;

			notifyAdditionalScrollListener(view, firstVisibleItem, visibleItemCount, totalItemCount);
		}

		/**
		 * Should be called in onScroll to keep take of current Velocity.
		 * 
		 * @param firstVisibleItem
		 *            The index of the first visible item in the ListView.
		 */
		private void setVelocity(int firstVisibleItem, int totalItemCount) {
			if (mMaxVelocity > MAX_VELOCITY_OFF && mPreviousFirstVisibleItem != firstVisibleItem) {
				long currTime = System.currentTimeMillis();
				long timeToScrollOneItem = currTime - mPreviousEventTime;
				if (timeToScrollOneItem < 1) {
					double newSpeed = ((1.0d / timeToScrollOneItem) * 1000);
					// We need to normalize velocity so different size item
					// don't
					// give largely different velocities.
					if (newSpeed < (0.9f * mSpeed)) {
						mSpeed *= 0.9f;
					} else if (newSpeed > (1.1f * mSpeed)) {
						mSpeed *= 1.1f;
					} else {
						mSpeed = newSpeed;
					}
				} else {
					mSpeed = ((1.0d / timeToScrollOneItem) * 1000);
				}

				mPreviousFirstVisibleItem = firstVisibleItem;
				mPreviousEventTime = currTime;
			}
		}

		/**
		 * 
		 * @return Returns the current Velocity of the ListView's scrolling in
		 *         items per second.
		 */
		private double getVelocity() {
			return mSpeed;
		}

		/**
		 * Initializes the item view and triggers the animation.
		 * 
		 * @param item
		 *            The view to be animated.
		 * @param position
		 *            The index of the view in the list.
		 * @param scrollDirection
		 *            Positive number indicating scrolling down, or negative
		 *            number indicating scrolling up.
		 */
		private void doJazziness(View item, int position, int scrollDirection) {
			if (mIsScrolling && item != null) {
				if (mOnlyAnimateNewItems && mAlreadyAnimatedItems.contains(position))
					return;

				if (mOnlyAnimateOnFling && !mIsFlingEvent)
					return;

				if (mMaxVelocity > MAX_VELOCITY_OFF && mMaxVelocity < getVelocity())
					return;
				ViewPropertyAnimator animator = item.animate().setDuration(DURATION).setInterpolator(new AccelerateDecelerateInterpolator());

				scrollDirection = scrollDirection > 0 ? 1 : -1;
				mTransitionEffect.initView(item, position, scrollDirection);
				mTransitionEffect.setupAnimation(item, position, scrollDirection, animator);
				animator.start();

				mAlreadyAnimatedItems.add(position);
			}
		}

		/**
		 * @see AbsListView.OnScrollListener#onScrollStateChanged
		 */
		@Override
		public void onScrollStateChanged(AbsListView view, int scrollState) {
			switch (scrollState) {
			case AbsListView.OnScrollListener.SCROLL_STATE_IDLE:
				mIsScrolling = false;
				mIsFlingEvent = false;
				break;
			case AbsListView.OnScrollListener.SCROLL_STATE_FLING:
				mIsFlingEvent = true;
				break;
			case AbsListView.OnScrollListener.SCROLL_STATE_TOUCH_SCROLL:
				mIsScrolling = true;
				mIsFlingEvent = false;
				break;
			default:
				break;
			}
			if (mAdditionalOnScrollListener != null) {
				mAdditionalOnScrollListener.onScrollStateChanged(view, scrollState);
			}
		}

		public void setShouldOnlyAnimateNewItems(boolean onlyAnimateNew) {
			mOnlyAnimateNewItems = onlyAnimateNew;
		}

		public void setShouldOnlyAnimateFling(boolean onlyFling) {
			mOnlyAnimateOnFling = onlyFling;
		}

		public void setMaxAnimationVelocity(int itemsPerSecond) {
			mMaxVelocity = itemsPerSecond;
		}

		/**
		 * Notifies the OnScrollListener of an onScroll event, since
		 * JazzyListView is the primary listener for onScroll events.
		 */
		private void notifyAdditionalScrollListener(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount) {
			if (mAdditionalOnScrollListener != null) {
				mAdditionalOnScrollListener.onScroll(view, firstVisibleItem, visibleItemCount, totalItemCount);
			}
		}
	}
}
