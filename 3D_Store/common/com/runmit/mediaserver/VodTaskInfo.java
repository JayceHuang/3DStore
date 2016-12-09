package com.runmit.mediaserver;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.R.integer;
import android.util.Log;

public class VodTaskInfo {
	
	private long fileSize;

	private int speed;
	private int cdn_speed;
	private int p2p_speed;

	private long downloadBytes;
	private long cdn_download_bytes;
	private long p2p_download_bytes;
	
	private int peer_res_num;
	private int connect_peer_res_num;
	private int can_download_peer_res_num;
	private int failed_peer_res_num;
	
	private int total_punch_hole_peer_res_num;
	private int total_punch_hole_success_connect_peer_res_num;
	private int total_punch_hole_icallsomeone_timeout_num;
	private int total_punch_hole_udt_connect_timeout_num;
	private int total_udp_broker_peer_res_num;
	private int total_udp_broker_success_connect_peer_res_num;
	private int total_udp_broker_udt_connect_timeout_num;
	private int total_udp_broker_req_cmd_timout_num;
	private int total_udt_direct_peer_res_num;
	private int total_udt_direct_success_connect_peer_res_num;
	private int total_tcp_broker_peer_res_num;
	private int total_tcp_broker_success_connect_peer_res_num;
	private int total_tcp_broker_req_cmd_timeout_num;
	private int total_tcp_direct_peer_res_num;
	private int total_tcp_direct_success_connect_peer_res_num;
	private int total_getpeersn_cmd_timeout_num;
	private int total_getpeersn_cmd_cancle_num;
	
	public VodTaskInfo() {
		
	}
	
	public boolean setJsonToObject(String resultJson, VodTaskInfo vodTaskInfo) {
		if (resultJson == null || !resultJson.contains("ret"))
			return false;
		
		JSONObject jobj = null;
		try {
			jobj = new JSONObject(resultJson);

		if (jobj.getInt("ret") == 0 ) {
			//接收成功
			JSONObject jobjResp = jobj.getJSONObject("resp");
			vodTaskInfo.fileSize = jobjResp.getLong("filesize");
			vodTaskInfo.speed = jobjResp.getInt("speed");
			vodTaskInfo.cdn_speed = jobjResp.getInt("cdn_speed");
			vodTaskInfo.p2p_speed = jobjResp.getInt("p2p_speed");
			vodTaskInfo.downloadBytes = jobjResp.getLong("downloadbytes");
			vodTaskInfo.cdn_download_bytes = jobjResp.getLong("cdn_download_bytes");
			vodTaskInfo.p2p_download_bytes = jobjResp.getLong("p2p_download_bytes");
			vodTaskInfo.peer_res_num = jobjResp.getInt("peer_res_num");
			vodTaskInfo.connect_peer_res_num = jobjResp.getInt("connect_peer_res_num");
			vodTaskInfo.can_download_peer_res_num = jobjResp.getInt("can_download_peer_res_num");
			vodTaskInfo.failed_peer_res_num = jobjResp.getInt("failed_peer_res_num");
			
			/*vodTaskInfo.total_punch_hole_peer_res_num = jobjResp.getInt("total_punch_hole_peer_res_num");
			vodTaskInfo.total_punch_hole_success_connect_peer_res_num = jobjResp.getInt("total_punch_hole_success_connect_peer_res_num");
			vodTaskInfo.total_punch_hole_icallsomeone_timeout_num = jobjResp.getInt("total_punch_hole_icallsomeone_timeout_num");
			vodTaskInfo.total_punch_hole_udt_connect_timeout_num = jobjResp.getInt("total_punch_hole_udt_connect_timeout_num");
			vodTaskInfo.total_udp_broker_peer_res_num = jobjResp.getInt("total_udp_broker_peer_res_num");
			vodTaskInfo.total_udp_broker_success_connect_peer_res_num = jobjResp.getInt("total_udp_broker_success_connect_peer_res_num");
			vodTaskInfo.total_udp_broker_udt_connect_timeout_num = jobjResp.getInt("total_udp_broker_udt_connect_timeout_num");
			vodTaskInfo.total_udp_broker_req_cmd_timout_num = jobjResp.getInt("total_udp_broker_req_cmd_timout_num");
			vodTaskInfo.total_udt_direct_peer_res_num = jobjResp.getInt("total_udt_direct_peer_res_num");
			vodTaskInfo.total_udt_direct_success_connect_peer_res_num = jobjResp.getInt("total_udt_direct_success_connect_peer_res_num");
			vodTaskInfo.total_tcp_broker_peer_res_num = jobjResp.getInt("total_tcp_broker_peer_res_num");
			vodTaskInfo.total_tcp_broker_success_connect_peer_res_num = jobjResp.getInt("total_tcp_broker_success_connect_peer_res_num");
			vodTaskInfo.total_tcp_broker_req_cmd_timeout_num = jobjResp.getInt("total_tcp_broker_req_cmd_timeout_num");
			vodTaskInfo.total_tcp_direct_peer_res_num = jobjResp.getInt("total_tcp_direct_peer_res_num");
			vodTaskInfo.total_tcp_direct_success_connect_peer_res_num = jobjResp.getInt("total_tcp_direct_success_connect_peer_res_num");
			
			vodTaskInfo.total_getpeersn_cmd_timeout_num = jobjResp.getInt("total_getpeersn_cmd_timeout_num");
			vodTaskInfo.total_getpeersn_cmd_cancle_num = jobjResp.getInt("total_getpeersn_cmd_cancle_num");*/
			return (vodTaskInfo.fileSize > 0);
		} else {
			vodTaskInfo.speed = 0;
			vodTaskInfo.cdn_speed = 0;
			vodTaskInfo.p2p_speed = 0;
		}
		
		} catch (JSONException e1) {
			e1.printStackTrace();
			vodTaskInfo.speed = 0;
			vodTaskInfo.cdn_speed = 0;
			vodTaskInfo.p2p_speed = 0;
		}
		return false;
	}
	
