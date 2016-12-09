/**
 * 
 */
package com.runmit.sweedee.player;

import java.util.ArrayList;
import java.util.List;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * @author Sven.Zhan
 * 真实播放地址的集合
 */
public class RealPlayUrlInfo implements Parcelable{

	private List<String> P720List;
	
	private List<String> P1080List;

	public RealPlayUrlInfo(List<String> p720List, List<String> p1080List) {
		super();
		P720List = p720List;
		P1080List = p1080List;
	}
	
	public boolean has720Redition(){
		return !(P720List == null || P720List.isEmpty());
	}
	
	public boolean has1080Redition(){
		return !(P1080List == null || P1080List.isEmpty());
	}

	public List<String> getP720List() {
		return P720List;
	}

	public List<String> getP1080List() {
		return P1080List;
	}
	
	public boolean isEmpty(){
		return !has720Redition() && !has1080Redition();
	}

	public String getCDNUrlForApp(){
		if(has720Redition()){
			return P720List.get(0);
		}else{
			return null;
		}
	}
	
	@Override
	public String toString() {
		return "RealPlayUrlInfo [P720List=" + P720List + ", P1080List=" + P1080List + "]";
	}
	
	
	public RealPlayUrlInfo() {
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public static final Parcelable.Creator<RealPlayUrlInfo> CREATOR = new Parcelable.Creator<RealPlayUrlInfo>() {
        public RealPlayUrlInfo createFromParcel(Parcel src) {
            return new RealPlayUrlInfo(src);
        }

        public RealPlayUrlInfo[] newArray(int size) {
            return new RealPlayUrlInfo[size];
        }
    };

    public RealPlayUrlInfo(Parcel src) {
        readFromParcel(src);
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeList(P1080List);
        dest.writeList(P720List);
    }

    public void readFromParcel(Parcel src) {
    	P1080List=new ArrayList<String>();
    	src.readList(P1080List, String.class.getClassLoader());
    	P720List=new ArrayList<String>();
    	src.readList(P720List, String.class.getClassLoader());
    }
	
}
