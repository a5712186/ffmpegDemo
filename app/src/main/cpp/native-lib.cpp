#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <unistd.h>
#include <android/log.h>
#include <assert.h>
extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
//
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}
static AVPacket *vPacket;
static AVFrame *vFrame, *pFrameRGBA;
static AVCodecContext *vCodecCtx;
struct SwsContext *img_convert_ctx;
static AVFormatContext *pFormatCtx;
static AVCodecParameters *avCodecParameters;
ANativeWindow* nativeWindow;
ANativeWindow_Buffer windowBuffer;
uint8_t *v_out_buffer;

#define FFLOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg",FORMAT,##__VA_ARGS__);
#define FFLOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg",FORMAT,##__VA_ARGS__);

extern "C" JNIEXPORT void JNICALL Java_com_bookt_ffmpegdemo_MainActivity_FFmpegTest(
        JNIEnv *env,
        jobject obj,
        jstring input,
        jstring output
) {
    //获取输入输出文件名
    const char *inputc = env->GetStringUTFChars((jstring) input, 0);
    const char *outputc = env->GetStringUTFChars((jstring) output, 0);
    //1.注册所有组件
    av_register_all();
    avformat_network_init();
    //2.打开输入视频文件
    //封装格式上下文，统领全局的结构体，保存了视频文件封装格式的相关信息
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    int ret = avformat_open_input(&pFormatCtx, inputc, NULL, NULL);
    if (ret != 0) {
        char errorbuf[1024] = {0};
        av_make_error_string(errorbuf, 1024, ret);
        FFLOGE("%s,%d,%s", "无法打开输入视频文件", ret, errorbuf);
        FFLOGE("%s", inputc);
    } else {
        FFLOGI("%s,%d,%lld", "视频长度：", ret, pFormatCtx->duration);
    }
}
extern "C" JNIEXPORT jint Java_com_bookt_ffmpegdemo_MainActivity_JniCppAdd(JNIEnv*env, jobject obj,jint a,jint b){
    int ia = a;
    int ib = b;
    printf("==================%d",a+b);
    return a+b;
}
extern "C" JNIEXPORT jint Java_com_bookt_ffmpegdemo_MainActivity_JniCppSub(JNIEnv*env, jobject obj,jint a,jint b){
    return a-b;
}

