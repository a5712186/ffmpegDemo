package com.bookt.ffmpegdemo;

import android.opengl.GLSurfaceView;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GLSurfaceHolderActivity extends AppCompatActivity implements GLSurfaceView.Renderer {
    private static final String TAG = GLSurfaceHolderActivity.class.getSimpleName();

    private GLSurfaceView view;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_glsurface_holder);
        view = findViewById(R.id.gls_id);
        view.setEGLContextClientVersion(2);
        view.setRenderer(this);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {

    }

    @Override
    public void onDrawFrame(GL10 gl) {

    }


}