/**
 * CPUTool.java 
 * com.xunlei.share.util.CPUTool
 * @author: DJ
 * @date: 2013-3-19 下午5:46:55
 */
package com.runmit.sweedee.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileFilter;
import java.io.FileReader;
import java.util.regex.Pattern;

/**
 * @author DJ
 *         实现的主要功能：cpu 信息处理类。
 *         修改记录：修改者，修改日期，修改内容
 */
public class CPUTool {

    private final static String kCpuInfoMinFreqFilePath = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq";
    private final static String kCpuInfoCurFreqFilePath = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";
    private final static String kCpuInfoMaxFreqFilePath = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";

    //public static boolean isHighendCpu = isHighEndCPU();

    /*获取cpu核心数目*/
    public static int getCpuCoresNum() {
        // Private Class to display only CPU devices in the directory listing
        class CpuFilter implements FileFilter {
            @Override
            public boolean accept(File pathname) {
                // Check if filename is "cpu", followed by a single digit number
                if (Pattern.matches("cpu[0-9]", pathname.getName())) {
                    return true;
                }
                return false;
            }
        }
        try {
            // Get directory containing CPU info
            File dir = new File("/sys/devices/system/cpu/");
            // Filter to only list the devices we care about
            File[] files = dir.listFiles(new CpuFilter());
            // Return the number of cores (virtual CPU devices)
            return files.length;
        } catch (Exception e) {
            // Default to return 1 core
            return 1;
        }
    }

    /* 获取CPU最大频率（单位KHZ） */
    public static int getMaxCpuFreq() {
        int result = 0;
        FileReader fr = null;
        BufferedReader br = null;
        try {
            fr = new FileReader(kCpuInfoMaxFreqFilePath);
            br = new BufferedReader(fr);
            String text = br.readLine();
            result = Integer.parseInt(text.trim());
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                if (fr != null) fr.close();
                if (br != null) br.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return result;
    }


    /* 获取CPU最小频率（单位KHZ） */
    public static int getMinCpuFreq() {
        int result = 0;
        FileReader fr = null;
        BufferedReader br = null;
        try {
            fr = new FileReader(kCpuInfoMinFreqFilePath);
            br = new BufferedReader(fr);
            String text = br.readLine();
            result = Integer.parseInt(text.trim());
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                if (fr != null) fr.close();
                if (br != null) br.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return result;
    }


    /* 实时获取CPU当前频率（单位KHZ） */
    public static int getCurCpuFreq() {
        int result = 0;
        FileReader fr = null;
        BufferedReader br = null;
        try {
            fr = new FileReader(kCpuInfoCurFreqFilePath);
            br = new BufferedReader(fr);
            String text = br.readLine();
            result = Integer.parseInt(text.trim());
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                if (fr != null) fr.close();
                if (br != null) br.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return result;
    }

    /* 获取CPU名字 */
    public static String getCpuName() {
        FileReader fr = null;
        BufferedReader br = null;
        try {
            fr = new FileReader("/proc/cpuinfo");
            br = new BufferedReader(fr);
            String text = br.readLine();
            String[] array = text.split(":\\s+", 2);
            for (int i = 0; i < array.length; i++) {
            }
            return array[1];
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                if (fr != null) fr.close();
                if (br != null) br.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    /*判断是否为高端机型cpu */
    public static boolean isHighEndCPU() {
        int core_num = getCpuCoresNum();
        int max_freq = getMaxCpuFreq();
        if (core_num >= 2 && max_freq >= 1000000) {
            return true;
        } else {
            return false;
        }
    }
}
