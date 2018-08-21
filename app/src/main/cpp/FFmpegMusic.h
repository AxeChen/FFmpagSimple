//
// Created by 11373 on 2018/8/15.
//

#ifndef FFMPAGSIMPLE_FFMPEGMUSIC_H
#define FFMPAGSIMPLE_FFMPEGMUSIC_H
#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <unistd.h>

extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"

#include "libswresample/swresample.h"
}

class FFmpegMusic {

};

int createFFmpe(int *rate,int *channel);

int getPcm(void **pcm,size_t *pcm_size);

void realseFFmpeg();


#endif //FFMPAGSIMPLE_FFMPEGMUSIC_H
