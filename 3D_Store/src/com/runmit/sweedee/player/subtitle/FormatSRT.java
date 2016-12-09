package com.runmit.sweedee.player.subtitle;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

import com.runmit.sweedee.util.Util;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.text.TextUtils;
import android.util.Log;

/**
 * This class represents the .SRT subtitle format <br>
 * <br>
 * Copyright (c) 2012 J. David Requejo <br>
 * j[dot]david[dot]requejo[at] Gmail <br>
 * <br>
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions: <br>
 * <br>
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. <br>
 * <br>
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @author J. David Requejo
 */
public class FormatSRT implements TimedTextFileFormat {

    public boolean loadFileToDB(String fileName, String charsetName,
                                Context context) {
        File file = new File(fileName);
        InputStream is = null;
        try {
            is = new FileInputStream(file);
        } catch (FileNotFoundException e1) {
            // TODO Auto-generated catch block
            e1.printStackTrace();
        }

        Caption caption = new Caption();
        int captionNumber = 1;

        // first lets load the file
        InputStreamReader in = null;
        SubTilteDatabaseHelper mSubTilteDatabaseHelper = SubTilteDatabaseHelper
                .getInstance(context, fileName + ".db");
        SQLiteDatabase db = mSubTilteDatabaseHelper.getWritableDatabase();
        try {
            in = new InputStreamReader(is, charsetName);
        } catch (UnsupportedEncodingException e1) {
            e1.printStackTrace();
        }

        BufferedReader br = new BufferedReader(in);
        String line = null;

        try {
            line = br.readLine().trim();
            while (line != null) {
                line = line.trim();
                // if its a blank line, ignore it, otherwise...
                if (line.length() > 0 && isNumeric(line)) { //轻度过滤标志行，以增加容错性
                    String timeline = null;
                    String contentline = null;

                    timeline = br.readLine().trim();
                    contentline = br.readLine().trim();
                    try {
                        line = timeline;
                        String start = line.substring(0, 12);
                        String end = line.substring(line.length() - 12,
                                line.length());
                        Time time = new Time("hh:mm:ss,ms", start);
                        caption.start = time;
                        time = new Time("hh:mm:ss,ms", end);
                        caption.end = time;
                    } catch (IndexOutOfBoundsException e) {
                        e.printStackTrace();
                        return false;
                    }

                    String text = "";
                    line = contentline;
                    while (line.length() > 0) {
                        text += line + "<br />";
                        line = br.readLine();
                        if (line == null) {
                            break;
                        }
                        line = line.trim();
                    }
                    caption.content = text;
                    db.insert(Caption.TABLE_NAME, null,
                            caption.toContentValues());

                    // we go to next blank
                    while (line != null && line.length() > 0) { // 防止空指针
                        line = br.readLine().trim();
                    }
                    caption = new Caption();
                }
                line = br.readLine();
            }
        } catch (NullPointerException e) {
            e.printStackTrace();
        } catch (IOException e2) {
            e2.printStackTrace();
        } finally {
            try {
                Log.d("finally", "formatsrt");
                is.close();
                db.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        mSubTilteDatabaseHelper.setVersion(SubTilteDatabaseHelper.VERSION_LOADED);
        return true;
    }

    /**
     * 判断一个字符串是否全由数字组成
     *
     * @param str
     * @return
     */
    private static boolean isNumeric(String str) {
        for (int i = str.length(); --i >= 0; ) {
            if (!Character.isDigit(str.charAt(i))) {
                return false;
            }
        }
        return true;
    }
}
