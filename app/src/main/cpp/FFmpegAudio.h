//
// Created by david on 2017/9/27.
//

#ifndef FFMPEGOPENSL_FFMPEGAUDIO_H
#define FFMPEGOPENSL_FFMPEGAUDIO_H



#include <queue>
#include <jni.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <pthread.h>
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
#include "Log.h"

class FFmpegAudio{
public:
    FFmpegAudio();

    ~FFmpegAudio();

    int enQueue(const AVPacket *packet);

    int deQueue(AVPacket *packet);
    void play();

    void setCodec(AVCodecContext *codec);

    double getClock();

    void setTimeBase(AVRational time_base);
    void stop();
public:
    AVCodecContext *codecContext;
    int isPlay;
//    音频播放线程
    pthread_t p_playid;
//    音频解码队列
    std::queue<AVPacket*> queue;
    AVRational time_base;
//
    int index;
    JavaVM *vm;
    /**
     * 同步
     */
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};
};
#endif //FFMPEGOPENSL_FFMPEGAUDIO_H