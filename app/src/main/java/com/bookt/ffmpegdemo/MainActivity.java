package com.bookt.ffmpegdemo;

import android.content.Intent;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("avfilter");
        System.loadLibrary("swresample");
        System.loadLibrary("avcodec");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swscale");
        System.loadLibrary("fdk-aac");

        System.loadLibrary("jni-lib");
    }

    private SurfaceHolder surfaceHolder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface_view);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d("MAIN", "调用了.........................");
        new Thread(new Runnable() {
            @Override
            public void run() {
                String input = new File(Environment.getExternalStorageDirectory(), "demo.mp4").getAbsolutePath();
                Voideplay(input, surfaceHolder.getSurface());
                Voideplay3(surfaceHolder.getSurface(),input);
            }
        }).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        holder.getSurface().release();
    }


    public void format(View view) {
        String input = new File(Environment.getExternalStorageDirectory(), "demo.mp4").getAbsolutePath();
        FFmpegTest(input, "");
    }

    public void codec(View view) {

//        String input = new File(Environment.getExternalStorageDirectory(), "demo.mp4").getAbsolutePath();
//        String output = new File(Environment.getExternalStorageDirectory(), "output.avi").getAbsolutePath();
//        Voideplay2(input, output);

    }

    public void filter(View view) {
        startActivity(new Intent(this,FFmpegActivity.class));
    }

    public void config(View view) {
      startActivity(new Intent(this,AudioTrackActivity.class));
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    //JNI
    public static native int JniCppAdd(int a, int b);

    public static native int JniCppSub(int a, int b);

    public static native void FFmpegTest(String input, String output);

//    public static native void Voideplay2(String input, String output);

    public static native int Voideplay(String url, Object surface);

    public static native int Voideplay3(Object surface,String url);
}
