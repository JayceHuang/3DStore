package com.runmit.sweedee.manager.data;

/**
 * @author Sven.Zhan
 * 各业务服务器对应的接口事件代码
 * 由于上报系统会上报每个接口的错误码，而其上报事件标示定义为moduleId*100+actionId;
 * 如果我们每次定义moduleId和actionId，则会出现大量代码来解析actionid
 * 所以此处我们将msg_event和上报的event统一起来。
 * 则我们在代码的上报处可以直接调用msg.what代表错误码上报的事件。
 * 其中模块id如下：
 * public static final int MODULE_CMS = 1;
 * public static final int MODULE_UC = 2;
 * public static final int MODULE_ACCOUNT = 3;	
 */
public class EventCode {
    
    /**
     * 计费系统
     */
    public static class AccountEventCode {

        /**
         * 获取商品定价信息
         */
        public static final int EVENT_GET_PRICE = 301;
        
        /**
         * 获取影片支付方式回调msg.what
         */
        public static final int EVENT_GET_MOVIE_PALY_STYLE = 302;
        
        /**
         * 请求生成订单
         */
        public static final int EVENT_GET_MOVIE_PALY_ORDER = 303;
        
        /**
         * 获取购买权限，即便是否已经购买
         */
        public static final int EVENT_GET_PERMISSION = 304;
        
        /**
         * 获取用户钱包账户信息
         */
        public static final int EVENT_GET_USER_ACCOUNT_MESSAGE = 305;

        /**
         * 获取用户购买记录
         */
        public static final int EVENT_GET_USER_PURCHASES = 306;
        
        /**
         * 获取用户充值记录
         */
        public static final int EVENT_GET_RECHARGE_RECORD = 307;
        
        /**
         * 获取可用充值额度
         */
        public static final int EVENT_GET_RECHARGE_QUOTA = 308;
        
        /**
         * 上报支付系统支付成功通知
         */
        public static final int EVENT_REPORT_PAY_SUCCESS = 309;
        
        /**
         * 请求支付系统查询paypal返回结果
         */
        public static final int EVENT_PAYPAL_CHECKOUT = 310;//paypal付款
    }

    /**
     * CMS
     */
    public static class CmsEventCode {
		
    	/**
    	 * 拉取首页module信息接口
    	 */
        public static final int EVENT_GET_HOME = 101;
        
        /**
         * 拉取视频分类信息，已经废弃此接口，1.0版本内容
         */
        public static final int EVENT_GET_FILTRATE_CONFIG = 102;
        
        /**
         * 拉取分类筛选详情，已经废弃此接口，1.0版本内容
         */
        public static final int EVENT_GET_FILTRATE = 103;

        /**
         * 拉取影片详情04
         */
        public static final int EVENT_GET_DETAIL = 104;

        /**
         * 拉取相关推荐05
         */
        public static final int EVENT_GET_RECOMMEND = 105;

        /**
         * 拉取热搜词06
         */
        public static final int EVENT_GET_HOT_SEARCHKEY = 106;

        /**
         * 影片搜索07
         */
        public static final int EVENT_GET_SEARCH = 107;
        
        /**
         * 拉取全局配置08
         */
        public static final int EVENT_GET_GLOBAL_CONFIG = 108;
        
        /**
         * 获取GSLB调度地址09
         */
        public static final int EVENT_GET_VEDIO_GSLB_URL = 109;
        
		/**
		 * 拉取影片评论10	
		 */
        public static final int EVENT_GET_MOVIE_COMMENTS = 110;
        
        public static final int EVENT_GET_ASSOCIATE_SEARCHKEY = 111;//(第一期不做)

        public static final int EVENT_GET_VEDIO_CDN_URL = 112;
        
        public static final int EVENT_GET_APP_LIST = 113;
        
        public static final int EVENT_GET_APP_GSLB_URL = 114;
        
        public static final int EVENT_GET_APP_CDN_URL = 115;
        
        public static final int EVENT_GET_ALBUMS = 116;
        
        public static final int EVENT_GET_APP_UPDATE_LIST = 117;

    }
}
