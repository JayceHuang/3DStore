package com.runmit.sweedee.player;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;

import android.content.Context;
import android.os.Handler;
import android.os.Message;

import com.runmit.sweedee.datadao.DataBaseManager;
import com.runmit.sweedee.datadao.PlayRecord;
import com.runmit.sweedee.datadao.PlayRecordDao;
import com.runmit.sweedee.model.CmsVedioDetailInfo.SubTitleInfo;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.XL_Log;

public class OnLinePlayMode extends VideoPlayMode {

	private XL_Log log = new XL_Log(OnLinePlayMode.class);
	
	private RealPlayUrlInfo mRealPlayUrlInfo;
	
	private String poster;
	
	private int albumid;
	
	private int currentUrlIndex = 0;//当前cdn的url列表获取的url位置
	
	private PlayRecord mPlayRecord;
	
	private boolean hasSwitchPlaySource = false;//是否已经切换过片源，如果切换过并且播放成功，则记录其ip或者域名
	
	private static final String SP_PLAY_HOST = "play_host";
	
	public int getAlbumid() {
		return albumid;
	}

	public String getPoster() {
		return poster;
	}

	/**
	 * sp中默认保存的播放清晰度
	 */
	public static final String SP_DEFAULT_PLAY_RESOLUTION="default_play_resolution";

	public OnLinePlayMode(Context context,int mode,String fileName, RealPlayUrlInfo realPlayUrlInfo,ArrayList<SubTitleInfo> mSubTitleList,String picUrl,int albumid,int movieid) {
		super(context,mode, PlayerConstant.ONLINE_PLAY_MODE);
		this.mRealPlayUrlInfo=realPlayUrlInfo;
		this.mFileName=fileName;
		this.mSubTitleInfos=mSubTitleList;
		this.poster=picUrl;
		this.albumid=albumid;
		this.videoid=movieid;
		long uid = UserManager.getInstance().getUserId();
		
		mPlayRecord =PlayRecordHelper.getPlayRecord(albumid, uid);
		mPlayedTime = PlayRecordHelper.getLastPlayPosByPlayRecord(mPlayRecord);
		
		SharePrefence mXlSharePrefence=SharePrefence.getInstance(mContext);
		mCurrentPlayUrlType = mXlSharePrefence.getInt(SP_DEFAULT_PLAY_RESOLUTION, Constant.NORMAL_VOD_URL);
		if(mRealPlayUrlInfo == null || (!mRealPlayUrlInfo.has1080Redition() && !mRealPlayUrlInfo.has720Redition())){
			//throw new RuntimeException("mRealPlayUrlInfo is null");
		}
		if(mCurrentPlayUrlType == Constant.HIGH_VOD_URL && !mRealPlayUrlInfo.has1080Redition()){
			mCurrentPlayUrlType = Constant.NORMAL_VOD_URL;
		}
		if(mCurrentPlayUrlType == Constant.NORMAL_VOD_URL && !mRealPlayUrlInfo.has720Redition()){
			mCurrentPlayUrlType = Constant.HIGH_VOD_URL;
		}
	}

	@Override
	public void saveVideoInfo(long mMovieDuration) {
		if(mPlayRecord != null){
			mPlayRecord.setPlayPos(mPlayedTime);
			mPlayRecord.setCreateTime(System.currentTimeMillis());
		}else{
			long uid = UserManager.getInstance().getUserId();
			mPlayRecord =  new PlayRecord(mode,(long)getAlbumid(),uid, getVideoid(), getFileName(), poster, (Long)System.currentTimeMillis(), mPlayedTime, (int)mMovieDuration, getSubTitleInfos(),System.currentTimeMillis());
		}
		PlayRecordHelper.addOrReplace(mPlayRecord);
	}

	public interface OnGetUrlListener{
		void onGetUrlFinish(String mPlayUrl);
	}
	
	/**
	 * 加载默认播放地址
	 * 以后会根据用户习惯去处理
	 */
	public void loadPlayUrlByDefault(OnGetUrlListener mOnGetUrlListener){
		SharePrefence mXlSharePrefence=SharePrefence.getInstance(mContext);
		String iphost = mXlSharePrefence.getString(SP_PLAY_HOST, null);
		log.debug("iphost="+iphost);
		if(iphost != null){
			List<String> mList = mRealPlayUrlInfo.getP1080List();
			if(mList != null && mList.size() > 0){
				for(int i = 0,size =mList.size() ;i<size;i++ ){
					if(mList.get(i).contains(iphost)){
						String url = mList.remove(i);
						log.debug("url="+url);
						mList.add(0,url);//此種方式不用考慮currentUrlIndex循環問題
						break;
					}
				}
			}
			
			mList = mRealPlayUrlInfo.getP720List();
			if(mList != null && mList.size() > 0){
				for(int i = 0,size =mList.size() ;i<size;i++ ){
					if(mList.get(i).contains(iphost)){
						String url = mList.remove(i);
						log.debug("url="+url);
						mList.add(0,url);
						break;
					}
				}
			}
		}
		loadUrlByType(mCurrentPlayUrlType, currentUrlIndex, mOnGetUrlListener);
	}
	
	/**
	 * 获取当前清晰度下的下一个播放片源
	 * @param mOnGetUrlListener
	 */
	public void loadNextIndexUrlByCurrentResolution(OnGetUrlListener mOnGetUrlListener){
		hasSwitchPlaySource = true;
		log.debug("currentUrlIndex="+currentUrlIndex);
		loadUrlByType(mCurrentPlayUrlType,currentUrlIndex + 1, mOnGetUrlListener);
	}
	
