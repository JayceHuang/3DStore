/**
 * BaseActivity.java 
 * com.xunlei.share.BaseActivity
 * @author: Administrator
 * @date: 2013-4-8 上午11:32:43
 */
package com.runmit.sweedee;


import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Toast;

import com.runmit.sweedee.action.login.LoginActivity;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.SharePrefence;

/**
 * @author Administrator
 *         实现的主要功能。
 *         <p/>
 *         修改记录：修改者，修改日期，修改内容
 */
public class BaseActivity extends Activity {

    public SharePrefence mXLSharePrefence;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mXLSharePrefence = SharePrefence.getInstance(getApplicationContext());
    }

    /**
     * 勿删，用来保存被系统kill之后的一些重要数据
     * 2.保存用户登陆信息
     */
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        UserManager.getInstance().onSaveInstanceState(outState);
    }

    /**
     * onSaveInstanceState函数调用时间是onCreate和onResume之间
     */
    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        UserManager.getInstance().onRestoreInstanceState(savedInstanceState);
    }

    protected boolean loginFilter() {
        if (!UserManager.getInstance().isLogined()) {
            Intent intent = new Intent(this, LoginActivity.class);
            startActivity(intent);
            overridePendingTransition(
                    R.anim.translate_between_interface_top_in,
                    R.anim.translate_between_interface_top_out);
            return false;
        } else {
            return true;
        }
    }

    protected boolean netWorkFilter() {
        if (!Util.isNetworkAvailable()) {
            Util.showToast(this,
                    getString(R.string.has_no_using_network),
                    Toast.LENGTH_LONG);
            return false;
        }
        return true;
    }
}
