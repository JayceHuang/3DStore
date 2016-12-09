package com.runmit.sweedee.action.game;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.widget.AdapterView;

import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.action.home.HeaderFragment;
import com.runmit.sweedee.action.home.HomeRecycleAdapter;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.data.EventCode.CmsEventCode;
import com.runmit.sweedee.model.AppItemInfo;
import com.runmit.sweedee.model.CmsModuleInfo;
import com.runmit.sweedee.model.CmsModuleInfo.CmsComponent;
import com.runmit.sweedee.model.CmsModuleInfo.CmsItem;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.EmptyView;

/**
 * 游戏精选界面
 */
public class SiftGameFragment extends HeaderFragment implements AdapterView.OnItemClickListener, StoreApplication.OnNetWorkChangeListener {

	private XL_Log log = new XL_Log(SiftGameFragment.class);

	private Handler mhandler = new Handler() {
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case CmsEventCode.EVENT_GET_HOME:
				log.debug("CmsEventCode.EVENT_GET_HOME ret = " + msg.arg1);
				int stateCode = msg.arg1;
				log.debug(" mCmsGameDataInfo msg.obj = " + msg.obj);
				if (msg.obj != null && stateCode == 0) {
					CmsModuleInfo mCmsGameDataInfo = (CmsModuleInfo) msg.obj;
					initView(mCmsGameDataInfo);
					mEmptyView.setStatus(EmptyView.Status.Gone);
				} else {
					mEmptyView.setStatus(EmptyView.Status.Empty);
				}
				break;
			}
		}
	};

	private boolean isLoadData = false;

	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {
		super.onViewCreated(view, savedInstanceState);
		if (!isLoadData) {
			isLoadData = true;
			initCmsData();
			StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
		}
	}

	private void initCmsData() {
		CmsServiceManager.getInstance().getModuleData(CmsServiceManager.MODULE_GAME, mhandler);
	}

	private void initView(CmsModuleInfo cmsModuleInfo) {
		if (cmsModuleInfo == null || cmsModuleInfo.data == null || cmsModuleInfo.data.isEmpty()) {
			return;// 数据为空
		}
		CmsModuleInfo.CmsComponent mTopComponet = cmsModuleInfo.data.get(0);
		mGuideGallery.setDataSource(mTopComponet);
		int length = cmsModuleInfo.data.size();
		if (length > 1) {
			List<CmsModuleInfo.CmsComponent> mListComponents = new ArrayList<CmsModuleInfo.CmsComponent>();
			mListComponents.addAll(cmsModuleInfo.data.subList(1, length));
			mainRecyclerView.setAdapter(new GameRecycleAdapter(mFragmentActivity, mListComponents,mFakeHeader));
		}
	}

	private class GameRecycleAdapter extends HomeRecycleAdapter{

		public GameRecycleAdapter(Context context, List<CmsComponent> mList, View header) {
			super(context, mList, header);
		}
		
		protected void initPositionData(List<CmsComponent> cmsComponents) {
			// 初始化head
			mPositionType.put(mItemCount, TYPE_HEADER);
			mPositionObject.put(mItemCount, null);
			mItemCount = 1;

			for (int i = 0, size = cmsComponents.size(); i < size; i++) {
				ArrayList<CmsItem> contents = cmsComponents.get(i).contents;
				mPositionType.put(mItemCount, TYPE_TITLE);
				mPositionObject.put(mItemCount, cmsComponents.get(i));
				mItemCount += 1;
				for (CmsItem mCmsItem : contents) {
					if (i == 0) {
						mPositionType.put(mItemCount, TYPE_GAME);
					} else {
						mPositionType.put(mItemCount, TYPE_APP);
					}
					mPositionObject.put(mItemCount, mCmsItem);
					mItemCount += 1;
				}
			}
		}
	}
	
	boolean isChange = false;

	public void onChange(boolean isNetWorkAviliable, StoreApplication.OnNetWorkChangeListener.NetType mNetType) {
		if (isNetWorkAviliable && !isChange) {
			initCmsData();
			isChange = true;
		} else {
			isChange = false;
		}
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
		AppItemInfo item = (AppItemInfo) parent.getAdapter().getItem(position);
        AppDetailActivity.start(mFragmentActivity, view, item);
	}
}
