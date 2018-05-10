package com.bookt.ffmpegdemo;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import java.io.File;

public class FFmpegActivity extends AppCompatActivity implements View.OnClickListener,SurfaceHolder.Callback{


    private SurfaceView surface;
    private SurfaceHolder surfaceHolder;
    private FFmpegJNI jni;
    private String input = new File(Environment.getExternalStorageDirectory(), "gf2.mp4").getAbsolutePath();
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ffmpeg);
        surface = findViewById(R.id.surface_vido);
        surfaceHolder = surface.getHolder();
        surfaceHolder.addCallback(this);
        findViewById(R.id.but_paly).setOnClickListener(this);
        findViewById(R.id.but_stop).setOnClickListener(this);
        findViewById(R.id.but_pause).setOnClickListener(this);
        findViewById(R.id.but_seeto).setOnClickListener(this);
        jni = FFmpegJNI.getIntelFFmpegJNI();
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.but_paly:
                jni.play2(input);
                break;
            case R.id.but_stop:
                jni.stop2();
                break;
            case R.id.but_pause:
                jni.pause2();
                break;
            case R.id.but_seeto:
                jni.seekTo2(5);
                break;
        }
    }



    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        jni.display2(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        holder.getSurface().release();
    }

}
