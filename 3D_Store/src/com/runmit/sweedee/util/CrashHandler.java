/**
 * CrashHandler.java 
 * com.xunlei.share.util.CrashHandler
 * @author: admin
 * @date: 2012-11-14 下午3:23:19
 */
package com.runmit.sweedee.util;

/**
 *
 * @author 周亮
 * 系统异常捕获
 *
 */

import java.io.File;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.lang.Thread.UncaughtExceptionHandler;
import java.lang.reflect.Field;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.TreeSet;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.os.Environment;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.downloadinterface.DownloadEngine;
import com.umeng.analytics.MobclickAgent;

public class CrashHandler implements UncaughtExceptionHandler {

    XL_Log log = new XL_Log(CrashHandler.class);

    /**
     * Debug Log tag
     */
    public static final String TAG = "CrashHandler";

    /**
     * 系统默认的UncaughtException处理类
     */
    private Thread.UncaughtExceptionHandler mDefaultHandler;
    /**
     * CrashHandler实例
     */
    private static CrashHandler INSTANCE;
    /**
     * 程序的Context对象
     */
    private Context mContext;
    /**
     * 使用Properties来保存设备的信息和错误堆栈信息
     */
//	private Properties mDeviceCrashInfo = new Properties(); 
    private static final String VERSION_NAME = "versionName";
    private static final String VERSION_CODE = "versionCode";
    private static final String VERSION_SDK = "version_sdk";
    private static final String VERSION_SDK_INT = "version_sdk_int";
    private static final String VERSION_RELEASE = "version_release";
    private static final String NET_TYPE = "net_type";
    private static final String NET_INFO = "net_info";
    /**
     * 错误报告文件的扩展名
     */
    private static final String CRASH_REPORTER_EXTENSION = ".cr";
    /**
     * 错误报告文件的前缀
     */
    private static final String CRASH_REPORTER_STARTWITH = "crash-";

    public static String filepath = Util.getSDCardDir() + XL_Log.LOG_PATH;//存储路径

    private Map<String, String> mDeviceCrashInfo = new HashMap<String, String>();

    /**
     * 保证只有一个CrashHandler实例
     */
    private CrashHandler() {
    }

    /**
     * 获取CrashHandler实例 ,单例模式
     */
    public static CrashHandler getInstance() {
        if (INSTANCE == null) {
            INSTANCE = new CrashHandler();
        }
        return INSTANCE;
    }

    /**
     * 初始化,注册Context对象,
     * 获取系统默认的UncaughtException处理器,
     * 设置该CrashHandler为程序的默认处理器
     *
     * @param ctx
     */
    public void init(Context ctx) {
        mContext = ctx;
        mDefaultHandler = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(this);
    }

    /**
     * 当UncaughtException发生时会转入该函数来处理
     */
    @Override
    public void uncaughtException(Thread thread, Throwable ex) {
        if (!handleException(ex) && mDefaultHandler != null) {
            //如果用户没有处理则让系统默认的异常处理器来处理
            mDefaultHandler.uncaughtException(thread, ex);
        } else {
            try {//Sleep一会后结束程序
                Thread.sleep(3000);
            } catch (InterruptedException e) {
                log.debug(e);
            }
            StoreApplication.INSTANCE.killSelf();
            android.os.Process.killProcess(android.os.Process.myPid());
            System.exit(10);
        }
    }

    /**
     * 自定义错误处理,收集错误信息
     * 发送错误报告等操作均在此完成.
     * 开发者可以根据自己的情况来自定义异常处理逻辑
     *
     * @param ex
     * @return true:如果处理了该异常信息;否则返回false
     */
    private boolean handleException(Throwable ex) {
        if (ex == null) {
            log.debug("handleException --- ex==null");
            return true;
        }
        ex.printStackTrace();
        //使用Toast来显示异常信息
        new Thread() {
            @Override
            public void run() {
                try {
                    Looper.prepare();
                    Toast.makeText(mContext, mContext.getString(R.string.application_crash_toast), Toast.LENGTH_LONG).show();
                    Looper.loop();
                } catch (Throwable e) {
                    //捕捉内存崩溃
                }
            }
        }.start();
        MobclickAgent.reportError(mContext, ex);
        //收集设备信息
        collectCrashDeviceInfo(mContext);
        //保存错误报告文件
        saveCrashInfoToFile(ex);
        return true;
    }

    /**
     * 在程序启动时候, 可以调用该函数来发送以前没有发送的报告
     */
    public void sendPreviousReportsToServer() {
        sendCrashReportsToServer(mContext);
    }

