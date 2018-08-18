//
// Created by david on 2017/9/27.
//

#include <unistd.h>
#include "FFmpegAudio.h"

FFmpegAudio:: FFmpegAudio() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

void *play_audio(void *args){
    LOGE("开启音频线程");
    FFmpegAudio* audio = (FFmpegAudio *) args;
    AVFrame* frame = av_frame_alloc();
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    //    mp3  里面所包含的编码格式   转换成  pcm
    SwrContext *swrContext = swr_alloc();
    int length=0;
    //    输出采样位数
    //输出的采样率必须与输入相同

    swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_S16,  audio->codecContext->sample_rate,
                       audio->codecContext->channel_layout, audio->codecContext->sample_fmt, audio->codecContext->sample_rate, 0,
                       0);
    swr_init(swrContext);
    uint8_t *out_buffer = (uint8_t *) av_malloc(44100 * 2);
    int channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    int got_frame = 0;
    while (audio->isPlay) {
        audio->deQueue(packet);
        avcodec_decode_audio4(audio->codecContext, frame, &got_frame, packet);
        if (got_frame) {
            LOGE("解码音频 %d",got_frame);
            /**
             * int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                            const uint8_t **in , int in_count);
             */
            swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
            //                通道数
            int out_buffer_size=av_samples_get_buffer_size(NULL,channels, frame->nb_samples,AV_SAMPLE_FMT_S16, 1);
            sleep(1);
            LOGE("解码音频 大小  %d",out_buffer_size);



        }
    }

}

void FFmpegAudio::setCodec(AVCodecContext *codec) {
    codecContext = codec;
}

double FFmpegAudio::getClock() {
    return 0;
}

void FFmpegAudio::play() {
    isPlay = 1;
    pthread_create(&p_playid,NULL, play_audio, this);
}

/**
 * 消费者线程
 * @param packet
 * @return
 */
int FFmpegAudio::deQueue(AVPacket *packet) {
    int ret;
    pthread_mutex_lock(&mutex);
    while (isPlay) {

        if (!queue.empty()) {
            if (av_packet_ref(packet, queue.front()) < 0) {
                ret = false;
                break;
            }
//            不明白
            AVPacket *pkt = queue.front();
            queue.pop();
//            av_packet_unref(pkt);
//            av_free_packet(pkt);
//            av_packet_unref(pkt);
//            av_free_packet(pkt);
            av_free(pkt);
            ret = 1;
            break;
        } else{
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}
/**
 * 压入一帧数据
 * @param packet
 * @return
 */
int FFmpegAudio::enQueue(const AVPacket *packet) {
    AVPacket *packet1 = (AVPacket *) av_malloc(sizeof(AVPacket));
    if (av_packet_ref(packet1, packet)) {
        return 0;
    }
    LOGE("压入一帧音频数据");
    pthread_mutex_lock(&mutex);
    queue.push(packet1);
//    通知消费者  消费帧  不再阻塞
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

FFmpegAudio::~FFmpegAudio() {

}

void FFmpegAudio::setTimeBase(AVRational time_base) {

}

void FFmpegAudio::stop() {

}
