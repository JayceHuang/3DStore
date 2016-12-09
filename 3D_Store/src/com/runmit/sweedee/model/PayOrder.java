package com.runmit.sweedee.model;

import java.io.Serializable;

public class PayOrder implements Serializable{
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	public static final int PAYMENT_ALIPAY=1;//支付id，阿里支付
	public static final int PAYMENT_UNIONPAY=2;//银联支付
	public static final int PAYMENT_SKYSMS=3;//指易付短信支付
	public static final int PAYMENT_WALLET=4;//钱包支付
	public static final int PAYMENT_VIRTUAL_WALLET=5;//虚拟货币支付
	public static final int PAYMENT_PAYPAL = 8;//paypal支付
	public static final int PAYMENT_WEIXIN = 9;//微信支付
	
	public int id;
	public int orderId;
	public long uid;
	public int productId;
	public int productType;
	public String createTime;
	public int status;
	public String payTime;
	public int paymentId;
	public int amount;
	public int currencyId;
	public String tn;
	public String error;
	public int channelId;
	public String alipayString;
	
	public String skymobipayString;
	
	public String paypalString;
	public String paypalToken;
	
	@Override
	public String toString() {
		return "PayOrder [id=" + id + ", orderId=" + orderId + ", uid=" + uid + ", productId=" + productId + ", productType=" + productType + ", createTime=" + createTime + ", status=" + status
				+ ", payTime=" + payTime + ", paymentId=" + paymentId + ", amount=" + amount + ", currencyId=" + currencyId + ", tn=" + tn + ", error=" + error + ", channelId=" + channelId
				+ ", alipayString=" + alipayString + "]";
	}
	
	
}
