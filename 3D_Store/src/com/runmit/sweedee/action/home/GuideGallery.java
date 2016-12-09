package com.runmit.sweedee.action.home;

import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

import android.content.Context;
import android.os.Handler;
import android.support.v4.view.PagerAdapter;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.SimpleBitmapDisplayer;
import com.runmit.sweedee.DetailActivity;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.game.AppDetailActivity;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.CmsItemable;
import com.runmit.sweedee.model.CmsModuleInfo;
import com.runmit.sweedee.model.CmsModuleInfo.CmsComponent;
import com.runmit.sweedee.model.CmsVedioBaseInfo;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.flippablestackview.FlippableStackView;
import com.runmit.sweedee.view.flippablestackview.RecyclingPagerAdapter;
import com.runmit.sweedee.view.flippablestackview.StackPageTransformer;

public class GuideGallery extends RelativeLayout {

	public static final float ASPECT_RATIO = 176f / 320;

	private static final int DEFINE_SCROLL_TIME = 4 * 1000;

	private XL_Log log = new XL_Log(GuideGallery.class);

	private LayoutInflater mInflater;

	private FlippableStackView mStackView;

	private ImageTimerTask mImageTimerTask;

	private Timer mTimer;

	private Handler mHandler = new Handler();

	private ImageLoader mImageLoader;

	private DisplayImageOptions mOptions;

	private MyPageAdapter mMyPageAdapter;

	public GuideGallery(Context context) {
		super(context);
		init();
	}

	public GuideGallery(Context context, AttributeSet attrs) {
		super(context, attrs);
		init();
	}

	private void init() {
		mInflater = LayoutInflater.from(getContext());
		int width = getResources().getDisplayMetrics().widthPixels;
		int height = (int) (width * ASPECT_RATIO);
		log.debug("guide width = " + width + " height = " + height);

		ImageView imageView = new ImageView(getContext());
		imageView.setImageResource(R.drawable.banner_light);
		RelativeLayout.LayoutParams imgParams = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		imgParams.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
		imgParams.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
		addView(imageView, imgParams);

		mStackView = new FlippableStackView(getContext());
		mStackView.initStack(5);

		RelativeLayout.LayoutParams stackParams = new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT, height);
		addView(mStackView, stackParams);
		mImageLoader = ImageLoader.getInstance();
		mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_gallery).setDisplayer(new SimpleBitmapDisplayer()).build();
		startTimeTask();
	}

	public int getHeadHeight() {
		int width = getResources().getDisplayMetrics().widthPixels;
		int height = (int) (width * ASPECT_RATIO);
		return height;
	}

	public void setDataSource(CmsComponent mCmsComponent) {
		mMyPageAdapter = new MyPageAdapter(mCmsComponent);
		mStackView.setAdapter(mMyPageAdapter);
		mStackView.setAccessibilityDelegate(new AccessibilityDelegate () {
		    @Override
		    public void onPopulateAccessibilityEvent(View host, android.view.accessibility.AccessibilityEvent event) {
		        super.onPopulateAccessibilityEvent(host, event);

		        if (event.getEventType() == AccessibilityEvent.TYPE_VIEW_SCROLLED) {
		            event.setContentDescription("");
		        }
		    }
		});
	}

	public void onResume() {
		log.debug("onResume=" + this);
		onStop();
		startTimeTask();
	}

	public void startTimeTask() {
		mTimer = new Timer();
		mImageTimerTask = new ImageTimerTask();
		mTimer.schedule(mImageTimerTask, DEFINE_SCROLL_TIME, DEFINE_SCROLL_TIME);
	}

	public void onStop() {
		log.debug("onStop=" + this);
		if (mImageTimerTask != null) {
			mImageTimerTask.cancel();
			mImageTimerTask = null;
		}
		if (mTimer != null) {
			mTimer.cancel();
			mTimer = null;
		}
	}

	private class ImageTimerTask extends TimerTask {
		public void run() {
			mHandler.post(new Runnable() {
				public void run() {
					if (mMyPageAdapter != null) {
						int mNextPos = mStackView.getCurrentItem() - 1;
						if (mNextPos < 0) {
							mNextPos = mMyPageAdapter.getCount() - 1;
						}
						mStackView.setCurrentItem(mNextPos, true);
					}
				}
			});
		}
	}

	private class MyPageAdapter extends RecyclingPagerAdapter {

		private ArrayList<CmsModuleInfo.CmsItem> items;

		public MyPageAdapter(CmsComponent ccomponent) {
			this.items = ccomponent.contents;
		}

		@Override
		public int getCount() {
			if (items != null && items.size() > 0) {
				return 10000;
			}
			return 0;
		}
		
		@Override
	    public View getView(int position, View view, ViewGroup container) {
			log.debug("position="+position+",view="+view);
	        final ViewHolder holder;
	        if (view != null) {
	            holder = (ViewHolder) view.getTag();
	        } else {
	            view = mInflater.inflate(R.layout.item_resource_suggest_head, container, false);
	            holder = new ViewHolder(view);
	            view.setTag(holder);
	        }
	        int realPisition = position % items.size();
	        holder.mTitleTextView.setAlpha(0.8f);

			final CmsItemable mItem = getItem(realPisition);
			if (mItem != null) {
				String imgUrl = TDImageWrap.getHomeBannerUrl(mItem);
				mImageLoader.displayImage(imgUrl, holder.gallery_image, mOptions);
				holder.mTitleTextView.setText(mItem.getTitle());
			}

			holder.gallery_image.setOnClickListener(new View.OnClickListener() {
				public void onClick(View v) {
					Context context = getContext();
					if (mItem instanceof CmsVedioBaseInfo) {
						DetailActivity.start(context, holder.gallery_image, mItem.getId());
					} else {
						AppItemInfo item = (AppItemInfo) mItem;
						AppDetailActivity.start(context, holder.gallery_image, item);
					}
				}
			});
			view.setId(position);
	        return view;
	    }

		private CmsItemable getItem(int position) {
			if (items.size() == 0) {
				return null;
			}
			return items.get(position % items.size()).item;
		}
		
		private class ViewHolder {
	        final ImageView gallery_image;
	        final TextView mTitleTextView;

	        public ViewHolder(View view) {
	        	gallery_image = (ImageView) view.findViewById(R.id.gallery_image);
	        	mTitleTextView = (TextView) view.findViewById(R.id.title_textview);
	        }
	    }
	}
}
