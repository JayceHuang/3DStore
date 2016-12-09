package com.runmit.sweedee.sdkex.httpclient;

import org.apache.http.Header;

public abstract class LoginAsyncHttpProxyListener {

    public void onSuccess(int statusCode, Header[] headers, String content) {
    }

    public void onFailure(Throwable error) {
    }

    public void onRetry(int count) {
    }
}
