/**
 *
 */
package com.runmit.sweedee.model;

import java.io.Serializable;

/**
 * @author Sven.Zhan
 *         CMS提供的影片评论信息
 */
public class CmsVedioComment implements Serializable{

	private static final long serialVersionUID = 1L;
	
    public String title;
    
    public String language;

    public String commentDate;
    
    public int albumId;
}
