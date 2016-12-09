package com.runmit.sweedee.model;

import com.runmit.sweedee.download.AppDownloadInfo;

import java.io.Serializable;
import java.util.ArrayList;

/***
 * @author Sven.Zhan
 * 
 *资源格式
 {
   "rtn": 0,
   "errorMsg": "",
   "data": {
      "id": 1000, //应用版本内部Id
      "appId": "xxxxxx", //应用Id,标识唯一的应用
      "appKey": "xxxxx", //应用当前版本Id
      "developerId": 10, //开发者Id
      "developerTitle": "superD", //开发者名称
      "title": "3D相册", //应用名称,
      "type": '1',//应用类型1:APP, 2:GAME
      "description": "超酷3D相册", //应用描述
      "installationTimes":  400, //安装次数
      "rate": 4.5, //应用评分
      "versionName": "0.2.5", //应用版本
      "versionCode": 1,//Integer,应用内部版本号
      "upgradeMessage": "修复bug1,2,3。新增功能1", //升级信息
      "genres":  [{"id": 100, title: "应用"}, {"id": 200, title: "系统工具"}], //应用分类
      "tags": ["系统工具", "系统优化"], //标签
      "releaseDate": "2014-11-11", //应用发布时间
      "updateDate": "2014-11-17", //应用更新时间
      "popular": 1, //应用热度
      "highlight": "火爆应用", //应用简版描述
      "packageName": "com.zzz.ssss" //应用包名
      "fileSize": 10023 //应用的体积，单位是byte
   }
}
 */
public class AppItemInfo implements CmsItemable{
	
	private static final long serialVersionUID = 1L;
	
	public static final int TYPE_APP = 1;
	
	public static final int TYPE_GAME= 2;
	
	public static final String STR_APP = "APP";
	
	public static final String STR_GAME = "GAME";
	
	/*应用id,资源标志，其他接口的标志，相当于主键*/
	/**
	 * appId代表一个应用
	 * appKey代表一个应用的一个版本
	 * id和appKey一样的，代表应用的一个版本
	 */
	public int id;
	
	/*大类型：1:APP, 2:GAME*/
	public int type;
	
	/*应用的版本号*/
	public int versionCode;
	
	/*应用的大小*/
	public long fileSize;
	
	/*开发者名称*/
	public String developerTitle;
	
	/*应用名称*/
	public String title;
	
	/*应用描述*/
	public String description;
	
	/*安装次数*/
	public int installationTimes;
	
	/*应用的包名*/
	public String packageName;
	
	/*应用版本名称*/
	public String versionName;
	
	/*新版更新信息*/
	public String upgradeMessage;
	
	/*应用更新时间*/
	public long updateDate;
	
	/*应用评分*/
	public String rate;
	
	/*GSLB 中 应用Id,标识唯一的应用  下载使用的参数*/
	public String appId;
	
	/*GSLB 中 应用当前版本Id 下载使用的参数*/
	public String appKey;
	
	/*应用的分类信息*/
	public ArrayList<AppGenre> genres;

	public AppDownloadInfo mDownloadInfo;//外部扩展字段，下载状态
	
	public int installState;//外部扩充字段，因为可能一个应用或者游戏不是经过sweedee下载安装的此时mDownloadInfo就有可能为空DownloadInfo.STATE_INSTALL,DownloadInfo.STATE_INSTALL
	
	@Override
	public int getId() {		
		return id;
	}

	@Override
	public String getTitle() {		
		return title;
	}

	@Override
	public String getScore() {		
		return rate;
	}

	@Override
	public String getMessage() {		
		return description;
	}

	@Override
	public long getSize() {		
		return fileSize;
	}

    @Override
    public int getInstallCount() {
        return installationTimes;
    }

    /**App的分类消息*/
	public static class AppGenre implements Serializable{
		
		/*分类Id*/
		public int id;
		
		/*分类名称，如应用，系统工具等*/
		public String title;
	}

	@Override
	public String getHighlight() {
		//app 信息暂无精简介绍字段
		return null;
	}

	@Override
	public int getVideoId() {
		throw new RuntimeException("getVideoId not support in AppItemInfo");
	}

	@Override
	public int getDuration() {
		throw new RuntimeException("getDuration not support in AppItemInfo");
	}
}
