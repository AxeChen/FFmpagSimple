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

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>


}
#include "FFmpegMusic.h"
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"jason",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);

extern "C" JNIEXPORT jstring
JNICALL
Java_com_app_axe_ffmpagsimple_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


extern "C"
JNIEXPORT void JNICALL
Java_com_app_axe_ffmpagsimple_MainActivity_change(JNIEnv *env, jobject instance, jstring inputStr_,
                                                  jstring outputStr_) {
    const char *inputStr = env->GetStringUTFChars(inputStr_, 0);
    const char *outputStr = env->GetStringUTFChars(outputStr_, 0);

    // 初始化ffmpag，这个是使用ffmpag前必须要使用的东西
    av_register_all();

    // 获取AVFormatContext ,这个类用于解码操作
    AVFormatContext *avFormatContext = avformat_alloc_context();

    // 打开一个输入流并读取标题。编解码器没有打开。
    // 流必须用avformatcloseinput（）关闭。 来自翻译  如果返回0则获取成功，-1则获取失败
    int code = avformat_open_input(&avFormatContext, inputStr, NULL, NULL);
    if (code < 0) {
        LOGE("打开一个输入流并读取标题失败");
        return;
    }

    // 读取一个媒体文件的数据包以获取流信息  如果返回0则获取成功，-1则获取失败
    int findCode = avformat_find_stream_info(avFormatContext, NULL);
    if (findCode < 0) {
        LOGE("读取一个媒体文件的数据包以获取流信息失败");
        return;
    }

    // 从视频的不同的流获取视频流坐标，视频一般分为音频流，视频流。
    // 从mp4格式中找到视频流的位置
    int video_stream_ids = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        // 通过获取codec_type 是否是 AVMEDIA_TYPE_VIDEO ， AVMEDIA_TYPE_VIDEO代码表视频流
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 在这里获取到视频流在流数组中的下标
            video_stream_ids = i;
            break;
        }
    }

    // Codec context associated with this stream. Allocated and freed by  libavformat.
    // 与此流相关的编解码上下文。由libavformat分配和释放。
    // 获取到解码器上下文。根据视频流id获取
    AVCodecContext *avCodecContext = avFormatContext->streams[video_stream_ids]->codec;

    // Find a registered decoder with a matching codec ID.
    // 找到一个带有匹配的编解码器ID的注册解码器。
    // 获取解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    // 打开解码器
    // Initialize the AVCodecContext to use the given AVCodec. Prior to using this
    // function the context has to be allocated with avcodec_alloc_context3().
    // 初始化AVCodecContext以使用给定的AVCodec。之前使用这个
    // 必须使用avcodecalloc context3（）分配上下文。
    int codecOpenCode = avcodec_open2(avCodecContext, avCodec, NULL);
    if (codecOpenCode < 0) {
        LOGE("初始化AVCodec失败");
        return;
    }


    // 开始解析视频流
    // 首先这是mp4 如果需要解析成 yuv 需要用到 SwsContext
    // 构造函数传入的参数为 原视频的宽高、像素格式、目标的宽高这里也取原视频的宽高（可以修改参数）
    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    // 解析的每一帧都是 AVPacket
    // 初始化AVPacket对象
    AVPacket *avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(avPacket);

    // 初始化AvFrame
    // This structure describes decoded (raw) audio or video data.
    // 这个结构描述了解码（原始的）音频或视频数据。
    // 这里需要将AvPacket(帧)里面的数据解析到AvFrame中去。
    AVFrame *srcFrame = av_frame_alloc();

    // 初始化 目标 Frame
    AVFrame *dstFrame = av_frame_alloc();
    // dstFrame分配内存
    u_int8_t *out_buffer = (u_int8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_YUV420P, avCodecContext->width, avCodecContext->height));

    // 基于指定的图像参数设置图像字段
    // 以及所提供的图像数据缓冲区。
    avpicture_fill((AVPicture *) (dstFrame), out_buffer, AV_PIX_FMT_YUV420P, avCodecContext->width,
                   avCodecContext->height);

    FILE *fp_yuv = fopen(outputStr, "wb");

    int got_frame;
    // Return the next frame of a stream.
    // 返回视频流的下一帧，这里会将帧保存在AvPacket。av_read_frame之后，avPacket中就会有 帧的数据
    while (av_read_frame(avFormatContext, avPacket) >= 0) {
        // 将原视频的帧数据（AvPacket）解析到 srcFrame中
        // got_frame  got_picture_ptr Zero if no frame could be decompressed, otherwise, it is nonzero.
        // 如果没有任何框架可以解压缩，那么它就是非零的。如果这个东西大于0则代表解析结束
        avcodec_decode_video2(avCodecContext, srcFrame, &got_frame, avPacket);

        // 这里用到了 SwsContext ，因为这了会将原视频的数据srcFrame转化在目标dstFrame中去。
        // Scale the image slice in srcSlice and put the resulting scaled
        // slice in the image in dst. A slice is a sequence of consecutive
        // rows in an image.
        //在src切片中缩放图像切片，并在dst的图像中放置相应的比例片。一个切片是连续的序列行一个图像。

        sws_scale(swsContext, (const uint8_t *const *) srcFrame->data, srcFrame->linesize, 0,
                  srcFrame->height, dstFrame->data,
                  dstFrame->linesize
        );

        // 当 这个值 > 0 就意味着解析完毕
        if (got_frame > 0) {
            // 解析完最后一帧
            sws_scale(swsContext, (const uint8_t *const *) srcFrame->data, srcFrame->linesize, 0,
                      srcFrame->height, dstFrame->data,
                      dstFrame->linesize
            );
            int y_size = avCodecContext->width * avCodecContext->height;
            // 如果解析完毕，就将文件写入到内置卡中去
            // 这里用到的时 fwrite方法。
            fwrite(dstFrame->data[0], 1, y_size, fp_yuv);
            fwrite(dstFrame->data[1], 1, y_size / 4, fp_yuv);
            fwrite(dstFrame->data[2], 1, y_size / 4, fp_yuv);
        }

        // 释放帧数据
        av_free_packet(avPacket);
    }

    // 解析完全之后，释放资源
    fclose(fp_yuv);
    av_frame_free(&srcFrame);
    av_frame_free(&dstFrame);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);
    env->ReleaseStringUTFChars(inputStr_, inputStr);
    env->ReleaseStringUTFChars(outputStr_, outputStr);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_app_axe_ffmpagsimple_play_VideoView_render(JNIEnv *env, jobject instance, jstring input_,
                                                    jobject surface) {
    const char *input = env->GetStringUTFChars(input_, 0);

    // TODO
    // 初始化ffmpag，这个是使用ffmpag前必须要使用的东西
    av_register_all();

    // 获取AVFormatContext ,这个类用于解码操作
    AVFormatContext *avFormatContext = avformat_alloc_context();

    // 打开一个输入流并读取标题。编解码器没有打开。
    // 流必须用avformatcloseinput（）关闭。 来自翻译  如果返回0则获取成功，-1则获取失败
    int code = avformat_open_input(&avFormatContext, input, NULL, NULL);
    if (code < 0) {
        LOGE("打开一个输入流并读取标题失败");
        return;
    }

    // 读取一个媒体文件的数据包以获取流信息  如果返回0则获取成功，-1则获取失败
    int findCode = avformat_find_stream_info(avFormatContext, NULL);
    if (findCode < 0) {
        LOGE("读取一个媒体文件的数据包以获取流信息失败");
        return;
    }

    // 从视频的不同的流获取视频流坐标，视频一般分为音频流，视频流。
    // 从mp4格式中找到视频流的位置
    int video_stream_ids = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        // 通过获取codec_type 是否是 AVMEDIA_TYPE_VIDEO ， AVMEDIA_TYPE_VIDEO代码表视频流
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 在这里获取到视频流在流数组中的下标
            video_stream_ids = i;
            break;
        }
    }

    // Codec context associated with this stream. Allocated and freed by  libavformat.
    // 与此流相关的编解码上下文。由libavformat分配和释放。
    // 获取到解码器上下文。根据视频流id获取
    AVCodecContext *avCodecContext = avFormatContext->streams[video_stream_ids]->codec;

    // Find a registered decoder with a matching codec ID.
    // 找到一个带有匹配的编解码器ID的注册解码器。
    // 获取解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    // 打开解码器
    // Initialize the AVCodecContext to use the given AVCodec. Prior to using this
    // function the context has to be allocated with avcodec_alloc_context3().
    // 初始化AVCodecContext以使用给定的AVCodec。之前使用这个
    // 必须使用avcodecalloc context3（）分配上下文。
    int codecOpenCode = avcodec_open2(avCodecContext, avCodec, NULL);
    if (codecOpenCode < 0) {
        LOGE("初始化AVCodec失败");
        return;
    }


    // 开始解析视频流
    // 首先这是mp4 如果需要解析成 yuv 需要用到 SwsContext
    // 构造函数传入的参数为 原视频的宽高、像素格式、目标的宽高这里也取原视频的宽高（可以修改参数）
    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);

    // 解析的每一帧都是 AVPacket
    // 初始化AVPacket对象
    AVPacket *avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(avPacket);

    // 初始化AvFrame
    // This structure describes decoded (raw) audio or video data.
    // 这个结构描述了解码（原始的）音频或视频数据。
    // 这里需要将AvPacket(帧)里面的数据解析到AvFrame中去。
    AVFrame *srcFrame = av_frame_alloc();

    // 初始化 目标 Frame
    AVFrame *dstFrame = av_frame_alloc();
    // dstFrame分配内存
    u_int8_t *out_buffer = (u_int8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_RGBA, avCodecContext->width, avCodecContext->height));

    // 基于指定的图像参数设置图像字段
    // 以及所提供的图像数据缓冲区。
    avpicture_fill((AVPicture *) (dstFrame), out_buffer, AV_PIX_FMT_RGBA, avCodecContext->width,
                   avCodecContext->height);

    int got_frame;
    // Return the next frame of a stream.
    // 返回视频流的下一帧，这里会将帧保存在AvPacket。av_read_frame之后，avPacket中就会有 帧的数据

    // Opaque type that provides access to a native window.
    //提供对本机窗口的访问
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
//    视频缓冲区
    ANativeWindow_Buffer outBuffer;
