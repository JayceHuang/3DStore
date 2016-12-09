/**
 * CaptionTextView.java 
 * com.xunlei.share.CaptionTextView
 * @author: Administrator
 * @date: 2013-5-13 下午6:32:03
 */
package com.runmit.sweedee;

import java.util.ArrayList;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.text.Html;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

import com.runmit.sweedee.player.subtitle.Caption;
import com.runmit.sweedee.player.subtitle.SubTilteDatabaseHelper;
import com.runmit.sweedee.util.XL_Log;

/**
 * @author Administrator 实现的主要功能。
 *         <p/>
 *         修改记录：修改者，修改日期，修改内容
 */
public class CaptionTextView extends TextView {

    private XL_Log log = new XL_Log(CaptionTextView.class);


    private String localFullPath = null;

    private static final int DEFINE_LOAD_SIZE = 100;

    private int currentDuration = 0;

    ArrayList<Caption> mCurrentList = new ArrayList<Caption>(DEFINE_LOAD_SIZE);

    int startPos;//mCurrentList的时间最小值
    int endPos;//的时间最大值

    private boolean isclose = false;//是否关闭字幕

    private static final int LOAD_FINISH = 123456;

    private boolean isLoading = false;

    SubTilteDatabaseHelper mSubTilteDatabaseHelper = null;

    private Object mSyncObj = new Object();

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case LOAD_FINISH:
                    if (mCurrentList != null && mCurrentList.size() > 0) {//防止进入死循环
                        if (currentDuration >= startPos && currentDuration <= endPos) {
                            loadText(currentDuration);
                        }
                    }
                    synchronized (mSyncObj) {
                        isLoading = false;
                        mSyncObj.notify();
                    }
                    break;
                default:
                    break;
            }
        }
    };


    public CaptionTextView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public CaptionTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public CaptionTextView(Context context) {
        super(context);
    }

    public void setVideoCaptionPath(String path) {
        if (path == null) {
            return;
        }
        if (path.equals(localFullPath) && !isclose) {
            return;
        }
        localFullPath = path;
        isclose = false;
        setVisibility(View.VISIBLE);
        setText("");
        mSubTilteDatabaseHelper = SubTilteDatabaseHelper.getInstance(getContext(), localFullPath + ".db");
        mCurrentList.clear();

        loadData(currentDuration);
    }

    public void loadText(final int duration) {
        if (isclose || localFullPath == null) {
            return;
        }
        if (currentDuration == duration) {
            return;//防止缓冲循环调用
        }
        currentDuration = duration;

        if (mCurrentList != null && mCurrentList.size() > 0) {
            if (duration >= startPos && duration <= endPos) {
                Caption mCaption = binSearch(0, mCurrentList.size() - 1, duration);
                if (mCaption != null) {
                    setText(Html.fromHtml(mCaption.content));
                    log.debug("mCaption=" + mCaption.content);
                } else {
                    setText("");
                }
            } else {
                setText("");
                if (duration < startPos) {//后退怎么load
                    loadData(currentDuration);
                } else {//前进
                    loadData(currentDuration);
                }
            }
        } else {
            setText("");
            loadData(currentDuration);
        }

    }

    public void close() {
        isclose = true;
        setVisibility(View.GONE);
    }

    // 二分查找递归实现
    public Caption binSearch(int start, int end, int duration) {
        if (start < 0 || end < 0) {
            return null;
        }
        if (start <= end) {
            int mid = (end - start) / 2 + start;
            Caption mCaption = null;
            if (mid < mCurrentList.size()) {
                mCaption = mCurrentList.get(mid);
            }
            if (mCaption == null) {
                return null;
            }
            if (mCaption.start.mseconds <= duration
                    && mCaption.end.mseconds >= duration) {
                return mCaption;
            }
            if (mCaption.end.mseconds < duration) {
                return binSearch(mid + 1, end, duration);
            } else if (mCaption.start.mseconds > duration) {
                return binSearch(start, mid - 1, duration);
            }
        }

        return null;

    }

    private void loadData(final int duration) {
        new Thread(new Runnable() {
            public void run() {
                synchronized (mSyncObj) {
                    while (isLoading) {
                        try {
                            mSyncObj.wait();
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                    isLoading = true;

                    mCurrentList = mSubTilteDatabaseHelper.findCaption(duration - 60, DEFINE_LOAD_SIZE);
                    if (mCurrentList != null && mCurrentList.size() > 0) {
                        startPos = mCurrentList.get(0).start.mseconds;
                        endPos = mCurrentList.get(mCurrentList.size() - 1).end.mseconds;
                    }
                    mHandler.obtainMessage(LOAD_FINISH).sendToTarget();
                }
            }
        }).start();
    }
}
