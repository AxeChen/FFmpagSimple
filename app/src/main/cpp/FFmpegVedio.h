//
// Created by david on 2017/9/27.
//



#ifndef FFMPEGOPENSL_FFMPEGVEDIO_H
#define FFMPEGOPENSL_FFMPEGVEDIO_H



#include "FFmpegAudio.h"
#include "Log.h"
extern "C"
{
#include <pthread.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

class FFmpegVedio{
public:
    FFmpegVedio();

    ~FFmpegVedio();

    int enQueue(AVPacket *avPacket);

    int deQueue(AVPacket *avPacket);

    void play(FFmpegAudio *audio);

    void setAvCodecContext(AVCodecContext *codec);
    void setTimeBase(AVRational time_base);

    void stop();

public:
    double clock;
    AVCodecContext *codec;
    pthread_t  p_id;
    int isPlay;
    int index;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    //    视频解码队列
    std::queue<AVPacket*> queue;
};
};

#endif //FFMPEGOPENSL_FFMPEGVEDIO_H