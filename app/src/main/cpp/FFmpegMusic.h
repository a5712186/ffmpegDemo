//
// Created by tjcs on 2018/4/28.
//

#include <jni.h>
#include <string>
#include <android/log.h>
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
#define FFLOGI(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"LC",FORMAT,##__VA_ARGS__);

int createFFmpeg(int *rate,int *channel,const char * input_jstr);

int getPcm(void **pcm,size_t *pcm_size);

void realseFFmpeg();
