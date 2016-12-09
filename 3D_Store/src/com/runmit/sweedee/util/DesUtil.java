package com.runmit.sweedee.util;

import java.net.URLDecoder;
import java.net.URLEncoder;
import java.security.Key;
import java.security.spec.AlgorithmParameterSpec;

import javax.crypto.Cipher;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.DESKeySpec;
import javax.crypto.spec.IvParameterSpec;

import android.util.Base64;

public class DesUtil {

    static AlgorithmParameterSpec iv = null;// 加密算法的参数接口，IvParameterSpec是它的一个实现
    private static Key key = null;

    public DesUtil(String password, String desiv) throws Exception {
        byte[] DESkey = new String(password).getBytes();// 设置密钥，略去
        byte[] DESIV = new String(desiv).getBytes();// 设置向量，略去
        DESKeySpec keySpec = new DESKeySpec(DESkey);// 设置密钥参数
        iv = new IvParameterSpec(DESIV);// 设置向量
        SecretKeyFactory keyFactory = SecretKeyFactory.getInstance("DES");// 获得密钥工厂
        key = keyFactory.generateSecret(keySpec);// 得到密钥对象

    }

    public String encodeECB(String data) throws Exception {
        Cipher enCipher = Cipher.getInstance("DES/ECB/PKCS5Padding");// 得到加密对象Cipher
        enCipher.init(Cipher.ENCRYPT_MODE, key);// 设置工作模式为加密模式，给出密钥和向量
        byte[] pasByte = enCipher.doFinal(data.getBytes("UTF-8"));
        return Base64.encodeToString(pasByte, Base64.DEFAULT);
    }

    public String decodeECB(String data) throws Exception {
        Cipher deCipher = Cipher.getInstance("DES/ECB/PKCS5Padding");
        deCipher.init(Cipher.DECRYPT_MODE, key);
        byte[] pasByte = deCipher.doFinal(Base64.decode(data, Base64.DEFAULT));
        return new String(pasByte, "UTF-8");
    }

    public static void main(String[] args) throws Exception {

        DesUtil tools = new DesUtil("sohu1234", "");
        System.out.println("加密:"
                + URLEncoder.encode(tools.encodeECB("18601317539"), "utf-8"));
        System.out.println("解密:"
                + tools.decodeECB(URLDecoder.decode(
                "yzDyBghgD4PpBjqSb9%2B9mA%3D%3D", "UTF-8")));
    }
}
