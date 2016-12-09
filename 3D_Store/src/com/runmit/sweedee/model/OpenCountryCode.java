package com.runmit.sweedee.model;

import java.io.Serializable;
import java.util.List;

public class OpenCountryCode implements Serializable{
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	int rtn;
	
	String rtnmsg;
	
	public List<CountryInfo> data;
	
	public class CountryInfo implements Serializable{
		/**
		 * 
		 */
		private static final long serialVersionUID = 1L;
		
		public String name;
		public String countrycode;
		public String locale;
		public String language;
	}
}
