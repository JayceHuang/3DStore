package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Date;

import android.text.TextUtils;

import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.model.CmsVedioDetailInfo.Vedio;

public class CmsVideoCatalog implements Serializable {
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	public int total;
	public int start;
	public int rows;
	public int length;
	public ArrayList<Catalog> list;
	
	public static class Catalog extends CmsVedioBaseInfo {
		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;
		public ArrayList<String> tags;	
		public Date updatedAt;
    	public int playCount;
		public boolean light;
		public ArrayList<Vedio> trailers;	
		public ArrayList<String> metaLanguages;
		public int[] genres_id;
		public ArrayList<ImageType> imageTypes;
		
	}

	public static class ImageType implements Serializable {
		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;
		public String name;	
		public String url;		
	}
		
		
}
