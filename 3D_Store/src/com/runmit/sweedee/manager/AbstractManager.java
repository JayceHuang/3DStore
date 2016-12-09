package com.runmit.sweedee.manager;
import android.os.Handler;
import android.os.HandlerThread;

/***
 * 
 * @author Sven.Zhan
 * 各业务Manager类的父类，提供一个全局静态的异步Handler
 */
public abstract class AbstractManager {
	
	protected String TAG = "AbstractManager"; 
	
	protected static Handler mAsyncHandler = null;
	
	static{
		if(mAsyncHandler == null){
			HandlerThread mHandlerThread = new HandlerThread("Manager_HandlerThread");
			mHandlerThread.start();
			mAsyncHandler = new Handler(mHandlerThread.getLooper());
		}
	}
	
	protected void reset(){}
}
