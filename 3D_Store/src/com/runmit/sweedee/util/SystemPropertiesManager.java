//package com.runmit.sweedee.util;
//
//import java.io.BufferedReader;
//import java.io.IOException;
//import java.io.InputStream;
//import java.io.InputStreamReader;
//
//import org.apache.http.Header;
//import org.apache.http.HttpEntity;
//import org.apache.http.HttpResponse;
//import org.apache.http.HttpStatus;
//import org.apache.http.client.ClientProtocolException;
//import org.apache.http.client.HttpClient;
//import org.apache.http.client.methods.HttpGet;
//import org.apache.http.impl.client.DefaultHttpClient;
//import org.apache.http.params.BasicHttpParams;
//import org.apache.http.params.HttpConnectionParams;
//import org.json.JSONException;
//import org.json.JSONObject;
//
//import android.content.Intent;
//import android.os.AsyncTask;
//
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.StoreApplication;
//
//public class SystemPropertiesManager {
//
//    XL_Log log = new XL_Log(SystemPropertiesManager.class);
//    //投票配置用
//    public static final String VOTE_PAGE_VERSION = "VOTE_PAGE_VERSION";
//    public static final String VOTE_PAGE_LAST = "VOTE_PAGE_LAST";
//    public static final String VOTE_PAGE_CURRENT = "VOTE_PAGE_CURRENT";
//    //搜索配置用
//    public static final String ADVICE_ENGINE_VERSION = "advice_engine_version";
//    public static final String ADVICE_ENGINE_ENABLE = "advice_engine_enable";
//
//    private static final String IS_FIRST_INIT_PM = "is_first_init_pm";
//
//    public static boolean isVoteUpdate;  //标志投票内容是否更新
//    public static boolean isEngineUpdate; //标志引擎配置是否更新
//    public static final String pageUrl = "http://wireless.yun.vip.xunlei.com/systemproperties.json";
//    private PageInfo info;
//    //广播事件action
////	public static final String PROPERTY_UPDATE_VOTE_BROADCAST = "property_update_vote_broadcast";
//    public static final String PROPERTY_UPDATE_SEARCH_BROADCAST = "property_update_search_broadcast";
//
//    public void getVoteInfo() {
//        new GetVotePageTask().execute(pageUrl);
//    }
//
//    class GetVotePageTask extends AsyncTask<String, Void, Void> {
//
//        @Override
//        protected Void doInBackground(String... params) {
//            handleResult(getVotePage(params[0]));
//            initPageInfo();
//            return null;
//        }
//    }
//
//    class PageInfo {
//        public int voteVersionCode;
//        public String lastUrl;
//        public String currentUrl;
//
//        public int engineVersionCode;
//        public boolean engineIsEnable;
//    }
//
//    private boolean handleResult(InputStream is) {
//        if (is != null) {
//            praseJsonInfo(is);
//            return true;
//        } else {
//            return false;
//        }
//    }
//
//    private void initPageInfo() {
//        log.debug("initPageInfo info is null = " + (info == null));
//        if (info != null) {
//            XLSharePrefence preferences = XLSharePrefence
//                    .getInstance(StoreApplication.INSTANCE);
//            int currentVersion = preferences.getInt(VOTE_PAGE_VERSION, 0);
//            if (currentVersion == 0 || info.voteVersionCode > currentVersion) {
//                if (info.voteVersionCode > 0) {
//                    preferences.putInt(VOTE_PAGE_VERSION, info.voteVersionCode);
//                    preferences.putString(VOTE_PAGE_LAST, info.lastUrl);
//                    preferences.putString(VOTE_PAGE_CURRENT, info.currentUrl);
//                    isVoteUpdate = true;
////					sendUpdateBroadCast(PROPERTY_UPDATE_VOTE_BROADCAST);
//                }
//            }
//            currentVersion = preferences.getInt(ADVICE_ENGINE_VERSION, 0);
//            if (currentVersion == 0 || info.engineVersionCode > currentVersion) {
//                if (info.engineVersionCode > 0) {
//                    preferences.putInt(ADVICE_ENGINE_VERSION, info.engineVersionCode);
//                    preferences.putBoolean(ADVICE_ENGINE_ENABLE, info.engineIsEnable);
//                    isEngineUpdate = true;
//                    sendUpdateBroadCast(PROPERTY_UPDATE_SEARCH_BROADCAST);
//                }
//            }
//        }
//    }
//
//    /*
//     * 发送更新通知广播
//     */
//    private void sendUpdateBroadCast(String aciton) {
//        Intent bIntent = new Intent();
//        bIntent.setAction(aciton);
//        StoreApplication.INSTANCE.sendBroadcast(bIntent);
//    }
//
//    private InputStream getVotePage(String url) {
//        InputStream is = null;
//        HttpClient client = null;
//        try {
//            String lastModifiedIn = XLSharePrefence.getInstance(StoreApplication.INSTANCE).getString(pageUrl, "null");//设置上一次的最后修改时间
//            HttpGet httpGet = new HttpGet(url);
//            httpGet.addHeader("If-Modified-Since", lastModifiedIn);
//            BasicHttpParams httpParams = new BasicHttpParams();
//            HttpConnectionParams.setConnectionTimeout(httpParams, 5 * 1000);
//            HttpConnectionParams.setSoTimeout(httpParams, 5 * 1000);
//            client = new DefaultHttpClient(httpParams);
//            HttpResponse resp = client.execute(httpGet);
//            if (HttpStatus.SC_OK == resp.getStatusLine().getStatusCode()) {
//                HttpEntity entity = resp.getEntity();
//                Header h = resp.getFirstHeader("Last-Modified");
//                String lastModified = h.getValue();
//                XLSharePrefence.getInstance(StoreApplication.INSTANCE).putString(pageUrl, lastModified);
//                is = entity.getContent();
//            }
//            return is;
//        } catch (ClientProtocolException e) {
//            e.printStackTrace();
//        } catch (IOException e) {
//            e.printStackTrace();
//        } catch (Exception e) {
//            e.printStackTrace();
//        } catch (AssertionError e) {
//            e.printStackTrace();
//        }
//        return null;
//    }
//
//    private void praseJsonInfo(InputStream is) {
//        if (is != null) {
//            BufferedReader reader = new BufferedReader(new InputStreamReader(is));
//            String line;
//            StringBuilder sb = new StringBuilder();
//            try {
//                while ((line = reader.readLine()) != null) {
//                    sb.append(line);
//                }
//                String json = sb.toString();
//                JSONObject jobj = new JSONObject(json);
//                log.debug("praseJsonInfo " + jobj.toString());
//                info = new PageInfo();
//                info.voteVersionCode = jobj.optInt("page_vote_version");
//                info.lastUrl = jobj.optString("last_vote_page");
//                info.currentUrl = jobj.optString("current_vote_page");
//                info.engineVersionCode = jobj.optInt("advice_engine_version");
//                info.engineIsEnable = jobj.optBoolean("advice_engine_enable", true);
//                if (XLSharePrefence.getInstance(StoreApplication.INSTANCE).getBoolean(IS_FIRST_INIT_PM, true))
//                    XLSharePrefence.getInstance(StoreApplication.INSTANCE).putBoolean(IS_FIRST_INIT_PM, false);
//                XLSharePrefence.getInstance(StoreApplication.INSTANCE).putBoolean(XLSharePrefence.NEW_FUN_CLICK, false);
//            } catch (JSONException e) {
//                e.printStackTrace();
//            } catch (IOException e) {
//                e.printStackTrace();
//            } finally {
//                try {
//                    is.close();
//                } catch (IOException e) {
//                    e.printStackTrace();
//                }
//            }
//        }
//    }
//}
