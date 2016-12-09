package com.runmit.sweedee.action.movie;

public class FilterPara {
    public boolean isSelected;
    public String title;
    public String urlParamList;

    public FilterPara(String title, String urlParamList) {
        this.isSelected = false;
        this.title = title;
        this.urlParamList = urlParamList;
    }

    public FilterPara(boolean isSelected, String title, String urlParamList) {
        this.isSelected = isSelected;
        this.title = title;
        this.urlParamList = urlParamList;
    }
}
