/**
 * 
 */
package com.runmit.sweedee.player;

import java.util.List;

import android.util.Log;

import com.runmit.sweedee.datadao.DataBaseManager;
import com.runmit.sweedee.datadao.PlayRecord;
import com.runmit.sweedee.datadao.PlayRecordDao;
import com.runmit.sweedee.datadao.PlayRecordDao.Properties;

/**
 * @author Sven.Zhan
 * 提供操作播放记录的一些封装方法
 */
public class PlayRecordHelper {	
	
	public static final int IGNORE_LEFT_TIME = 20;
	
	private static final long PLAY_OVERDUE_TIME = 30*24*60*60*1000L;
	
	/**查询某用户的播放记录*/
	public static List<PlayRecord> loadRecords(long uid){
		PlayRecordDao playRecordDao = DataBaseManager.getInstance().getPlayRecordDao();
		List<PlayRecord> list = playRecordDao.queryBuilder().where(Properties.Uid.eq(uid)).orderDesc(Properties.CreateTime).list();
		return list;
	}
	
	/**删除播放记录*/
	public static void deleteRecords(List<PlayRecord> list){
		PlayRecordDao playRecordDao = DataBaseManager.getInstance().getPlayRecordDao();
		playRecordDao.deleteInTx(list);
	}
	
	/**添加或替换播放记录*/
	public static void addOrReplace(PlayRecord record){
		DataBaseManager.getInstance().getPlayRecordDao().insertOrReplace(record);
	}
	
	public static PlayRecord getPlayRecord(long albumid, long uid){
		long primaryKey = PlayRecord.getHashKey(albumid, uid);
		PlayRecordDao playRecordDao = DataBaseManager.getInstance().getPlayRecordDao();
		PlayRecord pr = playRecordDao.load(primaryKey);
		return pr;
	}
	
	/**获取单个播放记录*/
	public static int getLastPlayPos(long albumid, long uid){
		int lastPlayPost = 0;
		long primaryKey = PlayRecord.getHashKey(albumid, uid);
		PlayRecordDao playRecordDao = DataBaseManager.getInstance().getPlayRecordDao();
		PlayRecord pr = playRecordDao.load(primaryKey);
		if(isBuyOverDue(pr)){
			lastPlayPost = 0;
		}else if(pr != null){
			int leftTime = pr.getDuration() - pr.getPlayPos();
			lastPlayPost = (leftTime > IGNORE_LEFT_TIME) ? pr.getPlayPos() : 0;
		}
		return lastPlayPost;
	}
	
	private static boolean isBuyOverDue(PlayRecord mPlayRecord){
		long duration = System.currentTimeMillis() - mPlayRecord.getOrinCreateTime();
		return duration >= PLAY_OVERDUE_TIME;
	}
	
	
	/**获取单个播放记录*/
	public static int getLastPlayPosByPlayRecord(PlayRecord mPlayRecord){
		if(mPlayRecord == null){
			return 0;
		}
		int lastPlayPost = 0;
		if(isBuyOverDue(mPlayRecord)){
			lastPlayPost = 0;
		}else if(mPlayRecord != null){
			int leftTime = mPlayRecord.getDuration() - mPlayRecord.getPlayPos();
			lastPlayPost = (leftTime > IGNORE_LEFT_TIME) ? mPlayRecord.getPlayPos() : 0;
		}
		return lastPlayPost;
	}
}