extern "C" JNIEXPORT jint Java_com_bookt_ffmpegdemo_MainActivity_Voideplay(JNIEnv*env, jobject obj, jstring url, jobject surface) {
    int i;
    AVCodec *vCodec;
    char input_str[500]={0};
    //读取输入的视频频文件地址
    sprintf(input_str, "%s", env->GetStringUTFChars(url, NULL));
    //初始化
    av_register_all();
    //分配一个AVFormatContext结构
    pFormatCtx = avformat_alloc_context();
    //打开文件
    if(avformat_open_input(&pFormatCtx,input_str,NULL,NULL)!=0){
        FFLOGI("Couldn't open input stream.\n");
        return -1;
    }
    //查找文件的流信息
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        FFLOGI("Couldn't find stream information.\n");
        return -1;
    }
    //在流信息中找到视频流
    int videoindex = -1;
    for(i=0; i<pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    if(videoindex == -1){
        FFLOGI("Couldn't find a video stream.\n");
        return -1;
    }

    //获取相应视频流的解码器
    avCodecParameters = pFormatCtx->streams[videoindex]->codecpar;
    if (avCodecParameters == NULL){
        printf("查找解码器失败\n");
        return -1;
    }
//    vCodecCtx = avcodec_alloc_context3(NULL);
//    if (vCodecCtx == NULL){
//        printf("查找解码器失败\n");
//        return -1;
//     }

//    vCodec = avcodec_find_decoder_by_name(vCodecCtx->codec_name);
    vCodec = avcodec_find_decoder(avCodecParameters->codec_id);
//    assert(vCodec != NULL);
    if(vCodec){
        FFLOGI("find decoder: %d", avCodecParameters->codec_id);
    }
    if (vCodec == NULL) {
        FFLOGI("Codec not found.");
        return -1; // Codec not found
    }

     //alloc a codecContext
    vCodecCtx = avcodec_alloc_context3(vCodec);


    //transform
    if(avcodec_parameters_to_context(vCodecCtx,avCodecParameters) < 0){
        FFLOGI("copy the codec parameters to context fail!");
        return -1;
    }

    //打开解码器
    if(avcodec_open2(vCodecCtx, vCodec,NULL)<0){
        FFLOGI("Couldn't open codec.\n");
        return -1;
    }
    //获取界面传下来的surface
    nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (0 == nativeWindow){
        FFLOGI("Couldn't get native window from surface.\n");
        return -1;
    }
    int width = vCodecCtx->width;
    int height = vCodecCtx->height;
    //分配一个帧指针，指向解码后的原始帧
    vFrame = av_frame_alloc();
    vPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
    pFrameRGBA = av_frame_alloc();
    //绑定输出buffer
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
    v_out_buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, v_out_buffer, AV_PIX_FMT_RGBA, width, height, 1);
    img_convert_ctx = sws_getContext(width, height, vCodecCtx->pix_fmt,
                                     width, height, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);
    if (0 > ANativeWindow_setBuffersGeometry(nativeWindow,width,height,WINDOW_FORMAT_RGBA_8888)){
        FFLOGI("Couldn't set buffers geometry.\n");
        ANativeWindow_release(nativeWindow);
        return -1;
    }
    //读取帧
    while(av_read_frame(pFormatCtx, vPacket)>=0){
        if(vPacket->stream_index==videoindex){
            //视频解码
            int ret = avcodec_send_packet(vCodecCtx, vPacket);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                return -1;
            ret = avcodec_receive_frame(vCodecCtx, vFrame);
            if (ret < 0 && ret != AVERROR_EOF)
                return -1;
            //转化格式
            sws_scale(img_convert_ctx, (const uint8_t* const*)vFrame->data, vFrame->linesize, 0, vCodecCtx->height,
                      pFrameRGBA->data, pFrameRGBA->linesize);
            if (ANativeWindow_lock(nativeWindow, &windowBuffer, NULL) < 0) {
                FFLOGI("cannot lock window");
            } else {
                //将图像绘制到界面上，注意这里pFrameRGBA一行的像素和windowBuffer一行的像素长度可能不一致
                //需要转换好，否则可能花屏
                uint8_t *dst = (uint8_t *) windowBuffer.bits;
                for (int h = 0; h < height; h++)
                {
                    memcpy(dst + h * windowBuffer.stride * 4,
                           v_out_buffer + h * pFrameRGBA->linesize[0],
                           pFrameRGBA->linesize[0]);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
                //延时绘制 否则视频快速播放
                usleep(1000 * 16);
            }
        }
        av_packet_unref(vPacket);
    }
    //释放内存
    sws_freeContext(img_convert_ctx);
    av_free(vPacket);
    av_free(pFrameRGBA);
    avcodec_close(vCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}


//extern "C" JNIEXPORT void JNICALL
//Java_com_bookt_ffmpegdemo_MainActivity_Voideplay2(JNIEnv *env, jobject type, jstring input_jstr,
//                                                  jstring output_jstr) {
//    const char* input_cstr = env -> GetStringUTFChars(input_jstr, NULL);
//    const char* output_cstr = env -> GetStringUTFChars(output_jstr, NULL);
//
//    //1. 注册所有组件
//    av_register_all();
//
//    //封装格式上下文
//    AVFormatContext* pFormatCtx = avformat_alloc_context();
//    //2. 打开输入视频文件，成功返回0，第三个参数为NULL，表示自动检测文件格式
//    if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
//        FFLOGI("%s", "打开输入视频文件失败");
//        return;
//    }
//
//    //3. 获取视频文件信息
//    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
//        FFLOGI("%s", "获取视频文件信息失败");
//        return;
//    }
//
//    //查找视频流所在的位置
//    //遍历所有类型的流（视频流、音频流可能还有字幕流），找到视频流的位置
//    int video_stream_index = -1;
//    int i = 0;
//    for(; i < pFormatCtx -> nb_streams; i++) {
//        if (pFormatCtx->streams[i]->codecpar-> codec_type == AVMEDIA_TYPE_VIDEO) {
//            video_stream_index = i;
//            break;
//        }
//    }
//
//    //编解码上下文
//    AVCodecContext* pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
//    //4. 查找解码器 不能通过pCodecCtx->codec获得解码器
//    AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
//    if (pCodec == NULL) {
//        FFLOGI("%s", "查找解码器失败");
//        return;
//    }
//
//    //5. 打开解码器
//    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
//        FFLOGI("%s", "打开解码器失败");
//        return;
//    }
//
//    //编码数据
//    AVPacket* pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
//
//    //像素数据（解码数据）
//    AVFrame* pFrame = av_frame_alloc();
//    AVFrame* pYuvFrame = av_frame_alloc();
//
//    FILE* fp_yuv = fopen(output_cstr, "wb");
//
////    int width = pCodecCtx->width;
////    int height = pCodecCtx->height;
//
//    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
//    //缓冲区分配内存
////    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
////    v_out_buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
//
//    uint8_t* out_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
//    //初始化缓冲区
//    avpicture_fill((AVPicture*)pYuvFrame, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
//
//    //srcW：源图像的宽
//    //srcH：源图像的高
//    //srcFormat：源图像的像素格式
//    //dstW：目标图像的宽
//    //dstH：目标图像的高
//    //dstFormat：目标图像的像素格式
//    //flags：设定图像拉伸使用的算法
//    struct SwsContext* pSwsCtx = sws_getContext(
//            pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
//            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
//            SWS_BILINEAR, NULL, NULL, NULL);
//
//    int got_frame, len, frameCount = 0;
//    //6. 从输入文件一帧一帧读取压缩的视频数据AVPacket
//    while(av_read_frame(pFormatCtx, pPacket) >= 0) {
//        if (pPacket->stream_index == video_stream_index) {
//            //7. 解码一帧压缩数据AVPacket ---> AVFrame，第3个参数为0时表示解码完成
//            len = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, pPacket);
//
//            if (len < 0) {
//                FFLOGI("%s", "解码失败");
//                return;
//            }
//            //AVFrame ---> YUV420P
//            //srcSlice[]、dst[]        输入、输出数据
//            //srcStride[]、dstStride[] 输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
//            //srcSliceY                输入数据第一列要转码的位置 从0开始
//            //srcSliceH                输入画面的高度
//            sws_scale(pSwsCtx,
//                      (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pFrame->height,
//                      pYuvFrame->data, pYuvFrame->linesize);
//
//            //非0表示正在解码
//            if (got_frame) {
//                //图像宽高的乘积就是视频的总像素，而一个像素包含一个y，u对应1/4个y，v对应1/4个y
//                int yuv_size = pCodecCtx->width * pCodecCtx->height;
//                //写入y的数据
//                fwrite(pYuvFrame->data[0], 1, yuv_size, fp_yuv);
//                //写入u的数据
//                fwrite(pYuvFrame->data[1], 1, yuv_size/4, fp_yuv);
//                //写入v的数据
//                fwrite(pYuvFrame->data[2], 1, yuv_size/4, fp_yuv);
//
//                FFLOGI("解码第%d帧", frameCount++);
//            }
//            av_packet_unref(pPacket);
//        }
//    }
//
//    fclose(fp_yuv);
//    av_frame_free(&pFrame);
//    av_frame_free(&pYuvFrame);
//    avcodec_free_context(&pCodecCtx);
//    avformat_free_context(pFormatCtx);
//
//    env -> ReleaseStringUTFChars(input_jstr, input_cstr);
//    env -> ReleaseStringUTFChars(output_jstr, output_cstr);
//}


extern "C" JNIEXPORT jint JNICALL Java_com_bookt_ffmpegdemo_MainActivity_Voideplay3
        (JNIEnv *env, jobject clazz, jobject surface,jstring input_jstr) {
    FFLOGI("play");

    // sd卡中的视频文件地址,可自行修改或者通过jni传入
    //char *file_name = "/storage/emulated/0/ws2.mp4";
    const char *file_name = env -> GetStringUTFChars(input_jstr, NULL);

    //注册ffmpeg
    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    // 打开文件
    if (avformat_open_input(&pFormatCtx, file_name, NULL, NULL) != 0) {

        FFLOGI("Couldn't open file:%s\n", file_name);
        return -1; // Couldn't open file
    }

    // 获取文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        FFLOGI("Couldn't find stream information.");
        return -1;
    }

    //查找视频流所在的位置
    //遍历所有类型的流（视频流、音频流可能还有字幕流），找到视频流的位置
    int videoStream = -1, i;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
            && videoStream < 0) {
            videoStream = i;
        }
    }
    if (videoStream == -1) {
        FFLOGI("Didn't find a video stream.");
        return -1; // Didn't find a video stream
    }


    // 获取视频解码信息
