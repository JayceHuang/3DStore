package com.runmit.sweedee.util;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.TimeZone;

import android.content.Context;
import android.util.Log;


public class DateUtils {
    private static final String PATTERN = "dd/MM";
    private static final String DATE_PATTERN = "yyyy-MM-dd";
    private static final String TODAY_PATTERN = "MM月dd日";

    public static String msToHMS(long ms) {
        Date date = new Date(ms);
        SimpleDateFormat format = new SimpleDateFormat("HH:mm:ss");
        format.setTimeZone(TimeZone.getTimeZone("GMT-0:00"));
        return format.format(date);
    }

    public static String msToMS(long ms) {
        Date date = new Date(ms);
        SimpleDateFormat format = new SimpleDateFormat("HH:mm:ss");
        format.setTimeZone(TimeZone.getTimeZone("GMT-0:00"));
        String[] str = format.format(date).split(":");
        int hour = Integer.parseInt(str[0]);
        int min = hour * 60 + Integer.parseInt(str[1]);
        int second = Integer.parseInt(str[2]);
        StringBuilder sb = new StringBuilder();
        sb.append(min > 9 ? min : "0" + min).append(":").append(second > 9 ? second : "0" + second);
        return sb.toString();
    }

//	public static String ToadyOrYesterdayToDM(Context context, String time) {
//		SimpleDateFormat sdf = new SimpleDateFormat(PATTERN);
//		Date date = new Date();
//		Calendar c = Calendar.getInstance();
//		c.setTime(date);
//		Date currentDate = c.getTime();
//		String currentTime = sdf.format(currentDate);
//		if (null!=time && "Today".equals(time.trim())) {
//			return currentTime;
//		} else {
//			return time;
//		}
//	}

    public static String getToadyTime() {
        SimpleDateFormat sdf = new SimpleDateFormat(DATE_PATTERN);
        Date date = new Date();
        Calendar c = Calendar.getInstance();
        c.setTime(date);
        Date currentDate = c.getTime();
        String currentTime = sdf.format(currentDate);
        return currentTime;
    }

    public static String getToadyDate() {
        SimpleDateFormat sdf = new SimpleDateFormat(TODAY_PATTERN);
        Date date = new Date();
        Calendar c = Calendar.getInstance();
        c.setTime(date);
        Date currentDate = c.getTime();
        String currentTime = sdf.format(currentDate);
        return currentTime;
    }

    public static String getTomorrowDayTime() {

        Calendar c = Calendar.getInstance();
        c.set(Calendar.DAY_OF_MONTH, c.get(Calendar.DAY_OF_MONTH) + 1);
        Date currentDate = c.getTime();

        SimpleDateFormat sdf = new SimpleDateFormat(DATE_PATTERN);
        String currentTime = sdf.format(currentDate);
        return currentTime;
    }

    public static String getDayAfterTomorrowDayTime() {
        Calendar c = Calendar.getInstance();
        c.set(Calendar.DAY_OF_MONTH, c.get(Calendar.DAY_OF_MONTH) + 2);
        Date currentDate = c.getTime();

        SimpleDateFormat sdf = new SimpleDateFormat(DATE_PATTERN);
        String currentTime = sdf.format(currentDate);
        return currentTime;
    }

    public static String getCurrentTime() {
        String pattern = "HH:mm";
        SimpleDateFormat sdf = new SimpleDateFormat(pattern);
        Date date = new Date();
        Calendar c = Calendar.getInstance();
        c.setTime(date);
        Date currentDate = c.getTime();
        return sdf.format(currentDate);
    }

//	public static String getSevenDayTimes() {
//		SimpleDateFormat sdf = new SimpleDateFormat(PATTERN);
//		Date date = new Date();
//		Calendar c = Calendar.getInstance();
//		c.setTime(date);
//		StringBuffer sBuffer = new StringBuffer();
//		sBuffer.append("(");
//		for (int i = 0; i < 7; i++) {
//			if (i == 6) {
//				sBuffer.append("'" + sdf.format(c.getTime()) + "')");
//			} else {
//				sBuffer.append("'" + sdf.format(c.getTime()) + "',");
//			}
//			c.add(Calendar.DAY_OF_YEAR, -1);
//		}
//		return sBuffer.toString();
//	}


//	public static String ymdToDm(String time) {
//		String patternString = "yyyyMMdd";
//		SimpleDateFormat sdf = new SimpleDateFormat(patternString);
//		try {
//			Date date = sdf.parse(time);
//			sdf = new SimpleDateFormat(PATTERN);
//			return sdf.format(date);
//		} catch (ParseException e) {
//			Log.e("DateUtils", e.getMessage());
//			return null;
//		}
//	}


}
