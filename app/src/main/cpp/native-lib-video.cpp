//
// Created by tjcs on 2018/4/29.
//

#include <jni.h>
#include <string>
#include "FFmpegVideoMusic.h"
#include "FFmpegVideo.h"
#include <android/native_window_jni.h>


extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"

#include <unistd.h>

}
const char *inputPath;
int64_t *totalTime;
FFmpegVideo *ffmpegVideo;
FFmpegVideoMusic *ffmpegMusic;
pthread_t p_tid;
int isPlay;
ANativeWindow *window = 0;
int64_t duration;
AVFormatContext *pFormatCtxs;
AVPacket *packets;
int step = 0;
jboolean isSeek = false;


//开始播放
void call_video_play(AVFrame *frame) {
    //是否设置界面
    if (!window) {
        return;
    }

    //获取窗口缓存
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        return;
    }

    LOGE("绘制 宽%d,高%d", frame->width, frame->height);
    LOGE("绘制 宽%d,高%d  行字节 %d ", window_buffer.width, window_buffer.height, frame->linesize[0]);

    //界面绘制帧
    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    //视频帧
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];

    // 由于window的stride和帧的stride不同,因此需要逐行复制
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    //刷新界面播放画面
    ANativeWindow_unlockAndPost(window);
}

void init() {
    LOGE("开启解码线程")
    //1.注册组件
    av_register_all();
//    avformat_network_init();//网络不需要
    //封装格式上下文
    pFormatCtxs = avformat_alloc_context();

    //2.打开输入视频文件
    if (avformat_open_input(&pFormatCtxs, inputPath, NULL, NULL) != 0) {
        LOGE("%s", "打开输入视频文件失败");
    }
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtxs, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
    }

    //得到播放总时间
    if (pFormatCtxs->duration != AV_NOPTS_VALUE) {
        duration = pFormatCtxs->duration;//微秒
    }
}

/*void swap(int *a,int *b)
{
    int t=*a;*a=*b;*b=t;
}*/

void seekTo(int mesc) {
    if (mesc <= 0) {
        mesc = 0;
    }
    //清空vector 清空队列
    ffmpegMusic->queue.clear();
    ffmpegVideo->queue.clear();
    //跳帧
    /* if (av_seek_frame(pFormatCtx, -1,  mesc * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD) < 0) {
         LOGE("failed")
     } else {
         LOGE("success")
     }*/

//    int64_t timeVideo = (int64_t) (mesc / av_q2d(ffmpegVideo->time_base));
//    int64_t timeMusic = (int64_t) (mesc / av_q2d(ffmpegMusic->time_base));

    av_seek_frame(pFormatCtxs, ffmpegVideo->index, (int64_t) (mesc / av_q2d(ffmpegVideo->time_base)),
                  AVSEEK_FLAG_BACKWARD);
    av_seek_frame(pFormatCtxs, ffmpegMusic->index, (int64_t) (mesc / av_q2d(ffmpegMusic->time_base)),
                  AVSEEK_FLAG_BACKWARD);

}

