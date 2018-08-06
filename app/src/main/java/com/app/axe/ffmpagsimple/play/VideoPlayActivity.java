package com.app.axe.ffmpagsimple.play;

import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Spinner;

import com.app.axe.ffmpagsimple.R;

import java.io.File;

public class VideoPlayActivity extends AppCompatActivity {


    private VideoView videoView;
    private Spinner spinner;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_video_play);
        videoView = findViewById(R.id.video_view);
        spinner = (Spinner) findViewById(R.id.sp_video);
        //多种格式的视频列表
        String[] videoArray = getResources().getStringArray(R.array.video_list);
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1,
                android.R.id.text1, videoArray);
        spinner.setAdapter(adapter);
    }

    public void play(View view){
        String video = spinner.getSelectedItem().toString();
        String input = new File(Environment.getExternalStorageDirectory(), video).getAbsolutePath();
        videoView.player(input);
    }
}
