///**
// * ResourceManager.java 
// * com.xunlei.cloud.action.resource.ResourceManager
// * @author: zhang_zhi
// * @date: 2013-6-24 下午5:07:22
// */
//package com.runmit.sweedee.action.home;
//
//import java.io.File;
//import java.io.FileNotFoundException;
//import java.io.FileOutputStream;
//import java.io.IOException;
//import java.io.InputStream;
//import java.net.HttpURLConnection;
//import java.net.MalformedURLException;
//import java.net.URL;
//import java.util.ArrayList;
//import java.util.LinkedList;
//import java.util.concurrent.Callable;
//import java.util.concurrent.ExecutorService;
//import java.util.concurrent.Executors;
//
//import org.apache.http.Header;
//import org.apache.http.HttpResponse;
//import org.apache.http.HttpStatus;
//import org.apache.http.ParseException;
//import org.apache.http.client.ClientProtocolException;
//import org.apache.http.client.HttpClient;
//import org.apache.http.client.methods.HttpGet;
//import org.apache.http.impl.client.DefaultHttpClient;
//import org.apache.http.params.BasicHttpParams;
//import org.apache.http.params.HttpConnectionParams;
//import org.apache.http.util.EntityUtils;
//
//import android.content.Context;
//import android.graphics.Bitmap;
//import android.graphics.Bitmap.CompressFormat;
//import android.graphics.BitmapFactory;
//import android.os.Handler;
//
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.StoreApplication;
//import com.runmit.sweedee.constant.MediaLibConstant;
//import com.runmit.sweedee.database.StatisticalReport;
//import com.runmit.sweedee.manager.DiskDataCache;
//import com.runmit.sweedee.model.MoiveCarousel;
//import com.runmit.sweedee.model.RecommendResourceInfo;
//import com.runmit.sweedee.update.Config;
//import com.runmit.sweedee.util.Util;
//import com.runmit.sweedee.util.XLSharePrefence;
//import com.runmit.sweedee.util.XL_Log;
//import com.xunlei.cloud.service.DataEngine;
//import com.xunlei.cloud.service.DataEngine.DataStruct;
//import com.xunlei.cloud.service.DataEngine.STATUS_CODE;
//
///**
// * @author zhang_zhi
// *         <p/>
// *         <p/>
// *         /**
// *         首先加载本地数据
// *         1.如果本地数据为空，则获取服务器数据，此时If-Modified-Since需要置空请求。
// *         2.如果本地数据不为空，则获取服务器数据，此时If-Modified-Since用sp里面保存的数据
// *         由于IS_LOGINED_ON_THIS_DEVICE可能刚好改变了，但是我们取的是两个不同的服务器配置文件他们的If-Modified-Since应该是不一样的
// *         所以就算登陆状态改变，我们取得数据还是我们想要的
// *         3.如果取服务器数据返回304或者其他失败，则用本地的，如果本地的也没有，则用app自带的那一份
// */
//public class ResourceManager extends Thread {
//
//    private XL_Log log = new XL_Log(ResourceManager.class);
//
//    private Context mContext;
//
//    private XLSharePrefence mXlSharePrefence;
//
//    private static final String LAST_RESOURCE_VERSION = "last_resource_version";
//
//    public static final String LOADING_BITMAP_DELAY_TIME = "loading_bitmap_delay";
//
//    private static final String Bitmap_Version = "loading_bitmap_version";
//
//    private String last_resource_version;
//
//    private DiskDataCache mDiskDataCache;
//
//    private static Object mSyncObj = new Object();
//
//    private static RecommendResourceInfo mRecommendResourceInfo;
//
////	private static ImageFetcher mImageFetcher;
//
//    public static int bigimg_width;
//    public static int bigimg_height;
//
//    public static int smallimg_width;
//    public static int smallimg_height;
//
//    private static final String YUNBO_LOADING_IMG = "http://i0.media.geilijiasu.com/cms/mobile/start.png";
//
//    private static final String YUNBO_HOMEPAGE_URL = "http://media.v.xunlei.com/mobile/wireless_homepage";
//
//    public static final int MSG_RELOAD_RESOURCE = 12345678;
//
//    /**
//     * 首页热播轮播图的宽高比
//     * 是230X480的效果图，放到其他手机上则是等比缩放
//     */
//    public static final float ASPECT_RATIO = 344.0f / 720;
//
//    public ResourceManager(Context context) {
//
//        this.mContext = context;
//        mXlSharePrefence = XLSharePrefence.getInstance(mContext);
//        last_resource_version = mXlSharePrefence.getString(LAST_RESOURCE_VERSION, Integer.toString(0));
//
//        mDiskDataCache = new DiskDataCache(context, YUNBO_HOMEPAGE_URL);
//
//        int SCREEN_WIDTH = Util.getScreenWidth(mContext);
//
//        bigimg_width = SCREEN_WIDTH;
//        bigimg_height = (int) (ASPECT_RATIO * SCREEN_WIDTH);
//
//        smallimg_width = context.getResources().getDimensionPixelSize(R.dimen.resource_small_width);
//        smallimg_height = context.getResources().getDimensionPixelSize(R.dimen.resource_small_height);
//
//        log.debug("bigimg_height=" + bigimg_height + ",smallimg_width=" + smallimg_width + ",smallimg_height=" + smallimg_height);
//
//    }
//
//    public static RecommendResourceInfo getRecommendResourceInfo() {
//        return mRecommendResourceInfo;
//    }
//
////	public static ImageFetcher getImageFetcher(Context mContext) {
////		if(mImageFetcher==null){
////			mImageFetcher=new RoundCornorFetcher(mContext.getApplicationContext());
////		}
////		return mImageFetcher;
////	}
//
//
//    private Bitmap downLoadBitmap(String url) {
//        log.debug("downLoadBitmap url =" + url);
//        URL fileUrl = null;
//        Bitmap bitmap = null;
//        HttpURLConnection conn = null;
//        InputStream is = null;
//
//        try {
//            fileUrl = new URL(url);
//        } catch (MalformedURLException e) {
//            e.printStackTrace();
//            return null;
//        }
//        try {
//            conn = (HttpURLConnection) fileUrl.openConnection();
//            conn.setConnectTimeout(4 * 1000);
//            conn.setReadTimeout(4 * 1000);
//            conn.setDoInput(true);
//            String bitmap_version = mXlSharePrefence.getString(Bitmap_Version, "");
//            conn.addRequestProperty("If-Modified-Since", bitmap_version);
//            conn.setRequestProperty("Referer", "http://pad.i.vod.xunlei.com");
//
//            conn.connect();
//            is = conn.getInputStream();
//            String ad_Time = conn.getHeaderField("Ad-Time");
//            log.debug("ad_Time=" + ad_Time);
//            try {
//                int delay_Time = Integer.parseInt(ad_Time + ",is=" + is);
//                mXlSharePrefence.putInt(LOADING_BITMAP_DELAY_TIME, delay_Time);
//            } catch (NumberFormatException e) {
//                e.printStackTrace();
//            }
//
//            bitmap = BitmapFactory.decodeStream(is);
//            log.debug("decodeStream=" + bitmap);
//            if (bitmap != null) {
//                File f = new File(getLoadingBitmapDirs());
//                if (!f.exists()) {
//                    f.mkdirs();
//                }
//                FileOutputStream fos = null;
//                try {
//                    fos = new FileOutputStream(getLoadingBitmapFullPath());
//                    bitmap.compress(CompressFormat.PNG, 100, fos);
//                    log.debug("loadLoadingImgToDisk save success=" + bitmap);
//
//                    String current_version = conn.getHeaderField("Last-Modified");
//                    log.debug("current_version=" + current_version);
//                    if (current_version != null) {
//                        mXlSharePrefence.putString(Bitmap_Version, current_version);
//                    }
//                    log.debug("downLoadBitmap bitmap=" + bitmap);
//                } catch (FileNotFoundException e) {
//                    e.printStackTrace();
//                } finally {
//                    if (fos != null) {
//                        try {
//                            fos.close();
//                        } catch (IOException e) {
//                            e.printStackTrace();
//                        }
//                    }
//                    if (bitmap != null) {
//                        bitmap.recycle();
//                        bitmap = null;
//                    }
//                }
//            }
//            return bitmap;
//        } catch (IOException e) {
//            e.printStackTrace();
//        } catch (OutOfMemoryError e) {
//            e.printStackTrace();
//        } finally {
//            try {
//                if (is != null) {
//                    is.close();
//                }
//            } catch (IOException e) {
//                e.printStackTrace();
//            }
//            if (conn != null) {
//                conn.disconnect();
//            }
//        }
//        return null;
//    }
//
//    /**
//     * 下载启动图到本地
//     */
//    private void loadLoadingImgToDisk() {
//        new Thread(new Runnable() {
//            @Override
//            public void run() {
//                Bitmap b = downLoadBitmap(YUNBO_LOADING_IMG);
//                log.debug("loadLoadingImgToDisk=" + b);
//            }
//        }).start();
//
//    }
//
//    private static String getLoadingBitmapDirs() {
//        File f = StoreApplication.INSTANCE.getCacheDir();
//        String cachePath = null;
//        if (f != null) {
//            cachePath = f.getPath() + "/loading/";
//        } else {
//            cachePath = Util.getSDCardDir() + "CLOUDPLAY/loading/";
//        }
//
//        return cachePath;
//    }
//
//    public static String getLoadingBitmapFullPath() {
//        return getLoadingBitmapDirs() + "startloading_img";
//    }
//
//    private synchronized String getServerDataImpl(String cient_version) {
//        log.debug("getServerDataImpl cient_version=" + cient_version);
//        DataStruct mDataStruct = null;
//        long starttime = System.currentTimeMillis();
//        String url = YUNBO_HOMEPAGE_URL;
//
//        mDataStruct = getNetDataByGet(url, "get_recommend_req", cient_version);
//        log.debug("rtn_code = " + mDataStruct.rtn_code);
//        StatisticalReport.getInstance().huoQuReBo(mDataStruct.rtn_code, (System.currentTimeMillis() - starttime));
//        if (mDataStruct.rtn_code == 0 && mDataStruct.mainData != null) {// 服务器返回错误码为0.表示请求成功并且数据改变
//            return mDataStruct.mainData.toString();
//        }
//
//        return null;
//    }
//
//    private void addDataToCache() {
//        if (mRecommendResourceInfo == null) {
//            return;
//        }
//        final int SEARCH_THREADS = 3;
//        final SyncStack queue = new SyncStack();
//
//        ArrayList<MoiveCarousel> topRecommendList = mRecommendResourceInfo.toprecommendlist;
//        if (topRecommendList != null) {
//            for (int i = 0, size = Math.min(topRecommendList.size(), SEARCH_THREADS); i < size; i++) {
//                MoiveCarousel mInfo = topRecommendList.get(i);
//                queue.push(mInfo);
//            }
//        }
//
//        final ExecutorService pool = Executors
//                .newScheduledThreadPool(SEARCH_THREADS);
//        for (int i = 0; i <= SEARCH_THREADS; i++) {
//            pool.submit(new LoadTask(queue));
//        }
//    }
//
//    public void loadRecommendResourceInfoAgain(final Handler mHandler) {
//        new Thread(new Runnable() {
//            @Override
//            public void run() {
//                synchronized (mSyncObj) {
//                    if (mRecommendResourceInfo == null) {
//                        String diskData = mDiskDataCache.loadDataFromDiskImpl();
//                        if (diskData != null) {
//                            mRecommendResourceInfo = RecommendResourceInfo.newInstance(diskData);
//                        }
//                        if (mRecommendResourceInfo == null) {// 此处为null,表示本地加载失败
//                            String serverData = mDiskDataCache.loadDataFromResource(R.raw.homepage);
//                            if (serverData != null) {
//                                mRecommendResourceInfo = RecommendResourceInfo.newInstance(serverData);
//                            }
//                        }
//                    }
//                    mHandler.obtainMessage(MSG_RELOAD_RESOURCE).sendToTarget();
//                }
//            }
//        }).start();
//    }
//
//    /**
//     * 首先加载本地数据
//     * 1.如果本地数据为空，则获取服务器数据，此时If-Modified-Since需要置空请求。
//     * 2.如果本地数据不为空，则获取服务器数据，此时If-Modified-Since用sp里面保存的数据
//     * 由于IS_LOGINED_ON_THIS_DEVICE可能刚好改变了，但是我们取的是两个不同的服务器配置文件他们的If-Modified-Since应该是不一样的
//     * 所以就算登陆状态改变，我们取得数据还是我们想要的
//     * 3.如果取服务器数据返回304或者其他失败，则用本地的，如果本地的也没有，则用app自带的那一份
//     */
//    @Override
//    public void run() {
//        synchronized (mSyncObj) {
//            mRecommendResourceInfo = null;//保证最新的
//
//            String diskData = mDiskDataCache.loadDataFromDiskImpl();
//            RecommendResourceInfo mDiskData = null;
//            if (diskData != null) {
//                mDiskData = RecommendResourceInfo.newInstance(diskData);
//            }//取本地数据到此结束
//            log.debug("mRecommendResourceInfo mDiskData=" + mDiskData);
//
//            String client_version = Integer.toString(0);
//            if (mDiskData == null) {// 此处为null,表示本地加载失败
//                client_version = Integer.toString(0);
//            } else {
//                client_version = last_resource_version;
//            }
//
//            String serverData = getServerDataImpl(client_version);
//            log.debug("serverData=" + serverData);
//            if (serverData != null) {
//                mRecommendResourceInfo = RecommendResourceInfo.newInstance(serverData);
//            }//获取服务器数据到此结束
//            log.debug("mRecommendResourceInfo=" + mRecommendResourceInfo);
//
//            if (mRecommendResourceInfo == null) {
//                mRecommendResourceInfo = mDiskData;//赋值给它
//                if (mRecommendResourceInfo == null) {
//                    String appData = mDiskDataCache.loadDataFromResource(R.raw.homepage);//赋值本地的给他
//                    mRecommendResourceInfo = RecommendResourceInfo.newInstance(appData);
//                }
//            }
//            loadLoadingImgToDisk();//加载loading图
//        }
//
//    }
//
//    class LoadTask implements Callable<Boolean> {
//
//        private SyncStack queue;
//
//        /**
//         * 消费者
//         *
//         * @param queue
//         */
//        public LoadTask(SyncStack queue) {
//            this.queue = queue;
//        }
//
//        @Override
//        public Boolean call() throws Exception {
//            boolean done = false;
//            while (!done) {
//                MoiveCarousel mInfo = queue.pop();
//                log.debug("mInfo=" + mInfo);
//                if (mInfo == null) {
//                    done = true;
//                } else {
//                    if (mInfo.getResType() == 0) {
////						boolean issuccess = mImageFetcher.loadLocalBitmapToCache(mInfo.getPoster(), bigimg_width, bigimg_height);
////						if(!issuccess){
////							Bitmap b= BitmapDecoder.getInSizeBitmapByUrl(mInfo.getPoster(), bigimg_width, bigimg_height);
////							mImageFetcher.addBitmapToCache(mInfo.getPoster(), b);
////							mImageFetcher.addBitmapToDiskCache(mInfo.getPoster(), b);
////						}
//                    }
//                }
//            }
//            return done;
//
//        }
//
//    }
//
//
//    private DataStruct getNetDataByGet(String getUrl, String command_id, String cilent_version) {
//        if (!Util.isNetworkAvailable()) {
//            return new DataStruct(DataEngine.ERROR_CODE_NET);
//        }
//
//        StringBuilder cookiesb = new StringBuilder();
//        cookiesb.append("client=").append(MediaLibConstant.CLIENT_ID).append(";").append("version=")
//                .append(Config.getVerName(StoreApplication.INSTANCE));
//
//        BasicHttpParams httpParams = new BasicHttpParams();
//        HttpConnectionParams.setConnectionTimeout(httpParams, 4 * 1000);
//        HttpConnectionParams.setSoTimeout(httpParams, 5 * 1000);
//        HttpClient httpclient = new DefaultHttpClient(httpParams);
//
//        HttpGet httpGet = new HttpGet(getUrl);
//        httpGet.addHeader("If-Modified-Since", cilent_version);
//        httpGet.addHeader("Cookie", cookiesb.toString());
//        httpGet.addHeader("Referer", "http://pad.i.vod.xunlei.com");
//        HttpResponse httpResponse = null;
//        try {
//            httpResponse = httpclient.execute(httpGet);
//        } catch (ClientProtocolException e) {
//            e.printStackTrace();
//            return new DataStruct(STATUS_CODE.ERROR_WRITEDATA);
//        } catch (IOException e) {
//            e.printStackTrace();
//            return new DataStruct(STATUS_CODE.ERROR_WRITEDATA);
//        }
//        int response_code = httpResponse.getStatusLine().getStatusCode();
//        log.debug("response_code=" + response_code);
//
//        if (response_code == HttpStatus.SC_OK) {
//            Header h = httpResponse.getFirstHeader("Last-Modified");
//            if (h != null) {
//                last_resource_version = h.getValue();
//                mXlSharePrefence.putString(LAST_RESOURCE_VERSION, last_resource_version);
//            }
//            String strResult = null;
//            try {
//                strResult = EntityUtils.toString(httpResponse.getEntity(), "utf-8");
//                mDiskDataCache.saveCacheData(strResult);
//                DataStruct result = new DataStruct(strResult);
//                result.rtn_code = 0;
//                return result;
//            } catch (ParseException e) {
//                e.printStackTrace();
//                return new DataStruct(STATUS_CODE.ERROR_GETINPUTSTREAM);
//            } catch (IOException e) {
//                e.printStackTrace();
//                return new DataStruct(STATUS_CODE.ERROR_GETINPUTSTREAM);
//            }
//        } else {
//            return new DataStruct(response_code);
//        }
//    }
//
//    class SyncStack {
//        private LinkedList<MoiveCarousel> queue = new LinkedList<MoiveCarousel>();
//
//        public synchronized void push(MoiveCarousel sst) {
//            queue.addLast(sst);
//        }
//
//        /**
//         * Retrieves and removes the first element of this deque, or returns null if this deque is empty.
//         *
//         * @return
//         */
//        public synchronized MoiveCarousel pop() {
//            return queue.pollFirst();
//        }
//    }
//
//}
