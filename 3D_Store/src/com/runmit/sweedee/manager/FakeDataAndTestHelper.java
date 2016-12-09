/**
 *
 */
package com.runmit.sweedee.manager;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.TreeMap;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.action.more.FeedBackActivity;
import com.runmit.sweedee.constant.ServiceArg;
import com.runmit.sweedee.datadao.PlayRecord;
import com.runmit.sweedee.downloadinterface.DownloadEngine;
import com.runmit.sweedee.manager.data.EventCode.AccountEventCode;
import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.Util;

/**
 * @author Sven.Zhan
 * sven用的各种测试接口
 */
public class FakeDataAndTestHelper {

    @SuppressLint("UseSparseArrays")
    static TreeMap<Integer, String> property = new TreeMap<Integer, String>();

    public static final int EVENT_ADD_NEW_DOWNLOAD_TAST = 99;
    public static final int EVENT_ADD_NEW_DOWNLOAD_TAST2 = 100;
    public static final int EVENT_ADD_NEW_DOWNLOAD_TAST3 = 1001;
    static ArrayList<Node> testList = new ArrayList<Node>();
    static {
        property.put(CmsEventCode.EVENT_GET_HOME, "app_home");
        property.put(CmsEventCode.EVENT_GET_DETAIL, "detail.json");
        property.put(CmsEventCode.EVENT_GET_FILTRATE, "filtrate.json");
        property.put(CmsEventCode.EVENT_GET_FILTRATE_CONFIG, "filtrateconfig.json");
        property.put(CmsEventCode.EVENT_GET_HOT_SEARCHKEY, "hotsearchkey.json");
        property.put(CmsEventCode.EVENT_GET_RECOMMEND, "recommend.json");
        property.put(CmsEventCode.EVENT_GET_MOVIE_COMMENTS, "comment.json");
        property.put(CmsEventCode.EVENT_GET_SEARCH, "searchresult.json");
        property.put(CmsEventCode.EVENT_GET_APP_LIST, "sever_app_test_list.json");
        
        property.put(AccountEventCode.EVENT_GET_PRICE, "price.json");
        property.put(AccountEventCode.EVENT_GET_USER_PURCHASES, "purchase.json");

        /**------------------------------------------------------------------*/
        testList.add(new Node(CmsEventCode.EVENT_GET_HOME, "首页"));
        testList.add(new Node(CmsEventCode.EVENT_GET_DETAIL, "详情页"));
        testList.add(new Node(CmsEventCode.EVENT_GET_FILTRATE, "筛选"));
        testList.add(new Node(CmsEventCode.EVENT_GET_FILTRATE_CONFIG, "筛选配置"));
        testList.add(new Node(CmsEventCode.EVENT_GET_HOT_SEARCHKEY, "热搜词"));
        testList.add(new Node(CmsEventCode.EVENT_GET_RECOMMEND, "影片推荐"));
        testList.add(new Node(CmsEventCode.EVENT_GET_MOVIE_COMMENTS, "影片评论"));
        testList.add(new Node(CmsEventCode.EVENT_GET_SEARCH, "影片搜索"));
        testList.add(new Node(AccountEventCode.EVENT_GET_PRICE, "商品价格"));
        testList.add(new Node(AccountEventCode.EVENT_GET_USER_PURCHASES, "购买记录"));

        /**------------------------------------------------------------------*/
        testList.add(new Node(EVENT_ADD_NEW_DOWNLOAD_TAST, "新建缓存任务"));
        testList.add(new Node(EVENT_ADD_NEW_DOWNLOAD_TAST2, "新建缓存任务2"));
        testList.add(new Node(EVENT_ADD_NEW_DOWNLOAD_TAST3, "用户上报"));
    }

