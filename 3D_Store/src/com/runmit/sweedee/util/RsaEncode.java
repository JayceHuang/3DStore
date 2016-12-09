package com.runmit.sweedee.util;

import java.math.BigInteger;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;

import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.interfaces.RSAPrivateKey;
import java.security.interfaces.RSAPublicKey;
import java.security.spec.RSAPrivateKeySpec;
import java.security.spec.RSAPublicKeySpec;


import javax.crypto.Cipher;


public class RsaEncode {
    public static byte[] encodeUseRSA(byte[] content, String m, String e) {
        byte[] enData = null;
        try {
            PublicKey pubKey = RsaEncode.getPublicKey(m, e);
            enData = RsaEncode.rsaEncode(pubKey, content);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        return enData;
    }

    public static byte[] decodeUseRSA(byte[] content, String m, String e) {
        byte[] deData = null;
        try {
            PrivateKey pvtKey = RsaEncode.getPrivateKey(m, e);
            deData = RsaEncode.rsaDecode(pvtKey, content);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        return deData;
    }

    public static void generateKeyPair() throws Exception {
        KeyPairGenerator keyPairGen = KeyPairGenerator.getInstance("RSA");
        // 瀵嗛挜浣嶆暟
        keyPairGen.initialize(1024);
        // 瀵嗛挜瀵�
        KeyPair keyPair = keyPairGen.generateKeyPair();
        // 鍏挜
        RSAPublicKey publicKey = (RSAPublicKey) keyPair.getPublic();
        // 绉侀挜
        RSAPrivateKey privateKey = (RSAPrivateKey) keyPair.getPrivate();
        BigInteger pubE = publicKey.getPublicExponent();
        BigInteger pubM = publicKey.getModulus();
        BigInteger pvtE = privateKey.getPrivateExponent();
        BigInteger pvtM = privateKey.getModulus();
    }

    public static PublicKey getPublicKey(String m, String e) throws Exception {
        BigInteger bigM = new BigInteger(m, 16);
        BigInteger bigE = new BigInteger(e, 16);
        RSAPublicKeySpec keySpec = new RSAPublicKeySpec(bigM, bigE);
        KeyFactory keyFactory = KeyFactory.getInstance("RSA");
        PublicKey publicKey = keyFactory.generatePublic(keySpec);
        return publicKey;
    }

    public static PrivateKey getPrivateKey(String m, String e) throws Exception {
        BigInteger bigM = new BigInteger(m, 16);
        BigInteger bigE = new BigInteger(e, 16);
        RSAPrivateKeySpec keySpec = new RSAPrivateKeySpec(bigM, bigE);
        KeyFactory keyFactory = KeyFactory.getInstance("RSA");
        PrivateKey privateKey = keyFactory.generatePrivate(keySpec);
        return privateKey;
    }

    public static byte[] rsaEncode(PublicKey key, byte[] plainText) throws Exception {
        // 瀹炰緥鍖栧姞瑙ｅ瘑绫�
        Cipher cipher = Cipher.getInstance("RSA/ECB/NoPadding");

        // 鍔犲瘑
        cipher.init(Cipher.ENCRYPT_MODE, key);
        //灏嗘槑鏂囪浆鍖栦负鏍规嵁鍏挜鍔犲瘑鐨勫瘑鏂囷紝涓篵yte鏁扮粍鏍煎紡
        byte[] enBytes = cipher.doFinal(plainText);
        return enBytes;
    }

    public static byte[] rsaDecode(PrivateKey key, byte[] screText) throws Exception {
        // 瀹炰緥鍖栧姞瑙ｅ瘑绫� "RSA/ECB/PKCS1Padding"   灏辨槸锛氣�绠楁硶/宸ヤ綔妯″紡/濉厖妯″紡鈥�
        Cipher cipher = Cipher.getInstance("RSA/ECB/NoPadding");
        // 瑙ｅ瘑
        cipher.init(Cipher.DECRYPT_MODE, key);

        //灏嗘槑鏂囪浆鍖栦负鏍规嵁鍏挜鍔犲瘑鐨勫瘑鏂囷紝涓篵yte鏁扮粍鏍煎紡
        byte[] deBytes = cipher.doFinal(screText);
        return deBytes;
    }

}
