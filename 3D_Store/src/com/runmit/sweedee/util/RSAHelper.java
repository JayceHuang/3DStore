package com.runmit.sweedee.util;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.math.BigInteger;
import java.security.Key;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.interfaces.RSAPrivateKey;
import java.security.interfaces.RSAPublicKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.RSAPrivateKeySpec;
import java.security.spec.RSAPublicKeySpec;
import java.security.spec.X509EncodedKeySpec;

import javax.crypto.Cipher;

import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;

/**
 * RSA加密解密算法,使用方法按test()来就可以
 * 重要：由于此类内部实现全部为static，所以使用此类前请务必调用init函数执行初始化，以防止相互干扰。
 * 且此类是非线程安全的，若有线程安全的需求，把此类的static去掉，改为非static实现
 * <p/>
 * RSA原理详见 ： http://www.ruanyifeng.com/blog/2013/06/rsa_algorithm_part_one.html
 * http://www.ruanyifeng.com/blog/2013/07/rsa_algorithm_part_two.html
 *
 * @author Roger
 */
public class RSAHelper {

    private static final String TAG = RSAHelper.class.getName();
    private static String ALGORITHM = "RSA";
    private static String TRANSFORMATION = "RSA/ECB/NoPadding"; // "RSA"
    // "RSA/ECB/NoPadding"
    // "RSA/ECB/PKCS1Padding"
    // "RSA/None/PKCS1Padding"

    /**
     * 初始化algorithm和transformation，如果某个参数为空，则用默认值
     * 重要：请务必在使用此类前调用此方法
     *
     * @param algorithm      默认"RSA"
     * @param transformation 默认"RSA/ECB/NoPadding"
     */
    public static void init(String algorithm, String transformation) {
        if (TextUtils.isEmpty(algorithm)) {
            // 默认值
            ALGORITHM = "RSA";
        } else {
            ALGORITHM = algorithm;
        }
        if (TextUtils.isEmpty(transformation)) {
            // 默认值
            TRANSFORMATION = "RSA/ECB/NoPadding";
        } else {
            TRANSFORMATION = transformation;
        }
    }

    /**
     * 通过公钥字符串得到公钥
     *
     * @param key
     * @return
     * @throws Exception
     */
    public static PublicKey getPublicKey(byte[] keyBytes) throws Exception {

        X509EncodedKeySpec keySpec = new X509EncodedKeySpec(keyBytes);
        KeyFactory keyFactory = KeyFactory.getInstance(ALGORITHM);
        PublicKey publicKey = keyFactory.generatePublic(keySpec);
        return publicKey;
    }

    /**
     * 从PEM文件获取PublicKey
     *
     * @param pemFile
     * @return
     * @throws Exception
     */
    public static PublicKey getPublicKey(File pemFile) throws Exception {
        FileInputStream fis = new FileInputStream(pemFile);
        DataInputStream dis = new DataInputStream(fis);
        byte[] keyBytes = new byte[(int) pemFile.length()];
        dis.readFully(keyBytes);
        dis.close();
        fis.close();

        String temp = new String(keyBytes, "UTF-8");
        String publicKeyPEMString = temp.replace(
                "-----BEGIN PUBLIC KEY-----\n", "");
        publicKeyPEMString = publicKeyPEMString.replace(
                "-----END PUBLIC KEY-----", "");
        Log.d(TAG, "publicKeyPEMString : " + publicKeyPEMString);

        keyBytes = Base64.decode(publicKeyPEMString, Base64.DEFAULT); // TODO
        // 是否需要解Base64？ 未测试
        PublicKey publicKey = getPublicKey(keyBytes);
        return publicKey;
    }

    /**
     * 从InputStream中获取公钥
     *
     * @param is
     * @param length ： 公钥的total bytes
     * @return
     * @throws Exception
     */
    public static PublicKey getPublicKey(InputStream is, int length)
            throws Exception {
        DataInputStream dis = new DataInputStream(is);
        byte[] keyBytes = new byte[length];
        dis.readFully(keyBytes);
        dis.close();
        is.close();

        String temp = new String(keyBytes, "UTF-8");
        String publicKeyPEMString = temp.replace(
                "-----BEGIN PUBLIC KEY-----\n", "");
        publicKeyPEMString = publicKeyPEMString.replace(
                "-----END PUBLIC KEY-----", "");
        Log.d(TAG, "publicKeyPEMString : " + publicKeyPEMString);

        keyBytes = Base64.decode(publicKeyPEMString, Base64.DEFAULT); // TODO
        // 是否需要解Base64？ 未测试
        PublicKey publicKey = getPublicKey(keyBytes);

        return publicKey;
    }

