package com.runmit.sweedee.sdk.user.member.task;

import java.util.LinkedList;
import java.util.List;

import com.runmit.sweedee.StoreApplication;

import android.os.Bundle;
import android.os.Handler;

public abstract class UserTask {

    protected enum TASKSTATE {TS_UNDO, TS_DOING, TS_DONE, TS_CANCELED}

    ;

    public static final int MSG_WHAT_REGIST_USER = 1000;

    private TASKSTATE tsTaskState = TASKSTATE.TS_UNDO;

    private static int s_iSequenceNo = 10000000;

    private int iSequenceNo = 0;

    private UserUtil userUtil = null;

    protected Object userData = null;

    private Handler mHandler = new Handler(StoreApplication.INSTANCE.getMainLooper());

    private List<OnUserListener> listListener = new LinkedList<OnUserListener>();

    public UserTask(UserUtil util) {
        userUtil = util;
        iSequenceNo = s_iSequenceNo++;
    }

    public void initTask() {
        tsTaskState = TASKSTATE.TS_UNDO;
        listListener.clear();
    }

    public void putTaskState(TASKSTATE ts) {
        tsTaskState = ts;
    }

    public TASKSTATE getTaskState() {
        return tsTaskState;
    }

    public UserUtil getUserUtil() {
        return userUtil;
    }

    public void putUserData(Object userdata) {
        userData = userdata;
    }

    public Object getUserData() {
        return userData;
    }

    public final int getSequenceNo() {
        return iSequenceNo;
    }

    public int getProtocolVersion() {
        return 100;
    }

    public int getPlatformVersion() {
        return 1;
    }

    public abstract boolean execute();

    public abstract boolean fireEvent(OnUserListener listen, Bundle bundle);

    public void attachListener(final OnUserListener listen) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (listen == null) {
                    return;
                }
                listListener.add(listen);
            }
        });

    }

    public void detachListener(final OnUserListener listen) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                listListener.remove(listen);
            }
        });

    }

    public void fireListener(final Bundle bundle) {
        mHandler.post(new Runnable() {

            @Override
            public void run() {
                if (TASKSTATE.TS_CANCELED == getTaskState()) {
                    return;
                }

                putTaskState(TASKSTATE.TS_DONE);
                for (int i = 0; i < listListener.size(); i++) {
                    OnUserListener listen = listListener.get(i);
                    if (!fireEvent(listen, bundle)) {
                        return;
                    }
                }
            }
        });
    }

    public boolean cancel() {
        if (getTaskState() == TASKSTATE.TS_DONE)
            return false;
        putTaskState(TASKSTATE.TS_CANCELED);
        return true;
    }

    ;
}
