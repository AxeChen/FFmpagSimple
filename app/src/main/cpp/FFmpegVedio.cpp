//
// Created by david on 2017/9/27.
//
#include <unistd.h>
#include "FFmpegVedio.h"

FFmpegVedio::FFmpegVedio() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

}

void FFmpegVedio::setAvCodecContext(AVCodecContext *codec) {
        this->codec=codec;
}

void FFmpegVedio::setTimeBase(AVRational time_base) {

}

void FFmpegVedio::stop() {

}

void *play_vedio(void *arg){
    LOGE("开启视频线程");
    FFmpegVedio *vedio = (FFmpegVedio *) arg;

    AVFrame *rgbFrame=av_frame_alloc();
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t    *out_buffer= (uint8_t *) av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA, vedio->codec->width, vedio->codec->height));
    LOGE("宽  %d,  高  %d  ",vedio->codec->width,vedio->codec->height);
//设置yuvFrame的缓冲区，像素格式
    int re= avpicture_fill((AVPicture *) rgbFrame, out_buffer, AV_PIX_FMT_RGBA, vedio->codec->width, vedio->codec->height);
    int frameCount=0;
    SwsContext *swsContext = sws_getContext(vedio->codec->width,vedio->codec->height,vedio->codec->pix_fmt,
                                            vedio->codec->width,vedio->codec->height,AV_PIX_FMT_RGBA
            ,SWS_BILINEAR,NULL,NULL,NULL
    );
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();
    int got_frame;
    int length;
    while (vedio->isPlay) {
//        解码一帧数据
        LOGE("视频 解码一帧数据 ");

        vedio->deQueue(packet);
        LOGE("视频队列的长度  %d",vedio->queue.size());
        length = avcodec_decode_video2(vedio->codec, frame, &got_frame, packet);

//非零   正在解码
        if (got_frame) {
            LOGE("视频 解码%d帧", frameCount++);
            //转为指定的YUV420P
            sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                      frame->height, rgbFrame->data,
                      rgbFrame->linesize);
            sleep(1);
        }
        av_packet_unref(packet);
        av_frame_unref(frame);
    }


}

void FFmpegVedio::play(FFmpegAudio *audio) {
    isPlay=1;
    pthread_create(&p_id,0,play_vedio,this);

}

int FFmpegVedio::enQueue(AVPacket *packet) {
    AVPacket *packet1 = (AVPacket *) av_malloc(sizeof(AVPacket));
    if (av_packet_ref(packet1, packet)) {
        return 0;
    }
    LOGE("压入一帧视频数据");
    pthread_mutex_lock(&mutex);
    queue.push(packet1);
//    通知消费者  消费帧  不再阻塞
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

int FFmpegVedio::deQueue(AVPacket *packet) {
    int ret;
    pthread_mutex_lock(&mutex);
    while (isPlay) {
        LOGE("视频取一帧");
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

