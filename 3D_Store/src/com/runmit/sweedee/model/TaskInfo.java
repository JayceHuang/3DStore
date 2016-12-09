package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.ArrayList;


import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;
import com.google.gson.reflect.TypeToken;
import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;

public class TaskInfo implements Serializable {

    /**
     *
     */
    private static final long serialVersionUID = 1L;

    public static class LOCAL_STATE {
        /*
         * 任务状态值组
         */
        public static final int WAITING = 0;
        public static final int RUNNING = WAITING + 1;
        public static final int PAUSED = RUNNING + 1;
        public static final int SUCCESS = PAUSED + 1;
        public static final int FAILED = SUCCESS + 1;
        public static final int DELETED = FAILED + 1;
        public static final int READY = DELETED + 1;
    }

    public static final int ADD_TASK_SUCCESS = 100;
    public static final int ADD_TASK_FAILED = 101;
    public static final int GET_TASKINFO_SUCCESS = 102;
    public static final int GET_TASKINFO_FAILED = 103;
    public static final int PAUSE_TASK_SUCCESS = 104;
    public static final int PAUSE_TASK_FAILED = 105;
    public static final int RESUME_TASK_SUCCESS = 106;
    public static final int RESUME_TASK_FAILED = 107;
    public static final int TASK_STATE_CHANGED_NOTIFY = 108;
    public static final int TASK_ALREADY_EXIST = 102409;
    public static final int FILE_ALERADY_EXIST = 102416;
    public static final int INVALID_TASK_ID = 102434;
    public static final int INSUFFICIENT_DISK_SPACE = 3173;
    public static final int EACCES_PERMISSION_DENIED = 13;    //Linux错误码，许可拒绝
    public static final int ERROR_DO_NOT_SUPPORT_FILE_BIGGER_THAN_4G = 1928;    //Linux错误码，许可拒绝
    public static final int ERROR_INFO_ERROR = 23233;
    public static final int TASK_URL_INVALIDED = 400;


    public boolean isSelected = false;    // 是否被选中

    /**
     * cms返回给客户端的albumnId
     */
    public int albumnId;
    
    
    public int videoId;//视频id
    
    public int mode;//视频上下左右格式
        
    /*
     * 任务ID
     */
    public int mTaskId;

    /*
     * 任务状态
     */
    public int mTaskState;

    /*
     * 任务相应的文件名
     */
    public String mFileName;

    /*
     * 已缓存文件大小
     */
    public long downloadedSize;

    /*
     * 任务相应的文件大小
     */
    public long fileSize;
    /*
     * 实时缓存速度
     */
    public int mDownloadSpeed;

    public String mUrl;

    //云点播相关
    public int progress;//离线文件缓存进度 9158  表示 91.58%

    public String cookie;//会员url认证

    public int failedCode;//失败code

    public String mfilePath;//任务存储路径

    public String poster;//图片地址

    public int error_code;//当且仅当LOCAL_STATE.FAIL时有效

    public int startTime;	//TODO	下载开始时间  替换下载链接后，会修改这个值？？
    public long finishedtime;

    public ArrayList<SubTitleInfo> mSubTitleInfos;

    public TaskInfo() {

    }

    public TaskInfo(String subtitleJsonString,int task_id, int task_state, String file_name_str,
                    String file_path_str, String file_url_str, long downloaded_size,
                    long file_size,String file_poster,int task_fail_code, int startTime, int finishtime, int albumid,int mode, int videoId) {
        this.mTaskId = task_id;
        this.mTaskState = task_state;
        this.mFileName = file_name_str;
        this.mfilePath = file_path_str;
        this.mUrl = file_url_str;
        this.downloadedSize = downloaded_size;
        this.fileSize = file_size;
        this.poster = file_poster;
        this.error_code = task_fail_code;
        this.videoId = videoId;
        this.startTime = startTime;
        if(subtitleJsonString!=null && subtitleJsonString.length()>0){
        	try {
        		this.mSubTitleInfos = new Gson().fromJson(subtitleJsonString,new TypeToken<ArrayList<SubTitleInfo>>(){}.getType());  
			} catch (JsonSyntaxException e) {
				e.printStackTrace();
			}
        }
        this.finishedtime = finishtime;
        this.albumnId = albumid;
        this.mode = mode;
    }

	@Override
	public String toString() {
		return "TaskInfo [isSelected=" + isSelected + ", albumnId=" + albumnId + ", videoId=" + videoId + ", mode=" + mode + ", mTaskId=" + mTaskId + ", mTaskState=" + mTaskState + ", mFileName="
				+ mFileName + ", downloadedSize=" + downloadedSize + ", fileSize=" + fileSize + ", mDownloadSpeed=" + mDownloadSpeed + ", mUrl=" + mUrl + ", progress=" + progress + ", cookie="
				+ cookie + ", failedCode=" + failedCode + ", mfilePath=" + mfilePath + ", poster=" + poster + ", error_code=" + error_code + ", startTime=" + startTime + ", finishedtime="
				+ finishedtime + ", mSubTitleInfos=" + mSubTitleInfos + "]";
	}
}
