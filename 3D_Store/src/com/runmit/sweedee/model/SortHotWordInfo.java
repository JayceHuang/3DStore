package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.List;

public class SortHotWordInfo implements CmsItemable{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public String title;
	
	public String poster;
	
	public String createdAt;
	
	public String updatedAt;
	
	public List<LinkInfo> links;
	
	public List<ConditionInfo> filters;
	
	public static class ConditionInfo implements Serializable{
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	public List<LinkInfo> conditions;
	public String label;
	public String name;
		
	}
	
	public static class LinkInfo implements Serializable{
		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;
		
		public String label;
		public String link;
	}
	
	@Override
	public int getId() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public int getVideoId() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public String getTitle() {
		return title;
	}

	@Override
	public String getScore() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public String getMessage() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public long getSize() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public int getInstallCount() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public String getHighlight() {
		return null;
	}

	@Override
	public int getDuration() {
		// TODO Auto-generated method stub
		return 0;
	}

}
