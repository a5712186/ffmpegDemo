//
// Created by tjcs on 2018/4/29.
//

#ifndef FFMPEGDEMO_LOG_H
#define FFMPEGDEMO_LOG_H

#include <android/log.h>
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"LC XXX",FORMAT,##__VA_ARGS__);

#endif //FFMPEGDEMO_LOG_H
