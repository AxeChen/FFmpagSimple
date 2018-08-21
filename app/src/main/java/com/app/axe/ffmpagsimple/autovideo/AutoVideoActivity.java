package com.app.axe.ffmpagsimple.autovideo;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;

import com.app.axe.ffmpagsimple.R;

import java.io.File;

public class AutoVideoActivity extends AppCompatActivity {


    AutoPlayer davidPlayer;
    SurfaceView surfaceView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_auto_play);
        surfaceView = (SurfaceView) findViewById(R.id.surface);
        davidPlayer = new AutoPlayer();
        davidPlayer.setSurfaceView(surfaceView);

    }
    public void player(View view) {
        davidPlayer.playJava("rtmp://live.hkstv.hk.lxdns.com/live/hks");
    }
    public void stop(View view) {
        davidPlayer.release();
    }
}
