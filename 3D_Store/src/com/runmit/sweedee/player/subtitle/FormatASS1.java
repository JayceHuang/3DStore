package com.runmit.sweedee.player.subtitle;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import com.runmit.sweedee.util.XL_Log;

/**
 * Class that represents the .ASS and .SSA subtitle file format
 * <p/>
 * <br><br>
 * Copyright (c) 2012 J. David Requejo <br>
 * j[dot]david[dot]requejo[at] Gmail
 * <br><br>
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 * <br><br>
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 * <br><br>
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @author J. David REQUEJO
 */
public class FormatASS1 implements TimedTextFileFormat {

    XL_Log log = new XL_Log(FormatASS1.class);

    public boolean loadFileToDB(String fileName, String charsetName, Context context) {
        File file = new File(fileName);
        InputStream is = null;
        try {
            is = new FileInputStream(file);
        } catch (FileNotFoundException e1) {
            e1.printStackTrace();
        }

        Caption caption = new Caption();
        // first lets load the file
        InputStreamReader in = null;
        SubTilteDatabaseHelper mSubTilteDatabaseHelper = SubTilteDatabaseHelper.getInstance(context, fileName + ".db");
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
                if (line.length() > 0) {
                    final String timeline = br.readLine().trim();
                    final String contentline = br.readLine().trim();

                    log.debug("timeline=" + timeline + ",contentline=" + contentline);

                    String start = timeline.substring(0, 10);
                    String end = timeline.substring(timeline.length() - 10, timeline.length());
                    Time time = new Time("h:mm:ss.ms", start);
                    caption.start = time;
                    time = new Time("h:mm:ss.ms", end);
                    caption.end = time;

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
                    caption.content = text.replace("\\N", "<br />");
                    db.insert(Caption.TABLE_NAME, null, caption.toContentValues());
                    // we go to next blank
                    while (line != null && line.length() > 0) {    //不判断空会有空指针，解决bug 107671
                        line = br.readLine().trim();
                    }
                    caption = new Caption();
                }
                line = br.readLine();

            }
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        } finally {
            try {
                is.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        db.close();
        mSubTilteDatabaseHelper.setVersion(SubTilteDatabaseHelper.VERSION_LOADED);
        Log.d("mSubTilteDatabaseHelper", "mSubTilteDatabaseHelper=" + mSubTilteDatabaseHelper.getVersion());
        return true;
    }
}
