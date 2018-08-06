package com.app.axe.ffmpagsimple.play;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class VideoView  extends SurfaceView {

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

    public VideoView(Context context) {
        super(context);
    }

    public VideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public VideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    private void init() {
        SurfaceHolder holder = getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
    }

    public void player(final String inputStr){
        new Thread(new Runnable() {
            @Override
            public void run() {
                render(inputStr,VideoView.this.getHolder().getSurface());
            }
        }).start();
    }

    private native void render(String input, Surface surface);
}
