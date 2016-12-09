package com.runmit.sweedee.player.uicontrol;

import android.content.Context;
import android.content.res.Resources;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager.BadTokenException;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.runmit.sweedee.R;
import com.runmit.sweedee.player.VideoPlayMode;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.view.PopupWindow;

public class ResolutionView {

    private Button mPlayCurMode;
    private LayoutInflater inflater;
    private Context context;
    private VideoPlayMode mVideoMode;
    private boolean mCanShow;
    private OnChangePlayModule mModule;
    private PopupWindow popupWindow;
    
    private String[] mVideoResolutionText;

    public ResolutionView(Button curResolutionIv, Context context, VideoPlayMode videoMode) {
        this.mPlayCurMode = curResolutionIv;
        this.context = context;
        this.mVideoMode = videoMode;
        mVideoResolutionText = context.getResources().getStringArray(R.array.video_resolution);

        if (!videoMode.isCloudPlay() || videoMode.getUrl_type() == Constant.ORIN_VOD_URL) {
            mPlayCurMode.setVisibility(View.GONE);
            mCanShow = false;
        } else {
            setCurPlayMode(videoMode.getUrl_type());
        }

    }

    public void setOnChangePlayModule(OnChangePlayModule module) {
        this.mModule = module;
    }

    public void setMiniMode() {
        if (mCanShow) {
            mPlayCurMode.setVisibility(View.VISIBLE);
            if (popupWindow != null) {
                popupWindow.dismiss();
            }
        }
    }

    public void setCurPlayMode(int type) {
    	Log.d("setCurPlayMode", "type="+type);
        switch (type) {
            case Constant.NORMAL_VOD_URL:
                mPlayCurMode.setText(mVideoResolutionText[0]);
                break;
            case Constant.HIGH_VOD_URL:
                mPlayCurMode.setText(mVideoResolutionText[1]);
                break;
        }
    }


    public void showChoiceMode(View viewParent) {
    	int currentType = mVideoMode.getUrl_type();
    	if(currentType == Constant.NORMAL_VOD_URL){
    		int nextType=Constant.HIGH_VOD_URL;
    		if(mVideoMode.hasPlayUrlByType(nextType) && mModule != null){
    			 mModule.onPlayModule(nextType);
    		}
    	}else if(currentType == Constant.HIGH_VOD_URL){
    		int nextType=Constant.NORMAL_VOD_URL;
    		if(mVideoMode.hasPlayUrlByType(nextType) && mModule != null){
   			 mModule.onPlayModule(nextType);
   		}
    	}
//        inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
//        View view = inflater.inflate(R.layout.dialog_resolution, null);
//        
//        TextView tvModeBetter = (TextView) view.findViewById(R.id.tv_mode_better);
//        ImageView ivModeBetter = (ImageView) view.findViewById(R.id.iv_mode_better);
//        setTextState(tvModeBetter, ivModeBetter, Constant.NORMAL_VOD_URL, mVideoMode.hasPlayUrlByType(Constant.NORMAL_VOD_URL));
//
//        TextView tvModeBest = (TextView) view.findViewById(R.id.tv_mode_best);
//        ImageView ivModeBest = (ImageView) view.findViewById(R.id.iv_mode_best);
//        setTextState(tvModeBest, ivModeBest, Constant.HIGH_VOD_URL, mVideoMode.hasPlayUrlByType(Constant.HIGH_VOD_URL));
//        tvModeBetter.setText(mVideoResolutionText[0]);
//        tvModeBest.setText(mVideoResolutionText[1]);
//        int[] location = new int[2];
//        viewParent.getLocationOnScreen(location);
//
//        DisplayMetrics dis = context.getResources().getDisplayMetrics();
//        float dialogHeight = dis.heightPixels / 2.3f;
//        float dialogWidth = dis.widthPixels / 8;
//        popupWindow = new PopupWindow(view, (int) dialogWidth, (int) dialogHeight);
//        try {
//            popupWindow.showAtLocation(viewParent, Gravity.NO_GRAVITY, location[0] - (int) dialogWidth + (int) dialogWidth * 2 / 3, location[1] - (int) dialogHeight);
//        } catch (BadTokenException e) {
//            e.printStackTrace();
//        }
//        popupWindow.setOutsideTouchable(true);
    	
    }


    public void setTextState(TextView textView, ImageView icon, int choose, boolean supported) {
        int curState = mVideoMode.getUrl_type();

        Resources resource = context.getResources();
        if (choose == curState) {
            //当前显示的状态
            textView.setTextColor(resource.getColor(R.color.resolution_selected));
            icon.setVisibility(View.INVISIBLE);
        } else {
            if (supported) {
                textView.setTextColor(resource.getColor(R.color.resolution_supported));
                icon.setVisibility(View.INVISIBLE);
            } else {
                textView.setTextColor(resource.getColor(R.color.resolution_not_supported));
                icon.setVisibility(View.VISIBLE);
            }
        }


        textView.setId(choose);
        textView.setOnClickListener(mClickListener);
    }

    OnClickListener mClickListener = new OnClickListener() {

        @Override
        public void onClick(View v) {

            popupWindow.dismiss();

            if (mModule != null) {
                mModule.onPlayModule(v.getId());
            }
        }
    };


    public void dismiss() {
        if (popupWindow != null && popupWindow.isShowing()) {
            popupWindow.dismiss();
        }
    }

    public boolean isShowing() {
        if (popupWindow != null) {
            return popupWindow.isShowing();
        }

        return false;
    }

    public interface OnChangePlayModule {
        public void onPlayModule(int type);
    }
}
