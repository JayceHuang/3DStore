package com.runmit.sweedee.util;

public class Constant {

    // 产品ID product ID
    public static int PRODUCT_ID = 1; // 1.android 2.iphone 3.androidpad

    /**
     * 项目id，包含不同端产品，比如sweedee android和ios端共用一个id，和上面的PRODUCT_ID不同，这个是粗粒度的
     */
    public static int superProjectId = 1;
    
    public static final int MANUAL = 1;
   
    //播放清晰度
    public static final int ORIN_VOD_URL = 0;    //原始,针对本地播放
    public static final int NORMAL_VOD_URL = 1; //标清720P
    public static final int HIGH_VOD_URL = 2; //高清1080P
    
    //缓存字幕类型 中/英
    public static final String DOWNLOAD_LAN_CH = "zh"; //中文
    public static final String DOWNLOAD_LAN_EN = "en"; //英文
    
    //音轨优先类型 
    public static final String AUDIO_TRACK_SETTING = "audio_track";//播放音轨选择
    public static final int AUDIO_FIRST_ORIN = 0; //优先原生音轨
    public static final int AUDIO_FIRST_LOCAL = 1; //优先系统设置语言音轨
    
    public static final String GAME_SETTING = "game_setting";//游戏设置
    
    //网络类型相关
    public static final String NETWORK_NONE = "none";
    public static final String NETWORK_WIFI = "wifi";
    public static final String NETWORK_2G_WAP = "2g_wap";
    public static final String NETWORK_2G_NET = "2g_net";
    public static final String NETWORK_3G = "3g";
    public static final String NETWORK_4G = "4g";
    public static final String NETWORK_OTHER = "other";
    public static final int NETWORK_CLASS_UNKNOWN = 0;
    public static final int NETWORK_CLASS_2G = 1;
    public static final int NETWORK_CLASS_3G = 2;
    public static final int NETWORK_CLASS_4G = 3;

    //屏幕长宽的全局变量
    public static int SCREEN_HEIGHT = 0;
    public static int SCREEN_WIDTH = 0;
    public static float DENSITY = 1.0f;

    //同时缓存数量
    public static final String DOWNLOAD_NUM = "downloadNum";

    //视频后缀名集合
    public static String[] VEDIO_SUFFIX = new String[]
            {"mpeg", "mpg", "mpe", "dat", "m1v", "m2v", "m2p", "m2ts", "mp2v", "mpeg1", "mpeg2", "tp", "ts", "avi", "asf", "wmv",
                    "wm", "wmp", "3gp", "3g2", "3gp2", "3gpp", "flv", "f4v", "swf", "hlv", "rmvb", "rm", "m4b", "m4p", "m4v", "mpeg4", "mp4", "amv",
                    "divx", "xv", "mkv", "xlmv", "pmp", "ogm", "ogv", "ogx", "mts", "mov", "voB", "dvd"};

    // 友盟事件名称
    public static final String PlayClick = "clickPlay";                 // 点击播放事件
    public static final String PurchaseClick = "purchaseClick";         // 点击购买事件
    public static final String MonPurchaseClick = "cashPurchaseClick";  // 现金支付
}
