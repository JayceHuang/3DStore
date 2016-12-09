/**
 * 
 */
package com.runmit.sweedee.manager;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;

import android.content.Context;
import android.os.Handler;
import android.os.Message;

import com.runmit.sweedee.model.CmsModuleInfo;
import com.runmit.sweedee.util.XL_Log;

/**
 * @author Sven.Zhan
 * 
 * 预加载器：各种初始化前预加载
 */
public class InitDataLoader {
	
	private XL_Log log = new XL_Log(InitDataLoader.class);
	
	private static InitDataLoader instance;
	
	private InitDataLoader(){}
	
	public static InitDataLoader getInstance(){
		if(instance == null){
			instance = new InitDataLoader();
		}
		return instance;
	}
	
	public void reset(){
		listenerMap.clear();
		dataMap.clear();
		instance = null;
	}
	
	private Context mGlobalContext;
	
	private Handler mHandler;
	
	private HashMap<LoadEvent,Set<LoadListener>> listenerMap = new HashMap<LoadEvent, Set<LoadListener>>();
	
	private HashMap<LoadEvent,Object> dataMap = new HashMap<LoadEvent, Object>();
	
	private boolean hasInit = false;
	
	public void init(Context ctx){
		if(hasInit){
			return;
		}		
		hasInit = true;
		mGlobalContext = ctx;
		mHandler = new Handler(mGlobalContext.getMainLooper());
	}

    public interface LoadListener{    	
    	
    	/**是否确认结果，比如失败时，返回false。后续可提供重试逻辑*/    	
    	public boolean onLoadFinshed(int ret,boolean isCache, Object result);
    }
    
    /**主线程调用，注册监听 ：如果有数据，表示数据加载快于界面加载，直接回调数据即可*/
    public void registerEvent(final LoadEvent event,final LoadListener listener){
    	mHandler.post(new Runnable() {
			
			@Override
			public void run() {
				log.debug("registerEvent="+dataMap.containsKey(event));
				Set<LoadListener> listenerSet = listenerMap.get(event);
		    	if(listenerSet == null){
		    		listenerSet = new HashSet<LoadListener>();
		    		listenerSet.add(listener);
			    	listenerMap.put(event, listenerSet);
		    	}else{
		    		if(!listenerSet.contains(listener)){
		    			listenerSet.add(listener);
		    		}
		    	}
		    	
		    	Object obj = dataMap.get(event);
		    	if(obj != null || dataMap.containsKey(event)){
		    		listener.onLoadFinshed(obj != null? 0 : -1, false, obj);
		    	}
			}
		});
    } 
    
    public enum LoadEvent{
    	LoadHomeData,//加载本地假数据
    }
    
    private void dispatchResponse(final LoadEvent event,final int ret,final boolean isCache, final Object obj){
    	dataMap.put(event, obj);
    	mHandler.post(new Runnable() {			
			@Override
			public void run() {
				Set<LoadListener> listenerSet = listenerMap.get(event);
				log.debug("listenerSet="+listenerSet);
				if(listenerSet != null){
					log.debug("listenerSet="+listenerSet.size());
					for(LoadListener listener:listenerSet){
						log.debug("listener="+listener);
						listener.onLoadFinshed(ret,isCache, obj);
					}
				}
			}
		});
    }
    
    
    public void initHomeFakeData() {        	
    	 //服务器配置的电影类型的真实数据
        CmsServiceManager.getInstance().getModuleData(CmsServiceManager.MODULE_HOME, new Handler(mGlobalContext.getMainLooper()){
			@Override
			public void handleMessage(Message msg) {
				CmsModuleInfo movieChdi = null;
				if(msg.arg1 == 0){
					movieChdi = (CmsModuleInfo) msg.obj;
				}
				log.debug("arg1="+msg.arg1+",movieChdi="+movieChdi);
				boolean isCache = msg.arg2 == 1 ? true : false;
				dispatchResponse(LoadEvent.LoadHomeData,msg.arg1, isCache ,movieChdi);
			}                    	
        }); 
    }

}
