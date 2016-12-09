package com.runmit.sweedee.player;

import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;

import com.runmit.sweedee.util.Util;

public class PlayerAudioMgr {
    private static final String TAG = "PlayerAudioMgr";

    public final static AudioManager.OnAudioFocusChangeListener mAudioFocusChangeListener =
            new AudioManager.OnAudioFocusChangeListener() {
                public void onAudioFocusChange(int focusChange) {
                    switch (focusChange) {
                        case AudioManager.AUDIOFOCUS_GAIN:
                        case AudioManager.AUDIOFOCUS_GAIN_TRANSIENT:
                            break;
                        case AudioManager.AUDIOFOCUS_LOSS:
                        case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                            //superPause();
                            break;
                        default:
                            break;
                    }
                }
            };


    public void requestAudioFocus(Context context) {
        Intent intent = new Intent("com.android.music.musicservicecommand.pause");
        context.sendBroadcast(intent);

        AudioManager mAudioMgr = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);

        int ret = mAudioMgr.requestAudioFocus(mAudioFocusChangeListener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        if (ret != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
        }
        android.provider.Settings.System.putString(context.getContentResolver(), "headsetowner", TAG);

    }

    public void abandonAudioFocus(Context context) {
        AudioManager mAudioMgr = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        if (mAudioMgr != null) {
            mAudioMgr.abandonAudioFocus(mAudioFocusChangeListener);
            mAudioMgr = null;
            android.provider.Settings.System.putString(context.getContentResolver(), "headsetowner", "");
        }
    }

}
