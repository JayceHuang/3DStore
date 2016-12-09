package com.runmit.sweedee.action.space;

import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.nhaarman.listviewanimations.AnimDeleteAdapter;
import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.runmit.sweedee.R;
import com.runmit.sweedee.download.VideoDownloadInfo;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;

import java.util.List;

public class VideoCacheListAdapter extends AnimDeleteAdapter {
    private XL_Log log = new XL_Log(VideoCacheListAdapter.class);
    private LayoutInflater inflater;
    
    private List<VideoDownloadInfo> data;
    private Context mContext;
    private ImageLoader mImageLoader;
    private View headView;
    private boolean isMultiSelectMode;
    
    private DisplayImageOptions mOptions;

    public VideoCacheListAdapter(Context context, List<VideoDownloadInfo> data) {
        this.inflater = LayoutInflater.from(context);
        this.data = data;
        mImageLoader = ImageLoader.getInstance();
        mContext = context;
        headView = inflater.inflate(R.layout.hotplay_head_view, null);
        headView.setClickable(false);
        headView.setVisibility(View.GONE);
        mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.defalut_movie_small_item).build();
    }

    @Override
    public int getCount() {
        return data.size();
    }

    @Override
    public VideoDownloadInfo getItem(int position) {
        return data.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public int getViewTypeCount() {
        return 2;
    }


	@Override
	protected View initConverView(int position, View convertView, ViewGroup parent) {
        MyFileLocalHolder myFileLocalHolder = null;
        if (convertView == null) {
            convertView = inflater.inflate(R.layout.item_video_cache, null);
            myFileLocalHolder = new MyFileLocalHolder();
            myFileLocalHolder.filename = (TextView) convertView.findViewById(R.id.download_file_name);
            myFileLocalHolder.fileSize = (TextView) convertView.findViewById(R.id.download_file_size);
            myFileLocalHolder.poster = (ImageView) convertView.findViewById(R.id.bt_icon);
            myFileLocalHolder.checkBoxSelect = (ImageView) convertView.findViewById(R.id.checkBoxSelect);
            myFileLocalHolder.line_start = convertView.findViewById(R.id.lineStart);
            myFileLocalHolder.line = convertView.findViewById(R.id.lineMid);
            myFileLocalHolder.line_end = convertView.findViewById(R.id.lineEnd);
            convertView.setTag(myFileLocalHolder);
        } else {
            myFileLocalHolder = (MyFileLocalHolder) convertView.getTag();
        }
        convertView.setVisibility(View.VISIBLE);
        return convertView;
	}

	@Override
	protected void showHolder(int position, AninViewHolder holder) {
		MyFileLocalHolder myFileLocalHolder = (MyFileLocalHolder) holder;		
        final VideoDownloadInfo temp = (VideoDownloadInfo) getItem(position);
        if (isMultiSelectMode && temp.isSelected) {
            myFileLocalHolder.checkBoxSelect.setVisibility(View.VISIBLE);
        } else {
            myFileLocalHolder.checkBoxSelect.setVisibility(View.INVISIBLE);
        }
        holder.isSelected = temp.isSelected;
        String poster = temp.poster;
        if (TextUtils.isEmpty(poster)) {
            myFileLocalHolder.poster.setImageResource(R.drawable.defalut_movie_small_item);//默认图
        } else {
        	//设置图片
        	String imgUrl = poster;
        	mImageLoader.displayImage(imgUrl, myFileLocalHolder.poster, mOptions);
        }

        myFileLocalHolder.filename.setText(temp.mTitle);

        String totalSize = Util.convertFileSize(temp.mFileSize, 1, true);
        myFileLocalHolder.fileSize.setText(totalSize);
        // 线条
        myFileLocalHolder.line_start.setVisibility(position == 0 ? View.VISIBLE : View.INVISIBLE);
        myFileLocalHolder.line.setVisibility(position == getCount() - 1 ? View.INVISIBLE : View.VISIBLE);
        myFileLocalHolder.line_end.setVisibility(position == getCount() - 1 ? View.VISIBLE : View.INVISIBLE);
	}
    

    class MyFileLocalHolder extends AninViewHolder {
    	public View line_end;
		public View line_start;
		public View line;
		public TextView filename;
        public TextView fileSize;
        public ImageView poster;
        public ImageView checkBoxSelect;
    }

    public boolean isMultiSelectMode() {
        return isMultiSelectMode;
    }

    public void setMultiSelectMode(boolean isMultiSelectMode) {
        this.isMultiSelectMode = isMultiSelectMode;
    }

    public static interface OnStateIconClickListener {
        public void onIconClick(int position);
    }
}
