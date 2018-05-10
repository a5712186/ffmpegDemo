package com.bookt.ffmpegdemo;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

import java.io.File;

public class AudioTrackActivity extends AppCompatActivity implements View.OnClickListener, FFmpegInterface {

    private String input = new File(Environment.getExternalStorageDirectory(), "gf2.mp4").getAbsolutePath();
    private AudioTrack audioTrack;
    private FFmpegJNI mFFmpegJNI;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio_track);
        findViewById(R.id.button_audio).setOnClickListener(this);
        findViewById(R.id.button_opensl_play).setOnClickListener(this);
        findViewById(R.id.button_opensl_stop).setOnClickListener(this);
        mFFmpegJNI = FFmpegJNI.getIntelFFmpegJNI();
        mFFmpegJNI.setInterface(this);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.button_audio:
                new Thread(new Runnable() {
                    @Override
                    public void run() {

                        mFFmpegJNI.Audioplay(input);
                    }
                }).start();
                break;
            case R.id.button_opensl_play:
                new Thread(new Runnable() {
                    @Override
                    public void run() {

                        mFFmpegJNI.play(input);
                    }
                }).start();

                break;
            case R.id.button_opensl_stop:
                mFFmpegJNI.stop();
                break;
        }


    }

    //    这个方法  是C进行调用
    public void createTrack(int sampleRateInHz, int nb_channals) {
        int channaleConfig;//通道数
        if (nb_channals == 1) {
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;
        } else if (nb_channals == 2) {
            channaleConfig = AudioFormat.CHANNEL_OUT_STEREO;
        } else {
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;
        }
        int buffersize = AudioTrack.getMinBufferSize(sampleRateInHz,
                channaleConfig, AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, channaleConfig,
                AudioFormat.ENCODING_PCM_16BIT, buffersize, AudioTrack.MODE_STREAM);
        audioTrack.play();
    }

    //C传入音频数据
    public void playTrack(byte[] buffer, int lenth) {
        if (audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            audioTrack.write(buffer, 0, lenth);
        }
    }


}
