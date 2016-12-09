package com.runmit.sweedee.player.subtitle;
/**
 * FileEncodType.java 
 * .FileEncodType
 * @author: zhang_zhi
 * @date: 2013-6-17 下午7:23:57
 */

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

/**
 * Java：判断文件的编码 首先，不同编码的文本，是根据文本的前两个字节来定义其编码格式的。定义如下：
 * <p/>
 * ANSI：　无格式定义 Unicode： 　前两个字节为FFFE Unicode文档以0xFFFE开头 Unicode big
 * endian：　前两字节为FEFF UTF-8：　前两字节为EFBB UTF-8以0xEFBBBF开头
 * <p/>
 * 知道了各种编码格式的区别，写代码就容易了
 * <p/>
 * 转自：http://www.cppblog.com/biao/archive/2009/11/04/100130.aspx
 */
public class FileEncodType {

    public static void main(String[] args) {

        File file = new File("D:\\75ee026a38b9df2bf60d25f0ef0fb79a.srt");
        String str = FileEncodType.getFilecharset(file);
        System.out.println(str);

//		fileEncodingToANSI(file, str);
    }

    /**
     * 将指定文本文件转换为ANSI 编码类型
     *
     * @param sourceFile 指定文本文件
     * @param encoding   指定文本文件原始编码
     */
    protected static void fileEncodingToANSI(File sourceFile, String encoding) {

        try {
            BufferedReader bufRead = new BufferedReader(new InputStreamReader(
                    new FileInputStream(sourceFile), encoding));

            if (encoding.equals("GBK") == false) {
                // 文件编码非ANSI跳过1个字节，避免文件起始出现 ？
                bufRead.skip(1);
            }

            BufferedWriter bufWriter = new BufferedWriter(new FileWriter(
                    "D:\\Test.txt"));

            String str = null;
            while ((str = bufRead.readLine()) != null) {
                // 写入文件
                bufWriter.write(str + "\r\n");
            }

            bufRead.close();
            bufWriter.flush();
            bufWriter.close();

        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {

            e.printStackTrace();
        }

    }

    /**
     * Java：判断文件的编码
     *
     * @param sourceFile 需要判断编码的文件
     * @return String 文件编码
     */
    public static String getFilecharset(File sourceFile) {
        String charset = "GBK";
        byte[] first3Bytes = new byte[3];
        try {
            // boolean checked = false;

            BufferedInputStream bis = new BufferedInputStream(
                    new FileInputStream(sourceFile));
            bis.mark(0);

            int read = bis.read(first3Bytes, 0, 3);
            System.out.println("字节大小：" + read);

            if (read == -1) {
                return charset; // 文件编码为 ANSI
            } else if (first3Bytes[0] == (byte) 0xFF
                    && first3Bytes[1] == (byte) 0xFE) {

                charset = "UTF-16LE"; // 文件编码为 Unicode
                // checked = true;
            } else if (first3Bytes[0] == (byte) 0xFE
                    && first3Bytes[1] == (byte) 0xFF) {

                charset = "UTF-16BE"; // 文件编码为 Unicode big endian
                // checked = true;
            } else if (first3Bytes[0] == (byte) 0xEF
                    && first3Bytes[1] == (byte) 0xBB
                    && first3Bytes[2] == (byte) 0xBF) {

                charset = "UTF-8"; // 文件编码为 UTF-8
                // checked = true;
            }
            bis.reset();

			/*
             * if (!checked) { int loc = 0;
			 * 
			 * while ((read = bis.read()) != -1) { loc++; if (read >= 0xF0)
			 * break; if (0x80 <= read && read <= 0xBF) // 单独出现BF以下的，也算是GBK
			 * break; if (0xC0 <= read && read <= 0xDF) { read = bis.read(); if
			 * (0x80 <= read && read <= 0xBF) // 双字节 (0xC0 - 0xDF) // (0x80 // -
			 * 0xBF),也可能在GB编码内 continue; else break; } else if (0xE0 <= read &&
			 * read <= 0xEF) {// 也有可能出错，但是几率较小 read = bis.read(); if (0x80 <=
			 * read && read <= 0xBF) { read = bis.read(); if (0x80 <= read &&
			 * read <= 0xBF) { charset = "UTF-8"; break; } else break; } else
			 * break; } } // System.out.println( loc + " " +
			 * Integer.toHexString( read ) // ); }
			 */

            bis.close();
        } catch (Exception e) {
            e.printStackTrace();
        }

        return charset;
    }

}
