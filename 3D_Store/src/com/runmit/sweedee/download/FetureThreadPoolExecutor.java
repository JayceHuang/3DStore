package com.runmit.sweedee.download;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Future;
import java.util.concurrent.FutureTask;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import com.runmit.sweedee.util.XL_Log;

public class FetureThreadPoolExecutor extends ThreadPoolExecutor{

	private XL_Log log = new XL_Log(FetureThreadPoolExecutor.class);
	
	private List<FileDownFutureTask> mRunningTasks = new ArrayList<FileDownFutureTask>();
	
	public FetureThreadPoolExecutor(int corePoolSize, int maximumPoolSize, long keepAliveTime, TimeUnit unit, BlockingQueue<Runnable> workQueue) {
		super(corePoolSize, maximumPoolSize, keepAliveTime, unit, workQueue);
	}

    /**
     * @throws NullPointerException       {@inheritDoc}
     */
    public <T> Future<T> submit(FutureTask<T> mFutureTask) {
        if (mFutureTask == null) throw new NullPointerException();
        execute(mFutureTask);
        return mFutureTask;
    }
    
    protected void beforeExecute(Thread t, Runnable r) { 
    	log.debug("beforeExecute r="+r);
    	if(r instanceof FileDownFutureTask){
    		mRunningTasks.add((FileDownFutureTask) r);
    	}
    }
    
    protected void afterExecute(Runnable r, Throwable t) { 
    	log.debug("afterExecute r="+r);
    	if(r instanceof FileDownFutureTask){
    		mRunningTasks.remove(r);
    	}
    }
    
	public FileDownFutureTask getFileDownloader(String key){
		FileDownFutureTask mFileDownFutureTask = getRunningFileDownloader(key);
		if(mFileDownFutureTask != null){
			return mFileDownFutureTask;
		}
		
		//如果下载队列没有查到，则查找等待队列
		Iterator<Runnable> mIterator = getQueue().iterator();
		while(mIterator.hasNext()){
			FileDownFutureTask mFileDownloader = (FileDownFutureTask) mIterator.next();
			if(mFileDownloader.getDownloadInfo().getBeanKey().equals(key)){
				return mFileDownloader;
			}
		}
		return null;
	}
	
	public FileDownFutureTask getRunningFileDownloader(String key){
		Iterator<FileDownFutureTask> mIterator = mRunningTasks.iterator();
		while(mIterator.hasNext()){
			FileDownFutureTask mFileDownloader = mIterator.next();
			if(mFileDownloader.getDownloadInfo().getBeanKey().equals(key)){
				return mFileDownloader;
			}
		}
		return null;
	}
	
	public void pauseAllTask(){
		getQueue().clear();//清除等待队列
		for(FileDownFutureTask mTask : mRunningTasks){
			mTask.pauseTaks();
		}
	}
}