//    AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
    if (pCodecCtx == NULL)
    {
        printf("Could not allocate AVCodecContext\n");
        return -1;
    }
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);

    // 获取视频解码器
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        FFLOGI("Codec not found.");
        return -1; // Codec not found
    }
    // 打开视频解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        FFLOGI("Could not open codec.");
        return -1; // Could not open codec
    }

    // 获取native window  SurfaceView
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    // 获取视频宽高
    int videoWidth = pCodecCtx->width;
    int videoHeight = pCodecCtx->height;

    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight,
                                     WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;

//    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
//        FFLOGI("Could not open codec.");
//        return -1; // Could not open codec
//    }

    //分配一个帧指针，指向解码后的原始帧
    AVFrame *pFrame = av_frame_alloc();

    // 用于渲染 RGB
    AVFrame *pFrameRGBA = av_frame_alloc();
    if (pFrameRGBA == NULL || pFrame == NULL) {
        FFLOGI("Could not allocate video frame.");
        return -1;
    }

    // Determine required buffer size and allocate buffer
    // buffer中数据就是用于渲染的,且格式为RGBA
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height,
                                            1);
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
                         pCodecCtx->width, pCodecCtx->height, 1);

    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,
                                                pCodecCtx->height,
                                                pCodecCtx->pix_fmt,
                                                pCodecCtx->width,
                                                pCodecCtx->height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

