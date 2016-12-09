package com.runmit.sweedee.manager;

import java.net.URI;
import java.net.URISyntaxException;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.apache.http.NameValuePair;
import org.apache.http.client.utils.URLEncodedUtils;
import org.json.JSONArray;
import org.json.JSONObject;

import android.net.Uri;
import android.os.Handler;

import com.google.gson.GsonBuilder;
import com.google.gson.reflect.TypeToken;
import com.runmit.sweedee.manager.ObjectCacheManager.DiskCacheOption;
import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
import com.runmit.sweedee.model.AppPageInfo;
import com.runmit.sweedee.model.CmsModuleInfo;
import com.runmit.sweedee.model.CmsModuleInfo.CmsItem;
import com.runmit.sweedee.model.CmsVedioBaseInfo;
import com.runmit.sweedee.model.CmsVedioComment;
import com.runmit.sweedee.model.CmsVedioDetailInfo;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.CmsVideoCatalog;
import com.runmit.sweedee.model.SearchRecommendList;
import com.runmit.sweedee.model.VOSearch;
import com.runmit.sweedee.model.VedioFilterType;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.player.RealPlayUrlInfo;
import com.runmit.sweedee.player.RealPlayUrlLoader;
import com.runmit.sweedee.player.RealPlayUrlLoader.LoaderListener;
import com.runmit.sweedee.player.RealPlayUrlLoader.Redition;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

/**
 * @author Sven.Zhan
 *  CMS相关服务的封装，单例模式
 */
public class CmsServiceManager extends HttpServerManager {
	
	private XL_Log log = new XL_Log(CmsServiceManager.class);
	
	public static final String MODULE_HOME = "home";
	public static final String MODULE_MOVIE = "movie";
	public static final String MODULE_GAME = "game";
	public static final String MODULE_SHORTVIDEO = "shortvideo_v1.5";
	
	public static final String GSLB_CKECK_ERROR_MODLUE = "PAY";
	public static final String GSLB_CKECK_ERROR_UC = "UC";

    /**
     * CMS module配置的BaseUrl
     */
    private static final String CMS_MODULE_BASEURL = CMS_V1_BASEURL + "modules/";

    /**
     * CMS 代理VRS的BaseUrl
     */
    private static final String CMS_VRS_BASEURL = CMS_V1_BASEURL + "vrs/api/";
    
    /**
     * CMS 代理VRS的图片BaseUrl
     */
    public static final String CMS_VRS_IMAGEBASE = CMS_V1_BASEURL+"img/vrs/";

    private static CmsServiceManager instance;

    public synchronized static CmsServiceManager getInstance() {
        if (instance == null) {
            instance = new CmsServiceManager();
        }
        return instance;
    }

    private CmsServiceManager() {
        TAG = CmsServiceManager.class.getSimpleName();
        gson = new GsonBuilder().registerTypeAdapter(CmsItem.class, CmsItem.createTypeAdapter()).create();
    }
    
    @Override
	public void reset() {		
		super.reset();
		instance = null;
	}