    /**
     * 通过modulus,publicExponent获得公钥
     *
     * @param modulus        即n
     * @param mRadix         modulus的进制，如果为16进制，则填16
     * @param publicExponent 即e
     * @param eRadix         publicExponent的进制，如果为16进制，则填16
     * @return
     * @throws Exception
     */
    public static PublicKey getPublicKey(String modulus, int mRadix,
                                         String publicExponent, int eRadix) throws Exception {

        BigInteger m = new BigInteger(modulus, mRadix);
        BigInteger e = new BigInteger(publicExponent, eRadix);
        RSAPublicKeySpec keySpec = new RSAPublicKeySpec(m, e);
        KeyFactory keyFactory = KeyFactory.getInstance(ALGORITHM);
        PublicKey publicKey = keyFactory.generatePublic(keySpec);
        return publicKey;
    }

    /**
     * 从密钥对里面获取公钥
     *
     * @param keyPair
     * @return
     */
    public static PublicKey getPublicKey(KeyPair keyPair) {
        return (RSAPublicKey) keyPair.getPublic();
    }

    /**
     * 通过私钥字符串得到私钥
     *
     * @param key
     * @return
     * @throws Exception
     */
    public static PrivateKey getPrivateKey(byte[] keyBytes) throws Exception {

        PKCS8EncodedKeySpec keySpec = new PKCS8EncodedKeySpec(keyBytes);
        KeyFactory keyFactory = KeyFactory.getInstance(ALGORITHM);
        PrivateKey privateKey = keyFactory.generatePrivate(keySpec);
        return privateKey;
    }

    /**
     * 从PEM文件获取PrivateKey
     *
     * @param pemFile
     * @return
     * @throws Exception
     */
    public static PrivateKey getPrivateKey(File pemFile) throws Exception {
        FileInputStream fis = new FileInputStream(pemFile);
        DataInputStream dis = new DataInputStream(fis);
        byte[] keyBytes = new byte[(int) pemFile.length()];
        dis.readFully(keyBytes);
        dis.close();
        fis.close();

        String temp = new String(keyBytes, "UTF-8");
        String privKeyPEMString = temp.replace("-----BEGIN PRIVATE KEY-----\n",
                "");
        privKeyPEMString = privKeyPEMString.replace(
                "-----END PRIVATE KEY-----", "");
        Log.d(TAG, "privKeyPEMString : " + privKeyPEMString);

        keyBytes = Base64.decode(privKeyPEMString, Base64.DEFAULT); // TODO
        // 是否需要解Base64？ 未测试
        PrivateKey privateKey = getPrivateKey(keyBytes);

        return privateKey;
    }

    /**
     * 从InputStream获取PrivateKey
     *
     * @param pemFile
     * @return
     * @throws Exception
     */
    public static PrivateKey getPrivateKey(InputStream is, int length)
            throws Exception {
        DataInputStream dis = new DataInputStream(is);
        byte[] keyBytes = new byte[length];
        dis.readFully(keyBytes);
        dis.close();
        is.close();

        String temp = new String(keyBytes, "UTF-8");
        String privKeyPEMString = temp.replace("-----BEGIN PRIVATE KEY-----\n",
                "");
        privKeyPEMString = privKeyPEMString.replace(
                "-----END PRIVATE KEY-----", "");
        Log.d(TAG, "privKeyPEMString : " + privKeyPEMString);

        keyBytes = Base64.decode(privKeyPEMString, Base64.DEFAULT); // TODO
        // 是否需要解Base64？ 未测试
        PrivateKey privateKey = getPrivateKey(keyBytes);

        return privateKey;
    }

    /**
     * 通过modulus，privateExponent得到私钥
     *
     * @param modulus         即n
     * @param mRadix          modulus的进制，如果为16进制，则填16
     * @param privateExponent 即d
     * @param pRadix          privateExponent的进制，如果为16进制，则填16
     * @return
     * @throws Exception
     */
    public static PrivateKey getPrivateKey(String modulus, int mRadix,
                                           String privateExponent, int pRadix) throws Exception {

        BigInteger m = new BigInteger(modulus, mRadix);
        BigInteger e = new BigInteger(privateExponent, mRadix);
        RSAPrivateKeySpec keySpec = new RSAPrivateKeySpec(m, e);
        KeyFactory keyFactory = KeyFactory.getInstance(ALGORITHM);
        PrivateKey privateKey = keyFactory.generatePrivate(keySpec);
        return privateKey;
    }

    /**
     * 从密钥对里面获取私钥
     *
     * @param keyPair
     * @return
     */
    public static PrivateKey getPrivateKey(KeyPair keyPair) {
        return (RSAPrivateKey) keyPair.getPrivate();
    }

    /**
     * 将私钥或公钥转化为byte数组
     *
     * @param key
     * @return
     * @throws Exception
     */
    public static byte[] getKeyString(Key key) throws Exception {

        return key.getEncoded();
    }

    /**
     * 根据密钥位数产生一个密钥对
     *
     * @param num 密钥位数
     * @return 密钥对
     * @throws Exception
     */
    public static KeyPair genKeyPair(int num) throws Exception {
        KeyPairGenerator keyPairGen = KeyPairGenerator.getInstance(ALGORITHM);
        // 密钥位数
        keyPairGen.initialize(num);
        // 密钥对
        KeyPair keyPair = keyPairGen.generateKeyPair();
        return keyPair;
    }