void *begin(void *args) {

    //找到视频流和音频流
    for (int i = 0; i < pFormatCtxs->nb_streams; ++i) {
        //获取解码器
//        AVCodecContext *avCodecContext = pFormatCtx->streams[i]->codec;
//        AVCodecContext *avCodecContext = avcodec_alloc_context3(NULL);
//        avcodec_parameters_to_context(avCodecContext, pFormatCtxs->streams[i]->codecpar);
//
//        AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

        //copy一个解码器，
        //        avcodec_copy_context(codecContext, avCodecContext);


        AVCodecContext *codecContext = avcodec_alloc_context3(NULL);

        avcodec_parameters_to_context(codecContext, pFormatCtxs->streams[i]->codecpar);

        AVCodec *avCodec = avcodec_find_decoder(codecContext->codec_id);

        if (avcodec_open2(codecContext, avCodec, NULL) < 0) {
            LOGE("打开解码器失败")
            continue;
        }
        //如果是视频流
        if (pFormatCtxs->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            ffmpegVideo->index = i;
            ffmpegVideo->setAvCodecContext(codecContext);
            ffmpegVideo->time_base = pFormatCtxs->streams[i]->time_base;
            if (window) {
                ANativeWindow_setBuffersGeometry(window,
                                                 codecContext->width,
                                                 codecContext->height,
                                                 WINDOW_FORMAT_RGBA_8888);
            }
        }//如果是音频流
        else if (pFormatCtxs->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            ffmpegMusic->index = i;
            ffmpegMusic->setAvCodecContext(codecContext);
            ffmpegMusic->time_base = pFormatCtxs->streams[i]->time_base;
        }
    }
//开启播放线程
    ffmpegVideo->setFFmepegMusic(ffmpegMusic);
    ffmpegMusic->play();
    ffmpegVideo->play();
    isPlay = 1;
    //seekTo(0);
    //解码packet,并压入队列中
    packets = (AVPacket *) av_mallocz(sizeof(AVPacket));
    //跳转到某一个特定的帧上面播放
    int ret;
    while (isPlay) {
        //
        ret = av_read_frame(pFormatCtxs, packets);
        if (ret == 0) {
            if (ffmpegVideo && ffmpegVideo->isPlay && packets->stream_index == ffmpegVideo->index
                    ) {
                //将视频packet压入队列
                ffmpegVideo->put(packets);
            } else if (ffmpegMusic && ffmpegMusic->isPlay &&
                    packets->stream_index == ffmpegMusic->index) {
                ffmpegMusic->put(packets);
            }
            av_packet_unref(packets);
        } else if (ret == AVERROR_EOF) {
            // 读完了
            //读取完毕 但是不一定播放完毕
            while (isPlay) {
                if (ffmpegVideo->queue.empty() && ffmpegMusic->queue.empty()) {//队列是否为空
                    break;
                }
                // LOGE("等待播放完成");
                av_usleep(10000);
            }
        }
    }
    //解码完过后可能还没有播放完
    isPlay = 0;
    if (ffmpegMusic && ffmpegMusic->isPlay) {
        ffmpegMusic->stop();
    }
    if (ffmpegVideo && ffmpegVideo->isPlay) {
        ffmpegVideo->stop();
    }
    //释放
    av_free(packets);
    avformat_free_context(pFormatCtxs);
    if (window) {//如果以前设置过，销毁界面释放窗口内存
        ANativeWindow_release(window);
        window = 0;
    }
    pthread_exit(0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_play2(JNIEnv *env, jobject instance, jstring inputPath_) {
    inputPath = env->GetStringUTFChars(inputPath_, 0);
    init();
    ffmpegVideo = new FFmpegVideo;
    ffmpegMusic = new FFmpegVideoMusic;
    ffmpegVideo->isPlay = -1;
    ffmpegVideo->isPause = -1;
    ffmpegMusic->isPlay=-1;
    ffmpegMusic->isPause=1;
    ffmpegVideo->setPlayCall(call_video_play);
    pthread_create(&p_tid, NULL, begin, NULL);//开启begin线程
    env->ReleaseStringUTFChars(inputPath_, inputPath);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_display2(JNIEnv *env, jobject instance, jobject surface) {
    //得到界面
    if (window) {//如果以前设置过，销毁界面释放窗口内存
        ANativeWindow_release(window);
        window = 0;
    }
    //获取窗口
    window = ANativeWindow_fromSurface(env, surface);
    //ffmpegVideo已NEW并获取获取解码器
//    if (ffmpegVideo && ffmpegVideo->codec) {
//        // 设置window的buffer大小,可自动拉伸
//        ANativeWindow_setBuffersGeometry(window, ffmpegVideo->codec->width,
//                                         ffmpegVideo->codec->height,
//                                         WINDOW_FORMAT_RGBA_8888);
//    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_stop2(JNIEnv *env, jobject instance) {
    //释放资源
    if (isPlay) {
        isPlay = 0;
        pthread_join(p_tid, 0);
    }
    if (ffmpegVideo) {
        if (ffmpegVideo->isPlay) {
            ffmpegVideo->stop();
        }
        delete (ffmpegVideo);
        ffmpegVideo = 0;
    }
    if (ffmpegMusic) {
        if (ffmpegMusic->isPlay) {
            ffmpegMusic->stop();
        }
        delete (ffmpegMusic);
        ffmpegMusic = 0;
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_pause2(JNIEnv *env, jobject instance) {
    //点击暂停按钮
    ffmpegMusic->pause();
    ffmpegVideo->pause();

}
extern "C"
JNIEXPORT jint JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_getTotalTime2(JNIEnv *env, jobject instance) {

//获取视频总时间
    return (jint) duration;
}
extern "C"
JNIEXPORT jdouble JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_getCurrentPosition2(JNIEnv *env, jobject instance) {
    //获取音频播放时间
    return ffmpegMusic->clock;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_seekTo2(JNIEnv *env, jobject instance, jint msec) {
    seekTo(msec / 1000);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_stepBack2(JNIEnv *env, jobject instance) {

}
extern "C"
JNIEXPORT void JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_stepUp2(JNIEnv *env, jobject instance) {
    //点击快进按钮
    seekTo(5);

}