    /**
     * 把错误报告发送给服务器,包含新产生的和以前没发送的.
     *
     * @param ctx
     */
    private void sendCrashReportsToServer(Context ctx) {
        String[] crFiles = getCrashReportFiles(ctx);
        if (crFiles != null && crFiles.length > 0) {
            TreeSet<String> sortedFiles = new TreeSet<String>();
            sortedFiles.addAll(Arrays.asList(crFiles));
            for (String fileName : sortedFiles) {
                File cr = new File(filepath + fileName);
                postReport(cr);
            }
        }
    }

    private void postReport(File file) {
    	//TODO http上报
    }

    /**
     * 获取错误报告文件名
     *
     * @param ctx
     * @return
     */
    private String[] getCrashReportFiles(Context ctx) {
        File filesDir = new File(filepath);
        FilenameFilter filter = new FilenameFilter() {
            public boolean accept(File dir, String name) {
                return name.endsWith(CRASH_REPORTER_EXTENSION) && name.startsWith(CRASH_REPORTER_STARTWITH);
            }
        };
        return filesDir.list(filter);
    }

    /**
     * 保存错误信息到文件中
     *
     * @param ex
     * @return
     */
    private String saveCrashInfoToFile(Throwable ex) {
    	log.debug("saveCrashInfoToFile");
        StringBuffer sb = new StringBuffer();
        for (Map.Entry<String, String> entry : mDeviceCrashInfo.entrySet()) {
            String key = entry.getKey();
            String value = entry.getValue();
            sb.append(key + "=" + value + "\n");
        }

        Writer writer = new StringWriter();
        PrintWriter printWriter = new PrintWriter(writer);
        ex.printStackTrace(printWriter);
        Throwable cause = ex.getCause();
        while (cause != null) {
            cause.printStackTrace(printWriter);
            cause = cause.getCause();
        }
        printWriter.close();
        String result = writer.toString();
        sb.append(result);
        try {
            DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
            long timestamp = System.currentTimeMillis();
            String time = formatter.format(new Date());
            String fileName = "crash-" + time + "-" + timestamp + ".cr";
            if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
                File dir = new File(filepath);
                if (!dir.exists()) {
                    dir.mkdirs();
                }
                FileOutputStream fos = new FileOutputStream(filepath + fileName);
                fos.write(sb.toString().getBytes());
                fos.close();

                postReport(new File(filepath + fileName));
            }
            return fileName;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * 收集程序崩溃的设备信息
     *
     * @param ctx
     */
    public void collectCrashDeviceInfo(Context ctx) {
        try {
            PackageManager pm = ctx.getPackageManager();
            PackageInfo pi = pm.getPackageInfo(ctx.getPackageName(),
                    PackageManager.GET_ACTIVITIES);
            if (pi != null) {
                mDeviceCrashInfo.put(VERSION_NAME,
                        pi.versionName == null ? "not set" : pi.versionName);
                mDeviceCrashInfo.put(VERSION_CODE, "" + pi.versionCode);
                mDeviceCrashInfo.put(VERSION_SDK, "" + android.os.Build.VERSION.SDK);
                mDeviceCrashInfo.put(VERSION_RELEASE, "" + android.os.Build.VERSION.RELEASE);
                mDeviceCrashInfo.put(VERSION_SDK_INT, "" + android.os.Build.VERSION.SDK_INT);
                if (Util.isWifiNet(ctx)) {
                    mDeviceCrashInfo.put(NET_TYPE, "WIFI");
                } else if (Util.isMobileNet(ctx)) {
                    ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
                    NetworkInfo netInfo = cm.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
                    mDeviceCrashInfo.put(NET_TYPE, "GPRS");
                    mDeviceCrashInfo.put(NET_INFO, netInfo.toString());
                }
            }
        } catch (NameNotFoundException e) {
            log.debug("Error while collect package info" + e);
        }
        //使用反射来收集设备信息.在Build类中包含各种设备信息,
        //例如: 系统版本号,设备生产商 等帮助调试程序的有用信息
        //具体信息请参考后面的截图
        Field[] fields = Build.class.getDeclaredFields();
        for (Field field : fields) {
            try {
                field.setAccessible(true);
                mDeviceCrashInfo.put(field.getName(), "" + field.get(null));
                log.debug(field.getName() + " : " + field.get(null));
            } catch (Exception e) {
                log.debug("Error while collect crash info" + e);
            }
        }
    }

}


