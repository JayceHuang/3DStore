package com.runmit.sweedee.action.game;

import java.util.ArrayList;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.RoundedBitmapDisplayer;
import com.runmit.sweedee.R;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.model.CmsItemable;
import com.runmit.sweedee.model.CmsModuleInfo;
import com.runmit.sweedee.util.Util;

public class GameGridAdapter extends BaseAdapter{
    
	private ArrayList<CmsModuleInfo.CmsItem> items;
	
	private DisplayImageOptions mOptions;
    
	private GridType mGridType; //用于判断是一行3列还是4列
	
	private LayoutInflater mInflater;
	
	private ImageLoader mImageLoader;
	
	private Context mContext;

	//ONE是一行3列  two是一行4列
    public enum GridType {
        ONE, TWO
    }
    
    public GameGridAdapter(Context context,CmsModuleInfo.CmsComponent ccomponent, GridType gt) {
    	mInflater = LayoutInflater.from(context);
    	mImageLoader = ImageLoader.getInstance();
        this.items = ccomponent.contents;
        this.mContext = context;
        mGridType = gt;
        float density = context.getResources().getDisplayMetrics().density;
        if (mGridType.equals(GridType.ONE)) {
            mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_game_horizontal).setDisplayer(new RoundedBitmapDisplayer((int) (3*density))).build();
        } else {
            mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.black_game_detail_icon).setDisplayer(new RoundedBitmapDisplayer((int) (8*density))).build();
        }
    }

    public int getCount() {
    	return items.size();
    }

    @Override
    public CmsItemable getItem(int position) {
        return items.get(position).item;
    }

    public View getView(final int position, View convertView, ViewGroup parent) {
        ViewHolder holder;
        if (convertView == null) {
            if (mGridType.equals(GridType.ONE)) {
                convertView = mInflater.inflate(R.layout.item_hotgame_gridview_one, parent,false);
            } else {
                convertView = mInflater.inflate(R.layout.item_hotgame_gridview_two, parent,false);
            }
            holder = new ViewHolder();
            holder.item_logo = (ImageView) convertView.findViewById(R.id.iv_newgame_item_logo);
            holder.item_name = (TextView) convertView.findViewById(R.id.tv_newgame_item_name);
            holder.hotgame_install_count = (TextView) convertView.findViewById(R.id.tv_hotgame_install_count);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }
        CmsItemable item = getItem(position);
        if (item != null) {
            holder.item_name.setText(item.getTitle());
            holder.hotgame_install_count.setText(Util.addComma(item.getInstallCount(),mContext));
            String imgUrl = mGridType == GridType.ONE ? TDImageWrap.getAppPoster(item) : TDImageWrap.getAppIcon(item);
            mImageLoader.displayImage(imgUrl, holder.item_logo, mOptions);
        }
        return convertView;
    }

    class ViewHolder {
        public ImageView item_logo;
        public TextView item_name;
        public TextView hotgame_install_count;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }
}
