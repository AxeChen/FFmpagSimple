//
// Created by 11373 on 2018/8/15.
//

#include "FFmpegMusic.h"
extern "C" {
#include "Log.h"
}

AVFormatContext *avFormatContext;
AVCodecContext *avCodecContext;
AVCodec *avCodec;
AVPacket *avPacket;
AVFrame *srcFrame;
SwrContext *swrContext;
u_int8_t *out_buffer;
int out_channel_nb;
int auto_stream_index;

int createFFmpe(int *rate,int *channel){
    const char *input = "/sdcard/input.mp3";
    // 初始化ffmpag，这个是使用ffmpag前必须要使用的东西
    av_register_all();

    /// 获取AVFormatContext ,这个类用于解码操作
    avFormatContext = avformat_alloc_context();

    // 打开一个输入流并读取标题。编解码器没有打开。
    // 流必须用avformatcloseinput（）关闭。 来自翻译  如果返回0则获取成功，-1则获取失败
    int code = avformat_open_input(&avFormatContext, input, NULL, NULL);
    if (code < 0) {
        LOGE("打开一个输入流并读取标题失败");
        return -1;
    }

    // 读取一个媒体文件的数据包以获取流信息  如果返回0则获取成功，-1则获取失败
    int findCode = avformat_find_stream_info(avFormatContext, NULL);
    if (findCode < 0) {
        LOGE("读取一个媒体文件的数据包以获取流信息失败");
        return -1;
    }

    // 从视频的不同的流获取视频流坐标，视频一般分为音频流，视频流。
    // 从mp4格式中找到音频流的位置
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
    avCodecContext = avFormatContext->streams[auto_stream_index]->codec;

    // Find a registered decoder with a matching codec ID.
    // 找到一个带有匹配的编解码器ID的注册解码器。
    // 获取解码器
    avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    // 打开解码器
    // Initialize the AVCodecContext to use the given AVCodec. Prior to using this
    // function the context has to be allocated with avcodec_alloc_context3().
    // 初始化AVCodecContext以使用给定的AVCodec。之前使用这个
    // 必须使用avcodecalloc context3（）分配上下文。
    int codecOpenCode = avcodec_open2(avCodecContext, avCodec, NULL);
    if (codecOpenCode < 0) {
        LOGE("初始化AVCodec失败");
        return -1;
    }

    // 解析的每一帧都是 AVPacket
    // 初始化AVPacket对象
    avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(avPacket);

    // 初始化AvFrame
    // This structure describes decoded (raw) audio or video data.
    // 这个结构描述了解码（原始的）音频或视频数据。
    // 这里需要将AvPacket(帧)里面的数据解析到AvFrame中去。
    srcFrame = av_frame_alloc();


    // 将mp3里面所包含的编码格式转化成 pcm格式
    // 音频格式需要有特定的context
    swrContext = swr_alloc();

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
    out_buffer = (u_int8_t *) av_malloc(44100 * 2);

    // 基于指定的图像参数设置图像字段
    // 以及所提供的图像数据缓冲区。
    avpicture_fill((AVPicture *) (dstFrame), out_buffer, AV_PIX_FMT_RGBA, avCodecContext->width,
                   avCodecContext->height);
//    获取通道数  2
    out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    *rate = avCodecContext->sample_rate;
    *channel = avCodecContext->channels;
    return 0;
}

int getPcm(void **pcm,size_t *pcm_size){
    int got_frame;
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
                *pcm = out_buffer;
                *pcm_size = size;
                break;
            }
        }
    }
    return 0;
}

void realseFFmpeg(){
    // 释放资源
    av_free_packet(avPacket);
    av_frame_free(&srcFrame);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(avCodecContext);
    avformat_close_input(&avFormatContext);
}