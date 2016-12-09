package com.runmit.sweedee.util;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

public class CharsetConvert {
    /**
     * 7位ASCII字符，也叫作ISO646-US、Unicode字符集的基本拉丁块
     */
    public static final String US_ASCII = "US-ASCII";

    /**
     * ISO 拉丁字母表 No.1，也叫作 ISO-LATIN-1
     */
    public static final String ISO_8859_1 = "ISO-8859-1";

    /**
     * 8 位 UCS 转换格式
     */
    public static final String UTF_8 = "UTF-8";

    /**
     * 16 位 UCS 转换格式，Big Endian（最低地址存放高位字节）字节顺序
     */
    public static final String UTF_16BE = "UTF-16BE";

    /**
     * 16 位 UCS 转换格式，Little-endian（最高地址存放低位字节）字节顺序
     */
    public static final String UTF_16LE = "UTF-16LE";

    /**
     * 16 位 UCS 转换格式，字节顺序由可选的字节顺序标记来标识
     */
    public static final String UTF_16 = "UTF-16";

    /**
     * 中文超大字符集
     */
    public static final String GBK = "GBK";

    public static String inputStreamToGBK(InputStream in) throws IOException {
        return inputStreamconvertToCharset(in, GBK);
    }

    public static String inputStreamToUTF8(InputStream in) throws IOException {
        return inputStreamconvertToCharset(in, UTF_8);
    }

    public static String StringToGBK(String src) throws IOException {
        return strconvertToCharset(src, GBK);
    }

    private static String strconvertToCharset(String src, String charSet) throws UnsupportedEncodingException {
        String gdkResult = null;
        byte[] utf8Bytes = src.getBytes();
        gdkResult = new String(utf8Bytes, charSet);
        return gdkResult;
    }

    private static String inputStreamconvertToCharset(InputStream in, String charset) throws IOException {
        InputStreamReader reader = new InputStreamReader(in, charset);
        StringBuffer buffer = new StringBuffer();
        char[] buf = new char[64];
        int count = 0;
        try {
            while ((count = reader.read(buf)) != -1) {
                buffer.append(buf, 0, count);
            }
        } finally {
            reader.close();
        }
        return buffer.toString();
    }
}