	/**
     * 获取CMS module配置数据  -OK
     * cms http://poseidon.d3dstore.com/api/v1/modules/movie?ct=cn&lg=zh
     *
     * @param moduleName CMS配置的具体模块的名字，必须符合 ，参见 http://poseidon.d3dstore.com/admin/console/channels/1/modules
     * @param callBack
     * @return
     */
    public void getModuleData(String moduleName, Handler callBack) {
        String urlFormat = CMS_MODULE_BASEURL + "{0}?{1}";
        String url = MessageFormat.format(urlFormat, moduleName, getClientInfo(false));
        log.debug("getModuleData url = " + url);
        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_HOME, callBack);
        CacheParam cparam = new CacheParam(url, CmsModuleInfo.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
        rhandler.setCacheParam(cparam);
        sendGetRequst(url, rhandler);
    }

//    /**
//     * 获取筛选配置的各项信息  -OK
//     * cms http: http://poseidon.d3dstore.com/api/v1/vrs/api/filter/configures?lg=zh&ct=cn
//     * vrs_test: http://hydra.d3dstore.com/api/filter/configures?language=zh
//     * 
//     * @param callBack
//     * @return
//     */
//    public void getMovieFilteConfig(Handler callBack) {
//        String url = CMS_VRS_BASEURL + "filter/configures?" + getClientInfo(false);
//        log.debug("getMovieFilteConfig url = " + url);
//        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_FILTRATE_CONFIG, callBack);
//        CacheParam cparam = new CacheParam(url, ArrayList.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
//        rhandler.setCacheParam(cparam);
//        sendGetRequst(url, rhandler);
//    }

//    /**
//     * 获取影片分类刷选结果   -OK
//     * cms: http://poseidon.d3dstore.com/api/v1/vrs/api/filter?sort=score%2Cdesc&lg=zh&ct=cn
//     * vrs_test: http://hydra.d3dstore.com/api/filter?sort=score%2Cdesc&language=zh
//     *
//     * @param filtMap 筛选维度和对应的筛选项Map。为空则无筛选条件，获取全部
//     */
//    public void getMovieFiltrate(Map<String, String> filtMap, Handler callBack) {
//        StringBuilder filter = new StringBuilder();
//        if (filtMap != null) {
//            Set<Entry<String, String>> entrySet = filtMap.entrySet();
//            for (Entry<String, String> entry : entrySet) {
//                filter.append(entry.getKey()).append("=").append(Uri.encode(Uri.decode(entry.getValue()))).append("&");
//            }
//        }
//        String url = CMS_VRS_BASEURL + "filter?" + filter.toString() + getClientInfo(false);
//        log.debug("getMovieFiltrate url = " + url);
//        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_FILTRATE, callBack);
//        CacheParam cparam = new CacheParam(url, ArrayList.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
//        rhandler.setCacheParam(cparam);
//        sendGetRequst(url, rhandler);
//    }
    
    /**
     * 根据筛选url获取影片信息
     * @param filterUrl
     */
    public void getMovieBySorterAndrFilter(String filterUrl,Handler callBack){
    	log.debug("filterUrl = " + filterUrl);
    	try {
    		URI mOriginUri = new URI(filterUrl);
    		List<NameValuePair> params = URLEncodedUtils.parse(mOriginUri, "UTF-8");
    		for(Iterator<NameValuePair> it = params.iterator();it.hasNext();){
    			NameValuePair s = it.next();
    			final String encodedName = s.getName();
    			if(encodedName.equals("ts") || encodedName.equals("ci") || encodedName.equals("lg") || encodedName.equals("gt")){
    				it.remove();
    			}
    	    }
    		URI mFormatUri = new URI(mOriginUri.getScheme(), mOriginUri.getUserInfo(), mOriginUri.getHost(), mOriginUri.getPort(), mOriginUri.getPath(), Util.format(params), mOriginUri.getFragment());
    		filterUrl = mFormatUri.toString() + "&"+getClientInfo(false);
    	} catch (URISyntaxException e) {
			e.printStackTrace();
		}
    	log.debug("filterUrl = " + filterUrl);
    	
        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_ALBUMS, callBack);
        CacheParam cparam = new CacheParam(filterUrl, ArrayList.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
        rhandler.setCacheParam(cparam);
        sendGetRequst(filterUrl, rhandler);	
    }

    /**
     * 获取影片详情   -OK
     * cms: http://poseidon.d3dstore.com/api/v1/vrs/api/albums/62?lg=zh&ct=cn
     * vrs_test: http://hydra.d3dstore.com/api/albums/62?language=zh
     */
    public void getMovieDetail(int albumid, Handler callBack) {
        String url = CMS_VRS_BASEURL + "albums/" + albumid + "?" + getClientInfo(false);
        log.debug("getMovieDetail url = " + url);
        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_DETAIL, callBack);
        CacheParam cparam = new CacheParam(url, CmsVedioDetailInfo.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
        rhandler.setCacheParam(cparam);
        sendGetRequst(url, rhandler);
    }

