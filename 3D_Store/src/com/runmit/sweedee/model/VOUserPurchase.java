/**
 *
 */
package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.ArrayList;

/**
 * @author Sven.Zhan
 *         计费系统提供的用户购买记录 VO对象
 */
public class VOUserPurchase implements Serializable{
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	public static final int STATUS_NOT_PAY = 1;
	public static final int STATUS_SUCCEED = 2;
	public static final int STATUS_FAILED = 3;

    public int curPage;

    public int curCount;

    public int totalPage;

    public int totalCount;

    public int countOfPage;

    public int from;

    public int to;

    public int recordsTotal;

    public boolean hasNext;

    public boolean hasPrevious;

    /**
     * 订单记录列表
     */
    public ArrayList<PurchaseRecord> data;

    /**
     * 订单记录类
     */
    public static class PurchaseRecord implements Serializable,Comparable<PurchaseRecord>{

        /**
         * 购买记录id
         */
        public int id;

        /**
         * 订单id
         */
        public long orderId;

        /**
         * 用户id
         */
        public long uid;

        /**
         * 产品id
         */
        public int productId;

        /**
         * 产品类型
         */
        public int productType;

        /**
         * 产品名称
         */
        public String productName;

        /**
         * 支付方式
         */
        public int paymentId;
        
        /**
         * 支付方式的名称
         */
        public String paymentName;

        /**
         * 状态标志:1,未付款；2,交易成功；3，交易失败
         */
        public int status;

        /**
         * 币种id
         */
        public int currencyId;
        
        /**
         * 币种符号
         */
        public String symbol;

        /**
         * 创建时间
         */
        public String createTime;

        /**
         * 支付时间
         */
        public String payTime;

        /**
         * 金额
         */
        public String amount;

        /**
         * 第三方支付流水号，目前用于银联
         */
        public String tn;

        /**
         * 错误信息
         */
        public String error;

        /**
         * 渠道id
         */
        public int channelId;

        /**
         * 支付宝支付请求字符串
         */
        public int alipayString;

		@Override
		public int compareTo(PurchaseRecord another) {
			try{
				long tPaytime = Long.parseLong(payTime);
				long aPaytime = Long.parseLong(another.payTime);
				return (int) ( aPaytime - tPaytime);
			}catch(Exception e){}
			return 0;
		}

		@Override
		public String toString() {
			return "PurchaseRecord [id=" + id + ", orderId=" + orderId + ", uid=" + uid + ", productId=" + productId + ", productType=" + productType + ", productName=" + productName + ", paymentId="
					+ paymentId + ", paymentName=" + paymentName + ", status=" + status + ", currencyId=" + currencyId + ", symbol=" + symbol + ", createTime=" + createTime + ", payTime=" + payTime
					+ ", amount=" + amount + ", tn=" + tn + ", error=" + error + ", channelId=" + channelId + ", alipayString=" + alipayString + "]";
		}
		
		
    }
}
