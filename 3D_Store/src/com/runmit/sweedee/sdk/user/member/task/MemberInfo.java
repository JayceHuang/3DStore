package com.runmit.sweedee.sdk.user.member.task;

import java.io.Serializable;

import org.json.JSONException;
import org.json.JSONObject;

import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;

public class MemberInfo implements Serializable {

    /**
     * 以name{description,type,value}格式来描述USERINFOKEY里面的字段
     * name：字段名 、description：字段描述、type：字段类型、value：字段值域
     * UserName{用户名,String,}
     * EncryptedPassword{一次MD5加密后密码,String,}
     * PasswordCheckNum{用户标识码,String,}
     * UserID{账号内部ID,int,}
     * IsVip{是不是会员,int,0:否 1:是}
     * VasType{会员类型,int,2:普通会员  3:白金会员 4:钻石会员}
     * UserNewNo{用户新数字ID,int,}
     * NickName{用户昵称,String utf8编码,}
     * SessionID{会话ID,String,}
     * PayId{会员支付类型,int,3 15:手机支付  年费支付:payid<=200 && 0==payid%5 && 0!=payid%10  >1000:体验会员}
     * PayName{会员支付途径,String,手机支付 其他}
     * ExpireDate{会员过期时间,String,}
     * isExpVip{是否是体验会员,int,0:否 1:是}
     * Account{用户经验值,int,}
     * JumpKey{直接登录web页面的jumpkey,String,}
     * IsSubAccount{是否是企业子账号,int,0:否 1:是}
     * Rank{军衔,int,}
     * Order{排名,int,}
     * IsSpecialNum{是否是靓号,int,0:否 1:是}
     * AllowScore{当天还允许增加分数,int,}
     * TodayScore{当天得分,int,}
     * Country{国家索引,String,}
     * Province{省份索引,String,}
     * City{城市索引,String,}
     * Role{角色,int,}
     * IsAutoDeduct{是否自动续费,int,0:否 1:是}
     * IsRemind{是否提醒,int,0:否 1:是}
     * PersonalSign{个性签名,String,}
     * VipGrow{vip成长值,int,}
     * VipDayGrow{vip当天成长值,int,}
     * ImgURL{获取头像url,String,}
     * vip_level{会员等级,int,}
     *
     * @author lzl
     */
    public enum USERINFOKEY {
        UserName, EncryptedPassword, PasswordCheckNum, UserID, UserNewNo, IsSubAccount,
        NickName, Account, SessionID, JumpKey, IsVip, Rank, Order,
        ExpireDate, VasType, PayId, PayName, isExpVip,
        Country, Province, City, IsSpecialNum, Role, IsAutoDeduct, IsRemind,
        TodayScore, AllowScore, PersonalSign, VipGrow, VipDayGrow, ImgURL, vip_level
    }

    ;

    /**
     *
     */
    private static final long serialVersionUID = 1L;

    //////////////////////////////////
    ////遗留字段需要删除
    public String local_icon_url;
    /////////////////////////////////

    public long userid;
    public String token;
    public int viplevel;
    public String account;
    public String name;
    public String nickname;
    public String email;
    public String occupation;//职业
    public String address;//地址
    public String location;//地区
    public String avatar;//头像
    public String age;
    public String gender;
    public String mobile;

    public MemberInfo() {

    }
    
    public void updateMemberInfo(MemberInfo mMemberInfo){
    	 this.viplevel=mMemberInfo.viplevel;
    	 this.account=mMemberInfo.account;
    	 this.name=mMemberInfo.name;
    	 this.nickname=mMemberInfo.nickname;
    	 this.email=mMemberInfo.email;
    	 this.occupation=mMemberInfo.occupation;
    	 this.address=mMemberInfo.address;
    	 this.location=mMemberInfo.location;
    	 this.avatar=mMemberInfo.avatar;
    	 this.age=mMemberInfo.age;
    	 this.gender=mMemberInfo.gender;
    	 this.mobile=mMemberInfo.mobile;
    }

    public static MemberInfo newInstance(String json) {
        if (json == null || json.length() == 0) {
            return null;
        }

        Gson gson = new Gson();
        MemberInfo obj = null;
        try {
            obj = gson.fromJson(json, MemberInfo.class);
        } catch (JsonSyntaxException e) {
            e.printStackTrace();
            return null;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        return obj;
    }

    public MemberInfo(JSONObject userJson) {
        try {
            this.userid = userJson.getLong("userid");
            this.account = userJson.getString("account");
            this.email = userJson.getString("mail");

            if (!userJson.isNull("name")) {
                this.name = userJson.getString("name");
            }
            if (!userJson.isNull("nickname")) {
                this.nickname = userJson.getString("nickname");
            }
            if (!userJson.isNull("occupation")) {
                this.occupation = userJson.getString("occupation");
            }
            if (!userJson.isNull("address")) {
                this.address = userJson.getString("address");
            }
            if (!userJson.isNull("location")) {
                this.location = userJson.getString("location");
            }
            if (!userJson.isNull("avatar")) {
                this.avatar = userJson.getString("avatar");
            }
            if (!userJson.isNull("age")) {
                this.age = userJson.getString("age");
            }
            if (!userJson.isNull("gender")) {
                this.gender = userJson.getString("gender");
            }
            if (!userJson.isNull("mobile")) {
                this.mobile = userJson.getString("mobile");
            }
            if (!userJson.isNull("viplevel")) {
                this.viplevel = userJson.getInt("viplevel");
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    @Override
    public String toString() {
        return "MemberInfo [userid=" + userid + ", token=" + token + ", viplevel=" + viplevel + ", account="
                + account + ", name=" + name + ", nickname=" + nickname + ", email=" + email + ", occupation=" + occupation + ", address=" + address + ", location=" + location + ", avatar=" + avatar
                + ", age=" + age + ", gender=" + gender + ", mobile=" + mobile + "]";
    }


}