    public static String getContentString(int eventCode) {
        String string = null;
        String jsonFilename = property.get(eventCode);
        if (jsonFilename != null) {
            try {
                InputStream is = StoreApplication.INSTANCE.getAssets().open(jsonFilename);
                string = Util.inputStream2String(is);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return string;
    }

    /**
     * CSM 各个假数据接口的测试窗口
     *
     * @param activity
     */
    public static void executeCmsInterfaceTest(Activity activity) {
        Dialog dialog = new Dialog(activity);
        dialog.setTitle("假数据测试触发点");
        ListView listView = new ListView(activity);
        final ArrayAdapter<String> adapter = new ArrayAdapter<String>(activity, android.R.layout.simple_expandable_list_item_1);
        for (Node node : testList) {
            adapter.addAll(node.toString());
        }
        listView.setAdapter(adapter);
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String str = adapter.getItem(position);
                String[] strs = str.split("_");
                int eventCode = Integer.parseInt(strs[0]);
                CmsServiceManager csm = CmsServiceManager.getInstance();
                AccountServiceManager asm = AccountServiceManager.getInstance();
                switch (eventCode) {
                    case CmsEventCode.EVENT_GET_HOME:
                    	csm.getModuleData(ServiceArg.CMS_MODULE_MOVIE, null);                        
                        break;
                    case CmsEventCode.EVENT_GET_DETAIL:
//                    	Intent inten = new Intent(StoreApplication.INSTANCE, PlayRecordActivity.class);
//                    	inten.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
//                    	StoreApplication.INSTANCE.startActivity(inten);
                      csm.getMovieDetail(16, null);
                        break;
                    case CmsEventCode.EVENT_GET_FILTRATE:
                    	HashMap<String, String> map = new HashMap<String, String>();
                    	map.put("genres", "动作");
                    	map.put("region", "美国");
//                        csm.getMovieFiltrate(null, null);    
                    	csm.getGslbPlayUrl(18,132,0, null);
                        break;
//                    case CmsEventCode.EVENT_GET_FILTRATE_CONFIG:
//                        csm.getMovieFilteConfig(null);
//                        break;
                    case CmsEventCode.EVENT_GET_RECOMMEND:
                        csm.getMovieRecommend(0,0,20, null);
                        break;
                    case CmsEventCode.EVENT_GET_HOT_SEARCHKEY:
                        csm.getHotSearchKey(null);
                        break;
                    case CmsEventCode.EVENT_GET_SEARCH:
                        csm.getSearch("冰川",0,20, null);
                        break;
                    case CmsEventCode.EVENT_GET_MOVIE_COMMENTS:
                        csm.getMovieComments(16,0,20, null);
                        break;
                    case AccountEventCode.EVENT_GET_USER_PURCHASES:
                        asm.getUserPurchases(1, 10, null);
                        break;
                    case AccountEventCode.EVENT_GET_PRICE:
//                        asm.getProductPrice(89, 1, null);
                        testListToStr();
                        break;
//                    case EVENT_ADD_NEW_DOWNLOAD_TAST:
//                    	createTaskForSublistRes();
//                        break;
//                    case EVENT_ADD_NEW_DOWNLOAD_TAST2:
//                    	createTaskForSublistRes2();
//                    	break;
//                    case EVENT_ADD_NEW_DOWNLOAD_TAST3:
//                    	FeedBackActivity.postTest(StoreApplication.INSTANCE.getApplicationContext());
//                    	break;
                }
            }
        });
        dialog.setContentView(listView);
        dialog.show();
    }
    
