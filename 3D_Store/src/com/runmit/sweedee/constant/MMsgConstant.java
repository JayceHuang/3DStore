package com.runmit.sweedee.constant;


/**
 * @author DJ
 *         各个业务Manager的回调消息类型（msg.what字段）
 */
public class MMsgConstant {

    //本地业务模块(2xxx)
    public final static int MLCCAL_TASK_STATE_CHANGE = 2000;
    public final static int MLCCAL_TIMER_UPDATE_TASK = MLCCAL_TASK_STATE_CHANGE + 1;
    public final static int MLCCAL_DOWNLOAD_TASK_FINISH = MLCCAL_TIMER_UPDATE_TASK + 1;
    public final static int MLCCAL_DOWNLOAD_TASK_DELETE = MLCCAL_DOWNLOAD_TASK_FINISH + 1;

}
