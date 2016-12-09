package com.runmit.sweedee.player.subtitle;

import android.content.ContentValues;

public class Caption {

    public Style style;
    public Region region;

    public Time start;
    public Time end;

    public String content = "";


    public static final String TABLE_NAME = "caption";

    public static final String _ID = "_id";
    public static final String START_TIME = "start";
    public static final String END_TIME = "end";
    public static final String CONTENT = "content";

    public ContentValues toContentValues() {
        ContentValues mContentValues = new ContentValues();
        mContentValues.put(START_TIME, start.mseconds);
        mContentValues.put(END_TIME, end.mseconds);
        mContentValues.put(CONTENT, content);
        return mContentValues;
    }

    @Override
    public String toString() {
        return "Caption [start=" + start.mseconds + ", end=" + end.mseconds + ", content="
                + content + "]";
    }


}
