package com.runmit.sweedee.action.home;

import java.util.ArrayList;

import android.content.Context;
import android.view.LayoutInflater;
import android.widget.BaseAdapter;

import com.google.gson.Gson;
import com.runmit.sweedee.model.CmsItemable;
import com.runmit.sweedee.model.CmsModuleInfo.CmsComponent;
import com.runmit.sweedee.model.CmsModuleInfo.CmsItem;

public abstract class ComponentAdapter extends BaseAdapter {

    protected LayoutInflater mInflater;

    protected CmsComponent component;

    protected ArrayList<CmsItem> items;

    protected Gson mGosn;

    private Context mContext;

    public ComponentAdapter(Context context, CmsComponent ccomponent) {
        this.component = ccomponent;
        this.mContext = context;
        this.items = ccomponent.contents;
        mInflater = LayoutInflater.from(mContext);
        mGosn = new Gson();
    }

    public CmsItemable getItem(int position) {
        return items.get(position).item;
    }
}