//    int frameFinished;
    AVPacket* packet  = (AVPacket *)av_malloc(sizeof(AVPacket));
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet->stream_index == videoStream) {

            //视频解码
            int ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                return -1;
            if(avcodec_receive_frame(pCodecCtx, pFrame) != 0)continue;

//            while(avcodec_receive_frame(pCodecCtx, pFrame) == 0){
                // lock native window buffer 写入界面
                ANativeWindow_lock(nativeWindow, &windowBuffer, 0);

                // 格式转换
                sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height,
                          pFrameRGBA->data, pFrameRGBA->linesize);

                // 获取stride
                uint8_t *dst = (uint8_t *) windowBuffer.bits;
                int dstStride = windowBuffer.stride * 4;
                uint8_t *src = (pFrameRGBA->data[0]);
                int srcStride = pFrameRGBA->linesize[0];

                // 由于window的stride和帧的stride不同,因此需要逐行复制
                int h;
                for (h = 0; h < videoHeight; h++) {
                    memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                }

                ANativeWindow_unlockAndPost(nativeWindow);
                //延时绘制 否则视频快速播放
                usleep(1000 * 16);

//            }
//            ret = avcodec_receive_frame(vCodecCtx, vFrame);
//            if (ret < 0 && ret != AVERROR_EOF)
//                return -1;

            // Decode video frame
//            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            // 并不是decode一次就可解码出一帧
//            if (frameFinished) {
//
//            }

        }
        av_packet_unref(packet);
    }

    //释放内存
    av_free(buffer);
    av_free(pFrameRGBA);

    // Free the YUV frame
    av_free(pFrame);

    // Close the codecs
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    av_free(packet);
    env -> ReleaseStringUTFChars(input_jstr, file_name);
    return 0;
}

