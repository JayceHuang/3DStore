/**
 *
 */
package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.ArrayList;

/**
 * @author Sven.Zhan VRS提供的影片详细信息
 */
public class CmsVedioDetailInfo extends CmsVedioBaseInfo {
	
	private static final long serialVersionUID = 1L;
	
	/**
	 * 总集数，为以后系列剧准备
	 */
	public int total;

	/**
	 * 当前更新集数
	 */
	public int episode;

	/**
	 * 预告片和花絮视频列表
	 */
	public ArrayList<Vedio> trailers;

	
	/**
	 * -------------视频基本信息-----------
	 */
	public static class Vedio implements Serializable {

		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;

		/**
		 * 视频Id，唯一标示
		 */
		public int videoId;

		/**
		 * 该视频对应的影片albumid
		 */
		public int albumId;

		/**
		 * 该Vedio的名字
		 */
		public String title;
		
		/**
		 * 该Vedio的时长
		 */
		public int duration;	
		
		/**
		 * 该视频的音轨语言
		 * */
		public String audioLanguage;
		
		/**
		 * 该视频的音轨语言是否是原生语言
		 */
		public boolean isOriginal;
		
		/**
		 * 视频的上下左右格式
		 * 0	SDS3D_ORDER_LEFT_RIGHT	true
		 * 1	SDS3D_ORDER_RIGHT_LEFT	true
		 * 2	SDS3D_ORDER_TOP_BOTTOM	true
		 * 3	SDS3D_ORDER_BOTTOM_TOP	true
		 */
		public int mode;
		
		/**
		 *  影片字幕信息列表
		 */
		public ArrayList<SubTitleInfo> subtitles;
	}

	/**
	 * -------------影片字幕信息----------
	 */
	public static class SubTitleInfo implements Serializable {

		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;

		/** 字幕名称 */
		public String title;
		
		/**字幕的语言版本*/
		public String language;

		/** 字幕的url */
		public String url;
	}
}
