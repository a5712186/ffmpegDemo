package com.bookt.ffmpegdemo;

public interface FFmpegInterface {

    void createTrack(int sampleRateInHz,int nb_channals);
    void playTrack(byte[] buffer, int lenth);

}

