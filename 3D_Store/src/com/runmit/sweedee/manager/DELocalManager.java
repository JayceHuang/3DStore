//package com.runmit.sweedee.manager;
//
//import java.io.File;
//import java.io.FileInputStream;
//import java.io.FileNotFoundException;
//import java.io.FileOutputStream;
//import java.io.IOException;
//import java.io.ObjectInputStream;
//import java.io.ObjectOutputStream;
//import java.io.Serializable;
//import java.net.HttpURLConnection;
//import java.net.URL;
//import java.text.MessageFormat;
//import java.util.ArrayList;
//import java.util.HashMap;
//import java.util.List;
//import java.util.Map.Entry;
//import java.util.Timer;
//import java.util.TimerTask;
//import java.util.concurrent.ExecutorService;
//import java.util.concurrent.Executors;
//
//import android.os.Handler;
//import android.os.Message;
//import android.text.TextUtils;
//import android.widget.Toast;
//
//import com.google.gson.Gson;
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.StoreApplication;
//import com.runmit.sweedee.downloadinterface.DownloadEngine;
//import com.runmit.sweedee.manager.data.DELocalData.StructTaskState;
//import com.runmit.sweedee.manager.interfaces.DELocalInterface;
//import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;
//import com.runmit.sweedee.model.TaskInfo;
//import com.runmit.sweedee.report.sdk.ReportActionSender;
//import com.runmit.sweedee.util.Constant;
//import com.runmit.sweedee.util.EnvironmentTool;
//import com.runmit.sweedee.util.ErrorCode;
//import com.runmit.sweedee.util.SharePrefence;
//import com.runmit.sweedee.util.Util;
//import com.runmit.sweedee.util.XL_Log;
//
//public class DELocalManager implements DELocalInterface {
//	
//	public final static int DELM_TASK_STATE_CHANGE = 10000;
//    public final static int DELM_TIMER_UPDATE_TASK = DELM_TASK_STATE_CHANGE + 1;
//    public final static int DELM_DOWNLOAD_TASK_FINISH = DELM_TIMER_UPDATE_TASK + 1;
//    public final static int DELM_DOWNLOAD_TASK_DELETE = DELM_DOWNLOAD_TASK_FINISH + 1;
//    
//    private final static int MAX_SPEEDS_LIST_SIEZE = 500;
//    private XL_Log log = new XL_Log(DELocalManager.class);
//
//    private final static ExecutorService nativeThreadPool = Executors.newFixedThreadPool(10);
//
//    static String path = EnvironmentTool.getExternalDataPath("downloadUrlLists");
//    private static String filename = "sweedee_download_urllists.txt";
//    
//    private DownloadEngine mDownloadEngine;
//
//    private Handler mAnsyHanler;
//    
//    /**
//     * 下载速度列表
//     * Inteter taskid
//     * List speeds
//     */
//    private HashMap<Integer, List<Integer>>  totalSpeedList;
//
//    private DELocalManager() {
//        super();
//        mDownloadEngine = DownloadEngine.getInstance();
//        mDownloadEngine.setDELocalInterface(this);
//        mAnsyHanler = mDownloadEngine.getAnsyHandler();
//        
//        getCacheData();
//
//    	taskCacheList = new HashMap<Integer, TaskInfo>();
//    	totalSpeedList = new HashMap<Integer, List<Integer>>();
//    }
//    
//    /**
//     * 懒汉式单例，线程安全
//     */
//	private static class DELocalManagerHolder {
//        public final static DELocalManager INSTANCE = new DELocalManager();
//    }
//
//    public static DELocalManager getInstance() {
//        return DELocalManagerHolder.INSTANCE;
//    }
//    
//    /**
//     * 保存下载地址的缓存数据
//     */
//    private List<DownloadListUrlObject> taskUrlList;
//    
//    /**
//     * 所有任务列表 的缓存 
//     */
//    private HashMap<Integer, TaskInfo>  taskCacheList;
//    private Timer cacheTimer;
//    
//    public void init() {
//    	// 等待mDownloadEngine初始化完毕
//        nativeThreadPool.execute(new Runnable() {
//			@Override
//			public void run() {
//				try {
//					Thread.sleep(500);	// DownloadEngine init time
//				} catch (InterruptedException e) {
//					e.printStackTrace();
//				}
//		    	initCacheList();
//				checkValidCache();
//			}        	
//        });
//    }
//    
//    // 如果文件被删除但是任务还在的情况，删除任务
//	private void checkValidCache() {
//		String path;
//		TaskInfo task;
//		int size = taskCacheList.size();
//		List<Integer> delList = new ArrayList<Integer>();
//		
//		for (Entry<Integer, TaskInfo> entry : taskCacheList.entrySet()) {
//			task = entry.getValue();
//    		path = task.mfilePath;
//			if (path == null || path.endsWith("null")) {// 首先判断是否有记录保存
//				delList.add(task.mTaskId);
//				log.debug("checkValidCache taskCacheList:" + taskCacheList.size() + " delete task:" + task.mTaskId + " path:"+path);
//				continue;
//			}
//			
//			boolean isFileExists = false;
//			if (!path.endsWith("/")) {
//				path += "/";
//			}
//			path = path + task.mFileName;
//			File file = new File(path);
//			isFileExists = file.exists();
//			
//			if(!isFileExists){
//				path = path + ".rf";
//				file = new File(path);
//				isFileExists = file.exists();
//			}
//			
//			if (!isFileExists && task.downloadedSize > 0) {// 再判断该文件是否存在
//				delList.add(task.mTaskId);
//				log.debug("checkValidCache taskCacheList:" + taskCacheList.size() + " delete task:" + task.mTaskId + " path:"+path);
//			}
//    	}
//		
//		for(int task_id : delList) {
//			mDownloadEngine.deleteTask(task_id, true);
//			taskCacheList.remove(task_id);
//		}
//		
//		if(size != taskCacheList.size()){
//			downloadTaskListUpdate();
//		}
//	}
//    /**
//     * 初始化任务缓存列表
//     */
//    private void initCacheList() {
//    	TaskInfo[] tl = mDownloadEngine.getAllLocalTasks();
//    	if(tl != null)
//    	for(TaskInfo t : tl){
//    		taskCacheList.put(t.mTaskId, t);
//    	}
//
//        log.debug("initCacheList size is:" + taskCacheList.size());
//        
//        // 同步状态的定时器 
//        startSyncCacheTimer();
//	}
//    
//    /**
//     * 定时更新任务中的数据
//     */
//    private void startSyncCacheTimer() {
//    	cacheTimer = new Timer();
//    	cacheTimer.schedule(new SyncCacheTask(), 0, 1500);
//	}
//    
//  	private class SyncCacheTask extends TimerTask {
//  		public void run() {
//			nativeThreadPool.execute(new Runnable() {
//				@Override
//				public void run() {
//					copyUserDataList();
//					checkValidCache();
//				}
//			});
//  		}
//    }
//  	
//    /**
//     * 同步用户数据
//     */
//    private void copyUserDataList() {
//    	TaskInfo taskNative = null;
//    	for (Entry<Integer, TaskInfo> entry : taskCacheList.entrySet()) {
//    		// entry.getKey();
//    		TaskInfo task = entry.getValue();
//        	// 只刷新正在运行的任务
//            if (task.mTaskState == TaskInfo.LOCAL_STATE.RUNNING) {
//                taskNative = mDownloadEngine.getTaskInfoByTaskId(task.mTaskId);
//                if (taskNative != null) {
//                    task.downloadedSize = taskNative.downloadedSize;
//                    task.fileSize = taskNative.fileSize;
//                    task.mTaskState = taskNative.mTaskState;
//                    task.mDownloadSpeed = taskNative.mDownloadSpeed;
//                    updateSpeed(task);
//                }
//            }
//        }
//	}
//    
//    private void updateSpeed(TaskInfo task){
//    	List<Integer> speedList = totalSpeedList.get(task.mTaskId);
//    	if(speedList == null) return;
//    	if(speedList.size() < MAX_SPEEDS_LIST_SIEZE){
//    		speedList.add(speedList.size(), task.mDownloadSpeed);
//    	} else {
//    		speedList.remove(0);
//    		speedList.add(speedList.size(), task.mDownloadSpeed);
//    	}
//    }
//    
//    private void clearSpeed(TaskInfo task, boolean isReport){
//    	List<Integer> speedList = totalSpeedList.get(task.mTaskId);
//    	if(speedList == null || speedList.size() == 0) return;
//    	int speed = 0, num = 0;
//    	for (int s : speedList){
//    		speed += s;
//    		num ++;
//    	}
//    	
//    	if(num > 0)
//    		speed = speed / num;
//    	String cdnUrl = "";
//    	
//    	for(DownloadListUrlObject urlObject : taskUrlList){
//    		if(urlObject.taskId == task.mTaskId){
//    			cdnUrl = urlObject.url;
//    			break;
//    		}
//    	}
//    	
//    	if(isReport)
//    		ReportActionSender.getInstance().videoDownloadSpeed(task.videoId, task.mFileName, getVideoCrt(task.mFileName), speed, cdnUrl);
//    	
//    	speedList.clear();
//    }
//    
//    private int getVideoCrt(String fileName){
//    	int crt = 0;
//		if(fileName.contains("_720P")) 
//			crt = 1;
//		else if(fileName.contains("_1080P")) 
//			crt = 2;
//		return crt;
//    }
//    
//    /**
//     * 加入缓存
//     */
//    private void addToTaskCache(int task_id) {
//    	if(task_id == -1) return;
//    	TaskInfo[] t = mDownloadEngine.getAllLocalTasks();
//    	if(t != null){
//    		for(TaskInfo mInfo:t){
//				if(mInfo.mTaskId == task_id) {
//					taskCacheList.put(task_id, mInfo);
//					break;
//				}
//			}
//    	}
//    	
//    } 
//    
//    /**
//     * 删除缓存任务 
//     * @param task_id
//     */
//    private void deleteCache(int task_id) {
//    	TaskInfo task = null;
//    	for (Entry<Integer, TaskInfo> entry : taskCacheList.entrySet()) {
//    		// entry.getKey();
//    		task = entry.getValue();
//        	if(task.mTaskId == task_id) {
//				taskCacheList.remove(task_id);
//				break;
//			}
//        }
//    }
//    
//    private boolean updateLocalCache(StructTaskState taskState) {
//    	TaskInfo task = taskCacheList.get(taskState.task_id);
//        if(task == null) return false;
//        
//        log.debug("task id is:" + taskState.task_id);
//        log.debug("file name is:" + taskState.file_name);
//        log.debug("rtn_code is:" + taskState.fail_code);
//        log.debug("filesize is:" + task.fileSize);
//        log.debug("url is:" + task.mUrl);
//        log.debug("new state:" + taskState.task_state);
//        
//        synchronized (task) {
//        	task.mTaskState = taskState.task_state;
//            task.mFileName = taskState.file_name;
//            task.mfilePath = taskState.file_path;
//		}
//        
//        // 下载失败状态
//        if(task.mTaskState == TaskInfo.LOCAL_STATE.FAILED) {
//        	clearSpeed(task, false);
//        	ReportActionSender.getInstance().videoDownloadfail(task.videoId, task.mFileName, getVideoCrt(task.mFileName), task.error_code);
//        }
//        else if(task.mTaskState == TaskInfo.LOCAL_STATE.SUCCESS) {
//        	totalSpeedList.remove(task.mTaskId);
//        }
//        return true;
//    }
//    
//    
//	/**
//	 * 持久化 下载链接列表
//	 */
//	public void saveUrlLists(){  
//		ObjectOutputStream oos = null;
//        try {	    	
//	    	if(taskUrlList == null)return;
//	    	File file = new File(path , filename);
//        	FileOutputStream outfile = new FileOutputStream(file);
//            oos = new ObjectOutputStream(outfile);
//            oos.writeObject(taskUrlList);
//            oos.flush();
//            oos.close();
//            log.debug("taskUrlList saved success. taskUrlList size:"+taskUrlList.size());
//        } catch (Exception e) {
//        	log.debug("taskUrlList saved error.");
//            e.printStackTrace();
//        }
//    }
//	
//    /**
//	 * 获取上次未完成的任务 
//	 */
//	private void getCacheData(){
//		File f = new File(path, filename);  
//		FileInputStream in;
//		ObjectInputStream ois = null;
//		try {
//			in = new FileInputStream(f);
//			ois = new ObjectInputStream(in);
//			taskUrlList = CastObj(ois.readObject());
//		} catch (FileNotFoundException e) {
//			e.printStackTrace();
//		} catch (IOException e) {
//			e.printStackTrace();
//		} catch (ClassNotFoundException e) {
//			e.printStackTrace();
//		}
//		if(taskUrlList == null)return;
//		
//		log.debug("getCacheData list size:" + taskUrlList.size());
//	}
//	
//	@SuppressWarnings("unchecked")
//	private static <T> T CastObj (Object obj){
//		T ret = null;
//		try{
//			ret = (T)obj;
//		}catch(ClassCastException e){
//		}
//		return ret;
//	}
//	
//	public static class DownloadListUrlObject implements Serializable{
//		/**
//		 * 
//		 */
//		private static final long serialVersionUID = 1260701962285814442L;
//		
//		public int taskId;
//		public String url;
//		
//		public DownloadListUrlObject (int id, String url){
//			this.taskId = id;
//			this.url = url;
//		}
//	}
//	
//    // ==============================================================================
//    // ================================= Etm同步接口  ================================
//    // ==============================================================================
//
//    public int getDownloadLimitSpeed() {
//        return mDownloadEngine.getDownloadLimitSpeed();
//    }
//
//    public String getDownloadPath() {
//        return mDownloadEngine.getDownloadPath();
//    }
//
//    public int limitdownloadspeed(int speed) {
//        return mDownloadEngine.setDownloadLimitSpeed(speed);
//    }
//
//    public int setMaxDownloadTasks(int max_num) {
//        return mDownloadEngine.setMaxDownloadTasks(max_num);
//    }
//
//    public int getDownloadNum() {
//        return 2;
//    }
//
//    public int setNetType(final int net_type) {
//        nativeThreadPool.execute(new Runnable() {
//            @Override
//            public void run() {
//                mDownloadEngine.setNetType(net_type);
//            }
//        });
//        return 0;
//    }
//
//    public int setDownloadLimitSpeed(int speed_kb) {
//        return mDownloadEngine.setDownloadLimitSpeed(speed_kb);
//    }
//
//    public int setDownloadPath(String path) {
//        return mDownloadEngine.setDownloadPath(path);
//    }
//    
//    /**
//     * 添加下载源
//     * @param task_id 下载任务id
//     * @param newUrl  下载链接
//     * @param cookie  cookie
//     * @param refUrl  refurl
//     * @return
//     */
//    public int addTaskUrl(int task_id, String newUrl, String cookie, String refUrl){
//    	int ret = mDownloadEngine.addTaskUrl(task_id, newUrl, cookie, refUrl);
//    	if(ret == 0){
//        	addToUrlList(task_id, newUrl);
//        }
//    	return ret;
//    }
//    
//    /**
//     * 创建下载任务
//     * @param mode 				视频模式
//     * @param albumId			albumId
//     * @param videoid			videoid
//     * @param resolution		分辨率
//     * @param url				下载链接
//     * @param file_path			下载路径
//     * @param file_name			文件名
//     * @param cookie			cookie
//     * @param file_size			文件大小
//     * @param poster			海报地址
//     * @param mSubTitleInfos	字幕信息
//     * @return					default 0
//     */
//    public int createTaskByUrl(final int mode, final int albumId,final int videoid,final int resolution,final String url, final String file_path, final String file_name, final String cookie, final long file_size, final String poster,final ArrayList<SubTitleInfo> mSubTitleInfos) {
//        mAnsyHanler.post(new Runnable() {
//
//            @Override
//            public void run() {
//            	synchronized (this) {
//            		log.debug("albumId="+albumId);
//                	String subtitleJson = new Gson().toJson(mSubTitleInfos);
//                    int ret = mDownloadEngine.createTaskByUrl(videoid, mode, albumId, url, file_path, file_name, cookie, file_size, poster,subtitleJson);
//                    // success
//                    if(ret == 0){
//                    	// 保存下载任务对应地址
//                    	addToUrlList((int)mDownloadEngine.mTaskId, url);
//                    	addToTaskCache((int)mDownloadEngine.mTaskId);
//                    	
//                    	totalSpeedList.put((int) mDownloadEngine.mTaskId, new ArrayList<Integer>());
//                    	mDownloadEngine.mTaskId = -1;
//                    }
//                    handleDownloadTaskFail(ret, file_name);
//                    ReportActionSender.getInstance().download(videoid, file_name, resolution);
//				}
//            }
//        });
//        return 0;
//    }
//    
//    /**
//     * 更新任务下载链接并持久化 
//     * @param task_id 任务id
//     * @param url 下载链接
//     */
//    private void addToUrlList(int task_id, final String url){
//    	if(taskUrlList == null){
//    		taskUrlList = new ArrayList<DownloadListUrlObject>();
//    	}
//    	boolean isUpdate = false;
//    	for(DownloadListUrlObject urlObject : taskUrlList){
//    		if(urlObject.taskId == task_id){
//    			urlObject.url = url;
//    			log.debug("addToUrlList update task:" + task_id + "  url:"+url);
//    			isUpdate = true;
//    			break;
//    		}
//    	}
//    	if(!isUpdate){
//    		log.debug("addToUrlList add new task:" + task_id + "  url:"+url);
//    		taskUrlList.add(new DownloadListUrlObject(task_id, url));
//    	}
//    	saveUrlLists();
//    }
//    
//    /**
//     * 删除任务链接列表对应的数据
//     * @param task_id
//     */
//    private void deleteUrl(int task_id){
//    	log.debug("deleteUrl task_id:"+task_id + " taskUrlList:"+taskUrlList);
//    	if(taskUrlList == null){
//    		return;
//    	}
//    	boolean isUpdate = false;
//    	for(DownloadListUrlObject urlObject : taskUrlList){
//    		if(urlObject.taskId == task_id){
//    			taskUrlList.remove(urlObject);
//    			isUpdate = true;
//    			break;
//    		}
//    	}
//    	if(isUpdate){
//    		saveUrlLists();
//    	}
//    }
//    
//    /**
//     * 创建任务后的返回处理
//     * @param ret
//     * @param fileName
//     */
//    private void handleDownloadTaskFail(int ret, String fileName) {
//    	String note = null;
//        switch (ret) {
//            case 0:
//                note = StoreApplication.INSTANCE.getString(R.string.download_tast_success);
//                break;
//            case TaskInfo.FILE_ALERADY_EXIST:
//            	note = StoreApplication.INSTANCE.getString(R.string.download_tast_file_already_exist);
//                note = MessageFormat.format(note, mDownloadEngine.getDownloadPath());
//                break;
//            case TaskInfo.TASK_ALREADY_EXIST:
//                note = StoreApplication.INSTANCE.getString(R.string.download_tast_already_exist);
//                break;
//            case TaskInfo.EACCES_PERMISSION_DENIED:
//                if (Util.isSDCardExist()) {
//                	note = StoreApplication.INSTANCE.getString(R.string.download_tast_acces_permission_denied);
//                	note = MessageFormat.format(note, fileName);
//                } else {
//                    note = StoreApplication.INSTANCE.getString(R.string.no_found_sdcard);
//                }
//                break;
//            case TaskInfo.INSUFFICIENT_DISK_SPACE:
//                note = StoreApplication.INSTANCE.getString(R.string.sdcard_full_failed);
//                break;
//            case TaskInfo.ERROR_DO_NOT_SUPPORT_FILE_BIGGER_THAN_4G:
//                note = StoreApplication.INSTANCE.getString(R.string.download_tast_maxsize_failed);
//                break;
//            case TaskInfo.ERROR_INFO_ERROR:
//                note = StoreApplication.INSTANCE.getString(R.string.download_tast_error_info);
//                break;
//            default:
//            	note = StoreApplication.INSTANCE.getString(R.string.download_tast_default_err);
//            	note = MessageFormat.format(note, fileName, ret);
//                break;
//        }
//        Util.showToast(StoreApplication.INSTANCE, note, Toast.LENGTH_SHORT);
//    }
//    
//    /**
//     * 暂停任务
//     * @param task
//     * @return
//     */
//    public int pauseTask(final TaskInfo task) {
//    	final int task_id = task.mTaskId;
//        nativeThreadPool.execute(new Runnable() {
//            @Override
//            public void run() {
//                int ret = mDownloadEngine.pauseTask(task_id);
//                if(ret == 0) {
//                	clearSpeed(task, true);
//                }
//            }
//        });
//        return 0;
//    }
//    
//    /**
//     * 继续任务
//     * @param task
//     * @param listener
//     * @return
//     */
//    public void resumeTask(final TaskInfo task, final LocalActionListener listener) {
//    	final int task_id = task.mTaskId;
//        nativeThreadPool.execute(new Runnable() {
//            @Override
//            public void run() {
//            	int ret = mDownloadEngine.resumeTask(task_id);
//                log.debug("resumeTask ret=" + ret);
//        		boolean isNeedReplaceUrl = checkUrlExpired(task_id);                
//                if (listener != null)
//                    listener.afterAction(isNeedReplaceUrl ? TaskInfo.TASK_URL_INVALIDED : ret, task_id, task);
//            }
//        });
//    }
//    
//    /**
//     * 处理失败任务
//     * @param task
//     * @param listener
//     */
//    public void handleFailedTask(final TaskInfo task, final LocalActionListener listener) {
//    	final int task_id = task.mTaskId;
//    	nativeThreadPool.execute(new Runnable() {
//			@Override
//			public void run() {
//				boolean isNeedReplaceUrl = checkUrlExpired(task_id);
//        		if(isNeedReplaceUrl && listener != null){
//        			listener.afterAction(TaskInfo.TASK_URL_INVALIDED, task.mTaskId, task);
//        		}
//			}
//    		
//    	});
//    }
//    
//    /**
//     * 检测下载链接是否过期
//     * @param taskid
//     * @return
//     */
//    private synchronized boolean checkUrlExpired(int taskid){
//    	if(taskUrlList == null || taskUrlList.size() == 0)return false;
//		try {
//			String surl = "";
//			boolean isMatch = false;
//			for(DownloadListUrlObject obj : taskUrlList){
//				if(obj.taskId == taskid){
//					surl = obj.url;
//					isMatch = true;
//					break;
//				}
//			}
//			log.debug("check Url Expired downloadurl=" + surl + "  list:"+taskUrlList.size());
//			if(!isMatch || TextUtils.isEmpty(surl))return false;
//			URL url = new URL(surl);
//			HttpURLConnection conn = (HttpURLConnection) url.openConnection();
//			conn.setConnectTimeout(2 * 1000);
//			conn.setRequestMethod("GET");
//			conn.setRequestProperty("Charset", "UTF-8");
//			conn.setRequestProperty("Connection", "Keep-Alive");
//			conn.setRequestProperty("Range", "bytes=0-"+(8*1024));// 设置获取实体数据的范围
//
//			int responseCode = conn.getResponseCode();
//			log.debug("check Url Expired downloadurl=" + surl+ "responseCode = "+responseCode);
//			if(responseCode < 300){
//				return false;
//			}else if(responseCode >= 400 && responseCode <= 500){
//				return true;
//			}
//		} catch (IOException e) {
//			e.printStackTrace();
//			return false;
//		}
//		return false;
//	}
//
//    public interface LocalActionListener {
//        void afterAction(int result, final int taskId, final TaskInfo task);
//    }
//    
//    /**
//     * 批量删除本地任务
//     *
//     * @param mlists
//     * @param isDeleteFile
//     */
//    public void deleteDownloadTaskList(final List<TaskInfo> mlists, final boolean isDeleteFile, final OnDeleteFileListener listener) {
//        log.debug("deleteDownloadTaskList mlist run out " + (mlists == null ? "is null" : "is not null"));
//        nativeThreadPool.execute(new Runnable() {
//            @Override
//            public void run() {
//                if (mlists == null) {
//                    return;
//                }
//                HashMap<Integer, Boolean> realDelMap = new HashMap<Integer, Boolean>();
//                int delSize = 0, size = mlists.size();
//                for (int i = 0; i < size; i++) {
//                    TaskInfo temp = mlists.get(i);
//                    int taskId = temp.mTaskId;
//                    int ret = -1;
//                    ret = mDownloadEngine.deleteTask(taskId, isDeleteFile);
//                    log.debug("deleteDownloadTaskList=" + ret + ",taskId=" + taskId);
//                    
//                    boolean isDelSucc = false;
//                    if (ret == 0) {
//                    	deleteUrl(taskId);
//                    	deleteCache(taskId);
//                    	totalSpeedList.remove(taskId);
//                    	isDelSucc = true;
//                    	delSize ++;
//					}
//                    realDelMap.put(taskId, isDelSucc);
//                }
//                
//                if (listener != null) {
//                    listener.onDelete(realDelMap, (delSize == size));
//                }
//            }
//        });
//    }
//    
//    // 继续缓存多个任务
//    public void resumeTasks(final List<TaskInfo> tasks, final LocalActionListener listener) {
//        nativeThreadPool.execute(new Runnable() {
//            @Override
//            public void run() {
//                int result = 0;
//                for (TaskInfo task : tasks) {
//                    result = mDownloadEngine.resumeTask(task.mTaskId);
//                    if (result != 0) {
//                        break;
//                    }
//                }
//                if (listener != null)
//                    listener.afterAction(result, -1, null);
//            }
//        });
//    }
//    
//    //暂停多个任务
//    public void pauseTasks(final List<TaskInfo> tasks, final Runnable runnable) {
//        nativeThreadPool.execute(new Runnable() {
//            @Override
//            public void run() {
//                for (TaskInfo task : tasks) {
//                    int ret = mDownloadEngine.pauseTask(task.mTaskId);
//                    if (ret != 0) {
//                        break;
//                    }
//                    clearSpeed(task, true);
//                }
//                if (runnable != null)
//                    runnable.run();
//            }
//        });
//    }
//    
//    /**
//     * 任务状态变更
//     * 只能在DownloadEngine调用 
//     */
//    @Override
//    public void onLocalTaskStateChangedCallBack(StructTaskState result_data) {
//        log.debug("onLocalTaskStateChangedCallBack");
//        StructTaskState temp = result_data;
//        if (updateLocalCache(temp)) {
//            if (localTaskListener != null)
//            	sendMessage(DELM_TASK_STATE_CHANGE, temp.task_id, temp.task_state, null);
//        }
//    }
//
//    /**
//     * 获取本地任务列表
//     *
//     * @return
//     */
//    public List<TaskInfo> getLocalTaskList() {
//    	List<TaskInfo> l = new ArrayList<TaskInfo>();
//    	for (Entry<Integer, TaskInfo> entry : taskCacheList.entrySet()) {
//    		l.add(entry.getValue());
//    	}
//        return l;
//    }
//
//    /**
//     * 开启定时更新列表的定时器
//     */
//    public void startUpdateRunningTasks() {
//        updateTimer = new Timer();
//        updateTimer.schedule(new SDTimerTask(), 420, 2000);//1秒实在太卡，暂定两秒       
//    }
//
//    /**
//     * 关闭定时更新列表的定时器
//     */
//    public void stopUpdateRunningTasks() {
//        if (updateTimer == null)
//            return;
//        updateTimer.cancel();
//        updateTimer = null;
//    }
//
//    private Timer updateTimer;
//
//    public class SDTimerTask extends TimerTask {
//        public void run() {
//        	downloadTaskListUpdate();
//        }
//    }
//
//    private void downloadTaskListUpdate() {
//    	List<TaskInfo> tempList = getLocalTaskList();
//    	sendMessage(DELM_TIMER_UPDATE_TASK, 0, 0, tempList);
//    }
//
//    /**
//     * 获取未完成的任务
//     *
//     * @return
//     */
//    public List<TaskInfo> getNoneFinTasks() {
//        List<TaskInfo> l = new ArrayList<TaskInfo>();
//    	for (Entry<Integer, TaskInfo> entry : taskCacheList.entrySet()) {
//    		TaskInfo ti = entry.getValue();
//    		if (ti != null && ti.mTaskState == TaskInfo.LOCAL_STATE.RUNNING || ti.mTaskState == TaskInfo.LOCAL_STATE.WAITING 
//    				|| ti.mTaskState == TaskInfo.LOCAL_STATE.PAUSED || ti.mTaskState == TaskInfo.LOCAL_STATE.FAILED) {
//                l.add(ti);
//            }
//    	}
//        return l;
//    }
//    
//    /**
//     * 获取未完成的任务
//     *
//     * @return
//     */
//    
//    public List<TaskInfo> getRunningTasks() {
//    	List<TaskInfo> l = new ArrayList<TaskInfo>();
//    	for (Entry<Integer, TaskInfo> entry : taskCacheList.entrySet()) {
//    		TaskInfo ti = entry.getValue();
//    		if (ti != null && ti.mTaskState == TaskInfo.LOCAL_STATE.RUNNING || ti.mTaskState == TaskInfo.LOCAL_STATE.WAITING) {
//                l.add(ti);
//            }
//    	}
//        return l;
//    }
//    
//    /**
//     * 获取已完成任务
//     * @return
//     */
//    public List<TaskInfo> getDownloadedTasks() {
//        List<TaskInfo> l = new ArrayList<TaskInfo>();
//    	for (Entry<Integer, TaskInfo> entry : taskCacheList.entrySet()) {
//    		TaskInfo ti = entry.getValue();
//    		if (ti != null && ti.mTaskState == TaskInfo.LOCAL_STATE.SUCCESS) {
//                l.add(ti);
//            }
//    	}
//        return l;
//    }
//
//
//    /**
//     * 判断现在是否有正在缓存的任务
//     *
//     * @return
//     */
//    public boolean isHaveTaskRunning() {
//    	int size = getRunningTasks().size();
//    	return size > 0;
//    }
//
//    /**
//     * 暂停所有任务
//     * <p>
//     * 方法详述（简单方法可不必详述）
//     * </p>
//     */
//    public String suspendAllTasks() {
//        StringBuffer sb = new StringBuffer();
//        
//        for (Entry<Integer, TaskInfo> entry : taskCacheList.entrySet()) {
//    		TaskInfo temp = entry.getValue();
//            if (temp.mTaskState == TaskInfo.LOCAL_STATE.RUNNING || temp.mTaskState == TaskInfo.LOCAL_STATE.WAITING) {
//                mDownloadEngine.pauseTask(temp.mTaskId);
//
//                sb.append(temp.mTaskId);
//                sb.append(SharePrefence.SPLIT_DOWNLOAD_TASK_ID);
//            }
//    	}
//        
//        log.debug("suspendAllTasks: " + sb.toString());
//        
//        return sb.toString();
//    }
//    
//    private Handler localTaskListener;
//    
//    /**
//     * 设置状态改变的回调监听
//     * @param localHandler
//     */
//    public void setTaskListener(Handler localHandler) {
//        this.localTaskListener = localHandler;
//    }
//    
//    private void sendMessage(int what, int arg1, int arg2, Object obj) {
//    	if(localTaskListener != null){
//    		Message msg = localTaskListener.obtainMessage(what, arg1, arg2, obj);
//    		msg.sendToTarget();
//    	}
//    }
//    
//    /**
//     * 文件删除回调接口
//     *
//     */
//    public interface OnDeleteFileListener {
//    	/**
//    	 * 删除结果回调
//    	 * @param realDelMap	实际删除的任务键值对 task_id : boolean
//    	 * @param isAllDone		是否成功删除全部任务
//    	 */
//        public void onDelete(HashMap<Integer, Boolean> realDelMap, boolean isAllDone);
//    }
//}
