/**
 * DateUtil.java 
 * com.xunlei.share.util.DateUtil
 * @author: admin
 * @date: 2012-11-14 下午4:22:11
 */
package com.runmit.sweedee.util;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

/**
 * @author 周亮
 *         实现的主要功能。
 *         <p/>
 *         修改记录：修改者，修改日期，修改内容
 */
public class DateUtil {

    public static final long DURATION = 60 * 60 * 24 * 1000;//一天的毫秒数
    public static final int LIMIT_DAYS = 5;

    public static boolean shouldPlatinumGetExp(String expire_date) {
        boolean should = false;
        SimpleDateFormat formatter = new SimpleDateFormat("yyyyMMdd");
        try {
            Date expire_d = formatter.parse(expire_date);
            Date now_d = new Date();
            float duration = expire_d.getTime() - now_d.getTime();
            float dur_day = duration / DURATION;
            should = (dur_day <= LIMIT_DAYS);
        } catch (Exception e) {
        }
        return should;
    }

    public static String convertTime(long filetime) {
        SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd kk:mm:ss");
        String ftime = formatter.format(new Date(filetime));
        return ftime;
    }

    public static String getDateString(long ms) {
        return getDateString(ms, "yyyy-MM-dd HH:mm:ss");
    }

    public static String getDateString(long ms, String format) {
        Date curDate = new Date(ms);
        return getDateString(curDate, format);
    }

    public static String getNowDateString() {
        return getNowDateString("yyyy-MM-dd HH:mm:ss");
    }

    public static String getNowDateString(String format) {
        Date curDate = new Date(System.currentTimeMillis());
        return getDateString(curDate, format);
    }

    public static String getDateString(Date date) {
        return getDateString(date, "yyyy-MM-dd HH:mm:ss");
    }

    public static String getDateString(Date date, String format) {
        SimpleDateFormat formatter = new SimpleDateFormat(format);
        return formatter.format(date);
    }

    public static long parseDateString(String time) {
        SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        long ltime = 0;
        try {
            Date date = formatter.parse(time);
            ltime = date.getTime();
        } catch (ParseException e) {

        }
        return ltime;
    }

    static SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");

    /**
     * 获取今天0点0分0秒时刻的毫秒
     *
     * @return 获取今天0点0分0秒时刻的毫秒
     */
    public static long getTodayMillis() {
        Calendar today = Calendar.getInstance();
        today.set(Calendar.HOUR, 0);
        today.set(Calendar.MINUTE, 0);
        today.set(Calendar.SECOND, 0);
        return today.getTimeInMillis();
    }

    public static class DateDesc {
        public String desc;
        public int desc_flag;

        public DateDesc(String desc, int desc_flag) {
            this.desc = desc;
            this.desc_flag = desc_flag;
        }

        @Override
        public String toString() {
            return "desc = " + desc + " desc_flag = " + desc_flag;
        }


    }
}
