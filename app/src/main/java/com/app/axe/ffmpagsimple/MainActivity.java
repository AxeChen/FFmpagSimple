package com.app.axe.ffmpagsimple;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity {

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
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }


    private native void change(String inputStr ,String outputStr);


    public native String stringFromJNI();

    public void change(View view) {
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(), "dst.yuv").getAbsolutePath();
        change(input, output);
    }
}
