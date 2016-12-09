//package com.runmit.sweedee;
//
//import java.util.ArrayList;
//import java.util.List;
//
//import android.content.ComponentName;
//import android.content.Context;
//import android.content.Intent;
//import android.content.pm.PackageManager;
//import android.content.res.TypedArray;
//import android.os.Bundle;
//import android.os.Parcelable;
//import android.view.KeyEvent;
//import android.view.LayoutInflater;
//import android.view.MotionEvent;
//import android.view.View;
//import android.view.View.OnClickListener;
//import android.view.View.OnTouchListener;
//import android.widget.ImageView;
//import android.widget.ImageView.ScaleType;
//
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.support.PagerAdapter;
//import com.runmit.sweedee.support.ViewPager;
//import com.runmit.sweedee.support.ViewPager.OnPageChangeListener;
//import com.runmit.sweedee.util.Constant;
//import com.runmit.sweedee.util.Util;
//import com.runmit.sweedee.util.XL_Log;
//import com.runmit.sweedee.view.UnderlinePageIndicator;
//
//public class FirstUseFlipActivity extends BaseActivity implements OnClickListener {
//
//    XL_Log log = new XL_Log(FirstUseFlipActivity.class);
//
//    private ViewPager mPager;
//
//    private List<View> listViews;
//
//    private int offset_size = 3;
//
//    private View[] LVS;
//
////	private ImageView[] BGs;
//
//    private MyOnPageChangeListener onPageChangeListener;
//
//    private int[] fipImage = new int[]{R.layout.flip_1, R.layout.flip_2, R.layout.flip_3/*,R.layout.flip_4*/};
//
//    private int[] LoadImageFirst = new int[]{R.drawable.flip_1, R.drawable.flip_2, R.drawable.flip_3/*,R.drawable.flip_4*/};
//
////	private int[] LoadImageCover = new int[] { R.drawable.flip_11, R.drawable.flip_22, R.drawable.flip_33};
//
//    private int currentPage = 0;
//    private View view;
//
//    boolean first_intall_state = true;
//    private String from;
//
//    UnderlinePageIndicator mUnderlinePageIndicator;
//
//    @Override
//    public void onCreate(Bundle savedInstanceState) {
//        super.onCreate(savedInstanceState);
//        setContentView(R.layout.xlshare_flip_view);
//        first_intall_state = getIntent().getBooleanExtra("first_intall_state", true);
//        from = getIntent().getStringExtra("from");
//        initTextView();
//
//        removeShortcut();
//    }
//
//    private void initTextView() {
////		BGs = new ImageView[offset_size];
////		BGs[0] = (ImageView) findViewById(R.id.cursor1);
////		BGs[1] = (ImageView) findViewById(R.id.cursor2);
////		BGs[2] = (ImageView) findViewById(R.id.cursor3);
////		BGs[3] = (ImageView) findViewById(R.id.cursor4);
//
//
//        listViews = new ArrayList<View>();
//
//        LVS = new View[offset_size];
//
//        for (int i = 0; i < offset_size; i++) {
////			BGs[i].setOnClickListener(new MyOnClickListener(i));
//
//            LayoutInflater mInflater = getLayoutInflater();
//            LVS[i] = mInflater.inflate(fipImage[i], null);
//            if (i == offset_size - 1) {
//                LVS[i].findViewById(R.id.start_experience).setOnClickListener(this);
//            }
//            if (true/*!first_intall_state*/) {
//                LVS[i].setBackgroundResource(LoadImageFirst[i]);
//            } else {
////				LVS[i].setBackgroundResource(LoadImageCover[i]);
//            }
//            listViews.add(LVS[i]);
//        }
//
//        mPager = (ViewPager) findViewById(R.id.vPager);
//
//
//        mPager.setOnTouchListener(new OnTouchListener() {
//
//            @Override
//            public boolean onTouch(View v, MotionEvent event) {
//
//                switch (event.getAction()) {
//                    case MotionEvent.ACTION_DOWN:
//                        startX = event.getX();
//                        break;
//                    case MotionEvent.ACTION_MOVE:
//                        break;
//                    case MotionEvent.ACTION_UP:
//                        float dx = event.getX() - startX;
//                        log.debug("onTouch=" + dx + ",currentPage=" + currentPage);
//                        if (dx < 0 && Math.abs(dx) > Constant.SCREEN_WIDTH / 4 && currentPage == offset_size - 1) {
//                            if (from == null) {
//                                startActivity(new Intent(FirstUseFlipActivity.this, TdStoreMainActivity.class));
//                                addShortcut();
//                            }
//                            //startActivity(new Intent(FirstUseFlipActivity.this, XlCloudPlayActivity.class));
//                            finish();
//                        }
//                        break;
//
//                    default:
//                        break;
//                }
//
//                return false;
//            }
//        });
//
//        onPageChangeListener = new MyOnPageChangeListener();
//        mPager.setAdapter(new MyPagerAdapter(listViews));
//        mPager.setCurrentItem(0);
//        mPager.setOnPageChangeListener(onPageChangeListener);
//        onPageChangeListener.onPageSelected(0);
//
//        final int defaultSelectedColor = getResources().getColor(R.color.default_flip_underline_indicator_selected_color);
//        mUnderlinePageIndicator = (UnderlinePageIndicator) findViewById(R.id.indicator);
//        mUnderlinePageIndicator.setViewPager(mPager);
//        mUnderlinePageIndicator.setFades(false);
//        mUnderlinePageIndicator.setSelectedColor(defaultSelectedColor);
//    }
//
//
//    float startX = 0;
//
//
//    @Override
//    protected void onDestroy() {
//        super.onDestroy();
//
//    }
//
//    /**
//     * ViewPager适配器
//     */
//    public class MyPagerAdapter extends PagerAdapter {
//        public List<View> mListViews;
//
//        public MyPagerAdapter(List<View> mListViews) {
//            this.mListViews = mListViews;
//        }
//
//        @Override
//        public void destroyItem(View arg0, int arg1, Object arg2) {
//            ((ViewPager) arg0).removeView(mListViews.get(arg1));
//        }
//
//        @Override
//        public void finishUpdate(View arg0) {
//        }
//
//        @Override
//        public int getCount() {
//            return mListViews.size();
//        }
//
//        @Override
//        public Object instantiateItem(View arg0, int arg1) {
//            ((ViewPager) arg0).addView(mListViews.get(arg1), 0);
//            return mListViews.get(arg1);
//        }
//
//        @Override
//        public boolean isViewFromObject(View arg0, Object arg1) {
//            return arg0 == (arg1);
//        }
//
//        @Override
//        public void restoreState(Parcelable arg0, ClassLoader arg1) {
//        }
//
//        @Override
//        public Parcelable saveState() {
//            return null;
//        }
//
//        @Override
//        public void startUpdate(View arg0) {
//        }
//
//        /* (non-Javadoc)
//         * @see com.xunlei.cloud.fragment.PagerAdapter#getRealCount()
//         */
//        @Override
//        public int getRealCount() {
//            return mListViews.size();
//        }
//    }
//
//    /**
//     * 头标点击监听
//     */
//    public class MyOnClickListener implements View.OnClickListener {
//        private int index = 0;
//
//        public MyOnClickListener(int i) {
//            index = i;
//        }
//
//        @Override
//        public void onClick(View v) {
//            mPager.setCurrentItem(index);
//        }
//    }
//
//    ;
//
//    /**
//     * 页卡切换监听
//     */
//    public class MyOnPageChangeListener implements OnPageChangeListener {
//        @Override
//        public void onPageSelected(int arg0) {
//            currentPage = arg0;
//            for (int i = 0; i < offset_size; i++) {
////				BGs[i].setImageResource(i == arg0?R.drawable.current_circle:R.drawable.circle);
//            }
//        }
//
//        @Override
//        public void onPageScrolled(int arg0, float arg1, int arg2) {
//        }
//
//        @Override
//        public void onPageScrollStateChanged(int arg0) {
//        }
//    }
//
//    private void removeShortcut() {
//        Intent shortcutIntent = new Intent(this, LoadingActivity.class);
//        shortcutIntent.setAction(Intent.ACTION_MAIN);
//        shortcutIntent.addCategory("android.intent.category.LAUNCHER");
//        shortcutIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
//
//        Intent addIntent = new Intent();
//        addIntent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
//        addIntent.putExtra(Intent.EXTRA_SHORTCUT_NAME, getString(R.string.app_name));
//        addIntent.setAction("com.android.launcher.action.UNINSTALL_SHORTCUT");
//        sendBroadcast(addIntent);
//    }
//
//    private void addShortcut() {
//        Intent addShortcut = new Intent("com.android.launcher.action.INSTALL_SHORTCUT");
//        String appName = getString(R.string.app_name);
//        Parcelable icon = null;
//        icon = Intent.ShortcutIconResource.fromContext(this, R.drawable.ic_launcher);
//        Intent desIntent = new Intent(this, LoadingActivity.class);
//        desIntent.setAction("android.intent.action.MAIN");
//        desIntent.addCategory("android.intent.category.LAUNCHER");
//        desIntent.setClassName(getPackageName(), LoadingActivity.class.getName());
//        desIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
//        addShortcut.putExtra(Intent.EXTRA_SHORTCUT_INTENT, desIntent);
//        addShortcut.putExtra(Intent.EXTRA_SHORTCUT_NAME, appName);
//        addShortcut.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE, icon);
//        addShortcut.putExtra("duplicate", false);
//        sendBroadcast(addShortcut);
//    }
//
//    @Override
//    public void onClick(View v) {
//        switch (v.getId()) {
//            case R.id.start_experience:
//                if (from == null) {
//                    startActivity(new Intent(FirstUseFlipActivity.this, TdStoreMainActivity.class));
//                    addShortcut();
//                }
//                this.finish();
//                break;
//            default:
//                break;
//        }
//
//    }
//
//    public boolean dispatchKeyEvent(KeyEvent event) {
//        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN) {
//            if (from == null) {
//                return true;
//            }
//        }
//        return super.dispatchKeyEvent(event);
//    }
//}