    /**
     * 获取影片评论   -OK
     * cms: http://poseidon.d3dstore.com/api/v1/vrs/api/albums/62/comments?start=0&rows=20&lg=zh&ct=cn
     * vrs_test: http://hydra.d3dstore.com/api/albums/62/comments?start=0&rows=20&language=zh
     *
     * @param movieid
     * @param start   起始位置
     * @param num     请求的条数
     */
    public void getMovieComments(int albumid, int start, int num, Handler callBack) {
        String url = CMS_VRS_BASEURL + "albums/" + albumid + "/comments?start=" + start + "&rows=" + num + "&" + getClientInfo(false);
        log.debug("getMovieComments url = " + url);
        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_MOVIE_COMMENTS, callBack);
        CacheParam cparam = new CacheParam(url, ArrayList.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
        rhandler.setCacheParam(cparam);
        sendGetRequst(url, rhandler);
    }

    /**
     * 获取影片相关推荐    - OK
     * cms: http://poseidon.d3dstore.com/api/v1/vrs/api/albums/20/related?start=0&rows=20&lg=zh&ct=cn
     * vrs_test: http://hydra.d3dstore.com/api/albums/20/related?language=zh&start=0&rows=20
     *
     * @param start 起始位置
     * @param num   请求的条数
     */
    public void getMovieRecommend(int albumid, int start, int num, Handler callBack) {
        String url = CMS_VRS_BASEURL + "albums/" + albumid + "/related?start=" + start + "&rows=" + num + "&" + getClientInfo(false);
        log.debug("getMovieRecommend url = " + url);
        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_RECOMMEND, callBack);
        CacheParam cparam = new CacheParam(url, ArrayList.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
        rhandler.setCacheParam(cparam);
        sendGetRequst(url, rhandler);
    }

    /**
     * 获取热搜词  -OK
     * cms: http://poseidon.d3dstore.com/api/v1/vrs/api/popularKeys?lg=zh&ct=cn
     * vrs_test : http://hydra.d3dstore.com/api/popularKeys?language=zh
     */
    public void getHotSearchKey(Handler callBack) {
    	String url = CMS_V1_BASEURL+"vrs/api/popularKeys?"+getClientInfo(false);
        //String url = CMS_VRS_BASEURL + "popularKeys?" + getClientInfo(false);
        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_HOT_SEARCHKEY, callBack);
        CacheParam cparam = new CacheParam(url, SearchRecommendList.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
        rhandler.setCacheParam(cparam);
        sendGetRequst(url, rhandler);
    }

    /**
     * 获取影片搜索  - OK
     * CMS：http://cms.runmit-dev.com/api/v1/vrs/api/search?searchKey=%E5%BE%AE&start=0&rows=20&k=superd3drocks&lg=zh&ci=1
     * @param searchKey 搜索词
     * @param start     起始位置
     * @param num       搜索条数
     */
    public void getSearch(String searchKey, int start, int num, Handler callBack) {
    	String baseUrl = CMS_V1_BASEURL+"vrs/api/search?searchKey={0}&start={1}&rows={2}&"+getClientInfo(false);
    	String url = MessageFormat.format(baseUrl, Uri.encode(Uri.decode(searchKey)),start,num);
        //String url = CMS_VRS_BASEURL + "search?searchKey=" + Uri.encode(Uri.decode(searchKey)) + "&start=" + start + "&rows=" + 20 + "&" + getClientInfo(false);
    	log.debug("getMovieSearch url = " + url);
        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_SEARCH, callBack);
        CacheParam cparam = new CacheParam(url, VOSearch.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
        rhandler.setCacheParam(cparam);
        sendGetRequst(url, rhandler);
    }