	public long getFileSize() {
		return fileSize;
	}
	
	public int getSpeed() {
		return speed;
	}

	public int getCdnSpeed() {
		return cdn_speed;
	}

	public int getP2pSpeed() {
		return p2p_speed;
	}

	public long getDownloadBytes() {
		return downloadBytes;
	}
	
	public long getCdnDownloadBytes() {
		return cdn_download_bytes;
	}
	
	public long getP2pDownloadBytes() {
		return p2p_download_bytes;
	}
	
	public int getPeerResNum() {
		return peer_res_num;
	}
	
	public int getConnectPeerResNum() {
		return connect_peer_res_num;
	}
	
	public int getCanDownloadPeerResNum() {
		return can_download_peer_res_num;
	}
	public int getFailPeerResNum() {
		return failed_peer_res_num;
	}
	public int getTotalPunchHoleNum() {
		return total_punch_hole_peer_res_num;
	}
	public int getTotalPunchHoleSuccessConnectNum() {
		return total_punch_hole_success_connect_peer_res_num;
	}
	public int getTotalPunchHoleIcallsomeoneTimeOutNum() {
		return total_punch_hole_icallsomeone_timeout_num;
	}
	public int getTotalPunchHoleConnectTimeoutNum() {
		return total_punch_hole_udt_connect_timeout_num;
	}
	public int getTotalUdpBrokerNum() {
		return total_udp_broker_peer_res_num;
	}
	public int getTotalUdpBrokerSuccessConnectNum() {
		return total_udp_broker_success_connect_peer_res_num;
	}
	public int getTotalUdpBrokerConnectTimeoutNum() {
		return total_udp_broker_udt_connect_timeout_num;
	}
	public int getTotalUdpBrokerReqCmdTimeOutNum() {
		return total_udp_broker_req_cmd_timout_num;
	}
	public int getUdtDirectNum() {
		return total_udt_direct_peer_res_num;
	}
	public int getUdtDirectSuccessConnectNum() {
		return total_udt_direct_success_connect_peer_res_num;
	}
	public int getTcpBrokerNum() {
		return total_tcp_broker_peer_res_num;
	}
	public int getTcpBrokerSuccessConnectNum() {
		return total_tcp_broker_success_connect_peer_res_num;
	}
	public int getTcpBrokerReqCmdTimeOutNum() {
		return total_tcp_broker_req_cmd_timeout_num;
	}
	public int getTcpDirectNum() {
		return total_tcp_direct_peer_res_num;
	}
	public int getTcpDirectSuccessConnectNum() {
		return total_tcp_direct_success_connect_peer_res_num;
	}
	
	public int getGetPeersnCmdTimeOutNum() {
		return total_getpeersn_cmd_timeout_num;
	}
	public int getGetPeersnCmdCancleNum() {
		return total_getpeersn_cmd_cancle_num;
	}
	
	public int getCheckSuccTimes() {
		return 0;
	}

	public int getCheckFailTimes() {
		return 0;
	}

	public int getCheckRangeNum() {
		return 0;
	}

	public int getRedispatchRangeNum() {
		return 0;
	}

	public int getMissCheckRangeNum() {
		return 0;
	}

	public int getUnableCheckRangeNum() {
		return 0;
	}

}
