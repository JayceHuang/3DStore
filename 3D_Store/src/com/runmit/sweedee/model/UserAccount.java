package com.runmit.sweedee.model;

import java.io.Serializable;

/** 
 * 获取用户账户信息
 * 定义:查询用户账户金额
 * GET:http://pay.runmit-dev.com/pay/userAccount/get?uid=16&ct=cn&accountType=1
 * accountType:账户类型,1货币账户，2虚拟账户
 * ResponseBody：
 * <pre>
//  {
//       "rtn": 0,
//       "errMsg": "succeed",
//       "data":
//       [
//           {
//               "id": 12,
//               "uid": 16,
//               "accountType": 2,
//               "amount": 0,
//               "currencyId": 3,
//               "createTime": 1413277116000,
//               "status": 1
//           }
//       ]
//    }
//
//       id:账户记录id
//       uid: 用户id
//       amount: 金额
//       currencyId: 币种
//       accountType:账户类型
//       status: 状态：1启用，2禁用
//       createTime: 创建时间
</pre>*/
public class UserAccount implements Serializable{
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	public int id;
	public int uid;
	public int accountType;
	public float amount;
	public int currencyId;
	public long createTime;
	public int status;
	public String symbol;
	
	
	@Override
	public String toString() {
		return "UserAccount [id=" + id + ", uid=" + uid + ", accountType=" + accountType + ", amount=" + amount + ", currencyId=" + currencyId + ", createTime=" + createTime + ", status=" + status
				+ ", symbol=" + symbol + "]";
	}
	
}
