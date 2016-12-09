package com.runmit.sweedee.action.login;

import java.util.Locale;

import android.app.Activity;
import android.app.ProgressDialog;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import com.umeng.analytics.MobclickAgent;
import com.runmit.sweedee.R;
import com.runmit.sweedee.util.Util;

public class AgreementActivity extends Activity {
   
	private WebView webView;
    
    private final String reg_protocal_dir = "file:///android_asset/";
    
    private ProgressDialog dialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login_agree);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        
        webView = (WebView) findViewById(R.id.webview);
        WebSettings ws = webView.getSettings();
        ws.setSupportZoom(true);
        ws.setBuiltInZoomControls(true);
//        webView.setInitialScale(80);
        dialog = new ProgressDialog(this,R.style.ProgressDialogDark);
        WebViewClient client = new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                return super.shouldOverrideUrlLoading(view, url);
            }

            @Override
            public void onPageFinished(WebView view, String url) {
                super.onPageFinished(view, url);
                Util.dismissDialog(dialog);
            }

            @Override
            public void onPageStarted(WebView view, String url, Bitmap favicon) {
                super.onPageStarted(view, url, favicon);
                Util.showDialog(dialog, getString(R.string.permission_opening));
            }

        };
        webView.setWebViewClient(client);
        String lan = Locale.getDefault().getCountry().toLowerCase(); // "CN" "TW"
        if(lan.equals("tw")){
        	lan = "_tw";
        }
        else if(lan.equals("cn")){
        	lan = "_cn";
        }else {
        	lan = "_en";
        }
        String uri = reg_protocal_dir + "duty" + lan + ".html";
        webView.loadUrl(uri);
        webView.setOnKeyListener(new View.OnKeyListener() {

			@Override
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				// TODO Auto-generated method stub
				if (event.getAction() == KeyEvent.ACTION_DOWN) {    
                  if (keyCode == KeyEvent.KEYCODE_BACK && webView.canGoBack()) {  //表示按返回键 时的操作  
                	  webView.goBack();   //后退    

                      //webview.goForward();//前进  
                      return true;    //已处理    
                  }    
              }    
              return false;
			}    
        });  
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

    public void screenSetting() {
        int screenDensity = getResources().getDisplayMetrics().densityDpi;
        WebSettings.ZoomDensity zoomDensity = WebSettings.ZoomDensity.MEDIUM;
        switch (screenDensity) {

            case DisplayMetrics.DENSITY_LOW:
                zoomDensity = WebSettings.ZoomDensity.CLOSE;
                break;

            case DisplayMetrics.DENSITY_MEDIUM:
                zoomDensity = WebSettings.ZoomDensity.CLOSE;
                break;

            case DisplayMetrics.DENSITY_HIGH:
                zoomDensity = WebSettings.ZoomDensity.FAR;
                break;
        }
        webView.getSettings().setDefaultZoom(zoomDensity);// webSettings.setDefaultZoom(zoomDensity);
    }

    private View.OnClickListener listener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            finish();
        }
    };
}
