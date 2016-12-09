package com.runmit.sweedee.action.search;

import java.util.LinkedList;

import com.runmit.sweedee.action.search.DefaultEffectFaces.DefaultEffectFacesParams;
import com.runmit.sweedee.action.search.DefaultEffectFaces.DefaultEffectFacesParams.EffectType;
import com.runmit.sweedee.util.StaticHandler;

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.os.Message;
import android.view.*;

public class FlipViewGroup extends ViewGroup {
	
	public static final int MSG_SURFACE_CREATED = 1001;
	public static final int MSG_ANIMATOIN_FIN = MSG_SURFACE_CREATED +1;
	public static final int MSG_UPDATE_TEXTURE = MSG_ANIMATOIN_FIN +1;
	private LinkedList<View> flipViews = new LinkedList<View>();
	
	private Handler handler = new Handler(new Handler.Callback() {		
		@Override
		public boolean handleMessage(Message msg) {
			switch (msg.what) {
			case MSG_SURFACE_CREATED:
				width = 0;
				height = 0;
				requestLayout();
				break;
			case MSG_UPDATE_TEXTURE:
				View view = flipViews.get(0);
				renderer.updateTexture(view);
			default:
				break;
			}
			return true;
		}
	});

	private GLSurfaceView surfaceView;
	private FlipRenderer renderer;

	private int width;
	private int height;

	private boolean flipping = false;
	private Handler mHandler;
	
	private int counts = 0;
	
	private boolean mUpdateTexture;
	private DefaultEffectFacesParams createParams;

	public FlipViewGroup(Context context, View firstView, DefaultEffectFacesParams params) {
		super(context);
		mUpdateTexture = true;
		counts = 0;
		new MyThread().start();
		initWithFirstView(firstView);
	}
	
	/**
	 * 需要实时更新纹理
	 * @param context
	 * @param firstView
	 * @param upDateTexture
	 */
	public FlipViewGroup(Context context, View firstView, boolean upDateTexture, DefaultEffectFacesParams params) {
		super(context);
		initWithFirstView(firstView);
		mUpdateTexture = upDateTexture;
		createParams = params;
		if (upDateTexture) {
			counts = 0;
			new MyThread().start();
		}		
	}
	
	public FlipViewGroup(Context context, View firstView, Handler handler, DefaultEffectFacesParams params) {
		super(context);
		mHandler = handler;
		createParams = params;
		// 默认实时更新纹理
		mUpdateTexture = true;
		counts = 0;
		initWithFirstView(firstView);
	}
	
	public FlipViewGroup(Context context, View firstView, boolean upDateTexture, Handler handler, DefaultEffectFacesParams params) {
		super(context);
		mHandler = handler;
		createParams = params;
		initWithFirstView(firstView);
		mUpdateTexture = upDateTexture;
		if (upDateTexture) {
			counts = 0;
			new MyThread().start();
		}		
	}

	private void initWithFirstView(View firstView) {
		setupSurfaceView();
		addFlipView(firstView);		
		flipping = true;
	}

	private void setupSurfaceView() {
		surfaceView = new GLSurfaceView(getContext());
		if (createParams == null) {
			createParams = new DefaultEffectFacesParams();
		}
		renderer = new FlipRenderer(this, createParams);
		
		surfaceView.setEGLConfigChooser(8, 8, 8, 8, 8, 0);
		surfaceView.setZOrderOnTop(true);
		surfaceView.setRenderer(renderer);
		surfaceView.getHolder().setFormat(PixelFormat.TRANSLUCENT);
		surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
		addView(surfaceView);
	}

	public GLSurfaceView getSurfaceView() {
		return surfaceView;
	}

	public FlipRenderer getRenderer() {
		return renderer;
	}

	public void addFlipView(View v) {
		flipViews.add(v);
//		v.setAlpha(0);
		v.setVisibility(View.INVISIBLE);
		addView(v);
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b) {
		for (View child : flipViews)
			child.layout(0, 0, r - l, b - t);

		if (changed || width == 0) {
			int w = r - l;
			int h = b - t;
			if (flipping) {
				surfaceView.layout(0, 0, w, h);
			}			
			
			if (width != w || height != h) {
				width = w;
				height = h;
				// 不需要实时更新纹理的话，首次需要加载纹理
				if (!mUpdateTexture && flipping && flipViews.size() >= 1) {
					View view = flipViews.get(0);
					renderer.updateTexture(view);
				}
			}
		}
	}
	
	private int sleepTime = 42;
	/**
	 * 设置每秒帧数
	 * @param rate
	 */
	public void setFrameRate(int rate) {
		int time = sleepTime;
		if (rate > 0 && rate <= 60) {
			time = 1000 / rate;	
		}
		sleepTime = time;
	}
	private class MyThread extends Thread {
		
		@Override
		public void run() {
			while (mUpdateTexture && counts ++< 301) {
				handler.sendEmptyMessage(MSG_UPDATE_TEXTURE);
				try {
					Thread.sleep(sleepTime);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		super.onMeasure(widthMeasureSpec, heightMeasureSpec);
		for (View child : flipViews)
			child.measure(widthMeasureSpec, heightMeasureSpec);
	}

	public void startFlipping() {
		flipping = true;
	}
	
	public void onResume() {
		if (surfaceView != null) {
			surfaceView.onResume();
		}
	}

	public void onPause() {
		if (surfaceView != null) {
			surfaceView.onPause();
		}
	}
	
	/**
	 * surface view 创建完成了, 需要知道宽高
	 */
	public void reloadTexture() {
		handler.sendMessage(Message.obtain(handler, MSG_SURFACE_CREATED));
	}
	
	public void setUpdateTexture(boolean flag){
		mUpdateTexture = flag;
	}

	/**
	 * 动画结束时的处理函数
	 */
	public void obtainAniFinished() {
		removeView(surfaceView);
		surfaceView = null;
		flipping = false;
		mUpdateTexture = false;
		if (mHandler != null) {
			mHandler.sendEmptyMessage(MSG_ANIMATOIN_FIN);
		}
	}
	
	/**
	 *  动画将要结束时回调处理
	 */
	public void obtainAniReachFinish() {
		View view = flipViews.get(0);
//		view.setAlpha(256);
		view.setVisibility(View.VISIBLE);
	}
}
