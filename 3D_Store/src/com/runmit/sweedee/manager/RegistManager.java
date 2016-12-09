/**
 * RegistManager.java 
 * 注册接口管理类，包含注册和获取短信验证码
 * @author: zhang_zhi
 */
package com.runmit.sweedee.manager;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.http.Header;
import org.apache.http.entity.ByteArrayEntity;

import android.os.Handler;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.runmit.sweedee.player.ExceptionCode;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.report.sdk.ReportEventCode.UcReportCode;
import com.runmit.sweedee.sdk.user.member.task.UserErrorCode;
import com.runmit.sweedee.sdk.user.member.task.UserUtil;
import com.runmit.sweedee.sdkex.httpclient.LoginAsyncHttpProxyListener;
import com.runmit.sweedee.util.MD5;
import com.runmit.sweedee.util.XL_Log;

/**
 *
 */
public class RegistManager {

    private XL_Log log = new XL_Log(RegistManager.class);
    // 交互url
    private final String url_register = "https://zeus.d3dstore.com/v1.5/register";// 注册post
    
    private final String url_verifymobile= "https://zeus.d3dstore.com/v1.5/sendVerifyCode";

    // 客户端产生的错误码
    public static final int ERROR_DEFAULT = -1;
    public static final int ERROR_CONTAIN_CHINESE = -3;
    
    public static final int ERROR_JSON=1000;

    // 账号类型
    public static final int TYPE_ACCONT_EMAIL = 1;
    public static final int TYPE_ACCONT_TELEPHONE = 2;

    // 回调消息类型
    public static final int MSG_REGISTER = 100;
    public static final int MSG_GET_VERIFYCODE = 101;
    public static final int MSG_CHECK_ACCOUNT = 102;
    public static final int MSG_CHECK_PASSWORD = 12536;
//
    public static final int REG_ACCOUT_ERROR_ACTIVE_FAIL = 3;//注册成功,激活邮件发送失败
    public static final int REG_ACCOUT_ERROR_VERIFYCODETIMEOUT = 9;//9-验证码超时
    public static final int REG_ACCOUT_ERROR_DEVICE_LIMIT = 11;//设备注册用户数超过限制
    public static final int REG_ACCOUT_ERROR_BIND_ERROR = 14;//绑定设备错误
    
    public static final int REG_ACCOUT_ERROR_EMAIL_INVALIDATE = 20002;//邮箱输入不正确，请重新输入
    public static final int REG_ACCOUT_ERROR_EMAIL_EXIT = 20001;//该邮箱已被注册，请选择其他邮箱注册
    public static final int REG_ACCOUT_ERROR_VERIFYCODE_ERROR = 20006;//验证码错误
    public static final int REG_ACCOUNT_ERROR_PHONE_EXIT = 20003;//该手机号已被注册，请选择其他号码注册

    // 注册服务器返回码-全局
    public static final int SUCCESS = 0;// 成功

    public static RegistManager getInstance() {
        return new RegistManager();
    }

    public RegistManager() {
    }

    /**
     * @param account_type
     * @param account
     * @param password
     * @param verify_code
     * @param mHandler
     */
    public void startRegister(final int sexy,int account_type, String account, String password, String verify_code, String nickname, String location ,final Handler mHandler) {
        if (account == null || account.length() == 0) {
            mHandler.obtainMessage(MSG_REGISTER, UserErrorCode.ACCOUNT_INVALID, 0).sendToTarget();
            return;
        }
        String sRequPwd = MD5.encrypt(password);
        if (sRequPwd.compareTo(MD5.encrypt("")) == 0) {
            mHandler.obtainMessage(MSG_REGISTER, UserErrorCode.PASSWORD_ERROR, 0).sendToTarget();
            return;
        }
        JsonObject jsonRequObj = new JsonObject();
        jsonRequObj.addProperty("useridtype", account_type);
        jsonRequObj.addProperty("account", account);
        jsonRequObj.addProperty("gender", Integer.toString(sexy));
        jsonRequObj.addProperty("password", sRequPwd);// 一次MD5后
        jsonRequObj.addProperty("verifycode", verify_code);
        jsonRequObj.addProperty("nickname", nickname);
        jsonRequObj.addProperty("location", location);

        String jsonContent = jsonRequObj.toString();
        ByteArrayEntity byteEntity = new ByteArrayEntity(jsonContent.getBytes());

        log.debug("execute jsonContent=" + jsonContent);
        UserUtil.getInstance().getHttpProxy().post(url_register, byteEntity, new LoginAsyncHttpProxyListener() {
            public void onSuccess(int responseCode, Header[] headers, String result) {
                log.debug("Login onSuccess result=" + result);

                JsonObject jsonRespObj = new JsonParser().parse(result).getAsJsonObject();
                int errorCode = jsonRespObj.get("rtn").getAsInt();
                int userid = 0;
                if (errorCode == UserErrorCode.SUCCESS) {
                    userid = jsonRespObj.get("userid").getAsInt();
                }
                ReportActionSender.getInstance().regist(errorCode,userid);
                ReportActionSender.getInstance().error(ReportActionSender.MODULE_UC, UcReportCode.UC_REPORT_REGISTER, errorCode);
                
                mHandler.obtainMessage(MSG_REGISTER, errorCode, userid).sendToTarget();
            }

            public void onFailure(Throwable error) {
                error.printStackTrace();
                int errorCode = ExceptionCode.getErrorCode(error);
                ReportActionSender.getInstance().regist(errorCode,0);
                ReportActionSender.getInstance().error(ReportActionSender.MODULE_UC, UcReportCode.UC_REPORT_REGISTER, errorCode);
                mHandler.obtainMessage(MSG_REGISTER, errorCode, 0).sendToTarget();
            }
        });
    }

