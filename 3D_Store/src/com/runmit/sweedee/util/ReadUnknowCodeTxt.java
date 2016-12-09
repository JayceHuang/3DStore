package com.runmit.sweedee.util;

import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

public class ReadUnknowCodeTxt {
    public byte[] readTxtFile(String filename) {
        byte[] b = new byte[104];
        FileInputStream fis = null;
        try {
            fis = new FileInputStream(filename);
            DataInputStream dis = new DataInputStream(fis);
            int len = dis.read(b);
            dis.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fis != null) {
                try {
                    fis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return b;
    }

    public boolean isUTF8(byte[] c) {
        boolean flag = true;
        int len = c.length - 4;//Ϊ�˷�ֹ���������ȡֵԽ��
        for (int i = 0; i < len; i++) {
//    		System.out.println("c["+i+"]"+Integer.toHexString(c[i] & 0xFF));
            if ((c[i] >> 7 & 0xff) == 0x00) {
                continue;
            }
            if ((c[i] >> 4 & 0xff) == 0xff && (c[++i] >> 6 & 0xff) == 0xfe && (c[++i] >> 6 & 0xff) == 0xfe && (c[++i] >> 6 & 0xff) == 0xfe) {
                continue;
            }
            if ((c[i] >> 5 & 0xff) == 0xff && (c[++i] >> 6 & 0xff) == 0xfe && (c[++i] >> 6 & 0xff) == 0xfe) {
                continue;
            }
            if ((c[i] >> 6 & 0xff) == 0xff && (c[++i] >> 6 & 0xff) == 0xfe) {
                continue;
            }
            flag = false;
            System.out.println("gbk:  i=" + i);
            break;
        }
        return flag;
    }

}
