package com.runmit.sweedee.model;

import java.util.List;

/* 
// 获取充值记录
//定义:
//GET:http://pay.runmit-dev.com/pay/userCharge/get?uid=12&ct=cn&page=1&count=3
//
//uid:用户id
//ct:国家代码
//status:（可选参数）状态，默认2。 0，全部记录；1，充值失败记录；2，充值成功记录。
//page:（可选参数）页码，默认1
//count:（可选参数）条数，默认10
//
//ResponseBody：
//<pre>
//
//    {
//       "rtn": 0,
//       "errMsg": "succeed",
//       "data":
//       {
//           "curPage": 1,
//           "curCount": 3,
//           "totalPage": 19,
//           "totalCount": 56,
//           "countOfPage": 3,
//           "from": 1,
//           "to": 3,
//           "hasNext": true,
//           "hasPrevious": false,
//           "data":
//           [
//                {
//               "id": 100,
//               "uid": 12,
//               "createTime": 1410505381,
//               "status": 1,
//               "chargeTime": 0,
//               "paymentId": 3,
//               "amount": 100,
//               "currencyId": 1,
//               "channelId": 123,
//               "symbol": "￥",
//               "paymentName": "短信支付"
//               },
//{
//               "id": 100,
//               "uid": 12,
//               "createTime": 1410505381,
//               "status": 1,
//               "chargeTime": 0,
//               "paymentId": 3,
//               "amount": 100,
//               "currencyId": 1,
//               "channelId": 123,
//               "symbol": "￥",
//               "paymentName": "短信支付"
//               },
//{
//               "id": 100,
//               "uid": 12,
//               "createTime": 1410505381,
//               "status": 1,
//               "chargeTime": 0,
//               "paymentId": 3,
//               "amount": 100,
//               "currencyId": 1,
//               "channelId": 123,
//               "symbol": "￥",
//               "paymentName": "短信支付"
//               }
//           
//           ]
//       }
//    }
//    
//</pre>
//    id:充值记录id
//    orderId:订单id
//    paymentId：充值方式
//    createTime：创建时间
//    status:状态标志:1,未付款；2,交易成功；3，交易失败
//    chargeTime:充值时间
//    paymentId：充值方式
//    amount：金额
//    currencyId：币种id
//    channelId：渠道id
*/
public class RechargeHistory {
	public int curPage;
	public int curCount;
	public int totalPage;
	public int totalCount;
	public int countOfPage;
	public int from;
	public int to;
	public boolean hasNext;
	public boolean hasPrevious;
	
	public List<Recharge> data;
	
	public static class Recharge{
		public int id;
        public int uid;
        public long createTime;
        public int status;
        public long chargeTime;
        public int paymentId;
        public int amount;
        public int currencyId;
        public int channelId;
        public String symbol;
        public String paymentName;
        
		@Override
		public String toString() {
			return "Recharge [id=" + id + ", uid=" + uid + ", createTime=" + createTime + ", status=" + status + ", chargeTime=" + chargeTime + ", paymentId=" + paymentId + ", amount=" + amount
					+ ", currencyId=" + currencyId + ", channelId=" + channelId + ", symbol=" + symbol + ", paymentName=" + paymentName + "]";
		}
        
        
	}
}
