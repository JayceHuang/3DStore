package com.runmit.sweedee.model;

import java.io.Serializable;
import java.lang.reflect.Type;
import java.util.ArrayList;


import com.google.gson.Gson;
import com.google.gson.JsonDeserializationContext;
import com.google.gson.JsonDeserializer;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParseException;

/**
 * @author Sven.Zhan
 * CMS 提供的 module的数据结构
 */
public class CmsModuleInfo implements Serializable {
	
	private static final long serialVersionUID = 1L;
	
	public ArrayList<CmsComponent> data;
	
	/**CMS 配置的CmsComponent 节点*/
	public static class CmsComponent implements Serializable{	
		
		private static final long serialVersionUID = 1L;
		
		public String name;
		
		public String title;
		
		public ArrayList<CmsItem>  contents;
	}
	
	
	/**CMS 配置的item 节点*/
	public static class CmsItem implements Serializable{
		
		private static final long serialVersionUID = 1L;
		
		/**item的类型 如电影，电子书，3D模型*/
		public VideoType type;
		
		/**jsonString 对应的各种itemable对象*/
		public CmsItemable item;
	
		
		/*----以下为 CmsItem的类型------*/
		public static final String TYPE_ALBUM  = "ALBUM";
		public static final String ALBUM_JCPH  = "ALBUM_JCPH";//精彩片花 
		public static final String ALBUM_YXYL  = "ALBUM_YXYL";//游戏预览 
		public static final String ALBUM_DCSH  = "ALBUM_DCSH";//多彩生活 
		public static final String HOTWORD  = "HOTWORD";//分类热词
		
		public static final String TYPE_BOOK  = "book";		
		public static final String TYPE_GAME  = "GAME";
		public static final String TYPE_APP  = "APP";
		
		/**JsonString 中的type*/
		private static final String FILED_TYPE = "type";
		
		/**JsonString 中的content*/
		private static final String FILED_CONTENT = "content";
		
		
		/***解析该抽象信息需要的Adapter*/
		public static JsonDeserializer<CmsItem> createTypeAdapter(){
			
			return new JsonDeserializer<CmsItem>() {
				CmsItemableFactory factory = new CmsItemableFactory();			
				@Override
				public CmsItem deserialize(JsonElement json, Type typeOfT, JsonDeserializationContext context) throws JsonParseException {
					CmsItem citem = new CmsItem();
					JsonObject jobj = json.getAsJsonObject();
					citem.type =  new Gson().fromJson(jobj.get(CmsItem.FILED_TYPE), VideoType.class);
					String contentStr =  jobj.get(CmsItem.FILED_CONTENT).toString();
					citem.item = factory.createItemable(citem.type.value, contentStr);
					return citem;
				}
			};
		}
	}
}
