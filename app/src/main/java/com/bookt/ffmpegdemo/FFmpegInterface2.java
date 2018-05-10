package com.bookt.ffmpegdemo;

public interface FFmpegInterface2 {

    void paly2(String url);
    void stop2();
    void pause2();
    void seeto(int seeto);
    int getTotalTime2();
    double getCurrentPosition2();
    void setDisplay2(Object surfaceView);
}
