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
    return 0;
}

int BaseInterface::getAudioStreamIndex(AVFormatContext *fmt_ctx) {
    return 0;
}

int BaseInterface::getVideoDecodeContext(AVFormatContext *ps, AVCodecContext **dec_ctx) {
    //传统做法遍历streamindex获得videostreamindex

    //直接通过bestIndex方法直接获取index
    avcodec_alloc_context3();
    return 0;
}

int BaseInterface::getAudioDecodeContext(AVFormatContext *ps, AVCodecContext **dec_ctx) {
    return 0;
}