//    ANativeWindow

    int length = 0;
    while (av_read_frame(avFormatContext, avPacket) >= 0) {
        length = avcodec_decode_video2(avCodecContext, srcFrame, &got_frame, avPacket);

        if (got_frame) {
            // 设置缓存区
            ANativeWindow_setBuffersGeometry(nativeWindow, avCodecContext->width,
                                             avCodecContext->height, WINDOW_FORMAT_RGBA_8888);
            ANativeWindow_lock(nativeWindow, &outBuffer, NULL);

            // 绘制
            // 将h264的格式转化成rgb
            sws_scale(swsContext, (const uint8_t *const *) srcFrame->data, srcFrame->linesize, 0,
                      srcFrame->height, dstFrame->data,
                      dstFrame->linesize
            );

            // 一帧的具体字节大小
            uint8_t *dst = static_cast<uint8_t *>(outBuffer.bits);

            // 每一个像素的字节  ARGB 一共是四个字节
            int dstStride = outBuffer.stride * 4;

            // 像素数据的首地址
            uint8_t *src = dstFrame->data[0];

            int srcStride = dstFrame->linesize[0];

            // 将 dstFrame的数据 一行行复制到屏幕上去
            for (int i = 0; i < avCodecContext->height; ++i) {
                memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
            }

            ANativeWindow_unlockAndPost(nativeWindow);
            usleep(1000 * 16);
        }

        av_free_packet(avPacket);

    }

    ANativeWindow_release(nativeWindow);

    av_frame_free(&srcFrame);
    av_frame_free(&dstFrame);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);


    env->ReleaseStringUTFChars(input_, input);
}

