
package com.runmit.sweedee.util;

import java.util.HashSet;
import java.util.Set;

import org.json.JSONArray;
import org.json.JSONException;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import com.runmit.sweedee.StoreApplication;

/**
 * @author Administrator
 *         项目保存键值对都放在此
 *         <p/>
 *         修改记录：修改者，修改日期，修改内容
 */
public class SharePrefence {
    private SharedPreferences mSharedPreferences = null;
    private Editor mEditor = null;

    public static final String PREFERENCES = "sweedee_sp";
    public static final String ACCOUNT = "ACCOUNT";
    public static final String PASSWORD = "PASSWORD";

    public static final String UPDATE_APP_CHECK_BOX = "update_app_checkbox";//升级不再提示

    public static final String ONLY_WIFI_DOWNLOAD = "only_wifi_down";//紧wifi缓存

    public static final String INIT_DOWNLOAD_FIRST = "int_download_first";//第一次初始化缓存库

    public static final String KEY_SCREEN_PAUSE_PLAYER = "screen_pause_player";

    public static final String KEY_SCREEN_BRIGHT_RECORD = "key_screen_bright_record";

    public static final String SPLIT_DOWNLOAD_TASK_ID = ",";
    
    public static final String REGIST_LOCATION = "selection_location";
    
    public static final String TASKMANAGER_UPDATE_TIPS = "taskmanager_update_tips";
    
    public static final String TEST_MODE = "test_mode";

    public static SharePrefence getInstance(Context c) {
        return new SharePrefence(StoreApplication.INSTANCE);
    }

    public SharePrefence(Context mContext) {
        mSharedPreferences = mContext.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE);
        mEditor = mSharedPreferences.edit();
    }

    public SharePrefence(String name) {
        mSharedPreferences = StoreApplication.INSTANCE.getSharedPreferences(name, Context.MODE_PRIVATE);
        mEditor = mSharedPreferences.edit();
    }

    public String getString(String key, String defValue) {
        return mSharedPreferences.getString(key, defValue);
    }

    public int getInt(String key, int defValue) {
        return mSharedPreferences.getInt(key, defValue);
    }


    public long getLong(String key, long defValue) {
        return mSharedPreferences.getLong(key, defValue);
    }

    public float getFloat(String key, float defValue) {
        return mSharedPreferences.getFloat(key, defValue);
    }

    public boolean containKey(String key) {
        return mSharedPreferences.contains(key);
    }

    public boolean getBoolean(String key, boolean defValue) {
        return mSharedPreferences.getBoolean(key, defValue);
    }

    public void putString(String key, String value) {
        mEditor.putString(key, value);
        mEditor.commit();
    }


    public void putInt(String key, int value) {
        mEditor.putInt(key, value);
        mEditor.commit();
    }

    public void putLong(String key, long value) {
        mEditor.putLong(key, value);
        mEditor.commit();
    }

    public void putFloat(String key, float value) {
        mEditor.putFloat(key, value);
        mEditor.commit();
    }

    public void putBoolean(String key, boolean value) {
        mEditor.putBoolean(key, value);
        mEditor.commit();
    }

    public void putStringArray(String key, Set<String> value) {
        if (value == null || value.size() == 0) {
            return;
        }
        if (Util.hasHoneycomb()) {
            mEditor.putStringSet(key, value);
            mEditor.commit();
        } else {
            JSONArray jsonArray = new JSONArray(value);
            mEditor.putString(key, jsonArray.toString());
            mEditor.commit();
        }
    }

    public void remove(String key) {
        mEditor.remove(key);
        mEditor.commit();
    }

    public Set<String> getStringSet(String key) {
        try {//防止手机刷机升级系统，保存问题
            HashSet<String> temp = (HashSet<String>) mSharedPreferences.getStringSet(key, null);
            if (temp != null) {    //此处重新复制一个对象，是因为4.2的sp会根据对象来决定是否覆盖提交(没有强制提交)。操
                return new HashSet<String>(temp);
            }
        } catch (ClassCastException e) {
            e.printStackTrace();
        }
        return null;
    }
}
