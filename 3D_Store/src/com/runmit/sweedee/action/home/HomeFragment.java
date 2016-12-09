package com.runmit.sweedee.action.home;

import android.os.Bundle;
import android.view.View;
import android.widget.ListView;

import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.manager.InitDataLoader;
import com.runmit.sweedee.manager.InitDataLoader.LoadEvent;

public class HomeFragment extends HeaderFragment implements StoreApplication.OnNetWorkChangeListener{
	
	private boolean isAlreadyViewCreated = false;
	
	public ListView getCurrentListView(){
		return null/*mMainListView*/;
	}
	 
	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {
		super.onViewCreated(view, savedInstanceState);
		
		if(!isAlreadyViewCreated){
			isAlreadyViewCreated = true;
			InitDataLoader.getInstance().registerEvent(LoadEvent.LoadHomeData, this);
			StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
		}
	}

	@Override
	public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
		if(isNetWorkAviliable && isDataEmpty()){
			InitDataLoader.getInstance().initHomeFakeData();
		}
	}
}
