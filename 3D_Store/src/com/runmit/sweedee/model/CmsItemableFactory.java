/**
 * 
 */
package com.runmit.sweedee.model;

import com.google.gson.Gson;
import com.runmit.sweedee.model.CmsModuleInfo.CmsItem;

/**
 * @author Sven.Zhan
 * CmsItemable 实例化具体对象的工厂类
 */
public class CmsItemableFactory {
	
	private Gson gson = new Gson();
	
	public CmsItemable createItemable(String type,String content){		
		if(CmsItem.TYPE_ALBUM.equals(type) || CmsItem.ALBUM_DCSH.equals(type) || CmsItem.ALBUM_JCPH.equals(type) || CmsItem.ALBUM_YXYL.equals(type)){
			return gson.fromJson(content, CmsVedioBaseInfo.class);
		}else if(CmsItem.TYPE_GAME.equals(type) || CmsItem.TYPE_APP.equals(type)){
			return gson.fromJson(content, AppItemInfo.class);
		}else if(CmsItem.HOTWORD.equals(type)){
			return gson.fromJson(content, SortHotWordInfo.class);
		}else{
			return null;
		}
	}
}
