package com.runmit.sweedee.player;

public class PlayerConstant {

    public static final int PLAYER_UPDATE_PROGRESS = 101;//更新进度

    public static final String INTENT_PLAY_FILE_NAME = "play_file_name";
    public static final String INTENT_PLAY_FILE_SIZE = "play_file_size";
    public static final String INTENT_PLAY_MODE = "play_mode";
    public static final String INTENT_PLAY_SUBTITLE="subtitleinfo";//字幕信息
    public static final String INTENT_PLAY_POSTER = "poster";//图片地址
    public static final String INTENT_PLAY_ALBUMID="albumid";//
    public static final String INTENT_PLAY_VIDEOID="videoid";
    public static final String INTENT_NEED_LOGIN="need_login";//此次播放是否需要登录，以此区分正片播放和片花播放
    public static final String PLAY_URL_LISTINFO="play_url_listinfo";//在线播放传过去的GSLBUrlInfo key
    public static final String INTENT_VIDEO_SOURCE_MODE="mode";//视频上下左右格式
    
    //播放模式
    public static final int LOCAL_PLAY_MODE = 0;//本地路径播放
    public static final int ONLINE_PLAY_MODE = LOCAL_PLAY_MODE + 1;//在线点播
    public static final int TRAILER_PLAY_MODE = ONLINE_PLAY_MODE + 1;//试播影片点播
    
    public static final String LOCAL_PLAY_PATH = "local_play_path";


    public class VideoPlayerStatus {


        //public static final int PLAYER_SWITCH = PLAYER_INITING + 1;

        public static final int PLAYER_NOTIFY_DURATION = PLAYER_UPDATE_PROGRESS + 1;
        public static final int PLAYER_SEEKTO = PLAYER_NOTIFY_DURATION + 1;
        public static final int PLAYER_SHOW_SCREEN_MODE = PLAYER_SEEKTO + 1;


        public static final int PLAYER_SHOW_CONTROL_BAR = 40;
        public static final int PLAYER_HIDE_CONTROL_BAR = PLAYER_SHOW_CONTROL_BAR + 1;

        public static final int PLAYER_SHOW_WAITING_BAR = PLAYER_HIDE_CONTROL_BAR + 1;
        public static final int PLAYER_HIDE_WAITING_BAR = PLAYER_SHOW_WAITING_BAR + 1;

        public static final int PLAYER_SHOW_MODE_BAR = PLAYER_HIDE_WAITING_BAR + 1;
        public static final int PLAYER_HIDE_MODE_BAR = PLAYER_SHOW_MODE_BAR + 1;

        public static final int PLAYER_SHOW_SCREEN_BAR = PLAYER_HIDE_MODE_BAR + 1;
        public static final int PLAYER_HIDE_SCREEN_BAR = PLAYER_SHOW_SCREEN_BAR + 1;

        public static final int PLAYER_SET_MOVIE_TITLE = PLAYER_HIDE_SCREEN_BAR + 1;
    }

    public class ScreenMode {
        public static final int ORIGIN_SIZE = 0;
        public static final int SUITABLE_SIZE = ORIGIN_SIZE + 1;
        public static final int FILL_SIZE = SUITABLE_SIZE + 1;
    }


}
