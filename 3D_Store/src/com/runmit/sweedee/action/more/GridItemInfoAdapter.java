package com.runmit.sweedee.action.more;

import java.util.ArrayList;
import java.util.List;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.manager.GetSysAppInstallListManager.AppInfo;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;
import android.content.Context;
import android.graphics.drawable.Drawable;

public class GridItemInfoAdapter extends BaseAdapter {

	private LayoutInflater inflater;
	List<AppInfo> mAppInfoList;

	public GridItemInfoAdapter(List<AppInfo> appInfoList) {
		super();
		this.inflater = LayoutInflater.from(StoreApplication.INSTANCE);
		mAppInfoList=appInfoList;

	}
	
	@Override
	public int getCount() {
		// TODO Auto-generated method stub
		return this.mAppInfoList.size();
	}

	@Override
	public AppInfo getItem(int position) {
		// TODO Auto-generated method stub
		return mAppInfoList.get(position);
	}

	@Override
	public long getItemId(int position) {
		// TODO Auto-generated method stub
		return position;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		// TODO Auto-generated method stub
		ItemView holder;
		if (convertView == null) {
			convertView = this.inflater.inflate(R.layout.grid_item_view, null);
			holder = new ItemView();
			holder.name_lable = (TextView) convertView.findViewById(R.id.item_name);
			holder.bg_image = (ImageView) convertView.findViewById(R.id.item_bgImg);
			convertView.setTag(holder);
		}else {
			holder = (ItemView) convertView.getTag();
		}
		//....获取列表数据并设置
	    holder.bg_image.setImageDrawable(mAppInfoList.get(position).appicon);
		holder.name_lable.setText(mAppInfoList.get(position).appName);
	
		return convertView;
	}
	
    class ItemView {
        TextView name_lable;
        ImageView bg_image;
    }

}