//    /**
//     * 通过moiveid和submoiveid缓存
//     */
//    private static void createTaskForSublistRes() {
//        String url = "http://dl76.80s.la:920/1408/[尸兄]第29集/[尸兄]第29集_hd.mp4";
//        long file_size = 20971520;
//        String fileName = "nn29.mp4";
//        String poster = "http://i1.media.geilijiasu.com/m/5f/dd/5fdd9d897ea08d6598698d03795bb2b1.jpg";
//        String cookie = "BAIDU_DUP_lcr=http://www.baidu.com/s?ie=utf-8&f=3&tn=baidu&wd=mp4&oq=html5%20v&rsv_bp=1&rsv_enter=1&rsv_sug3=10&rsv_sug4=970&rsv_sug1=9&rsv_sug2=0&rsp=0&inputT=1539; Hm_lvt_e55ff7844747a41e412fd2b38266f729=1410515535; Hm_lpvt_e55ff7844747a41e412fd2b38266f729=1410515648";
//        DELocalManager.getInstance().createTaskByUrl(1237,Constant.NORMAL_VOD_URL,url, DownloadEngine.CacheDir, fileName, cookie, file_size, poster,new ArrayList<SubTitleInfo>());
//    }
//    
//    private static void createTaskForSublistRes2() {
//        String url = "ftp://r:r@d3.dl1234.com:8006/[电影天堂www.dy2018.com]白发魔女传之明月天国DVDSrc中字.rmvb";
//        long file_size = 20971520;
//        String fileName = "w白发*&*~!monv.mp4";
//        String poster = "http://i1.media.geilijiasu.com/m/5f/dd/5fdd9d897ea08d6598698d03795bb2b1.jpg";
//        String cookie = "BAIDU_DUP_lcr=http://www.baidu.com/s?ie=utf-8&f=3&tn=baidu&wd=mp4&oq=html5%20v&rsv_bp=1&rsv_enter=1&rsv_sug3=10&rsv_sug4=970&rsv_sug1=9&rsv_sug2=0&rsp=0&inputT=1539; Hm_lvt_e55ff7844747a41e412fd2b38266f729=1410515535; Hm_lpvt_e55ff7844747a41e412fd2b38266f729=1410515648";
//        DELocalManager.getInstance().createTaskByUrl(1237,Constant.NORMAL_VOD_URL,url,  DownloadEngine.CacheDir, fileName, cookie, file_size, poster,new ArrayList<SubTitleInfo>());
//    }

    public static class Node {

        public int eventCode;

        public String desc;

        public Node(int eventCode, String desc) {
            this.eventCode = eventCode;
            this.desc = desc;
        }

        public String toString() {
            return eventCode + "_" + desc;
        }
    }
    
    public static String testListToStr(){
    	ArrayList<SubTitleInfo> subtilteInfo = new ArrayList<SubTitleInfo>();   
    	
    	SubTitleInfo st1 = new SubTitleInfo();
    	st1.title = "[冰河世纪3].Ice.Age.Dawn.Of.The.Dinosaurs.BDRip.XviD-iMBT.eng.srt";
    	st1.url = "http://pub.vrs.runmit-dev.com/public/subtitle/2014-09-15/1/1/b6aef7700165ee28d991a135496a8093.srt";    	
    	SubTitleInfo st2 = new SubTitleInfo();
    	
    	st2.title = "Ice.Age.Dawn.Of.The.Dinosaurs.BDRip.XviD-iMBT.eng.srt";
    	st2.url = "http://pub.vrs.runmit-dev.com/public/subtitle/2014-09-15/1/1/b6aa8093.srt";
    	subtilteInfo.add(st1);
    	subtilteInfo.add(st2);
    	
    	String str = new Gson().toJson(subtilteInfo, new TypeToken<ArrayList<SubTitleInfo>>(){}.getType());
    	Log.d("testListToStr", "testListToStr = "+str);
    	return str;
    }
    
    
