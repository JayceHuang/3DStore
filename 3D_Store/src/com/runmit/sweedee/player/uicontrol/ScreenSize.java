package com.runmit.sweedee.player.uicontrol;

import com.runmit.sweedee.R;
import com.runmit.sweedee.player.PlayerConstant;

import android.view.View;
import android.widget.ImageView;


public abstract class ScreenSize {
    private int mCurScreenMode = PlayerConstant.ScreenMode.SUITABLE_SIZE;
    private ImageView mScreenImage;

    private int mScreenId[] =
            {
                    R.drawable.play_screen_suitable_gnr,
                    R.drawable.play_screen_fill_gnr,
                    R.drawable.play_screen_origin_gnr
            };

    public ScreenSize(ImageView imageView) {
        this.mScreenImage = imageView;
    }

    public void setScreenModeVisibility(int visibility) {
        mScreenImage.setVisibility(visibility);
    }

    public void setCurMode(int mode) {
        this.mCurScreenMode = mode;
    }

    public int getCurMode() {
        return this.mCurScreenMode;
    }

    public void showSreenMode() {
        mScreenImage.setVisibility(View.VISIBLE);
        showCurMode();
    }

    //public void hideScreenMode()
    //{
    //	mScreenImage.setVisibility(View.GONE);
    //}

    public void switch2NextMode() {

        if (mCurScreenMode == PlayerConstant.ScreenMode.FILL_SIZE) {
            mCurScreenMode = PlayerConstant.ScreenMode.ORIGIN_SIZE;
        } else {
            ++mCurScreenMode;
        }
        showCurMode();
    }

    public void reset() {
        mCurScreenMode = PlayerConstant.ScreenMode.SUITABLE_SIZE;
    }

    private void showCurMode() {
        mScreenImage.setBackgroundResource(mScreenId[mCurScreenMode]);
        setScreenMode(mCurScreenMode);
    }

    abstract public void setScreenMode(int mode);
}
