package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.List;

/**
 * @author Sven.Zhan
 *  影片筛选的维度信息
 */
public class VedioFilterType implements Serializable {
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	/**维度名称，如地区，国家*/
	public String title;
	
	/**维度标志*/		
	public String code;
	
	/**该维度下的筛选项*/
	public List<Item> items;
	
	/**筛选项信息*/
	public static class Item implements Serializable{
		
		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;

		/**筛选项名称*/
		public String title;
		
		/**筛选项标志*/
		public String code;
	}
}
