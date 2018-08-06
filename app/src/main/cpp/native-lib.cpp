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

}
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);

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
        if(avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
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
    int codecOpenCode = avcodec_open2(avCodecContext,avCodec,NULL);
    if(codecOpenCode < 0){
        LOGE("初始化AVCodec失败");
        return;
    }


    // 开始解析视频流
    // 首先这是mp4 如果需要解析成 yuv 需要用到 SwsContext
    // 构造函数传入的参数为 原视频的宽高、像素格式、目标的宽高这里也取原视频的宽高（可以修改参数）
    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL );

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

        sws_scale(swsContext, (const uint8_t *const *) srcFrame->data, srcFrame->linesize, 0, srcFrame->height, dstFrame->data,
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
        if(avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
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
    int codecOpenCode = avcodec_open2(avCodecContext,avCodec,NULL);
    if(codecOpenCode < 0){
        LOGE("初始化AVCodec失败");
        return;
    }


    // 开始解析视频流
    // 首先这是mp4 如果需要解析成 yuv 需要用到 SwsContext
    // 构造函数传入的参数为 原视频的宽高、像素格式、目标的宽高这里也取原视频的宽高（可以修改参数）
    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL );

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

        if(got_frame){
            // 设置缓存区
            ANativeWindow_setBuffersGeometry(nativeWindow,avCodecContext->width, avCodecContext->height,WINDOW_FORMAT_RGBA_8888);
            ANativeWindow_lock(nativeWindow, &outBuffer, NULL);

            // 绘制
            // 将h264的格式转化成rgb
            sws_scale(swsContext, (const uint8_t *const *) srcFrame->data, srcFrame->linesize, 0, srcFrame->height, dstFrame->data,
                      dstFrame->linesize
            );

            // 一帧的具体字节大小
            uint8_t *dst = static_cast<uint8_t *>(outBuffer.bits);

            // 每一个像素的字节  ARGB 一共是四个字节
            int dstStride = outBuffer.stride*4;

            // 像素数据的首地址
           uint8_t *src = dstFrame->data[0];

            int srcStride = dstFrame->linesize[0];

            // 将 dstFrame的数据 一行行复制到屏幕上去
            for (int i = 0; i < avCodecContext->height; ++i) {
                memcpy(dst+i*dstStride,src+i*srcStride,srcStride);
            }

            ANativeWindow_unlockAndPost(nativeWindow);
            usleep(1000*16);
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