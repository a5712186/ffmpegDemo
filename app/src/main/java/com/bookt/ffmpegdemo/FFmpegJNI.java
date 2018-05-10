package com.bookt.ffmpegdemo;

public class FFmpegJNI {

    private static FFmpegJNI mFFmpegJNI;

    private FFmpegInterface mInterface;
    private FFmpegInterface2 mInterface2;

    public static FFmpegJNI getIntelFFmpegJNI(){
        if(null == mFFmpegJNI){
            mFFmpegJNI = new FFmpegJNI();
        }
        return mFFmpegJNI;
    }

    public void setInterface(FFmpegInterface i){
        this.mInterface = i;
    }
    public void setInterface2(FFmpegInterface2 i){
        this.mInterface2 = i;
    }

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

    public native int Audioplay(String url);

    public native void play(String url);
    public native void  stop();

    public void createTrack(int sampleRateInHz,int nb_channals) {
       if(null != mInterface){
           mInterface.createTrack(sampleRateInHz,nb_channals);
       }
    }
    public void playTrack(byte[] buffer, int lenth) {
        if(null != mInterface){
            mInterface.playTrack(buffer,lenth);
        }
    }


    public native void play2(String url);
    public native void display2(Object url);
    public native void stop2();
    public native int getTotalTime2();
    public native void pause2();
    public native double getCurrentPosition2();
    public native void seekTo2(int date);
    public native void stepBack2();
    public native void stepUp2();
}
