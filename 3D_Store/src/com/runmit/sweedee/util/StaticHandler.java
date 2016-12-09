/**
 * StaticHandler.java 
 * com.xunlei.share.util.StaticHandler
 * @author: zhangzhi
 * @date: 2013-4-25 上午10:58:23
 */
package com.runmit.sweedee.util;

import java.lang.ref.WeakReference;

import android.os.Handler;
import android.os.Message;

/**
 * 用来消除Handle可能导致的泄漏
 */
public class StaticHandler extends Handler {
    WeakReference<MessageListener> listener;

    /**
     * @param listener 必读: 此listener必须由Activity实现该接口(推荐)或者是宿主Activity的类成员 : 这里是弱引用,
     *                 不会增加变量的引用计数, 使用匿名变量会导致listener过早释放(请参考此类的引用示例)
     */
    public StaticHandler(MessageListener listener) {
        this.listener = new WeakReference<MessageListener>(listener);
    }

    public void handleMessage(Message msg) {
        MessageListener listener = this.listener.get();
        if (listener != null) {
            listener.handleMessage(msg);
        }
    }

    public interface MessageListener {
        public void handleMessage(Message msg);
    }
}
