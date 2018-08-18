//
// Created by liuxiang on 2017/9/24.
//

#ifndef DNPLAYER_LOG_H
#define DNPLAYER_LOG_H

#include <android/log.h>

#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"david",FORMAT,##__VA_ARGS__);

#endif //DNPLAYER_LOG_H
