/**
 *
 */
package com.runmit.sweedee.model;

import java.util.ArrayList;
import android.text.TextUtils;
import com.runmit.sweedee.model.CmsVedioDetailInfo.Vedio;

/**
 * @author Sven.Zhan
 *  VRS提供的影片简版信息（CMS返回）
 */
public class CmsVedioBaseInfo implements CmsItemable{

	private static final long serialVersionUID = 1L;
	
    /** 影片albumid，唯一标示*/
    public int albumId;

    /** 影片名称*/
    public String title;

    /** 影片评分*/
    public String score;
    
    /**
	 * 影片发行方
	 */
	public String producer;

    /**影片上映时间*/
    public String releaseDate;
    
	/**
	 * 影片语言
	 */
	public String language;

	/**
	 * 影片简介
	 */
	public String summary;

	/**
	 * 该影片是否可被缓存
	 **/
	public boolean downloadable = true;
	
	/**
	 * 影片发布区域
	 */
	public String region;
	
	/**
	 * 影片是否免费
	 */
	public boolean isFree = false;
    
    /**影片分类*/
    public ArrayList<String> genres; 
	
	/**
	 * 演员列表
	 */
	public ArrayList<String> actors;

	/**
	 * 导演列表
	 */
	public ArrayList<String> directors;

	/**
	 * 编剧列表
	 */
	public ArrayList<String> editors;   
	
	/**
	 *精简介绍
	 **/
	public String highlight;
	
	/**
	 * 正片列表
	 */
	public ArrayList<Vedio> films;
	
	/**
	 * 正片列表
	 */
	public String uploadUser;
	
	@Override
	public int getId() {
		return albumId;
	}
	
	@Override
	public String getTitle() {
		return title;
	}

	@Override
	public String getScore() {
		if(TextUtils.isEmpty(score)){
			score = "6.0";
		}else if(score.length() == 1){
			score += ".0";
		}else if(score.length() == 2 || score.length() > 3){
			throw new RuntimeException("score lenght is wrong because the score is " + score);
		}
		return score;
	}

	@Override
	public String getMessage() {		
		return null;
	}

	@Override
	public long getSize() {
		throw new RuntimeException("getSize not support in CmsVedioBaseInfo");
	}

	@Override
	public int getInstallCount() {
		return 0;
	}

	@Override
	public String getHighlight() {		
		return highlight;
	}

	@Override
	public int getVideoId() {
		if(films != null && films.size() > 0){
			return films.get(0).videoId;
		}
		return 0;
	}
	
	public int getDuration(){
		if(films != null && films.size() > 0){
			return films.get(0).duration;
		}
		return 0;
	}
}
