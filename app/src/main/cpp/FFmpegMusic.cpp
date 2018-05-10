#include "FFmpegMusic.h"

AVFormatContext *pFormatCtx;
AVCodecContext *pCodecCtx;
AVCodec *pCodex;
AVPacket *packet;
AVFrame *frame;
SwrContext *swrContext;
uint8_t *out_buffer;
int out_channer_nb;
int audio_stream_idx;

//opensl es调用 int * rate,int *channel
int createFFmpeg(int *rate, int *channel, const char *input) {

    av_register_all();

    pFormatCtx = avformat_alloc_context();
    FFLOGI("Lujng %s", input);

    // 打开文件
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {

        FFLOGI("打开音频文件失败:%s\n", input);
        return -1;
    }

    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        FFLOGI("%s", "获取视频信息失败");
        return -1;
    }

    //查找视频流所在的位置
    //遍历所有类型的流（视频流、音频流可能还有字幕流），找到视频流的位置
    audio_stream_idx = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
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

// mp3的解码器

//    获取音频编解码器
//    pCodecCtx=pFormatCtx->streams[audio_stream_idx]->codec;
    pCodecCtx = avcodec_alloc_context3(NULL);
    if (pCodecCtx == NULL) {
        printf("Could not allocate AVCodecContext\n");
        return -1;
    }
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[audio_stream_idx]->codecpar);
    FFLOGI("获取视频编码器上下文 %p  ", pCodecCtx);

    pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodex == NULL) {
        FFLOGI("Codec not found.");
        return -1; // Codec not found
    }
    FFLOGI("获取视频编码 %p", pCodex);

    // 打开视频解码器
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {
        FFLOGI("Could not open codec.");
        return -1; // Could not open codec
    }

    packet = (AVPacket *) av_malloc(sizeof(AVPacket));
//    av_init_packet(packet);
//    音频数据

    frame = av_frame_alloc();

//    mp3  里面所包含的编码格式   转换成  pcm   SwcContext
    swrContext = swr_alloc();

//   //缓存区 44100*2
    out_buffer = (uint8_t *) av_malloc(44100 * 2);
    //输出的声道布局（立体声）
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
//    输出采样位数  16位
    enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
//输出的采样率必须与输入相同
    int out_sample_rate = pCodecCtx->sample_rate;

//swr_alloc_set_opts将PCM源文件的采样格式转换为自己希望的采样格式
    swr_alloc_set_opts(swrContext, out_ch_layout, out_formart, out_sample_rate,
                       pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0,
                       NULL);

    swr_init(swrContext);
//    获取通道数  2
    out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    *rate = pCodecCtx->sample_rate;
    *channel = pCodecCtx->channels;
    return 0;
}

//
int getPcm(void **pcm, size_t *pcm_size) {
//    int got_frame;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
//            解码  mp3   编码格式frame----pcm   frame
            int ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                return -1;
            if(avcodec_receive_frame(pCodecCtx, frame) != 0)continue;

            FFLOGI("解码");
            swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data,
                        frame->nb_samples);
//                缓冲区的大小
            int size = av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,
                                                  AV_SAMPLE_FMT_S16, 1);
            *pcm = out_buffer;
            *pcm_size = size;
            break;

//            while (avcodec_receive_frame(pCodecCtx, frame) == 0) {
//                FFLOGI("解码");
//                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data,
//                            frame->nb_samples);
////                缓冲区的大小
//                int size = av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,
//                                                      AV_SAMPLE_FMT_S16, 1);
//                *pcm = out_buffer;
//                *pcm_size = size;
//            break;
//            }

//            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
//            if (got_frame) {
//                FFLOGI("解码");
//                /**
//                 * int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
//                                const uint8_t **in , int in_count);
//                 */
//                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
////                缓冲区的大小
//                int size = av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,
//                                                      AV_SAMPLE_FMT_S16, 1);
//                *pcm = out_buffer;
//                *pcm_size = size;
//                break;
//            }
        }
    }
    return 0;
}


void realseFFmpeg() {
    av_free(packet);
    av_free(out_buffer);
    av_frame_free(&frame);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}