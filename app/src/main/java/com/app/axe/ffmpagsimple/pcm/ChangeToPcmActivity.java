package com.app.axe.ffmpagsimple.pcm;

import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import com.app.axe.ffmpagsimple.R;

import java.io.File;

public class ChangeToPcmActivity extends AppCompatActivity {

    // 这边的加载动态库的顺序也是由规定的，必须将被调用者放在面，没有调用的放在后面
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
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp3").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(), "output.pcm").getAbsolutePath();
        changeMp3(input, output);
    }

    public native void changeMp3(String input, String outStr);

}