	/**
	 * 开始播放
	 */
	public void onVideoPlay(){
		if(hasSwitchPlaySource){
			final String mCurrentUrl = mUrlReady;
			if(mUrlReady != null){
				int start = mCurrentUrl.indexOf("//");
				log.debug("start="+start);
				if(start > 0 && start < mCurrentUrl.length()){
					String fixUrl = mCurrentUrl.substring(start + 2);
					log.debug("fixUrl="+fixUrl);
					int end = fixUrl.indexOf("/") + start + 2;
					if(end > 0 && end < mCurrentUrl.length()){
						String iphost = mCurrentUrl.substring(start + 2, end);//截取的最终ip或者host
						log.debug("iphost="+iphost);
						SharePrefence mXlSharePrefence=SharePrefence.getInstance(mContext);
						mXlSharePrefence.putString(SP_PLAY_HOST, iphost);
					}
				}
			}
		}
	}
	/**
	 * 
	 * @param playType:播放清晰度选择
	 * @param urlIndex:当前清晰度下获取的url列表的index位置，切换清晰度或者首次进入时候务必是0
	 * @param mOnGetUrlListener
	 */
	public void loadUrlByType(final int playType,final int urlIndex,final OnGetUrlListener mOnGetUrlListener){
		log.debug("mRealPlayUrlInfo="+mRealPlayUrlInfo +",playType="+playType+",1080="+mRealPlayUrlInfo.has1080Redition()+",720="+mRealPlayUrlInfo.has720Redition());
		if(mRealPlayUrlInfo!=null){
			if(playType==Constant.HIGH_VOD_URL){
				if(mRealPlayUrlInfo.has1080Redition()){
					mCurrentPlayUrlType=Constant.HIGH_VOD_URL;
					loadReadPlay(mCurrentPlayUrlType,urlIndex,mRealPlayUrlInfo.getP1080List(),mOnGetUrlListener);
					return;
				}
			}else if(playType==Constant.NORMAL_VOD_URL){
				if(mRealPlayUrlInfo.has720Redition()){//720P的视频格式存在
					mCurrentPlayUrlType=Constant.NORMAL_VOD_URL;
					loadReadPlay(mCurrentPlayUrlType,urlIndex,mRealPlayUrlInfo.getP720List(),mOnGetUrlListener);
					return;
				}
			}
		}
	}
	
	/**
	 * 是否有当前清晰度的下一个cdn地址
	 * @return
	 */
	public boolean hasResourceForResolution(){
		if(mRealPlayUrlInfo!=null){
			if(mCurrentPlayUrlType==Constant.HIGH_VOD_URL){
				if(mRealPlayUrlInfo.has1080Redition()){
					final List<String> urlList = mRealPlayUrlInfo.getP1080List();
					log.debug("urlList="+urlList.size()+",currentUrlIndex="+currentUrlIndex);
					return urlList.size() > (currentUrlIndex + 1);
				}
			}else if(mCurrentPlayUrlType==Constant.NORMAL_VOD_URL){
				if(mRealPlayUrlInfo.has720Redition()){//720P的视频格式存在
					final List<String> urlList = mRealPlayUrlInfo.getP720List();
					log.debug("urlList="+urlList.size()+",currentUrlIndex="+currentUrlIndex);
					return urlList.size() > (currentUrlIndex + 1);
				}
			}
		}
		return false;
	}
	
	private void loadReadPlay(final int playType,final int urlIndex,final List<String> urlList,final OnGetUrlListener mOnGetUrlListener){
		log.debug("urlIndex="+urlIndex+",urlList="+urlList.size());
		if(urlList!=null && urlList.size()>0){
			try {
				mUrlReady=urlList.get(urlIndex);
				currentUrlIndex = urlIndex;
				mOnGetUrlListener.onGetUrlFinish(mUrlReady);
				log.debug("currentUrlIndex="+currentUrlIndex+",mUrlReady="+mUrlReady);
			} catch (IndexOutOfBoundsException e) {
				e.printStackTrace();
			}
			
		}
		if(urlIndex == 0){//此处是第一次启动
			new Thread(new Runnable() {//开启小线程循环判断地址是否可以访问
				
				@Override
				public void run() {
					HttpParams httpParameters = new BasicHttpParams();   
			        HttpConnectionParams.setConnectionTimeout(httpParameters, 5*1000);
			        HttpConnectionParams.setSoTimeout(httpParameters, 5*1000);
			        HttpClient httpclient = new DefaultHttpClient(httpParameters);
			        
					for(int i = 0 ,size = urlList.size() ;i<size;i++){
						String mUrl = null;
						try {
							mUrl = urlList.get(i);
						} catch (IndexOutOfBoundsException e) {
							e.printStackTrace();
						}
						log.debug("mUrl="+mUrl);
						if(mUrl != null){
							HttpGet httpRequst = new HttpGet(mUrl);  
				            HttpResponse httpResponse;
							try {
								httpResponse = httpclient.execute(httpRequst);
								log.debug("responsecode="+httpResponse.getStatusLine().getStatusCode());
					            if(httpResponse.getStatusLine().getStatusCode() != 200){
					            	urlList.remove(mUrl);
					            }
							} catch (IOException e) {
								e.printStackTrace();
								log.debug("IOException url="+mUrl);
								urlList.remove(mUrl);
							}
						}
					}
					for(int i = 0 ,size = urlList.size() ;i<size;i++){
						String mUrl = urlList.get(i);
						log.debug("StringmUrl="+mUrl);
					}
					
				}
			}).start();
		}
	}

	@Override
	public boolean hasPlayUrlByType(int playtype) {
		if(mRealPlayUrlInfo!=null){
			if(playtype == Constant.HIGH_VOD_URL){
				return mRealPlayUrlInfo.has1080Redition();
			}else if(playtype == Constant.NORMAL_VOD_URL){
				return mRealPlayUrlInfo.has720Redition();
			}
		}
		return false;
	}
}
