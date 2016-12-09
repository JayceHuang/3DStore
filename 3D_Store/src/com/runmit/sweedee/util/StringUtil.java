package com.runmit.sweedee.util;

public class StringUtil {


    final static byte[] HEX_DATA_MAP = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    // 以下方法设和GBK字符串，如果是其他编码的字符串，要修改一下
    public static String subString(String str, int bytelength) {
        if (str == null) {
            return null;
        }
        if (bytelength <= 0) {
            return "";
        }
        try {
            if (str.getBytes("GBK").length <= bytelength) {
                return str;
            }
        } catch (Exception ex) {
        }
        StringBuffer buff = new StringBuffer();

        int index = 0;
        char c;
        while (bytelength > 0) {
            c = str.charAt(index);
            if (c < 128) {
                bytelength--;
            } else {// GBK编码汉字占2字节，其他编码的不一样，要修改下面几句
                bytelength--;
                if (bytelength < 1)// 被截断了
                    break;
                bytelength--;
            }
            buff.append(c);
            index++;
        }
        buff.append("...");
        return buff.toString();
    }


    /**
     * 将byte转换为16进制
     *
     * @param b
     * @return
     */
    public static byte[] byte_to_hex(byte b) {
        byte[] buf = new byte[2];
        try {
            int h1 = (b & 0xF0) >> 4;
            int h2 = b & 0xF;
            buf[0] = HEX_DATA_MAP[h1];
            buf[1] = HEX_DATA_MAP[h2];
        } catch (RuntimeException e) {
            e.printStackTrace();
        }
        return buf;
    }

    /**
     * 将byte[]转换为16进制
     *
     * @param data
     * @param len
     * @return
     */
    public static byte[] bytes_to_hex(byte[] data, int len) {
        if (data == null || len <= 0)
            return null;
        byte[] hexData = new byte[len * 2];
        byte[] buf = null;
        try {
            int i = 0;
            for (; i < len; i++) {
                buf = byte_to_hex(data[i]);
                hexData[2 * i] = buf[0];
                hexData[2 * i + 1] = buf[1];
            }
        } catch (RuntimeException e) {
            e.printStackTrace();
        }
        return hexData;
    }

    /**
     * 将16进制byte还原
     *
     * @param c
     * @return
     */
    public static int hex_char_value(byte c) {
        if (c >= '0' && c <= '9')
            return c - '0';
        else if (c >= 'A' && c <= 'Z')
            return c - 'A' + 10;
        else if (c >= 'a' && c <= 'z')
            return c - 'a' + 10;
        else
            return -1;
    }

    /**
     * 将16进制byte[]还原
     *
     * @param hex
     * @param len
     * @return
     */
    public static byte[] hex_to_bytes(byte[] hex, int len) {
        if (len % 2 != 0)
            return null;
        int nbytes = len / 2;
        byte[] buffer = new byte[nbytes];
        try {
            int i = 0;
            for (; i < nbytes; i++) {
                byte c1 = hex[i * 2];
                int h1 = hex_char_value(c1);
                if (h1 < 0)
                    return null;
                byte c2 = hex[i * 2 + 1];
                int h2 = hex_char_value(c2);
                if (h2 < 0)
                    return null;
                byte b = (byte) ((h1 << 4) | h2);
                buffer[i] = b;
            }
        } catch (RuntimeException e) {
            e.printStackTrace();
        }
        return buffer;
    }

}
