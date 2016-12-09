package com.runmit.sweedee.model;

import java.io.Serializable;

public class Quota implements Serializable{
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public int amount;
	
	public int currencyId;
	
	public String symbol;
	
	public String toString(){
		if(symbol == null){
			return Float.toString(amount/100f);
		}else{
			return symbol+amount/100f;
		}
	}
}