    /**
     * 获取点播URL  -OK
     * http://poseidon.d3dstore.com/api/v1/gslb/album/62/187?bt=0&lg=zh&pm=ZTE_Q705U&di=devicehwid1030&sv=4.2.2&an=Android_Hi3D&uid=313&token=c78020761ae6ee87f6a892283bd9db94&ct=cn
     * 
     * 预告片例子：
     * http://poseidon.d3dstore.com/api/v1/gslb/album/16/154?bt=0&lg=zh&pm=ZTE_Q705U&di=devicehwid1030&sv=4.2.2&an=Android_Hi3D&uid=313&token=c78020761ae6ee87f6a892283bd9db94&ct=cn
     * @param albumid      媒资id
     * @param vedioid      具体某一播放vedio的id
     * @param businessType 业务类型 0：播放  1：缓存
     * @param callBack
     * @return
     */
    public void getGslbPlayUrl(int albumid, int vedioid, int businessType, Handler callBack) {
        String urlFormat = CMS_V1_BASEURL + "gslb/album/{0}/{1}?bt={2}&{3}";
        String url = MessageFormat.format(urlFormat, Integer.toString(albumid), Integer.toString(vedioid), businessType, getClientInfo(true));
        log.debug("getGslbPlayUrl url = " + url);
        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_VEDIO_GSLB_URL, callBack);
        sendGetRequst(url, rhandler);
    }
    
    /**
     * 获取app的下载Url
     * @param appId
     * @param appKey
     * @param callBack
     */
    public void getAppGslbUrl(AppItemInfo itemInfo, Handler callBack){
    	String baseurl = CMS_V1_BASEURL+"gslb/apps?appId={0}&appKey={1}&{2}";
    	String url = MessageFormat.format(baseurl, itemInfo.appId, itemInfo.appKey,getClientInfo(true));
    	ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_APP_GSLB_URL, callBack);
    	rhandler.setTag(itemInfo);
    	log.debug("baseurl="+url);
        sendGetRequst(url, rhandler);
    }
    
    public void getAppGslbUrl(String appId,String appKey, Handler callBack){
    	String baseurl = CMS_V1_BASEURL+"gslb/apps?appId={0}&appKey={1}&{2}";
    	String url = MessageFormat.format(baseurl, appId, appKey,getClientInfo(true));
    	ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_APP_GSLB_URL, callBack);
    	log.debug("baseurl="+url);
        sendGetRequst(url, rhandler);
    }
    

