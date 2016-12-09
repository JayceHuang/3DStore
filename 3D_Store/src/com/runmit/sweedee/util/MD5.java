/*
 * 文件名称 : MD5.java
 * <p>
 * 作者信息 : liuzongyao
 * <p>
 * 创建时间 : 2013-9-10, 下午7:46:26
 * <p>
 * 版权声明 : Copyright (c) 2009-2012 Hydb Ltd. All rights reserved
 * <p>
 * 评审记录 :
 * <p>
 */

package com.runmit.sweedee.util;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * 请在这里增加文件描述
 * <p/>
 */
public class MD5 {

    public static byte[] encrypt(byte[] content) {
        try {
            MessageDigest md;
            md = MessageDigest.getInstance("MD5");
            md.update(content, 0, content.length);
            return md.digest();
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
        return null;
    }

    public static String encrypt(String key) {
        if (key == null) {
            return "";
        }
        char[] hex = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
        byte[] bytes = encrypt(key.getBytes());
        if (bytes == null) {
            return "";
        }

        StringBuilder sb = new StringBuilder(32);
        for (byte b : bytes) {
            sb.append(hex[((b >> 4) & 0xF)]).append(hex[((b >> 0) & 0xF)]);
        }
        return sb.toString();
    }

}
