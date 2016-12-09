package com.runmit.mediaserver;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

public class UtilityHandlerThread extends HandlerThread {

	private MyHandler mMyHandler = null;
	
	public UtilityHandlerThread() {
		super(UtilityHandlerThread.class.getName());
		// TODO Auto-generated constructor stub
	}

	
	public void init() {
		super.start();
		mMyHandler = new MyHandler(getLooper());
	}
	
	public void uninit() {
		super.quit();
	}
	
	public boolean ExecuteRunnable(Runnable runnable) {
		return mMyHandler.post(runnable);
	}
	
	private class MyHandler extends Handler {
		public MyHandler(Looper looper) {
			super(looper);
		}
	}
	
}
