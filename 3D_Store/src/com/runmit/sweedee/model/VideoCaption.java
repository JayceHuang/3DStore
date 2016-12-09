/**
 * VideoCaption.java 
 * com.xunlei.share.model.VideoCaption
 * @author: Administrator
 * @date: 2013-5-13 下午3:28:21
 */
package com.runmit.sweedee.model;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * @author zhangzhi
 *         字幕类
 *         <p/>
 *         修改记录：修改者，修改日期，修改内容
 */
public class VideoCaption implements Parcelable {

    public String filename;//原始文件名
    public String local_path;
    public String sname;
    public String language;
    public String scid;
    public String surl;
    //public boolean is_select;
    public int state;

    public final static int STATE_LOADING_NOT = 0;
    public final static int STATE_LOADING_ING = 1;
    public final static int STATE_LOADING_SUCC = 2;
    public final static int STATE_LOADING_FAIL = 3;


    public VideoCaption() {
        super();
    }

    public VideoCaption(JSONObject tempJsonObject) {
        super();
        try {
            this.sname = tempJsonObject.getString("sname");
            this.language = tempJsonObject.getString("language");
            this.scid = tempJsonObject.getString("scid");
            this.surl = tempJsonObject.getString("surl");
            int index = -1;
            if (sname != null && sname.length() > 0) {
                index = sname.lastIndexOf(".");
            }
            if (index < 0) {//无法解析到格式
                sname = surl;//用url替换文件名
            }
        } catch (JSONException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

    }

    private VideoCaption(Parcel in) {
        this.local_path = in.readString();
        this.sname = in.readString();
        this.language = in.readString();
        this.scid = in.readString();
        this.surl = in.readString();
    }

    public static final Parcelable.Creator<VideoCaption> CREATOR = new Parcelable.Creator<VideoCaption>() {
        public VideoCaption createFromParcel(Parcel in) {
            return new VideoCaption(in);
        }

        public VideoCaption[] newArray(int size) {
            return new VideoCaption[size];
        }
    };

    @Override
    public int describeContents() {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(local_path);
        dest.writeString(sname);
        dest.writeString(language);
        dest.writeString(scid);
        dest.writeString(surl);
    }

}