/**
 * 将mp3转化成pcm
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_app_axe_ffmpagsimple_pcm_ChangeToPcmActivity_changeMp3(JNIEnv *env, jobject instance,
                                                                jstring input_, jstring outStr_) {
    const char *inputStr = env->GetStringUTFChars(input_, 0);
    const char *outputStr = env->GetStringUTFChars(outStr_, 0);

    // 初始化ffmpag，这个是使用ffmpag前必须要使用的东西
    av_register_all();

    // 获取AVFormatContext ,这个类用于解码操作
    AVFormatContext *avFormatContext = avformat_alloc_context();

    // 打开一个输入流并读取标题。编解码器没有打开。
    // 流必须用avformatcloseinput（）关闭。 来自翻译  如果返回0则获取成功，-1则获取失败
    int code = avformat_open_input(&avFormatContext, inputStr, NULL, NULL);
    if (code < 0) {
        LOGE("打开一个输入流并读取标题失败");
        return;
    }

    // 读取一个媒体文件的数据包以获取流信息  如果返回0则获取成功，-1则获取失败
    int findCode = avformat_find_stream_info(avFormatContext, NULL);
    if (findCode < 0) {
        LOGE("读取一个媒体文件的数据包以获取流信息失败");
        return;
    }

    // 从视频的不同的流获取视频流坐标，视频一般分为音频流，视频流。
    // 从mp4格式中找到视频流的位置
    int auto_stream_index = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        // 通过获取codec_type 是否是 AVMEDIA_TYPE_VIDEO ， AVMEDIA_TYPE_VIDEO代码表视频流
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            // 在这里获取到视频流在流数组中的下标
            auto_stream_index = i;
            break;
        }
    }

    // Codec context associated with this stream. Allocated and freed by  libavformat.
    // 与此流相关的编解码上下文。由libavformat分配和释放。
    // 获取到解码器上下文。根据视频流id获取
    AVCodecContext *avCodecContext = avFormatContext->streams[auto_stream_index]->codec;

    // Find a registered decoder with a matching codec ID.
    // 找到一个带有匹配的编解码器ID的注册解码器。
    // 获取解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    // 打开解码器
    // Initialize the AVCodecContext to use the given AVCodec. Prior to using this
    // function the context has to be allocated with avcodec_alloc_context3().
    // 初始化AVCodecContext以使用给定的AVCodec。之前使用这个
    // 必须使用avcodecalloc context3（）分配上下文。
    int codecOpenCode = avcodec_open2(avCodecContext, avCodec, NULL);
    if (codecOpenCode < 0) {
        LOGE("初始化AVCodec失败");
        return;
    }

    // 解析的每一帧都是 AVPacket
    // 初始化AVPacket对象
    AVPacket *avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(avPacket);

    // 初始化AvFrame
    // This structure describes decoded (raw) audio or video data.
    // 这个结构描述了解码（原始的）音频或视频数据。
    // 这里需要将AvPacket(帧)里面的数据解析到AvFrame中去。
    AVFrame *srcFrame = av_frame_alloc();


    // 将mp3里面所包含的编码格式转化成 pcm格式
    // 音频格式需要有特定的context
    SwrContext *swrContext = swr_alloc();

    // 定义输出的通道
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    // 采样位数
    AVSampleFormat out_format = AV_SAMPLE_FMT_S16;

    // 输出的采样率必须和输入相同
    int out_sample_rate = avCodecContext->sample_rate;

    swr_alloc_set_opts(swrContext,
                       out_ch_layout,
                       out_format,
                       out_sample_rate,
                       avCodecContext->channel_layout,
                       avCodecContext->sample_fmt,
                       avCodecContext->sample_rate,
                       0, NULL);
    swr_init(swrContext);

    FILE *pcm = fopen(outputStr, "wb");
    int got_frame;


    // 初始化 目标 Frame
    AVFrame *dstFrame = av_frame_alloc();
    // dstFrame分配内存
    // 人的最大识别hz * 2 双通道
    u_int8_t *out_buffer = (u_int8_t *) av_malloc(44100 * 2);

    // 基于指定的图像参数设置图像字段
    // 以及所提供的图像数据缓冲区。
    avpicture_fill((AVPicture *) (dstFrame), out_buffer, AV_PIX_FMT_RGBA, avCodecContext->width,
                   avCodecContext->height);
//    获取通道数  2
    int out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

    while (av_read_frame(avFormatContext, avPacket) >= 0) {
        if (avPacket->stream_index == auto_stream_index) {
            avcodec_decode_audio4(avCodecContext, srcFrame, &got_frame, avPacket);
            if (got_frame) {
                LOGE("解码")
                swr_convert(swrContext, &out_buffer, 44100 * 2,
                            (const uint8_t **) (srcFrame->data),
                            srcFrame->nb_samples);
                size_t size = static_cast<size_t>(av_samples_get_buffer_size(NULL, out_channel_nb,
                                                                             srcFrame->nb_samples,
                                                                             AV_SAMPLE_FMT_S16, 1));
                fwrite(out_buffer, 1, size, pcm);
            }
        }
    }
    // 释放资源
    fclose(pcm);
    av_frame_free(&srcFrame);
    swr_free(&swrContext);
    avcodec_close(avCodecContext);
    avformat_close_input(&avFormatContext);

    env->ReleaseStringUTFChars(input_, inputStr);
    env->ReleaseStringUTFChars(outStr_, outputStr);
}

/**
 * 将MP3的数据直接播放
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_app_axe_ffmpagsimple_pcm_PlayMp3Activity_play(JNIEnv *env, jobject instance,
                                                       jstring input_) {
    const char *input = env->GetStringUTFChars(input_, 0);
    // 初始化ffmpag，这个是使用ffmpag前必须要使用的东西
    av_register_all();

    /// 获取AVFormatContext ,这个类用于解码操作
    AVFormatContext *avFormatContext = avformat_alloc_context();

    // 打开一个输入流并读取标题。编解码器没有打开。
    // 流必须用avformatcloseinput（）关闭。 来自翻译  如果返回0则获取成功，-1则获取失败
    int code = avformat_open_input(&avFormatContext, input, NULL, NULL);
    if (code < 0) {
        LOGE("打开一个输入流并读取标题失败");
        return;
    }

    // 读取一个媒体文件的数据包以获取流信息  如果返回0则获取成功，-1则获取失败
    int findCode = avformat_find_stream_info(avFormatContext, NULL);
    if (findCode < 0) {
        LOGE("读取一个媒体文件的数据包以获取流信息失败");
        return;
    }

    // 从视频的不同的流获取视频流坐标，视频一般分为音频流，视频流。
    // 从mp4格式中找到音频流的位置
    int auto_stream_index = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        // 通过获取codec_type 是否是 AVMEDIA_TYPE_VIDEO ， AVMEDIA_TYPE_VIDEO代码表视频流
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            // 在这里获取到视频流在流数组中的下标
            auto_stream_index = i;
            break;
        }
    }

    // Codec context associated with this stream. Allocated and freed by  libavformat.
    // 与此流相关的编解码上下文。由libavformat分配和释放。
    // 获取到解码器上下文。根据视频流id获取
    AVCodecContext *avCodecContext = avFormatContext->streams[auto_stream_index]->codec;

    // Find a registered decoder with a matching codec ID.
    // 找到一个带有匹配的编解码器ID的注册解码器。
    // 获取解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    // 打开解码器
    // Initialize the AVCodecContext to use the given AVCodec. Prior to using this
    // function the context has to be allocated with avcodec_alloc_context3().
    // 初始化AVCodecContext以使用给定的AVCodec。之前使用这个
    // 必须使用avcodecalloc context3（）分配上下文。
    int codecOpenCode = avcodec_open2(avCodecContext, avCodec, NULL);
    if (codecOpenCode < 0) {
        LOGE("初始化AVCodec失败");
        return;
    }

    // 解析的每一帧都是 AVPacket
    // 初始化AVPacket对象
    AVPacket *avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(avPacket);

    // 初始化AvFrame
    // This structure describes decoded (raw) audio or video data.
    // 这个结构描述了解码（原始的）音频或视频数据。
    // 这里需要将AvPacket(帧)里面的数据解析到AvFrame中去。
    AVFrame *srcFrame = av_frame_alloc();


    // 将mp3里面所包含的编码格式转化成 pcm格式
    // 音频格式需要有特定的context
    SwrContext *swrContext = swr_alloc();

    // 定义输出的通道
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    // 采样位数
    AVSampleFormat out_format = AV_SAMPLE_FMT_S16;

    // 输出的采样率必须和输入相同
    int out_sample_rate = avCodecContext->sample_rate;

    swr_alloc_set_opts(swrContext,
                       out_ch_layout,
                       out_format,
                       out_sample_rate,
                       avCodecContext->channel_layout,
                       avCodecContext->sample_fmt,
                       avCodecContext->sample_rate,
                       0, NULL);
    swr_init(swrContext);

    // 开始执行解码操作

    int got_frame;


    // 初始化 目标 Frame
    AVFrame *dstFrame = av_frame_alloc();
    // dstFrame分配内存
    // 人的最大识别hz * 2 双通道
    u_int8_t *out_buffer = (u_int8_t *) av_malloc(44100 * 2);

    // 基于指定的图像参数设置图像字段
    // 以及所提供的图像数据缓冲区。
    avpicture_fill((AVPicture *) (dstFrame), out_buffer, AV_PIX_FMT_RGBA, avCodecContext->width,
                   avCodecContext->height);
//    获取通道数  2
    int out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

    //    反射得到Class类型 （c层调用java层的代码）
    jclass david_player = env->GetObjectClass(instance);
//    反射得到createAudio方法
    jmethodID createAudio = env->GetMethodID(david_player, "createAudio", "(II)V");
//    反射调用createAudio
    env->CallVoidMethod(instance, createAudio, 44100, out_channel_nb);
    jmethodID audio_write = env->GetMethodID(david_player, "playTrack", "([BI)V");
////    输出文件

    while (av_read_frame(avFormatContext, avPacket) >= 0) {
        if (avPacket->stream_index == auto_stream_index) {
            avcodec_decode_audio4(avCodecContext, srcFrame, &got_frame, avPacket);
            if (got_frame) {
                LOGE("解码")
                swr_convert(swrContext, &out_buffer, 44100 * 2,
                            (const uint8_t **) (srcFrame->data),
                            srcFrame->nb_samples);
                size_t size = static_cast<size_t>(av_samples_get_buffer_size(NULL, out_channel_nb,
                                                                             srcFrame->nb_samples,
                                                                             AV_SAMPLE_FMT_S16, 1));
                jbyteArray audio_sample_array = env->NewByteArray(size);
                // 这边都是jni层调用Java代码
                env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
                env->CallVoidMethod(instance, audio_write, audio_sample_array, size);
                env->DeleteLocalRef(audio_sample_array);
            }
        }
    }

    // 释放资源
    av_frame_free(&srcFrame);
    swr_free(&swrContext);
    avcodec_close(avCodecContext);
    avformat_close_input(&avFormatContext);

    env->ReleaseStringUTFChars(input_, input);
}


SLObjectItf engineObject;
SLEngineItf  engineEngine;
SLObjectItf outputMixObject;
SLEnvironmentalReverbItf slEnvironmentalReverbItf;
SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
SLObjectItf slPlayItf;
SLPlayItf  bqPlayerPlay;
SLAndroidSimpleBufferQueueItf  bqPalyerQueue;
//音量对象
SLVolumeItf bqPalyerVolume;

int sLresult;
void *buffer;
size_t bufferSize = 0;
//只要喇叭一读完  就会回调此函数，添加pcm数据到缓冲区
void bqPlayerCallBack(SLAndroidSimpleBufferQueueItf bq, void *context){
    bufferSize=0;
//    取到音频数据了
    getPcm(&buffer, &bufferSize);
    if (NULL != buffer && 0 != bufferSize) {
//        播放的关键地方
        SLresult  lresult=(*bqPalyerQueue)->Enqueue(bqPalyerQueue, buffer, bufferSize);
        LOGE("正在播放%d ",lresult);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_app_axe_ffmpagsimple_opensl_OpenSlEsPlayActivity_play(JNIEnv *env, jobject instance) {

    // 初始化OpenSL引擎
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    // OpenSL Realize
    (*engineObject)->Realize(engineObject,SL_BOOLEAN_FALSE);

    // 获取到引擎接口
    (*engineObject)->GetInterface(engineObject,SL_IID_ENGINE,&engineEngine);
    LOGE("引擎地址%p",engineEngine)
    // 创建混音器
    (*engineEngine)->CreateOutputMix(engineEngine,&outputMixObject,0,0,0);
    // 混音器Realize
    (*outputMixObject)->Realize(outputMixObject,SL_BOOLEAN_FALSE);

    // 设置环境混响
    sLresult = (*outputMixObject)->GetInterface(outputMixObject,SL_IID_ENVIRONMENTALREVERB,&slEnvironmentalReverbItf);
    LOGE("环境混响地址%p",slEnvironmentalReverbItf)

    if(SL_RESULT_SUCCESS==sLresult){
        (*slEnvironmentalReverbItf)->SetEnvironmentalReverbProperties(slEnvironmentalReverbItf,&settings);
    }

    int rate;
    int channers;
    createFFmpe(&rate, &channers);
    SLDataLocator_AndroidBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
    SLDataFormat_PCM slDataFormat_pcm = {
            SL_DATAFORMAT_PCM,
            (const SLuint32 )channers,
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource slDataSource={
        &android_queue,&slDataFormat_pcm
    };


    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX,outputMixObject};

    const SLInterfaceID  ids[3]={SL_IID_BUFFERQUEUE,SL_IID_EFFECTSEND,SL_IID_VOLUME};
    const SLboolean req[3]={SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE};
    // 设置混音器
    SLDataSink audioSDK = {&outputMix,NULL};
    // 创建播放接口
    (*engineEngine)->CreateAudioPlayer(engineEngine,&slPlayItf,&slDataSource,&audioSDK,3,ids,req);

    (*slPlayItf)->Realize(slPlayItf,SL_BOOLEAN_FALSE);

    (*slPlayItf)->GetInterface(slPlayItf,SL_IID_PLAY,&bqPlayerPlay);

    // 注册缓冲区
    sLresult=(*slPlayItf)->GetInterface(slPlayItf, SL_IID_BUFFERQUEUE, &bqPalyerQueue);

    // 设置回调接口

    (*bqPalyerQueue)->RegisterCallback(bqPalyerQueue, bqPlayerCallBack, NULL);

    (*slPlayItf)->GetInterface(slPlayItf, SL_IID_VOLUME,&bqPalyerVolume);
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay,SL_PLAYSTATE_PLAYING);

//    播放第一帧
    bqPlayerCallBack(bqPalyerQueue, NULL);
}
