//    public static List<PlayRecord> createFakePlayRecord(){
//    	PlayRecord pr1 = new PlayRecord(1L,0L,2, "变形金刚1","http://ceto.d3dstore.com/public/album/2014-09-28/1/1/82606c5e02bbfd86ab1b348a2645d3d7.png", System.currentTimeMillis(), 150*2, 150*60,null);
//    	PlayRecord pr2 = new PlayRecord(2L,0L,2, "变形金刚2","http://ceto.d3dstore.com/public/album/2014-09-29/1/1/b446c1ba63db5dfd87d2e22e42ff7be6.png", System.currentTimeMillis(), 150*10, 150*60,null);
//    	PlayRecord pr3 = new PlayRecord(4L,0L,2, "变形金刚3","http://ceto.d3dstore.com/public/album/2014-09-29/1/1/4e3b2678cebd0abc7a32f32bb582cb70.png", System.currentTimeMillis(), 150*50, 150*60,null);
//    	PlayRecord pr4 = new PlayRecord(3L,0L,2, "变形金刚4","http://ceto.d3dstore.com/public/album/2014-09-25/1/1/14a8f30f0d11bd867bf8eb92e640a3a5.png", System.currentTimeMillis(), 150*32, 150*60,null);
//    	PlayRecord pr5 = new PlayRecord(5L,0L,2, "变形金刚5","http://ceto.d3dstore.com/public/album/2014-09-25/1/1/19c8e72c73f5c6098cd0634a43310f7d.png", System.currentTimeMillis(), 150*41, 150*60,null);
//    	PlayRecord pr6 = new PlayRecord(6L,0L,2, "变形金刚6","http://ceto.d3dstore.com/public/album/2014-09-25/1/1/b9d712d1774fc9583a366a2b9fe08e56.png", System.currentTimeMillis(), 150*22, 150*60,null);
//    	PlayRecord pr7 = new PlayRecord(7L,0L,2, "变形金刚7","http://ceto.d3dstore.com/public/album/2014-09-29/1/1/b446c1ba63db5dfd87d2e22e42ff7be6.png", System.currentTimeMillis(), 150*59, 150*60,null);
//    	
//    	PlayRecord pr11 = new PlayRecord(11L,0L,2, "变形金刚1","http://ceto.d3dstore.com/public/album/2014-09-25/1/1/e59db9b44938816abfe642d509ea106d.png", System.currentTimeMillis(), 150*2, 150*60,null);
//    	PlayRecord pr21 = new PlayRecord(21L,0L,2, "变形金刚2","http://ceto.d3dstore.com/public/album/2014-09-28/1/1/82606c5e02bbfd86ab1b348a2645d3d7.png", System.currentTimeMillis(), 150*10, 150*60,null);
//    	PlayRecord pr31 = new PlayRecord(41L,0L,2, "变形金刚3","http://ceto.d3dstore.com/public/album/2014-09-26/1/1/431c0d52848b62a2e7e82bccfecd922e.jpg", System.currentTimeMillis(), 150*50, 150*60,null);
//    	PlayRecord pr41 = new PlayRecord(31L,0L,2, "变形金刚4","http://ceto.d3dstore.com/public/album/2014-09-29/1/1/4e3b2678cebd0abc7a32f32bb582cb70.png", System.currentTimeMillis(), 150*32, 150*60,null);
//    	PlayRecord pr51 = new PlayRecord(51L,0L,2, "变形金刚5","http://pub.vrs.runmit-dev.com/public/album/2014-09-12/1/1/3ab1a6f455ef05105489d2135fe704d3.jpg", System.currentTimeMillis(), 150*41, 150*60,null);
//    	PlayRecord pr61 = new PlayRecord(61L,0L,2, "变形金刚6","http://pub.vrs.runmit-dev.com/public/album/2014-09-12/1/1/d1aaa592e7a5300013e3c8b1e8412eb0.png", System.currentTimeMillis(), 150*22, 150*60,null);
//    	PlayRecord pr71 = new PlayRecord(71L,0L,2, "变形金刚7","http://pub.vrs.runmit-dev.com/public/album/2014-09-13/1/1/c30c708680b00482032657ada6cc1829.jpg", System.currentTimeMillis(), 150*59, 150*60,null);
//    	
//    	List<PlayRecord> list = new ArrayList<PlayRecord>();
//    	list.add(pr1);
//    	list.add(pr2);
//    	list.add(pr3);
//    	list.add(pr4);
//    	list.add(pr5);
//    	list.add(pr6);
//    	list.add(pr7);
//    	list.add(pr11);
//    	list.add(pr21);
//    	list.add(pr31);
//    	list.add(pr41);
//    	list.add(pr51);
//    	list.add(pr61);
//    	list.add(pr71);
//    	return list;
//    }
 
}
