/**
 * 
 */
package com.runmit.sweedee.model;

import java.io.Serializable;

/**
 * @author Sven.Zhan
 * 配置界面，首页，电影tab等页面展示的Item对象
 */
public interface CmsItemable extends Serializable{

	/**Item在服务器资源中对应的id*/
	public int getId();
	
	/**
	 * 获取videoId,只针对Video
	 * @return
	 */
	public int getVideoId();
	
	/**资源的名称*/
	public String getTitle();	
	
	/**资源的评分*/
	public String getScore();

	/**资源的简要信息*/
	public String getMessage();
	
	/**资源的大小*/
	public long getSize();
    
    /**资源的安装次数**/
    public int getInstallCount();
    
    /**获取精简介绍*/
    public String getHighlight();
    
    /**
     * 获取视频时常
     * @return
     */
    public int getDuration();
}
