//
// Created by Administrator on 2024/7/25.
//

#include "base_interface.h"

int BaseInterface::open_input_file(const char *filename, AVFormatContext **avformatCtx) {
    int ret = avformat_open_input(avformatCtx, filename, NULL, NULL);
    if (ret < 0) {
        LOGE("avformat_open_input fail:%s", filename);
        return ret;
    }
    if (avformatCtx == NULL) {
        LOGE("alloc AVFormatContext fail");
        return -1;
    }
    //寻找编码
    ret = avformat_find_stream_info(*avformatCtx, NULL);
    if (ret < 0) {
        LOGE("avformat_find_stream_info fail:%s", filename);
        return ret;
    }
    return 1;
}

int BaseInterface::getVideoStreamIndex(AVFormatContext *fmt_ctx) {
    if (videoStreamIndex == -1) {
        videoStreamIndex = av_find_best_stream(fmt_ctx, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1,
                                               NULL,
                                               0);
    }
    return videoStreamIndex;
}

int BaseInterface::getAudioStreamIndex(AVFormatContext *fmt_ctx) {
    if (audioStreamIndex == -1) {
        audioStreamIndex = av_find_best_stream(fmt_ctx, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1,
                                               NULL, 0);
    }
    return audioStreamIndex;
}

/**
 * 对codecContex赋值
 * @param avFormatContext
 * @param dec_ctx
 * @return
 */
int BaseInterface::getVideoDecodeContext(AVFormatContext *avFormatContext,
                                         AVCodecContext **codecContext) {
    //传统做法遍历streamindex获得videostreamindex
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;

        } else if (avFormatContext->streams[i]->codecpar->codec_type ==
                   AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
        }
    }
    //直接通过bestIndex方法直接获取index
//    videoStreamIndex = av_find_best_stream(avFormatContext, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1,
//                                           NULL, 0);
//    audioStreamIndex = av_find_best_stream(avFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1,
//                                           NULL, 0);

    AVCodec *avCodec;
    *codecContext = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(*codecContext,
                                  avFormatContext->streams[videoStreamIndex]->codecpar);
    int ret = avcodec_open2(*codecContext, avCodec, NULL);
    if (ret < 0) {
        LOGE("avcodec_open2 fail");
        return -1;
    }

    return audioStreamIndex;
}

int BaseInterface::getAudioDecodeContext(AVFormatContext *ps, AVCodecContext **dec_ctx) {
    int ret;
    audioStreamIndex = av_find_best_stream(ps, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

    //对引入入参进行赋值
    AVCodec *avCodec = NULL;
    *dec_ctx = avcodec_alloc_context3(avCodec);

    avcodec_parameters_to_context(*dec_ctx, ps->streams[audioStreamIndex]->codecpar);

    //打开编码
    ret = avcodec_open2(*dec_ctx, avCodec, NULL);
    if (ret < 0) {
        LOGE("avcodec_open2 fail");
        return -1;
    }

    return audioStreamIndex;
}

BaseInterface::BaseInterface() {
    timeBaseFFmpeg = AVRational{1, AV_TIME_BASE};
}

BaseInterface::~BaseInterface() {

}
