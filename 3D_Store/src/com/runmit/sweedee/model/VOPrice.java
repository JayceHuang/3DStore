package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.List;
import java.util.Locale;


/**
 * @author Sven.Zhan
 * 计费系统提供的商品定价信息 VO对象
 * {
    "id": 1028,
    "productId": 89,
    "productName": "驯龙高手",
    "productType": 1,
    "countryCode": "cn",
    "channelId": 123,
    "prices": [
        {
            "price": 800,
            "realPrice": 500,
            "symbol": "￥",
            "currencyId": 1,
            "channelId": 123
        }
    ]
   }
 */
public class VOPrice implements Serializable{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	

	
	public static final int CHINA_CURRENCYID = 1;//人民币
	
	public static final int US_CURRENCYID = 2;//美元
	
	public static final int GOLD_COIN_CURRENCYID = 3;//金币
	
	public static final int SD_CURRENCYID = 4;//sd币
	
	public static final int TAIWAN_CURRENCYID = 5;//新台币
	
	public static final int VIRTUAL_CURRENCYID = 6;//虚拟币种id,水滴币
	
	public int id;

    /** 产品id*/
    public int productId;
    
    /**产品名字*/
    public String productName;

    /**产品类型*/
    public int productType;
    
    /**定价币种的国家码*/
    public String countryCode;
    
    /**产品价格列表*/
    public List<PriceInfo> prices;

    /**
     * 获取虚拟货币价格
     * @return
     */
    public PriceInfo getVirtualPriceInfo(){
    	if(prices != null && prices.size() > 0){
    		for(PriceInfo mPriceInfo : prices){
    			if(mPriceInfo.currencyId == VIRTUAL_CURRENCYID){
    				return mPriceInfo;
    			}
    		}
    	}
    	return null;
    }
    
    public PriceInfo getDefaultRealPriceInfo(){
    	Locale locale = Locale.getDefault();
    	int currencyId;
    	if(locale.getCountry().equals(Locale.TAIWAN.getCountry())){//台湾地区
    		currencyId = TAIWAN_CURRENCYID;
    	}else if(locale.getCountry().equals(Locale.CHINA.getCountry())){
    		currencyId = CHINA_CURRENCYID;
    	}else{
    		currencyId = US_CURRENCYID;
    	}
    	if(prices != null && prices.size() > 0){
    		for(PriceInfo mPriceInfo : prices){
    			if(mPriceInfo.currencyId == currencyId){
    				return mPriceInfo;
    			}
    		}
    		for(PriceInfo mPriceInfo : prices){
    			if(mPriceInfo.currencyId == TAIWAN_CURRENCYID || mPriceInfo.currencyId == CHINA_CURRENCYID || mPriceInfo.currencyId == US_CURRENCYID){
    				return mPriceInfo;
    			}
    		}
    		
    	}
		return null;
    }

    public class PriceInfo implements Serializable{

		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;

		/** 原价*/
        public String price;
        
        /** 现价 */
        public String realPrice;
        
        /** 币种符号*/
        public String symbol;

        /** 币种id*/
        public int currencyId;

        /** 渠道id*/
        public int channelId;

		@Override
		public String toString() {
			return "PriceInfo [price=" + price + ", realPrice=" + realPrice + ", symbol=" + symbol + ", currencyId=" + currencyId + ", channelId=" + channelId + "]";
		}
    }
    

	@Override
	public String toString() {
		return "VOPrice [id=" + id + ", productId=" + productId + ", productName=" + productName + ", productType=" + productType + ", countryCode=" + countryCode + 
				 ", prices=" + prices + "]";
	}
    
    
}
