package com.runmit.sweedee.model;

import java.io.Serializable;

/***
 * @author Sven.Zhan
 * 商品权限VO，即是否已购买
 */
public class VOPermission implements Serializable{
    
    /**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public long uid;
    
    public int productId;
    
    public int productType;
    
    public boolean valid;
}
