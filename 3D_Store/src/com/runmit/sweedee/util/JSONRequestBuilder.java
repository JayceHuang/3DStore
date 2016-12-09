package com.runmit.sweedee.util;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONArray;

import com.runmit.sweedee.sdk.user.member.task.UserManager;

public class JSONRequestBuilder {
    private static final String XL_LocationProtocol = "1.0";
    private JSONObject jobj;
    private UserManager mDEEtmManager;

    public JSONRequestBuilder() throws JSONException {
        jobj = new JSONObject();
        mDEEtmManager = UserManager.getInstance();
        jobj.put("session_id", mDEEtmManager.getUserSessionId());
        jobj.put("XL_LocationProtocol", XL_LocationProtocol);
    }

    public JSONRequestBuilder setUserId() throws JSONException {
        jobj.put("user_id", Long.toString(UserManager.getInstance().getUserId()));
        return this;
    }

    public JSONRequestBuilder setUserId(String userId) throws JSONException {
        jobj.put("user_id", userId);
        return this;
    }

    public JSONRequestBuilder setCommand(String command) throws JSONException {
        jobj.put("Command_id", command);
        return this;
    }

    public JSONRequestBuilder entry(String key, String value) throws JSONException {
        jobj.put(key, value);
        return this;
    }

    public JSONRequestBuilder entry(String key, Long value) throws JSONException {
        jobj.put(key, value);
        return this;
    }

    public JSONObject getJSONObject() {
        return jobj;
    }

    public String build() {
        return jobj.toString();
    }

    public JSONRequestBuilder strArrayEntry(String key, String[] array) throws JSONException {
        if (key == null || array == null)
            return this;
        JSONArray jArray = new JSONArray();
        for (String str : array) {
            jArray.put(str);
        }
        jobj.put(key, jArray);
        return this;
    }
}
