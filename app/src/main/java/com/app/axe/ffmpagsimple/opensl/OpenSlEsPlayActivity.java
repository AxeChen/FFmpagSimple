package com.app.axe.ffmpagsimple.opensl;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import com.app.axe.ffmpagsimple.R;

public class OpenSlEsPlayActivity extends AppCompatActivity {

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
        setContentView(R.layout.activity_change_mp3);
    }

    public void change(View view) {
        play();
    }

    public void stop(View view){
        stopMusic();
    }

    public native void play();

    public native void stopMusic();

}
