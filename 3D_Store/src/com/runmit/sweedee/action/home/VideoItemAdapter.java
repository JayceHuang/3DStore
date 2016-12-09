package com.runmit.sweedee.action.home;

import android.content.Context;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.AbsoluteSizeSpan;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.FadeInBitmapDisplayer;
import com.runmit.sweedee.R;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.model.CmsModuleInfo.CmsComponent;
import com.runmit.sweedee.model.CmsModuleInfo.CmsItem;

public class VideoItemAdapter extends ComponentAdapter {
	
    private DisplayImageOptions mOptions;

    private ImageLoader mImageLoader = ImageLoader.getInstance();
    
    public VideoItemAdapter(Context context, CmsComponent ccomponent) {
        super(context, ccomponent);
        mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.defalut_movie_small_item).setDisplayer(new FadeInBitmapDisplayer(300)).build();
    }

    public int getCount() {
        return items.size();
    }

    public View getView(final int position, View convertView, ViewGroup parent) {
        ViewHolder viewHolder = null;
        if (convertView == null) {
            convertView = mInflater.inflate(R.layout.item_homemovie_gridview, null);
            viewHolder = new ViewHolder();
            viewHolder.item_logo = (ImageView) convertView.findViewById(R.id.iv_item_logo);
            viewHolder.item_grade = (TextView) convertView.findViewById(R.id.tv_commend_grade);
            viewHolder.item_name = (TextView) convertView.findViewById(R.id.tv_item_name);
            viewHolder.item_highlight = (TextView) convertView.findViewById(R.id.tv_highlight);
            convertView.setTag(viewHolder);
        } else {
            viewHolder = (ViewHolder) convertView.getTag();
        }
        CmsItem ci = items.get(position);
        if (ci != null && ci.item != null) {
            viewHolder.item_name.setText(ci.item.getTitle());
            if(ci.item.getScore().length() >= 3){
            	Spannable WordtoSpan = new SpannableString(ci.item.getScore());
     			WordtoSpan.setSpan(new AbsoluteSizeSpan(14, true), 0, 1, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
     			WordtoSpan.setSpan(new AbsoluteSizeSpan(10, true), 1, 3, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                viewHolder.item_grade.setText(WordtoSpan);
            }
            
            String hlight = ci.item.getHighlight() ;
            if(hlight != null){
            	viewHolder.item_highlight.setText(hlight);	
            }                
            String imgUrl = TDImageWrap.getHomeVideoPoster(ci.item);
            mImageLoader.displayImage(imgUrl, viewHolder.item_logo, mOptions);
        }
        return convertView;
    }

    class ViewHolder {
        public ImageView item_logo;
        public TextView item_grade;
        public TextView item_name;
        public TextView item_highlight;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

}