extern "C" JNIEXPORT jint JNICALL Java_com_bookt_ffmpegdemo_FFmpegJNI_Audioplay
        (JNIEnv *env, jobject clazz,jstring input_jstr) {
    FFLOGI("play");

    // sd卡中的视频文件地址,可自行修改或者通过jni传入
    //char *file_name = "/storage/emulated/0/ws2.mp4";
    const char *file_name = env -> GetStringUTFChars(input_jstr, NULL);

    //    反射得到Class类型
    jclass david_player = env->GetObjectClass(clazz);
//    反射得到createAudio方法
    jmethodID audio_write = env->GetMethodID(david_player, "playTrack", "([BI)V");
    jmethodID createAudio = env->GetMethodID(david_player, "createTrack", "(II)V");

    //注册ffmpeg
    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    // 打开文件
    if (avformat_open_input(&pFormatCtx, file_name, NULL, NULL) != 0) {

        FFLOGI("Couldn't open file:%s\n", file_name);
        return -1; // Couldn't open file
    }

    // 获取文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        FFLOGI("Couldn't find stream information.");
        return -1;
    }

    //查找视频流所在的位置
    //遍历所有类型的流（视频流、音频流可能还有字幕流），找到视频流的位置
    int audio_stream_idx = -1, i;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
            && audio_stream_idx < 0) {
            FFLOGI("  找到音频id %d", pFormatCtx->streams[i]->codecpar->codec_type);
            audio_stream_idx = i;
            break;
        }
    }
    if (audio_stream_idx == -1) {
        FFLOGI("Didn't find a video stream.");
        return -1; // Didn't find a video stream
    }


    // 获取视频解码信息
//    AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
    if (pCodecCtx == NULL)
    {
        printf("Could not allocate AVCodecContext\n");
        return -1;
    }
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[audio_stream_idx]->codecpar);

    // 获取视频解码器
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        FFLOGI("Codec not found.");
        return -1; // Codec not found
    }
    // 打开视频解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        FFLOGI("Could not open codec.");
        return -1; // Could not open codec
    }

    //申请avframe，装解码后的数据
    AVFrame *pFrame = av_frame_alloc();

    //得到SwrContext ，进行重采样
    SwrContext *swrContext = swr_alloc();
    //缓存区
    uint8_t *out_buffer = (uint8_t *) av_malloc(44100 * 2);
    //输出的声道布局（立体声）
    uint64_t  out_ch_layout=AV_CH_LAYOUT_STEREO;
    //输出采样位数  16位
    enum AVSampleFormat out_formart=AV_SAMPLE_FMT_S16;
    //输出的采样率必须与输入相同
    int out_sample_rate = pCodecCtx->sample_rate;
    //swr_alloc_set_opts将PCM源文件的采样格式转换为自己希望的采样格式
    swr_alloc_set_opts(swrContext, out_ch_layout, out_formart, out_sample_rate,
                       pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0,
                       NULL);
    swr_init(swrContext);
    //    获取通道数  2
    int out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

//    反射调用createAudio
    env->CallVoidMethod(clazz, createAudio, 44100, out_channer_nb);


    AVPacket* packet  = (AVPacket *)av_malloc(sizeof(AVPacket));
//    int got_frame;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
//            解码  mp3   编码格式frame----pcm   frame
            //视频解码
            int ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                return -1;
            if(avcodec_receive_frame(pCodecCtx, pFrame) != 0)continue;

            FFLOGI("解码");
            swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) pFrame->data, pFrame->nb_samples);
//                缓冲区的大小
            int size = av_samples_get_buffer_size(NULL, out_channer_nb, pFrame->nb_samples,
                                                  AV_SAMPLE_FMT_S16, 1);
            jbyteArray audio_sample_array = env->NewByteArray(size);
            env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
            env->CallVoidMethod(clazz, audio_write, audio_sample_array, size);
            env->DeleteLocalRef(audio_sample_array);

//            break;
//            while(avcodec_receive_frame(pCodecCtx, pFrame) == 0){
//
//            }
//            avcodec_decode_audio4(pCodecCtx, pFrame, &got_frame, packet);
//            if (got_frame) {
//                FFLOGI("解码");
//                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) pFrame->data, pFrame->nb_samples);
////                缓冲区的大小
//                int size = av_samples_get_buffer_size(NULL, out_channer_nb, pFrame->nb_samples,
//                                                      AV_SAMPLE_FMT_S16, 1);
//                jbyteArray audio_sample_array = env->NewByteArray(size);
//                env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
//                env->CallVoidMethod(clazz, audio_write, audio_sample_array, size);
//                env->DeleteLocalRef(audio_sample_array);
//            }
        }
    }

    av_frame_free(&pFrame);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    env->ReleaseStringUTFChars(input_jstr, file_name);
    av_free(packet);
    return 0;
}

