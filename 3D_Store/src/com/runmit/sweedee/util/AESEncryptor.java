package com.runmit.sweedee.util;

import javax.crypto.Cipher;
import javax.crypto.spec.SecretKeySpec;

/**
 * AES加密器
 *
 * @author Eric_Ni
 */
public class AESEncryptor {

    private final static String AES_KEY = "41227677";

    // AES加密,用getRawKey生成Key
//    public static String encryptString(String cleartext) throws Exception { 
//        byte[] rawKey = getRawKey();  
//        byte[] result = encrypt(rawKey, cleartext.getBytes());
//        return toHex(result);  
//    }  
//      
//    // AES解密,用getRawKey生成Key
//    public static String decryptString(String encrypted) throws Exception {  
//        byte[] rawKey = getRawKey();          
//        byte[] enc = toByte(encrypted);  
//        byte[] result = decrypt(rawKey, enc);
//        return new String(result);  
//    }  

    // AES加密,用getMD5Byte生成Key
    public static String encryptStringMD5(String cleartext) throws Exception {
        byte[] key = Util.getMD5Byte(getAccountKey().getBytes("utf-8"));
        byte[] result = encrypt(key, cleartext.getBytes());
        return toHex(result);
    }

    private static String getAccountKey() {
        return AES_KEY;
    }

    // AES解密,用getMD5Byte生成Key
    public static String decryptStringMD5(String encrypted) throws Exception {
        byte[] key = Util.getMD5Byte(getAccountKey().getBytes("utf-8"));
        byte[] enc = toByte(encrypted);
        byte[] result = decrypt(key, enc);
        return new String(result);
    }

//    private static byte[] getRawKey() throws Exception {  
//    	byte[] seed = AES_KEY.getBytes();
//        KeyGenerator kgen = KeyGenerator.getInstance("AES");  
//        SecureRandom sr = SecureRandom.getInstance("SHA1PRNG");  
//        sr.setSeed(seed);  
//        kgen.init(128, sr); // 192 and 256 bits may not be available  
//        SecretKey skey = kgen.generateKey();  
//        byte[] raw = skey.getEncoded();  
//        return raw;  
//    }  


    public static byte[] encrypt(byte[] raw, byte[] clear) throws Exception {
        SecretKeySpec skeySpec = new SecretKeySpec(raw, "AES");
        Cipher cipher = Cipher.getInstance("AES");
        cipher.init(Cipher.ENCRYPT_MODE, skeySpec);
        byte[] encrypted = cipher.doFinal(clear);
        return encrypted;
    }

    public static byte[] decrypt(byte[] raw, byte[] encrypted) throws Exception {
        SecretKeySpec skeySpec = new SecretKeySpec(raw, "AES");
        Cipher cipher = Cipher.getInstance("AES");
        cipher.init(Cipher.DECRYPT_MODE, skeySpec);
        byte[] decrypted = cipher.doFinal(encrypted);
        return decrypted;
    }

    public static String toHex(String txt) {
        return toHex(txt.getBytes());
    }

    public static String fromHex(String hex) {
        return new String(toByte(hex));
    }

    public static byte[] toByte(String hexString) {
        int len = hexString.length() / 2;
        byte[] result = new byte[len];
        for (int i = 0; i < len; i++)
            result[i] = Integer.valueOf(hexString.substring(2 * i, 2 * i + 2), 16).byteValue();
        return result;
    }

    public static String toHex(byte[] buf) {
        if (buf == null)
            return "";
        StringBuffer result = new StringBuffer(2 * buf.length);
        for (int i = 0; i < buf.length; i++) {
            appendHex(result, buf[i]);
        }
        return result.toString();
    }

    private final static String HEX = "0123456789ABCDEF";

    private static void appendHex(StringBuffer sb, byte b) {
        sb.append(HEX.charAt((b >> 4) & 0x0f)).append(HEX.charAt(b & 0x0f));
    }

//    private static byte[] getMD5Byte(byte[] source) {
//		MessageDigest md5 = null;
//		try {
//			md5 = MessageDigest.getInstance("MD5");
//		} catch (NoSuchAlgorithmException e) {
//			e.printStackTrace();
//		}
//		md5.update(source);
//		return md5.digest();
//	}
}
