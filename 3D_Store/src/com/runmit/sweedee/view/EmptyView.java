/**
 *
 */
package com.runmit.sweedee.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.runmit.sweedee.R;

/**
 * @author Sven.Zhan
 *         各异步加载界面的加载界面和空界面
 */
public class EmptyView extends RelativeLayout {

    private View mainView;

    private View emptyView;

    private View loadingView;

    private TextView tvEmptyTip;

    private ImageView ivEmptyPic;

    private TextView tvLoadingTip;

    public EmptyView(Context context) {
        super(context);
        init(context);
    }

    public EmptyView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public EmptyView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);

    }

    private void init(Context context) {
        mainView = LayoutInflater.from(context).inflate(R.layout.layout_empty_view, this);
        emptyView = mainView.findViewById(R.id.empty_relativelayout);
        loadingView = mainView.findViewById(R.id.loading_view);
        tvEmptyTip = (TextView) mainView.findViewById(R.id.tv_empty_tip);
        ivEmptyPic = (ImageView) mainView.findViewById(R.id.iv_empty_tip);
        tvLoadingTip = (TextView) mainView.findViewById(R.id.tv_loadingtip);
    }

    public EmptyView setStatus(Status status) {
        if (status == Status.Gone) {
            setVisibility(View.GONE);
            mainView.setVisibility(View.GONE);
        } else {
            setVisibility(View.VISIBLE);
            mainView.setVisibility(View.VISIBLE);
            loadingView.setVisibility(status == Status.Loading ? View.VISIBLE : View.INVISIBLE);
            emptyView.setVisibility(status == Status.Empty ? View.VISIBLE : View.INVISIBLE);
        }
        return this;
    }
    
    public void setTextColor(int color){
    	tvEmptyTip.setTextColor(color);
    	tvLoadingTip.setTextColor(color);
    }

    public EmptyView setEmptyTip(int resId) {
        tvEmptyTip.setText(resId);
        return this;
    }

    public EmptyView setEmptyTop(Context context, float dpValue) {
        final float scale = context.getResources().getDisplayMetrics().density;
        int px = (int) (dpValue * scale + 0.5f);
        ivEmptyPic.setPadding(0, px, 0, 0);
        tvEmptyTip.setPadding(0,0,0,0);
        return this;
    }

    public TextView gettvEmptyTip(){
    	return tvEmptyTip;
    }
    
    public EmptyView setLoadingTip(int resId) {
        tvLoadingTip.setText(resId);
        return this;
    }

    public EmptyView setEmptyPic(int resId) {
        ivEmptyPic.setImageResource(resId);
        return this;
    }
    
    public EmptyView setEmptyPicVisibility(int visibility) {
        ivEmptyPic.setVisibility(visibility);
        return this;
    }


    public enum Status {
        Loading,
        Empty,
        Gone,
    }
}
