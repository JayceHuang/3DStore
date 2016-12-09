package com.runmit.sweedee.model;


import android.os.Parcel;
import android.os.Parcelable;

public class PayStyle implements Parcelable{

	public int id;
	
	public String payment;
	
	public int currencyId;
	
	 private PayStyle(Parcel in) {
	        this.id = in.readInt();
	        this.payment = in.readString();
	        this.currencyId = in.readInt();
	    }

	    public static final Parcelable.Creator<PayStyle> CREATOR = new Parcelable.Creator<PayStyle>() {
	        public PayStyle createFromParcel(Parcel in) {
	            return new PayStyle(in);
	        }

	        public PayStyle[] newArray(int size) {
	            return new PayStyle[size];
	        }
	    };

	    @Override
	    public int describeContents() {
	        // TODO Auto-generated method stub
	        return 0;
	    }

	    @Override
	    public void writeToParcel(Parcel dest, int flags) {
	        dest.writeInt(id);
	        dest.writeString(payment);
	        dest.writeInt(currencyId);
	    }

	    

	@Override
	public String toString() {
		return "PayStyle [id=" + id + ", payment=" + payment + ", currencyId=" + currencyId + "]";
	}
	
	

}
