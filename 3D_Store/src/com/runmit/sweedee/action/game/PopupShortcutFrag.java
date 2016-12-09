package com.runmit.sweedee.action.game;

import java.util.ArrayList;

import android.app.Activity;
import android.os.Bundle;
import android.support.v4.view.ViewPager;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.SimpleBitmapDisplayer;
import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.view.RecyclingPagerAdapter;

public class PopupShortcutFrag extends BaseFragment {
	private ViewPager screen_shoots_pager;
	private LayoutInflater mInflater;
	private Activity mActivity;
	private View mParentView;
	private AppDetailActivity.ScreenShotsAdapter adapter;
	private ImageAdapter mImageAdapter;
	private boolean isShow;
	private View rootView;
	private int currentPos;
	private DisplayImageOptions mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_app_detai_listitem).displayer(new SimpleBitmapDisplayer()).build();

	public PopupShortcutFrag(AppDetailActivity.ScreenShotsAdapter adapter) {
		this.adapter = adapter;
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mActivity = getActivity();
		mInflater = LayoutInflater.from(mActivity);
	}

	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		rootView = inflater.inflate(R.layout.pop_screenshoot_viewer, container, false);
		initViews();
		return rootView;
	}

	private void initViews() {
		screen_shoots_pager = (ViewPager) rootView.findViewById(R.id.screen_shoots);
		setAdapter(adapter);
	}

	public boolean getIsShow() {
		return isShow;
	}

	public void show(int pos) {
		isShow = true;
		currentPos = pos;
	}

	public void dismiss() {
		isShow = false;
	}

	private boolean setTrackOnce = false;

	private void setAdapter(AppDetailActivity.ScreenShotsAdapter adapter) {
		mImageAdapter = new ImageAdapter(adapter.mList);
		screen_shoots_pager.setAdapter(mImageAdapter);
		screen_shoots_pager.setCurrentItem(currentPos);
	}

	private class ImageAdapter extends RecyclingPagerAdapter {

		private ArrayList<String> items;

		public ImageAdapter(ArrayList<String> datas) {
			this.items = new ArrayList<String>();
			items.addAll(datas);
		}

		public int getCount() {
			return Integer.MAX_VALUE;
		}

		public String getItem(int position) {
			return items.get(position);
		}

		public View getView(final int position, View convertView, ViewGroup parent) {
			ViewHolder vh;
			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.item_pop_screenshoot, null);
				vh = new ViewHolder();
				vh.gallery_image = (ImageView) convertView.findViewById(R.id.image);
				convertView.setTag(vh);
			} else {
				vh = (ViewHolder) convertView.getTag();
			}

			convertView.setOnTouchListener(new View.OnTouchListener() {
				float pX, pY;

				@Override
				public boolean onTouch(View view, MotionEvent motionEvent) {
					switch (motionEvent.getAction()) {
					case MotionEvent.ACTION_DOWN:
						pX = motionEvent.getX();
						pY = motionEvent.getY();
						break;
					case MotionEvent.ACTION_UP:
						float dx = motionEvent.getX() - pX;
						float dy = motionEvent.getY() - pY;
						if (Math.abs(dx) < .01 && Math.abs(dy) < .01) {
							((AppDetailActivity) getActivity()).hideFrag();
						}
						break;
					}
					return true;
				}
			});
			String url = items.get(position);
			ImageView view = vh.gallery_image;
			ImageLoader.getInstance().displayImage(url, view, mOptions);
			return convertView;
		}

		class ViewHolder {
			public ImageView gallery_image;
		}

		@Override
		public int getRealCount() {
			if (items != null) {
				return items.size();
			}
			return 0;
		}
	}
}
