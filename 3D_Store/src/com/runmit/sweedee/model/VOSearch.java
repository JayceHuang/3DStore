package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.ArrayList;;

public class VOSearch implements Serializable{
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public VedioGroup ALBUM;
	
	public VedioGroup LIGHTALBUM;
	
	public AppGroup GAME;
	
	public AppGroup APP;
	
	/**每一个搜索组的信息*/
	public abstract static class SerchGroup implements Serializable{
		
		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;

		public int total;
		
		public int start;
		
		public int rows;
		
		public int length;
	}
	
	public static class VedioGroup extends SerchGroup{		
		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;
		
		public ArrayList<CmsVedioBaseInfo> list;
	}
	
	public static class AppGroup extends SerchGroup{		
		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;
		
		public ArrayList<AppItemInfo> list;
	}
}

