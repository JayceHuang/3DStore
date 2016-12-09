package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.List;

public class AppPageInfo implements Serializable{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public int total;
	
	public int start;
	
	public int rows;
	
	public int length;
	
	public List<AppItemInfo> list;
	
	
}
