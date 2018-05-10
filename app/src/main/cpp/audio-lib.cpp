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
#include <android/native_window_jni.h>
#include <unistd.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}

#include "FFmpegMusic.h"

#define LOGE_Android(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"LC",FORMAT,##__VA_ARGS__);

SLObjectItf engineObject = NULL;//用SLObjectItf声明引擎接口对象
SLEngineItf engineEngine = NULL;//声明具体的引擎对象


SLObjectItf outputMixObject = NULL;//用SLObjectItf创建混音器接口对象
SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;////具体的混音器对象实例
SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;//默认情况


SLObjectItf audioplayer = NULL;//用SLObjectItf声明播放器接口对象
SLPlayItf slPlayItf = NULL;//播放器接口
SLAndroidSimpleBufferQueueItf slBufferQueueItf = NULL;//缓冲区队列接口



size_t buffersize = 0;
void *buffer;

//将pcm数据添加到缓冲区中
void getQueueCallBack(SLAndroidSimpleBufferQueueItf slBufferQueueItf, void *context) {

    buffersize = 0;

    int stat = getPcm(&buffer, &buffersize);
    if (stat == -1) {
        LOGE_Android("获取缓存失败");
    }
    if (buffer != NULL && buffersize != 0) {
        //将得到的数据加入到队列中
        (*slBufferQueueItf)->Enqueue(slBufferQueueItf, buffer, buffersize);
    }else{
        LOGE_Android("获取缓存为0");
    }
}

//创建引擎
void createEngine() {
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);//创建引擎
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);//实现engineObject接口对象
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                  &engineEngine);//通过引擎调用接口初始化SLEngineItf
}

//创建混音器
void createMixVolume() {
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);//用引擎对象创建混音器接口对象
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);//实现混音器接口对象
    SLresult sLresult = (*outputMixObject)->GetInterface(outputMixObject,
                                                         SL_IID_ENVIRONMENTALREVERB,
                                                         &outputMixEnvironmentalReverb);//利用混音器实例对象接口初始化具体的混音器对象
    //设置
    if (SL_RESULT_SUCCESS == sLresult) {
        (*outputMixEnvironmentalReverb)->
                SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &settings);
    }
}

//创建播放器
void createPlayer(const char *file_name) {
    //初始化ffmpeg

    int rate;
    int channels;
    createFFmpeg(&rate, &channels,file_name);
    LOGE_Android("RATE %d", rate);
    LOGE_Android("channels %d", channels);
    /*
     * typedef struct SLDataLocator_AndroidBufferQueue_ {
    SLuint32    locatorType;//缓冲区队列类型
    SLuint32    numBuffers;//buffer位数
} */

    SLDataLocator_AndroidBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    /**
    typedef struct SLDataFormat_PCM_ {
        SLuint32 		formatType;  pcm
        SLuint32 		numChannels;  通道数
        SLuint32 		samplesPerSec;  采样率
        SLuint32 		bitsPerSample;  采样位数
        SLuint32 		containerSize;  包含位数
        SLuint32 		channelMask;     立体声
        SLuint32		endianness;    end标志位
    } SLDataFormat_PCM;
     */
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, (unsigned int)channels, (SLuint32)rate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};

    /*
     * typedef struct SLDataSource_ {
	        void *pLocator;//缓冲区队列
	        void *pFormat;//数据样式,配置信息
        } SLDataSource;
     * */
    SLDataSource dataSource = {&android_queue, &pcm};


    SLDataLocator_OutputMix slDataLocator_outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};


    SLDataSink slDataSink = {&slDataLocator_outputMix, NULL};


    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_FALSE, SL_BOOLEAN_FALSE, SL_BOOLEAN_FALSE};

    /*
     * SLresult (*CreateAudioPlayer) (
		SLEngineItf self,
		SLObjectItf * pPlayer,
		SLDataSource *pAudioSrc,//数据设置
		SLDataSink *pAudioSnk,//关联混音器
		SLuint32 numInterfaces,
		const SLInterfaceID * pInterfaceIds,
		const SLboolean * pInterfaceRequired
	);
     * */
    LOGE_Android("执行到此处")
    (*engineEngine)->CreateAudioPlayer(engineEngine, &audioplayer, &dataSource, &slDataSink, 3, ids,
                                       req);
    LOGE_Android("执行到此处s")
    (*audioplayer)->Realize(audioplayer, SL_BOOLEAN_FALSE);
    LOGE_Android("执行到此处2")
    (*audioplayer)->GetInterface(audioplayer, SL_IID_PLAY, &slPlayItf);//初始化播放器
    //注册缓冲区,通过缓冲区里面 的数据进行播放
    LOGE_Android("执行到此处:注册缓冲区")
    (*audioplayer)->GetInterface(audioplayer, SL_IID_BUFFERQUEUE, &slBufferQueueItf);
    //设置回调接口
    LOGE_Android("执行到此处:回调接口")
    (*slBufferQueueItf)->RegisterCallback(slBufferQueueItf, getQueueCallBack, NULL);
    //播放
    LOGE_Android("执行到此处:播放")
    (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_PLAYING);
    LOGE_Android("执行到此处3")
    //开始播放
    getQueueCallBack(slBufferQueueItf, NULL);

}

//释放资源
void realseResource() {
    if (audioplayer != NULL) {
        (*audioplayer)->Destroy(audioplayer);
        audioplayer = NULL;
        slBufferQueueItf = NULL;
        slPlayItf = NULL;
    }
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }
    realseFFmpeg();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_play(JNIEnv *env, jobject instance,jstring input_jstr) {
    createEngine();
    createMixVolume();
    const char *file_name = env -> GetStringUTFChars(input_jstr, NULL);
    createPlayer(file_name);
    env->ReleaseStringUTFChars(input_jstr, file_name);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_bookt_ffmpegdemo_FFmpegJNI_stop(JNIEnv *env, jobject instance) {
    realseResource();

}