    /**
     * 请求获取验证码
     *
     * @param phonenumber
     * @param mHandler
     */
    public void getVerifyCode(String phonenumber,String countrycode, final Handler mHandler) {
        if (phonenumber == null || phonenumber.length() == 0) {
            mHandler.obtainMessage(MSG_GET_VERIFYCODE, UserErrorCode.ACCOUNT_INVALID, 0).sendToTarget();
            return;
        }
        JsonObject jsonRequObj = new JsonObject();
        jsonRequObj.addProperty("phonenumber", phonenumber);
        jsonRequObj.addProperty("countrycode", countrycode);
    
        String jsonContent = jsonRequObj.toString();
        ByteArrayEntity byteEntity = new ByteArrayEntity(jsonContent.getBytes());
        log.debug("execute jsonContent=" + jsonContent);
        UserUtil.getInstance().getHttpProxy().post(url_verifymobile, byteEntity, new LoginAsyncHttpProxyListener() {
            public void onSuccess(int responseCode, Header[] headers, String result) {
                log.debug("Login onSuccess result=" + result);
                JsonObject jsonRespObj = new JsonParser().parse(result).getAsJsonObject();
                int errorCode = jsonRespObj.get("rtn").getAsInt();
                ReportActionSender.getInstance().error(ReportActionSender.MODULE_UC, UcReportCode.UC_REPORT_GET_AUTHCODE, errorCode);
                mHandler.obtainMessage(MSG_GET_VERIFYCODE, errorCode, 0).sendToTarget();
            }

            public void onFailure(Throwable error) {
            	log.debug("onFailure error =" +error.getMessage());
                error.printStackTrace();
                int errorCode = ExceptionCode.getErrorCode(error);
                ReportActionSender.getInstance().error(ReportActionSender.MODULE_UC, UcReportCode.UC_REPORT_GET_AUTHCODE, errorCode);
                mHandler.obtainMessage(MSG_GET_VERIFYCODE, errorCode, 0).sendToTarget();
            }
        });
    }

    // 检测账号类型：邮箱|手机号码,可同步调用
    public int checkAccountType(String account_string) {
        if (account_string == null || "".equals(account_string)) {
            return ERROR_DEFAULT;
        }
        if (isContainChinese(account_string)) {
            return ERROR_CONTAIN_CHINESE;
        }
        String email_check = "\\w+([-+.]\\w+)*@\\w+([-.]\\w+)*\\.\\w+([-.]\\w+)*";
        String phone_check = "^[1][3,5,8]+\\d{9}";
        Pattern regex = Pattern.compile(email_check);
        Matcher matcher = regex.matcher(account_string);
        if (matcher.matches()) {
            return TYPE_ACCONT_EMAIL;
        } else {
            regex = Pattern.compile(phone_check);
            matcher = regex.matcher(account_string);
            if (matcher.matches()) {
                return TYPE_ACCONT_TELEPHONE;
            } else {
                return ERROR_DEFAULT;
            }
        }
    }

    // 检查一串字符中是否含有中文
    public boolean isContainChinese(String string) {
        boolean res = false;
        char[] cTemp = string.toCharArray();
        for (int i = 0; i < string.length(); i++) {
            if (isChinese(cTemp[i])) {
                res = true;
                break;
            }
        }
        return res;
    }

    // 判定是否是中文字符
    private boolean isChinese(char c) {
        Character.UnicodeBlock ub = Character.UnicodeBlock.of(c);
        if (ub == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS || ub == Character.UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS || ub == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A
                || ub == Character.UnicodeBlock.GENERAL_PUNCTUATION || ub == Character.UnicodeBlock.CJK_SYMBOLS_AND_PUNCTUATION || ub == Character.UnicodeBlock.HALFWIDTH_AND_FULLWIDTH_FORMS) {
            return true;
        }
        return false;
    }

}
