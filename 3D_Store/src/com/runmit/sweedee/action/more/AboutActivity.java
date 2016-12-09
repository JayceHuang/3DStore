package com.runmit.sweedee.action.more;

import java.io.File;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Paint;
import android.net.Uri;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.runmit.libsdk.update.Update;
import com.runmit.sweedee.R;
import com.runmit.sweedee.action.login.AgreementActivity;
import com.runmit.sweedee.update.Config;
import com.runmit.sweedee.util.Constant;
import com.runmit.sweedee.util.EnvironmentTool;
import com.runmit.sweedee.util.SharePrefence;
import com.runmit.sweedee.util.Util;

public class AboutActivity extends NewBaseRotateActivity {
    
	private TextView version;

	private TextView qq_group_by_app;
	
	private TextView tv_testMode;

	private static final String QQ = "171874973";
	
	private static final String QQ_LINK = "http://qm.qq.com/cgi-bin/qm/qr?k=-4fDbBIBQxbqoD-XkDuAmyAkDOEIUu1y";
	
	private static final String EMAIL_LINK = "mailto:kf@runmit.com";
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_setting_about);
        version = (TextView) findViewById(R.id.version);
        version.setText(String.format(getString(R.string.current_version), Config.getVerName(AboutActivity.this)));
        findViewById(R.id.check_update_btn).setOnClickListener(listener);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        
        TextView tvQQ = (TextView) findViewById(R.id.qq_group_by_app);
        tvQQ.getPaint().setFlags(Paint.UNDERLINE_TEXT_FLAG);
        tvQQ.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				Uri uri = Uri.parse(QQ_LINK);  
				Intent it = new Intent(Intent.ACTION_VIEW, uri);  
				startActivity(it);
			}
		});
        
        TextView tvEmail = (TextView) findViewById(R.id.email_group_by_app);
        tvEmail.getPaint().setFlags(Paint.UNDERLINE_TEXT_FLAG);
        tvEmail.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				Uri uri = Uri.parse(EMAIL_LINK); 
				Intent it = new Intent(Intent.ACTION_SENDTO, uri);  
				it.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				startActivity(it);   
			}
		});
        
        TextView agree_btn = (TextView) findViewById(R.id.agree_btn);
		agree_btn.getPaint().setFlags(Paint.UNDERLINE_TEXT_FLAG); // 下划线
        agree_btn.getPaint().setAntiAlias(true);// 抗锯齿
        agree_btn.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				Intent intent = new Intent(AboutActivity.this, AgreementActivity.class);
				startActivity(intent);
			}
		});
        
        final boolean testMode =  isTestMode();     
        tv_testMode = (TextView) findViewById(R.id.tv_testMode);
        if(testMode){
       	  tv_testMode.setText(R.string.testmode);
        }else{
          tv_testMode.setText("");	
        }
        tv_testMode.setOnClickListener(new OnClickListener() {
			
        	int clickTime = 0;
        	
        	long clickTimePos = 0;
        	
			@Override
			public void onClick(View v) {
				
				if(clickTime >= 8){
					boolean tmode = !isTestMode();
					enterTestMode(tmode);
					clickTime = 0;
					clickTimePos = 0;
					return;
				}
				
				long thisTime = System.currentTimeMillis();
				if(clickTimePos == 0 || thisTime - clickTimePos < 600){
					clickTime ++;
				}else{
					clickTime = 0;
					clickTimePos = 0;
				}
			}
		});
        
    }
    
    private boolean isTestMode(){
    	return !SharePrefence.getInstance(getApplicationContext()).getString(SharePrefence.TEST_MODE, "").isEmpty();  
    }

	private void enterTestMode(boolean testMode) {
		final SharePrefence sp = SharePrefence.getInstance(getApplicationContext());
		if (testMode) {
			AlertDialog.Builder builder = new AlertDialog.Builder(this);
			builder.setTitle("Input");
			View view = getLayoutInflater().inflate(R.layout.testmode_input, null);
			builder.setView(view);
			final EditText text = (EditText) view.findViewById(R.id.editText);
			builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					String input = text.getText().toString();
					if (input == null || input.trim().isEmpty()) {
						
					} else {
						Util.showToast(getApplication(), getString(R.string.testmode_tip), Toast.LENGTH_LONG);
						tv_testMode.setText(R.string.testmode);
					}
					sp.putString(SharePrefence.TEST_MODE, input.trim());
					dialog.dismiss();
					
					File file = getExternalCacheDir();
					deleteCacheFiles(file);
				}
			});
			AlertDialog dialog = builder.create();
			dialog.setCanceledOnTouchOutside(false);
			dialog.show();
		} else {
			Util.showToast(getApplication(), getString(R.string.testmode_no_tip), Toast.LENGTH_LONG);
			tv_testMode.setText("");
			sp.putString(SharePrefence.TEST_MODE, "");
			
			File file = getExternalCacheDir();
			deleteCacheFiles(file);
		}
		
	}
    
    private void deleteCacheFiles(File file){
      if(file.getPath().equals(EnvironmentTool.getFileDownloadCacheDir()) && !file.isDirectory()){
    	  return;
      }
      if(file.isDirectory()){
    	 File[] files = file.listFiles();
    	 for(File f:files){
    		 deleteCacheFiles(f);
    	 }
      }else{
    	 file.delete();
      }
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
    protected void onDestroy() {
        super.onDestroy();
    }

    private View.OnClickListener listener = new View.OnClickListener() {

        @Override
        public void onClick(View v) {
            checkUpdateAction();
        }
    };

    private void checkUpdateAction() {
        if (Util.isNetworkAvailable(this)) {// 只在有网络时才检查更新
            Update mUpdate = new Update(this);
            mUpdate.checkUpdate(Constant.PRODUCT_ID,true);
        } else {
            Util.showToast(this, getString(R.string.has_no_using_network), Toast.LENGTH_SHORT);
        }
    }
}
