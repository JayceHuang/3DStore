package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.ArrayList;

/**
 * 
 * @author Sven.Zhan
 * 热搜词的实体类
 */
public class VOSearchKey implements Serializable {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	/*热搜词的分类  ALBUM：电影；APP：应用；GAME ： 游戏*/
	public String type;
	
	/*对应分类下的热搜词集合*/
	public ArrayList<String> list;
}