    /**
     * 加密
     *
     * @param publicKey 公钥
     * @param plainText 明文
     * @return
     */
    public static byte[] encrypt(PublicKey publicKey, byte[] plainText)
            throws Exception {

        Cipher cipher = null;
        byte[] enBytes = null;
        cipher = Cipher.getInstance(TRANSFORMATION);
        cipher.init(Cipher.ENCRYPT_MODE, publicKey);
        enBytes = cipher.doFinal(plainText);
        return enBytes;
    }

    /**
     * 加密
     *
     * @param modulus        即n
     * @param publicExponent 即e
     * @param radix          modulus和publicExponent的进制
     * @param plainText      明文
     * @return
     * @throws Exception
     */
    public static byte[] encrypt(String modulus, String publicExponent,
                                 int radix, byte[] plainText) throws Exception {
        PublicKey publicKey = getPublicKey(modulus, radix, publicExponent,
                radix);
        byte[] enBytes = encrypt(publicKey, plainText);
        return enBytes;
    }

    /**
     * 加密
     *
     * @param publicKey 公钥
     * @param plainText 明文
     * @return
     */
    public static String encrypt(String publicKey, String plainText)
            throws Exception {

        PublicKey key = getPublicKey(publicKey.getBytes("UTF-8"));
        byte[] plainByte = plainText.getBytes("UTF-8");
        byte[] enBytes = encrypt(key, plainByte);
        return new String(enBytes, "UTF-8");
    }

    /**
     * 解密
     *
     * @param privateKey 私钥
     * @param enText     暗文
     * @return
     */
    public static byte[] decrypt(PrivateKey privateKey, byte[] enText)
            throws Exception {

        Cipher cipher = null;
        byte[] deBytes = null;
        cipher = Cipher.getInstance(TRANSFORMATION);
        cipher.init(Cipher.DECRYPT_MODE, privateKey);
        deBytes = cipher.doFinal(enText);
        return deBytes;
    }

    /**
     * 解密
     *
     * @param modulus         即n
     * @param privateExponent 即d
     * @param pRadix          modulus和privateExponent的进制
     * @param enText          密文
     * @return
     * @throws Exception
     */
    public static byte[] decrypt(String modulus, String privateExponent,
                                 int radix, byte[] enText) throws Exception {
        PrivateKey privateKey = getPrivateKey(modulus, radix, privateExponent,
                radix);
        byte[] deBytes = decrypt(privateKey, enText);
        return deBytes;
    }

    /**
     * 解密
     *
     * @param privateKey 私钥
     * @param enText     暗文
     * @return
     */
    public static String decrypt(String privateKey, String enText)
            throws Exception {

        PrivateKey key = getPrivateKey(privateKey.getBytes("UTF-8"));
        byte[] enByte = enText.getBytes("UTF-8");
        byte[] deBytes = decrypt(key, enByte);
        return new String(deBytes, "UTF-8");
    }

    /**
     * 测试用例，按此方法使用RSAHelper
     */
    public static void test() {
        Log.d(TAG, "********************RSA Test*********************");
        try {
            String ALGORITHM = "RSA";
            String TRANSFORMATION = "RSA/ECB/NoPadding";
            // 请务必先调用init初始化
            RSAHelper.init(ALGORITHM, TRANSFORMATION);
            // 产生密钥对
            KeyPair keyPair = RSAHelper.genKeyPair(1024);
            // 公钥
            PublicKey publicKey = RSAHelper.getPublicKey(keyPair);
            // 私钥
            PrivateKey privateKey = RSAHelper.getPrivateKey(keyPair);
            byte[] publicKeyBytes = RSAHelper.getKeyString(publicKey);
            byte[] privateKeyBytes = RSAHelper.getKeyString(privateKey);
            // 打印公钥和私钥
            Log.d(TAG,
                    "publickey : "
                            + Base64.encodeToString(publicKeyBytes,
                            Base64.DEFAULT)
            );
            Log.d(TAG,
                    "privateKey : "
                            + Base64.encodeToString(privateKeyBytes,
                            Base64.DEFAULT)
            );
            // 明文
            byte[] plainText = "我们都很好！邮件：@sina.com".getBytes("UTF-8");
            // 加密
            byte[] enBytes = RSAHelper.encrypt(publicKey, plainText);
            // 通过密钥字符串得到密钥
            publicKey = RSAHelper.getPublicKey(publicKeyBytes);
            privateKey = RSAHelper.getPrivateKey(privateKeyBytes);
            // 解密
            byte[] deBytes = RSAHelper.decrypt(privateKey, enBytes);
            Log.d(TAG, "decrypt text : " + new String(deBytes, "UTF-8"));
            publicKeyBytes = RSAHelper.getKeyString(publicKey);
            privateKeyBytes = RSAHelper.getKeyString(privateKey);
            Log.d(TAG,
                    "publickey 2 : "
                            + Base64.encodeToString(publicKeyBytes,
                            Base64.DEFAULT)
            );
            Log.d(TAG,
                    "privateKey 2 : "
                            + Base64.encodeToString(privateKeyBytes,
                            Base64.DEFAULT)
            );

        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }
}
