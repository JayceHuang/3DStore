package com.runmit.sweedee.imageloaderex;

import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import android.graphics.drawable.Drawable;
import android.util.Log;
import android.util.SparseArray;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.model.CmsItemable;
import com.runmit.sweedee.model.CmsVedioBaseInfo;
import com.runmit.sweedee.model.CmsVedioDetailInfo;

/**
 * @author Sven.Zhan
 * APP，电影，试播片 各种类型图片URL获取的封装类
 */
public class TDImageWrap {
	
	private static SparseArray<Drawable> drawableMap = new SparseArray<Drawable>();
	
	public static class ImgSize{
		
		private int width;
		
		private int height;
		
		public ImgSize(int width, int height) {
			this.width = width;
			this.height = height;
		}
		
		public String getWidth(){
			return ""+width;
		}
		
		public String getHeight(){
			return ""+height;
		}

		@Override
		public String toString() {
			return "ImgSize [width=" + width + ", height=" + height + "]";
		}
	}	
	
	/***
	 * 获取客户端某张图片在该设备下的Drawable 长宽
	 * @param exampleDrawableId 图片的资源ID
	 * @return
	 */
	public static ImgSize getImgSize(int resId){
		int width = 0;
		int height = 0;
		Drawable drawable = drawableMap.get(resId);		
		if(drawable == null){
			try {
				drawable = StoreApplication.INSTANCE.getResources().getDrawable(resId);
				drawableMap.put(resId, drawable);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}		
		if(drawable != null){
			width = (int) (drawable.getIntrinsicWidth()*0.7f);
			height = (int) (drawable.getIntrinsicHeight()*0.7f);
		}		
		ImgSize isize = new ImgSize(width, height);
		return isize;
	}
	
	/**
	 * 获取app 截图列表
	 * @return
	 */
	public static ArrayList<String> getAppSnapshotUrls(int id){
		ArrayList<String> list = new ArrayList<String>();
		ImgSize iszie = getImgSize(R.drawable.default_app_detai_listitem);	
		for(int i = 1;i <= 5;i++){
			String suffix = MessageFormat.format(ImgString.AppSnapshot,Integer.toString(id),String.valueOf(i),iszie.getWidth(),iszie.getHeight());
			list.add(suffix);
		}
		return list;
	}
	
	/**
	 * 获取首页banner
	 * @param cvbi
	 * @return
	 */
	public static String getHomeBannerUrl(CmsItemable cvbi){
		ImgSize iszie = getImgSize(R.drawable.default_banner_image);	
		if(cvbi instanceof CmsVedioBaseInfo){
			return MessageFormat.format(ImgString.HomeBanner, Integer.toString(cvbi.getId()),getLanguageCode(),iszie.getWidth(),iszie.getHeight());
		}else{
			return MessageFormat.format(ImgString.AppBanner, Integer.toString(cvbi.getId()),iszie.getWidth(),iszie.getHeight());
		}
	}
	
	/**
	 * 首页poster
	 * @param cvbi
	 * @return
	 */
	public static String getHomeVideoPoster(CmsItemable cvbi){
		ImgSize iszie = getImgSize(R.drawable.defalut_movie_small_item);	
		return MessageFormat.format(ImgString.HomePoster, Integer.toString(cvbi.getId()),getLanguageCode(),iszie.getWidth(),iszie.getHeight());
	}
	
	/**
	 * 获取视频详情页图片
	 * @param cvbi
	 * @return
	 */
	public static String getVideoDetailPoster(CmsVedioDetailInfo cvbi){
		ImgSize iszie = getImgSize(R.drawable.default_movie_image);	
		return MessageFormat.format(ImgString.VideoDetailPoster, Integer.toString(cvbi.getId()),getLanguageCode(),iszie.getWidth(),iszie.getHeight());
	}
	
	/**
	 * 视频详情页中的相关视频的海报,
	 * @return
	 */
	public static String getDetailRecommendPoster(CmsVedioBaseInfo cvbi){
		ImgSize iszie = getImgSize(R.drawable.default_movie_image);	
		return MessageFormat.format(ImgString.DetailRecommendPoster, Integer.toString(cvbi.getId()),getLanguageCode(),iszie.getWidth(),iszie.getHeight());
	}
	
	/**
	 * 获取视频详情页预告片截图
	 * @return
	 */
	public static String getSnapshotPoster(int vedioId){
		ImgSize iszie = getImgSize(R.drawable.default_trailer_image_480x270);	
		return MessageFormat.format(ImgString.SnapshotPoster, Integer.toString(vedioId), iszie.getWidth(), iszie.getHeight());
	}
	
	/**
	 * 获取应用icon
	 * @param cvbi
	 * @return
	 */
	public static String getAppIcon(CmsItemable cvbi){
		ImgSize iszie = getImgSize(R.drawable.black_game_detail_icon);	
		return MessageFormat.format(ImgString.AppIcon, Integer.toString(cvbi.getId()),iszie.getWidth(),iszie.getHeight());
	}
	
	/**
	 * 获取应用海报
	 * @return
	 */
	public static String getAppPoster(CmsItemable cvbi){
		ImgSize iszie = getImgSize(R.drawable.default_game_horizontal);	
		return MessageFormat.format(ImgString.AppPoster, Integer.toString(cvbi.getId()),iszie.getWidth(),iszie.getHeight());
	}
	
	/**
	 * 获取视频普通图片，标准版
	 * @param cvbi
	 * @return
	 */
	public static String getVideoPoster(CmsItemable cvbi){
		ImgSize iszie = getImgSize(R.drawable.default_movie_image);	
		return MessageFormat.format(ImgString.DetailRecommendPoster, Integer.toString(cvbi.getId()),getLanguageCode(),iszie.getWidth(),iszie.getHeight());
	}
	
	/**
	 * 获取小视频的横版海报
	 * @param cvbi
	 * @return
	 */
	public static String getShortVideoPoster(CmsItemable cvbi){
		ImgSize iszie = getImgSize(R.drawable.default_home_movie);	
		return MessageFormat.format(ImgString.VideoPoster, Integer.toString(cvbi.getId()),getLanguageCode(),iszie.getWidth(),iszie.getHeight());
	}
	
	private static String getLanguageCode(){
		String lg = Locale.getDefault().getLanguage();		 
		String country = Locale.getDefault().getCountry();
		if(country.toLowerCase().equals("tw") && lg.toLowerCase().equals("zh")){//台湾的语言加上地区号
			lg = "zh_tw";
		}
		return lg;
	}
	/*** 客户端图片的类型和比例，和服务器是约定好的*/
	private class ImgString {
		
		/**
		 * 图片地址，原格式为http://ceto.d3dstore.com/public/image/albums/{albumId}/{languageCode}/img.png?ratio=banner
		 * 第一个是视频的albumid，第二个是languageCode国家码，以此类推
		 */
		public static final String HomeBanner = "http://ceto.d3dstore.com/public/image/albums/{0}/{1}/img.png?ratio=banner&width={2}&height={3}";//首页banner
		public static final String HomePoster = "http://ceto.d3dstore.com/public/image/albums/{0}/{1}/img.png?ratio=tile&width={2}&height={3}";//首页poster
		public static final String VideoPoster = "http://ceto.d3dstore.com/public/image/albums/{0}/{1}/img.png?ratio=list&width={2}&height={3}";// 列表页poster, 
		     
		public static final String VideoDetailPoster = "http://ceto.d3dstore.com/public/image/albums/{0}/{1}/img.png?ratio=poster&width={2}&height={3}";//视频详情页海报
		public static final String SnapshotPoster = "http://ceto.d3dstore.com/public/image/videos/{0}/img.png?ratio=snapshot&width={1}&height={2}";//视频详情页预告片截图, 
		public static final String DetailRecommendPoster = "http://ceto.d3dstore.com/public/image/albums/{0}/{1}/img.png?ratio=poster&width={2}&height={3}";//视频详情页中的相关视频的海报,

		//2、应用
		public static final String AppBanner = "http://ceto.d3dstore.com/public/image/apps/{0}/img.png?ratio=banner&width={1}&height={2}" ;// 首页banner, 
		public static final String AppPoster = "http://ceto.d3dstore.com/public/image/apps/{0}/img.png?ratio=poster&width={1}&height={2}";//应用poster
		public static final String AppSnapshot = "http://ceto.d3dstore.com/public/image/apps/{0}/{1}-img.png?ratio=snapshot&width={2}&height={3}";//  应用snapshot, 1是idx
		public static final String AppIcon = "http://ceto.d3dstore.com/public/image/apps/{0}/img.png?ratio=icon&width={1}&height={2}";//应用icon,  //1是languageCode

	}	
}
