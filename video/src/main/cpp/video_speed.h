//
// Created by Administrator on 2024/8/17.
//

#ifndef MEDIAPACKAGE_VIDEO_SPEED_H
#define MEDIAPACKAGE_VIDEO_SPEED_H

#include "base_interface.h"
#include "android_log.h"
#include "filter_interface.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
};

/**
 * 用ffmpeg内部的滤镜实现视频的变速播放
 */
class VideoSpeed : FilterInterface {
public:
    char *inputPath;
    char *outputPath;
    //input
    AVFormatContext *inputFormatCtx;
    AVCodecContext *inputCodecCtxV;
    AVCodecContext *inputCodecCtxA;
    int inputStreamIndexV;
    int inputStreamIndexA;

    //output
    AVFormatContext *outputForamtCtx;
    AVCodecContext *outputCodecCtxV;
    AVCodecContext *outputCodecCtxA;
    int outputStreamIndexV;
    int outputStreamIndexA;

public:
    VideoSpeed(char *inputPath, char *outpaht, int speed);

    ~VideoSpeed();

    void init();

    void startSpeed();

    //初始化输入输出、滤镜
    int buildInitInput();

    int buildOutput();

    int buildFilter();

};


#endif //MEDIAPACKAGE_VIDEO_SPEED_H
