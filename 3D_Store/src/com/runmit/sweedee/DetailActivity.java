package com.runmit.sweedee;

import java.util.ArrayList;

import org.json.JSONArray;
import org.json.JSONException;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.AbsoluteSizeSpan;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.display.SimpleBitmapDisplayer;
import com.runmit.sweedee.action.movie.DetailAboutFragment;
import com.runmit.sweedee.action.movie.DetailCommentFragment;
import com.runmit.sweedee.action.movie.DetailIntroFragment;
import com.runmit.sweedee.action.search.Activity3DEffectTool;
import com.runmit.sweedee.action.search.DefaultEffectFaces;
import com.runmit.sweedee.action.search.FlipViewGroup;
import com.runmit.sweedee.constant.ServiceArg;
import com.runmit.sweedee.imageloaderex.TDImagePlayOptionBuilder;
import com.runmit.sweedee.imageloaderex.TDImageWrap;
import com.runmit.sweedee.manager.AccountServiceManager;
import com.runmit.sweedee.manager.CmsServiceManager;
import com.runmit.sweedee.manager.PlayerEntranceMgr;
import com.runmit.sweedee.manager.data.EventCode;
import com.runmit.sweedee.manager.data.EventCode.AccountEventCode;
import com.runmit.sweedee.model.CmsVedioDetailInfo;
import com.runmit.sweedee.model.CmsVedioDetailInfo.Vedio;
import com.runmit.sweedee.model.VOPermission;
import com.runmit.sweedee.model.VOPrice;
import com.runmit.sweedee.model.VOPrice.PriceInfo;
import com.runmit.sweedee.sdk.user.member.task.UserManager;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.TDString;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.BlurImageView;
import com.runmit.sweedee.view.PagerSlidingTabStrip;
import com.tdviewsdk.activity.transforms.AnimActivity;
import com.tdviewsdk.activity.transforms.DetailActivityAnimTool;
import com.umeng.analytics.MobclickAgent;

/**
 * 详情页面
 */
public class DetailActivity extends AnimActivity implements View.OnClickListener, StoreApplication.OnNetWorkChangeListener {

    private static final String ALBUMID = "albumid";

    private static final String ANIM_ENABLED = "anim_enabled";
    private static final String ANIM_OPENGL_ENABLED = "anim_opengl_enabled";
    private static final String ANIM_SCALE_DATA = "anim_scale_data";

    private boolean isAnimLaunch = false;

    XL_Log log = new XL_Log(DetailActivity.class);
    private PagerSlidingTabStrip mTabs;
    private UIFragmentPageAdapter mUIFragmentPageAdapter;
    private ViewPager mViewPager;
    private Button play_btn;
    private Button download_btn;
    private DisplayMetrics dm;
    private ImageButton actionbar_back;
    private int albumid = 16;
    private VOPrice mVoPrice;
    private DetailIntroFragment introFragment;
    private DetailCommentFragment commentFragment;
    private DetailAboutFragment aboutFragment;
    private TextView movie_director;
    private TextView movie_title;
    private TextView movie_actor;
    // private TextView movie_time;
    private TextView movie_totalTime;
    private TextView movie_region;
    private TextView movie_genres;
    private TextView movie_english_origin;
    private BlurImageView mBlurImageView;
    private TextView movie_score;
    private CmsVedioDetailInfo mDetailInfo;
    private int stateCode;
    private TextView detail_movie_real_price;
    private TextView detail_movie_virtual_price;
    private LinearLayout layout_virtual_price;
    private TextView iv_detail_water;

    private boolean isReceiveData = false;// 此值用来判断服务器有没有返回数据,决定简介页是loading状态还是empty状态

    private DisplayImageOptions mOptions = new TDImagePlayOptionBuilder().setDefaultImage(R.drawable.default_movie_image).displayer(new SimpleBitmapDisplayer()).build();
    
    private VOPermission permission;