//    /**
//     * 获取全局配置项  -No Config Now
//     */
//    public void getGlobalConfig() {
//        String url = CMS_V1_BASEURL + "/configs?" + getClientInfo(false);
//        log.debug("getGlobalConfig url = " + url);
//        ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_GLOBAL_CONFIG, null);
//        sendGetRequst(url, rhandler);
//    }
       
    /**
     * 获取应用或游戏列表 
     * @param type 应用：APP  游戏：GAME
     * @param start
     * @param num
     * @param callBack
     */
    public void getAppList(boolean usingCache,String type,int start,int num,Handler callBack){
		String baseUrl = CMS_V1_BASEURL+"vrs/api/apps?type={0}&start={1}&rows={2}&" + getClientInfo(false);
		String url = MessageFormat.format(baseUrl, type, start, num);
		log.debug("url="+url);
		ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_APP_LIST, callBack);
		if(usingCache){
			CacheParam cparam = new CacheParam(url, ArrayList.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
			rhandler.setCacheParam(cparam);
		}
		sendGetRequst(url, rhandler);
    }   
    

    public void getAppUpdateList(String pkgParam,Handler callBack){
		String baseUrl =CMS_V1_BASEURL+"vrs/api/apps/versions?"+ getClientInfo(false);
		log.debug("updatelisturl="+baseUrl);
		ResponseHandler rhandler = new ResponseHandler(CmsEventCode.EVENT_GET_APP_UPDATE_LIST, callBack);
		CacheParam cparam = new CacheParam(baseUrl, ArrayList.class, DiskCacheOption.DEFAULT, CachePost.StopButRefresh);
		rhandler.setCacheParam(cparam);
		sendPostRequest(baseUrl,pkgParam,rhandler);
    }    
    
    public String asImgKeyUrl(String url){
    	return asKey(url);
    }
    
    @Override
    protected void dispatchSuccessResponse(final ResponseHandler responseHandler, String responseString) {
        final int eventCode = responseHandler.getEventCode();
        try {
            JSONObject jobj = new JSONObject(responseString);
            int rtn = jobj.getInt(JSON_RTN_FIELD);
            if (rtn != 0) {
            	if(eventCode == CmsEventCode.EVENT_GET_VEDIO_GSLB_URL){
            		if(rtn == 607){
            			responseHandler.sendCallBackMsg(eventCode, rtn, responseString);
            		}else{
            			responseHandler.sendCallBackMsg(eventCode, rtn, null);
            		}
            	}else{
            		responseHandler.sendCallBackMsg(eventCode, rtn, null);
            	}
                return;
            }

            switch (eventCode) {
                case CmsEventCode.EVENT_GET_HOME://首页数据
                    String homeStr = jobj.getJSONObject(JSON_DATA_FIELD).toString();
                    CmsModuleInfo chdi = gson.fromJson(homeStr, CmsModuleInfo.class);
                    log.debug("CmsModuleInfo="+chdi.data.size());
                    responseHandler.sendCallBackMsg(eventCode, rtn, chdi);
                    break;
                case CmsEventCode.EVENT_GET_DETAIL://影片详情
                    String detailStr = jobj.getJSONObject(JSON_DATA_FIELD).toString();
                    CmsVedioDetailInfo cvdi = gson.fromJson(detailStr, CmsVedioDetailInfo.class);
                    responseHandler.sendCallBackMsg(eventCode, rtn, cvdi);
                    break;
                case CmsEventCode.EVENT_GET_FILTRATE:    //影片筛选
                    String filtrateStr = jobj.getJSONObject(JSON_DATA_FIELD).getJSONArray(JSON_LIST_FIELD).toString();
                    ArrayList<CmsVedioBaseInfo> cvbiList = gson.fromJson(filtrateStr, new TypeToken<ArrayList<CmsVedioBaseInfo>>() {}.getType());
                    responseHandler.sendCallBackMsg(eventCode, rtn, cvbiList);
                    break;
                case CmsEventCode.EVENT_GET_FILTRATE_CONFIG://影片筛选项配置
                    JSONArray jarray = jobj.getJSONArray(JSON_DATA_FIELD);
                    for (int i = 0; i < jarray.length(); i++) {
                        JSONObject tempObj = jarray.getJSONObject(i);
                        if (tempObj.optString("type").equals("movie")) {
                            String filtrateConfigStr = tempObj.getJSONArray(JSON_GROUPS_FIELD).toString();
                            ArrayList<VedioFilterType> cvfList = gson.fromJson(filtrateConfigStr, new TypeToken<ArrayList<VedioFilterType>>() {}.getType());
                            responseHandler.sendCallBackMsg(eventCode, rtn, cvfList);
                            break;
                        }
                    }
                    responseHandler.sendCallBackMsg(eventCode, ExceptionCode.ERROR_DEFUALT, null);
                    break;
                case CmsEventCode.EVENT_GET_RECOMMEND://影片推荐
                    String recommendStr = jobj.getJSONObject(JSON_DATA_FIELD).optJSONArray(JSON_LIST_FIELD).toString();
                    ArrayList<CmsVedioBaseInfo> cvbiRecommendList = gson.fromJson(recommendStr, new TypeToken<ArrayList<CmsVedioBaseInfo>>() {}.getType());
                    responseHandler.sendCallBackMsg(eventCode, rtn, cvbiRecommendList);
                    break;
                case CmsEventCode.EVENT_GET_SEARCH://影片搜索
                    String searchStr = jobj.getJSONObject(JSON_DATA_FIELD).toString();
                    VOSearch searchRes = gson.fromJson(searchStr, VOSearch.class);
                    responseHandler.sendCallBackMsg(eventCode, rtn, searchRes);
                    break;
                case CmsEventCode.EVENT_GET_HOT_SEARCHKEY://热门搜索词
                	String hotSearchKeyStr = jobj.getJSONObject(JSON_DATA_FIELD).toString();
                	SearchRecommendList recommendData = gson.fromJson(hotSearchKeyStr, new TypeToken<SearchRecommendList>() {}.getType());
                    responseHandler.sendCallBackMsg(eventCode, rtn, recommendData);
                    break;
                case CmsEventCode.EVENT_GET_MOVIE_COMMENTS://影片评论
                    String commentStr = jobj.getJSONObject(JSON_DATA_FIELD).optJSONArray(JSON_LIST_FIELD).toString();
                    ArrayList<CmsVedioComment> commentList = gson.fromJson(commentStr, new TypeToken<ArrayList<CmsVedioComment>>() {}.getType());
                    responseHandler.sendCallBackMsg(eventCode, rtn, commentList);
                    break;
                case CmsEventCode.EVENT_GET_VEDIO_GSLB_URL://点播url
                case CmsEventCode.EVENT_GET_APP_GSLB_URL://app下载url
                	 String str = jobj.getJSONArray(JSON_DATA_FIELD).toString();
                	 ArrayList<Redition> redtions = new ArrayList<Redition>();
                	 if(eventCode == CmsEventCode.EVENT_GET_VEDIO_GSLB_URL){
                		 redtions = gson.fromJson(str, new TypeToken<ArrayList<Redition>>() {}.getType());
                	 }else{
                		 ArrayList<String> urlList = gson.fromJson(str, new TypeToken<ArrayList<String>>(){}.getType());
                		 Redition appRedition = new Redition(RealPlayUrlLoader.P720, urlList);
                		 redtions.add(appRedition);
                	 }
                     new RealPlayUrlLoader(redtions, new LoaderListener() {
                         @Override
                         public void onRealPlayUrlLoaded(int result, RealPlayUrlInfo rInfo) {
                        	 if(eventCode == CmsEventCode.EVENT_GET_VEDIO_GSLB_URL){
                        		 responseHandler.sendCallBackMsg(CmsEventCode.EVENT_GET_VEDIO_CDN_URL, result, rInfo);
                        	 }else{
                        		 String appCdnUrl = (result == 0 && rInfo != null) ? rInfo.getCDNUrlForApp() : null;
                        		 responseHandler.sendCallBackMsg(CmsEventCode.EVENT_GET_APP_CDN_URL, result, appCdnUrl);
                        	 }
                         }
                     }).load();
                     break;
                case CmsEventCode.EVENT_GET_GLOBAL_CONFIG://客户端全局配置项
                    //to-do;
                    break;
                case CmsEventCode.EVENT_GET_APP_LIST:
                	String appListStr = jobj.getJSONObject(JSON_DATA_FIELD).toString();
                	AppPageInfo mAppPageInfo = gson.fromJson(appListStr, new TypeToken<AppPageInfo>(){}.getType());
                	responseHandler.sendCallBackMsg(eventCode, rtn, mAppPageInfo);
                	break;
                case CmsEventCode.EVENT_GET_ALBUMS:
                	 log.debug("albums="+responseString);
                	 String catalogStr = jobj.getJSONObject(JSON_DATA_FIELD).toString();
                	 CmsVideoCatalog catalog = gson.fromJson(catalogStr, new TypeToken<CmsVideoCatalog>(){}.getType());
                	 responseHandler.sendCallBackMsg(eventCode, rtn, catalog);
                	break;
                case CmsEventCode.EVENT_GET_APP_UPDATE_LIST:
               	    log.debug("updatelist="+responseString);
               	    String updateListStr = jobj.getJSONArray(JSON_DATA_FIELD).toString();
               	    ArrayList<AppItemInfo> appItemInfos = gson.fromJson(updateListStr, new TypeToken<ArrayList<AppItemInfo>>(){}.getType());       
               	    responseHandler.sendCallBackMsg(eventCode,rtn,appItemInfos);
               	break;
                default:
                    break;
            }
        } catch (Exception e) {
        	e.printStackTrace();
            responseHandler.handleException(e);
        }
    }  
}
