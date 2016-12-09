package com.runmit.sweedee.util;

import java.security.SecureRandom;

import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;

/**
 * AES加密解密算法，使用方法按test()来就可以
 * 重要：由于此类内部实现全部为static，所以使用此类前请务必调用init函数执行初始化，以防止相互干扰。
 * 且此类是非线程安全的，若有线程安全的需求，把此类的static去掉，改为非static实现
 *
 * @author Roger
 */
public class AESHelper {

    private static final String TAG = AESHelper.class.getName();

    private static String ALGORITHM = "AES";                                // "AES"
    private static String CIPHER_TRANSFORMATION = "AES/ECB/PKCS5Padding";    // "AES/ECB/NoPadding"
    // "AES/ECB/PKCS1Padding"
    // "AES/None/PKCS1Padding"
    private static String RANDOM_TRANSFORMATION = "SHA1PRNG";                // 当使用getRawKey函数时，最好初始化RANDOM_TRANSFORMATION


    /**
     * 初始化algorithm和transformation，如果某个参数为空，则用默认值
     * 重要：请务必在使用此类前调用此方法
     *
     * @param algorithm            默认"AES"
     * @param cipherTransformation 默认"AES/ECB/PKCS5Padding"
     * @param randomTransformation 默认"SHA1PRNG"
     */
    public static void init(String algorithm, String cipherTransformation, String randomTransformation) {
        if (TextUtils.isEmpty(algorithm)) {
            // 默认值
            ALGORITHM = "AES";
        } else {
            ALGORITHM = algorithm;
        }
        if (TextUtils.isEmpty(cipherTransformation)) {
            // 默认值
            CIPHER_TRANSFORMATION = "AES/ECB/PKCS5Padding";
        } else {
            CIPHER_TRANSFORMATION = cipherTransformation;
        }
        if (TextUtils.isEmpty(randomTransformation)) {
            // 默认值
            RANDOM_TRANSFORMATION = "SHA1PRNG";
        } else {
            RANDOM_TRANSFORMATION = randomTransformation;
        }

    }

    /**
     * 初始化algorithm和transformation，如果某个参数为空，则用默认值
     * 重要：请务必在使用此类前调用此方法，
     * RANDOM_TRANSFORMATION使用了默认值
     *
     * @param algorithm            默认"AES/ECB/PKCS5Padding" "
     * @param cipherTransformation 默认"AES
     */
    public static void init(String algorithm, String cipherTransformation) {
        init(algorithm, cipherTransformation, null);
    }

    /**
     * 根据seed获得密钥 调用此方法前，请使用init(String, String, String)初始化RANDOM_TRANSFORMATION
     *
     * @param keysize 128, 192, 256. the size of the key (in bits), (192 and 256 bits may not be available, hav’ent test)
     * @param seed
     * @return
     * @throws Exception
     */
    public static byte[] getRawKey(int keysize, byte[] seed) throws Exception {
        KeyGenerator kgen = KeyGenerator.getInstance(ALGORITHM);
        SecureRandom sr = SecureRandom.getInstance(RANDOM_TRANSFORMATION);
        sr.setSeed(seed);
        kgen.init(keysize, sr); // 128 OK, 192 and 256 bits may not be available
        SecretKey skey = kgen.generateKey();
        byte[] raw = skey.getEncoded();
        return raw;
    }

    /**
     * 加密，根据密钥加密
     *
     * @param rawKey     密钥
     * @param plainBytes 明文
     * @return
     * @throws Exception
     */
    public static byte[] encrypt(byte[] rawKey, byte[] plainBytes)
            throws Exception {
        SecretKeySpec skeySpec = new SecretKeySpec(rawKey, ALGORITHM);
        Cipher cipher = Cipher.getInstance(CIPHER_TRANSFORMATION);
        cipher.init(Cipher.ENCRYPT_MODE, skeySpec);
        byte[] encrypted = cipher.doFinal(plainBytes);
        return encrypted;
    }

    /**
     * 解密，根据密钥解密
     *
     * @param rawKey
     * @param enBytes
     * @return
     * @throws Exception
     */
    public static byte[] decrypt(byte[] rawKey, byte[] enBytes)
            throws Exception {
        SecretKeySpec skeySpec = new SecretKeySpec(rawKey, ALGORITHM);
        Cipher cipher = Cipher.getInstance(CIPHER_TRANSFORMATION);
        cipher.init(Cipher.DECRYPT_MODE, skeySpec);
        byte[] decrypted = cipher.doFinal(enBytes);
        return decrypted;
    }

    /**
     * 测试用列，按此方法使用AESHelper
     */
    public static void test() {
        Log.d(TAG, "********************AES Test*********************");
        String ALGORITHM = "AES";
        String CIPHER_TRANSFORMATION = "AES/ECB/PKCS5Padding";
        // 请务必先调用init初始化
        AESHelper.init(ALGORITHM, CIPHER_TRANSFORMATION);

        String masterKey = "ab1231245adfasdf";
        String originalText = "0123456789asdfasdfa1234523@#@#$%";
        try {
            byte[] encryptingCode = AESHelper
                    .encrypt(masterKey.getBytes("UTF-8"),
                            originalText.getBytes("UTF-8"));
            Log.d(TAG,
                    "加密结果为 : "
                            + Base64.encodeToString(encryptingCode,
                            Base64.DEFAULT)
            );
            byte[] decryptingCode = AESHelper.decrypt(
                    masterKey.getBytes("UTF-8"), encryptingCode);
            Log.d(TAG, "解密结果 : " + new String(decryptingCode, "UTF-8"));
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }
}