    private Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
            	case AccountEventCode.EVENT_GET_PERMISSION:
            		if(msg.arg1 == 0){
            			permission = (VOPermission) msg.obj;
            			setPriceViewInfo();
            		}
            		break;
                case AccountEventCode.EVENT_GET_PRICE:
                    int priceStateCode = msg.arg1;
                    if (priceStateCode == 0 && msg.obj != null) {
                        mVoPrice = (VOPrice) msg.obj;
                        setPriceViewInfo();
                    }
                    break;
                case EventCode.CmsEventCode.EVENT_GET_DETAIL:
                    isReceiveData = true;
                    stateCode = msg.arg1;
                    if (stateCode == 0 && msg.obj != null) {
                        if (mDetailInfo == null) {
                            mDetailInfo = (CmsVedioDetailInfo) msg.obj;
                            log.debug(" mDetailInfo= " + mDetailInfo + " stateCode= " + stateCode + " arg2= " + msg.arg2);
                            if (mDetailInfo.films != null && mDetailInfo.films.size() > 0) {
                                Vedio videoInfo = mDetailInfo.films.get(0);
                                if (videoInfo != null) {
                                    movie_totalTime.setText(String.valueOf(videoInfo.duration / (1000 * 60)) + getString(R.string.movie_minText));
                                }
                                log.debug(" videoInfo= " + videoInfo.isOriginal + " audioLanguage= " + videoInfo.audioLanguage);
                                if(videoInfo.isOriginal && "en".equals(videoInfo.audioLanguage)){
                                	movie_english_origin.setVisibility(View.VISIBLE);
                                }else{
                                	movie_english_origin.setVisibility(View.GONE);
                                }
                            }
                            // 让评分字体大小不一样
                            Spannable WordtoSpan = new SpannableString(mDetailInfo.getScore());
                            WordtoSpan.setSpan(new AbsoluteSizeSpan(30, true), 0, 2, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                            WordtoSpan.setSpan(new AbsoluteSizeSpan(25, true), 2, 3, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                            movie_score.setText(WordtoSpan);

                            movie_region.setText(mDetailInfo.region);
                            movie_title.setText(mDetailInfo.title);
                            if(mDetailInfo.uploadUser==null || mDetailInfo.uploadUser.equals("")){
                            	JsonArrayToString(movie_actor, mDetailInfo.actors, " ");
                                JsonArrayToString(movie_director, mDetailInfo.directors, " "); 	
                            }else{
                            	movie_director.setText(getString(R.string.videodetail_uploader)+mDetailInfo.uploadUser);
                            	movie_region.setVisibility(View.GONE);
                            	movie_actor.setVisibility(View.GONE);
                            }
                            JsonArrayToString(movie_genres, mDetailInfo.genres, "/");
                            String imgUrl = TDImageWrap.getVideoDetailPoster(mDetailInfo);
                            log.debug("DetailUrl=" + imgUrl);
                            ImageLoader.getInstance().displayImage(imgUrl, mBlurImageView, mOptions);
                            log.debug("DetailIntroFragment1 introDataCallBack = " + introDataCallBack);
                            if (introDataCallBack != null) {
                                introDataCallBack.onIntroSuccess(stateCode, mDetailInfo);
                            }
                            if (mDetailInfo.isFree) {
                            	setPriceViewInfo();
                            }
                        }
                    } else {
                        if (introDataCallBack != null) {
                            introDataCallBack.onIntroFailed();
                        }
                    }
                    break;
                case FlipViewGroup.MSG_ANIMATOIN_FIN:
                	Activity3DEffectTool.isLaunching = false;
                default:
            }
        }
    };

    private View actView;

    private FlipViewGroup effectView;

    private void setPriceViewInfo(){
    	if (mDetailInfo != null && mDetailInfo.isFree) {//是免费视频
    		layout_virtual_price.setVisibility(View.VISIBLE);
			detail_movie_real_price.setVisibility(View.INVISIBLE);
			detail_movie_virtual_price.setText(R.string.free_buy);
			findViewById(R.id.iv_detail_water).setVisibility(View.GONE);
        }else if(permission != null && permission.valid){
        	layout_virtual_price.setVisibility(View.VISIBLE);
			detail_movie_real_price.setVisibility(View.INVISIBLE);
			detail_movie_virtual_price.setText(R.string.has_pay);
			findViewById(R.id.iv_detail_water).setVisibility(View.GONE);
        }else if(mVoPrice != null){
        	PriceInfo mDefault = mVoPrice.getDefaultRealPriceInfo();
            log.debug("mDefault=" + mDefault);
            if (mDefault != null) {
                String realPrice = mDefault.realPrice;
                float price = Float.parseFloat(realPrice) / 100;
                if (price != 0.0f) {
                    detail_movie_real_price.setText(mDefault.symbol + price);
                } else {
                    detail_movie_real_price.setVisibility(View.INVISIBLE);
                }
            } else {
                detail_movie_real_price.setVisibility(View.INVISIBLE);
            }
            PriceInfo mVirtual = mVoPrice.getVirtualPriceInfo();
            if (mVirtual != null) {
                layout_virtual_price.setVisibility(View.VISIBLE);
                detail_movie_virtual_price.setText(mVirtual.price);
            }
        }
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
        }
        
        actView = View.inflate(this, R.layout.activity_detail, null);
        checkIfAnim();
        init();
        initViewPager();
        initData();
        StoreApplication.INSTANCE.addOnNetWorkChangeListener(this);
    }
    
    @Override
	protected void onDestroy() {
		super.onDestroy();
		Activity3DEffectTool.isLaunching = false;
	}

    private void checkIfAnim() {
        Intent intent = getIntent();
        isAnimLaunch = intent.getBooleanExtra(ANIM_ENABLED, false);
        log.debug("isAnimLaunch OLD = " + isAnimLaunch);
        /** 旧的方式 */
        if (isAnimLaunch) {
            DetailActivityAnimTool.prepareAnimation(this);
            DetailActivityAnimTool.animate(this);
        }

        isAnimLaunch = intent.getBooleanExtra(ANIM_OPENGL_ENABLED, false);

        log.debug("isAnimLaunch OPENGL = " + isAnimLaunch);
        if (isAnimLaunch) {
            DefaultEffectFaces.DefaultEffectFacesParams params = new DefaultEffectFaces.DefaultEffectFacesParams(DefaultEffectFaces.DefaultEffectFacesParams.EffectType.Scale,
                    DefaultEffectFaces.DefaultEffectFacesParams.EasyingType.Easying);
            params.scaleRange = intent.getIntArrayExtra(ANIM_SCALE_DATA);
            effectView = new FlipViewGroup(this, actView, false, mHandler, params);
            setContentView(effectView);
        } else {
            setContentView(actView);
        }
    }

    private void initData() {
        Intent intent = getIntent();
        albumid = intent.getIntExtra(ALBUMID, -1);
        if (albumid == -1) {
            Util.showToast(this, getString(R.string.no_albumid), Toast.LENGTH_SHORT);
        } else {
            AccountServiceManager.getInstance().getProductPrice(albumid, true, 3, mHandler); // movieid
            CmsServiceManager.getInstance().getMovieDetail(albumid, mHandler);// 获取详情页信息
        }
    }
    
    @Override
    protected void onResume() {
    	super.onResume();
    	if(UserManager.getInstance().isLogined()){
    		 AccountServiceManager.getInstance().getPermission(albumid, mHandler, null);
    	}
    }

    private void init() {
        mTabs = (PagerSlidingTabStrip) findViewById(R.id.detail_tabs);
        mViewPager = (ViewPager) findViewById(R.id.detail_viewPager);
        movie_title = (TextView) findViewById(R.id.tv_movie_title);
        detail_movie_real_price = (TextView) findViewById(R.id.tv_detail_movie_real_price);
        detail_movie_virtual_price = (TextView) findViewById(R.id.tv_detail_movie_virtual_price);
        layout_virtual_price = (LinearLayout) findViewById(R.id.layout_virtual_price);
        play_btn = (Button) findViewById(R.id.bt_movie_play);
        download_btn = (Button) findViewById(R.id.bt_movie_download);
        actionbar_back = (ImageButton) findViewById(R.id.btn_actionbar_back);
        movie_score = (TextView) findViewById(R.id.btn_movie_score);
        movie_director = (TextView) findViewById(R.id.tv_movie_director);
        movie_actor = (TextView) findViewById(R.id.tv_movie_actors);
        movie_genres = (TextView) findViewById(R.id.tv_movie_genres);
        movie_english_origin = (TextView) findViewById(R.id.tv_movie_english_origin);
        // movie_time = (TextView) findViewById(R.id.tv_movie_totalTime);
        movie_totalTime = (TextView) findViewById(R.id.tv_movie_totalTime);
        movie_region = (TextView) findViewById(R.id.tv_movie_country);
        mBlurImageView = (BlurImageView) findViewById(R.id.movie_icon);
        RelativeLayout source = (RelativeLayout) findViewById(R.id.rly_source);
        source.getBackground().setAlpha(60);
        // 设置背景模糊
        Bitmap loadingImg = BitmapFactory.decodeResource(getResources(), R.drawable.bg_detail_overlying);
        mBlurImageView.setBlurView(findViewById(R.id.bg_layout), loadingImg);

        play_btn.setOnClickListener(this);
        download_btn.setOnClickListener(this);
        actionbar_back.setOnClickListener(this);
    }

    private void initViewPager() {
        mUIFragmentPageAdapter = new UIFragmentPageAdapter(getSupportFragmentManager());
        mViewPager.setAdapter(mUIFragmentPageAdapter);
        mTabs.setViewPager(mViewPager);
        dm = getResources().getDisplayMetrics();
        mTabs.setDividerColorResource(R.color.background_green);
        mTabs.setIndicatorColorResource(R.color.thin_blue);
        mTabs.setSelectedTextColorResource(R.color.thin_blue);
        mTabs.setDividerColor(Color.TRANSPARENT);
        mTabs.setUnderlineHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, dm));
        mTabs.setIndicatorHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 3, dm));
        mTabs.setTextSize((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 16, dm));
        mTabs.setTextColor(getResources().getColor(R.color.home_tab_text_color));
    }

    // 假数据都为json数组，抽一个公共方法
    private void JsonArrayToString(TextView view, ArrayList<String> array, String divideString) {
        JSONArray jsonArray = new JSONArray(array);
        if (jsonArray != null) {
            for (int i = 0; i < jsonArray.length(); i++) {
                try {
                    if (i + 1 == jsonArray.length()) {
                        view.append(jsonArray.getString(i));
                    } else {
                        view.append(jsonArray.getString(i) + divideString);
                    }
                } catch (JSONException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.bt_movie_play:
                if (mDetailInfo != null && mDetailInfo.films != null && mDetailInfo.films.size() > 0) {
                    if (mVoPrice != null) {
                        mVoPrice.productName = mDetailInfo.title;
                    }

                    log.debug("bt_movie_play =" + mDetailInfo.films + mDetailInfo.films.size());
                    Vedio vedio = PlayerEntranceMgr.getVedioByAudioSetting(mDetailInfo.films);
                    String poster = TDImageWrap.getShortVideoPoster(mDetailInfo);
                    new PlayerEntranceMgr(DetailActivity.this).startgetPlayUrl(mVoPrice, vedio.mode, poster, vedio.subtitles, mDetailInfo.title, albumid, vedio.videoId, ServiceArg.URL_TYPE_PLAY, true);
                    MobclickAgent.onEvent(this, Constant.PlayClick);
                } else {
                    Toast.makeText(this, TDString.getStr(R.string.data_null_and_wait_for_server), Toast.LENGTH_SHORT).show();
                }
                break;
            case R.id.bt_movie_download:
                if (mDetailInfo != null && mDetailInfo.films != null && mDetailInfo.films.size() > 0) {
                    if (mVoPrice != null) {
                        mVoPrice.productName = mDetailInfo.title;
                    }
                    // String poster = mDetailInfo.getPosterUrl(PosterType.POSTER);
                    String poster = TDImageWrap.getShortVideoPoster(mDetailInfo);
                    Vedio vedio = PlayerEntranceMgr.getVedioByAudioSetting(mDetailInfo.films);
                    new PlayerEntranceMgr(DetailActivity.this).startgetPlayUrl(mVoPrice, vedio.mode, poster, vedio.subtitles, mDetailInfo.title, albumid, vedio.videoId, ServiceArg.URL_TYPE_DOWNLOAD, true);
                } else {
                    Toast.makeText(this, TDString.getStr(R.string.data_null_and_wait_for_server), Toast.LENGTH_SHORT).show();
                }
                break;
            case R.id.btn_actionbar_back:
                finish();
                break;
        }
    }

    boolean flag = false;

    @Override
    public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
        if (isNetWorkAviliable && !flag) {
            initData();
            flag = true;
        } else {
            flag = false;
        }
    }

    class UIFragmentPageAdapter extends FragmentStatePagerAdapter {

        private final String[] TITLES = {getString(R.string.video_intro), getString(R.string.video_comment)/*, getString(R.string.video_about)*/};

        public UIFragmentPageAdapter(FragmentManager fm) {
            super(fm);
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return TITLES[position];
        }

        @Override
        public Fragment getItem(int positon) {
            switch (positon) {
                case 0:
                    if (introFragment == null) {
                        introFragment = new DetailIntroFragment();
                    }
                    return introFragment;
                case 1:
                    if (commentFragment == null) {
                        commentFragment = new DetailCommentFragment();
                        Bundle bundle = new Bundle();
                        bundle.putInt("albumid", albumid);
                        commentFragment.setArguments(bundle);
                    }
                    return commentFragment;
                case 2:
                    if (aboutFragment == null) {
                        aboutFragment = new DetailAboutFragment();
                        Bundle bundle = new Bundle();
                        bundle.putInt("albumid", albumid);
                        aboutFragment.setArguments(bundle);
                    }
                    return aboutFragment;
            }
            return null;
        }

        @Override
        public int getCount() {
            return TITLES.length;
        }
    }

    // ---------------------------------------------------------
    public static void start(Context activity, View clickItemView, int albumnId) {
    	if (Util.isDoubleClick(2000)) {return;}
        Intent intent = new Intent(activity, DetailActivity.class);
        intent.putExtra(ALBUMID, albumnId);
        intent.putExtra(ANIM_OPENGL_ENABLED, true);
        int[] loc = new int[2];
        clickItemView.getLocationOnScreen(loc);
        int l = loc[0];
        int t = Constant.SCREEN_HEIGHT - loc[1];
        int r = l + clickItemView.getMeasuredWidth();
        int b = Constant.SCREEN_HEIGHT - (loc[1] + clickItemView.getMeasuredHeight());
        intent.putExtra(ANIM_SCALE_DATA, new int[]{l, t, r, b});
        Activity3DEffectTool.startActivity((Activity) activity, intent);
    }

    @Override
    protected boolean isFinishAnimEnable() {
        return isAnimLaunch;
    }

    private IntroDataCallBack introDataCallBack;

    /**
     * 此接口用于简介界面的数据刷新
     */
    public interface IntroDataCallBack {
        /**
         * 获取到详情页数据的回调
         *
         * @param statecode
         * @param info
         */
        public void onIntroSuccess(int statecode, CmsVedioDetailInfo info);

        /**
         * 数据获取失败的回调
         */
        public void onIntroFailed();

    }

    public void addIntroListener(IntroDataCallBack callBack) {
        introDataCallBack = callBack;
        if (mDetailInfo != null && stateCode == 0) {
            callBack.onIntroSuccess(stateCode, mDetailInfo);
        }
        // 此种情况是为了避免callBack没生成之前服务器先返回了null的数据会造成一直转圈的情况
        if (mDetailInfo == null && stateCode != 0 && isReceiveData) {
            callBack.onIntroFailed();
        }
    }
}
