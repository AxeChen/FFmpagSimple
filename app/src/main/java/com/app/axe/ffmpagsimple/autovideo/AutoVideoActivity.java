package com.app.axe.ffmpagsimple.autovideo;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.Surface;
import android.view.View;

import com.app.axe.ffmpagsimple.R;

public class AutoVideoActivity extends AppCompatActivity {


    static {
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_auto_play);
    }

    public void play(View view) {
        play("rtmp://live.hkstv.hk.lxdns.com/live/hks");
    }

    /**
     * 播放
     *
     * @param path
     */
    public native void play(String path);

    public native void display(Surface surface);

    public native void stop();

    public native void release();
}
