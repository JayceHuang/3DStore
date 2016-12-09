package com.runmit.sweedee.action.more;

import android.graphics.Bitmap;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentTransaction;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

import com.umeng.analytics.MobclickAgent;
import com.runmit.sweedee.R;

public class UserAndSettingActivity extends FragmentActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_userorsetting);
        FragmentTransaction mFragmentTransaction = getSupportFragmentManager().beginTransaction();
        mFragmentTransaction.add(R.id.main_layout, new MineSettingFragment());
        mFragmentTransaction.commitAllowingStateLoss();
        getActionBar().setDisplayHomeAsUpEnabled(true);
        
    }

//    @Override
//    public boolean onCreateOptionsMenu(Menu menu) {
//        getMenuInflater().inflate(R.menu.user_senter_menu, menu);
//        return super.onCreateOptionsMenu(menu);
//    }

    public void createViewBitmap() {
        View view = getWindow().getDecorView();
        view.setDrawingCacheEnabled(true);
        Bitmap b1 = view.getDrawingCache();

        Rect frame = new Rect();
        getWindow().getDecorView().getWindowVisibleDisplayFrame(frame);
        NewBaseRotateActivity.frameTop = frame.top;
        NewBaseRotateActivity.bitmap = Bitmap.createBitmap(b1, 0, frame.top, b1.getWidth(), b1.getHeight() - frame.top);
        view.setDrawingCacheEnabled(false);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onResume() {
        super.onResume();
        MobclickAgent.onResume(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        MobclickAgent.onPause(this);
    }
}
