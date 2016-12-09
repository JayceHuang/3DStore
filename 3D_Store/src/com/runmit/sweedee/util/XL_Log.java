package com.runmit.sweedee.util;

import java.io.File;
import java.io.IOException;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import android.os.Environment;

import de.mindpipe.android.logging.log4j.LogConfigurator;


public class XL_Log {
	
	public static final String LOG_PATH = "SweeDee/log/";
	
    public static boolean isSDCanRead = false;//sd卡可读，由于其他地方调用，使用static

    public static final boolean DEBUG = false;//是否打日志

    static {
        if (isSdcardExist()) {
            final LogConfigurator logConfigurator = new LogConfigurator();
            //配置生成的日志文件路径,log4j会自动备份文件，最多5个文件
            String filepath = Util.getSDCardDir() + LOG_PATH + "SweeDee.log";
            Util.ensureDir(Util.getSDCardDir() + LOG_PATH);
            File f = new File(filepath);
            if (!f.exists()) {
                try {
                    f.createNewFile();
                } catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
            logConfigurator.setFileName(filepath);
            // Set log level of a specific logger
            //等级可分为OFF、FATAL、ERROR、WARN、INFO、DEBUG、ALL   设置为OFF即关闭,开发调试时一般设置成DEBUG
            String packageName="com.runmit.sweedee";
            if (DEBUG) {
                logConfigurator.setLevel(packageName, Level.ALL);
            } else {
                logConfigurator.setLevel(packageName, Level.OFF);
            }

            try {
                if (f.canWrite()) {
                    isSDCanRead = true;
                    logConfigurator.configure();
                } else {
                    isSDCanRead = false;
                }
            } catch (Exception e) {
                isSDCanRead = false;
            }


        }

    }

    private Logger log = null;

    public XL_Log(Class<?> obj) {
        if (isSdcardExist()) {
            log = Logger.getLogger(obj);
        }
    }

    //输出Level.DEBUG级别日志,一般开发调试信息用
    public void debug(Object message) {
        if (null != log && log.isDebugEnabled()) {
            log.debug(getFunctionName() + " msg= " + message);
        }
    }

    //输出Level.INFO级别日志
    public void info(Object message) {
        if (null != log && log.isInfoEnabled()) {
            log.info("currentThread id=" + Thread.currentThread().getId() + " msg= " + message);
        }
    }

    //输出Level.WARN级别日志
    public void warn(Object message) {
        if (null != log && log.isEnabledFor(Level.WARN)) {
            log.warn("currentThread id=" + Thread.currentThread().getId() + " msg= " + message);
        }
    }

    //输出Level.ERROR级别日志,一般catch住异常后使用,使用e.printStackTrace()打印出错误信息;
    public void error(Object message) {
        if (null != log && log.isEnabledFor(Level.ERROR)) {
            log.error(getFunctionName() + " msg= " + message);
        }
    }

    //输出Level.FATAL级别日志
    public void fatal(Object message) {
        if (null != log && log.isEnabledFor(Level.FATAL)) {
            log.fatal("currentThread id=" + Thread.currentThread().getId() + " msg= " + message);
        }
    }

    private static boolean isSdcardExist() {
        return Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);
    }

    //获取线程ID log所在方法名 log所在行数
    private String getFunctionName() {
        StackTraceElement[] sts = Thread.currentThread().getStackTrace();
        if (sts == null) {
            return null;
        }
        for (StackTraceElement st : sts) {
            if (st.isNativeMethod()) {
                //本地方法native  jni
                continue;
            }
            if (st.getClassName().equals(Thread.class.getName())) {
                //线程  
                continue;
            }
            if (st.getClassName().equals(this.getClass().getName())) {
                //构造方法
                continue;
            }
//            return  "[ " + Thread.currentThread().getName() + ": "
//                    + st.getFileName() + ":" + st.getLineNumber() + " "
//                    + st.getMethodName() + " ]";
            return "currentThread id=" + Thread.currentThread().getId() + " methodName = " + st.getMethodName() + ":line=" + st.getLineNumber();
        }
        return null;
    }
}
