package com.runmit.sweedee.action.search;

import java.util.ArrayList;
import java.util.Collections;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

//{
//    "XL_LocationProtocol": "1.1",
//    "rtn_code": "0",
//    "command_id": "sense_keyword_resp",
//    "keyword": "zhuwen",
//    "sense_keyword_list": [
//        "keyword_1",
//        "keyword_2",
//        "keyword_3",
//        "keyword_4",
//        "keyword_5"
//    ]
//}

public class SenseKeywordResp {

    public String Command_id;
    public String XL_LocationProtocol;
    public int rtn_code;
    public String keyword;
    public ArrayList<String> sense_keyword_list;

    public static SenseKeywordResp newInstance(JSONObject jobj) {
        if (jobj == null) {
            return null;
        }

        SenseKeywordResp data = new SenseKeywordResp();

        data.Command_id = jobj.optString("Command_id");
        data.XL_LocationProtocol = jobj.optString("XL_LocationProtocol");
        data.rtn_code = jobj.optInt("rtn_code");
        data.keyword = jobj.optString("keyword");

        try {
            JSONArray senseKeywordArray = jobj
                    .getJSONArray("sense_keyword_list");
            if (senseKeywordArray != null) {
                data.sense_keyword_list = new ArrayList<String>();
                for (int i = 0; i < senseKeywordArray.length(); i++) {
                    String word = senseKeywordArray.getString(i);
                    data.sense_keyword_list.add(word);
                }
            }
            Collections.sort(data.sense_keyword_list);
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return data;
    }

}
