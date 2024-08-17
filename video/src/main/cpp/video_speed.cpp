//
// Created by Administrator on 2024/8/17.
//

#include "video_speed.h"

VideoSpeed::VideoSpeed(char *inputPath, char *outpath, int speed) {
    this->inputPath = inputPath;
    this->outputPath = outpath;
}

VideoSpeed::~VideoSpeed() {

}

void VideoSpeed::init() {

}

void VideoSpeed::startSpeed() {
    //初始化输入、输出
    int ret = buildInitInput();
    if (ret < 0) {
        LOGE("buildInitInput fail");
    }
    ret = buildOutput();
    if (ret < 0) {
        LOGE("buildOutput fail");
    }
    ret = buildFilter();
    if (ret < 0) {
        LOGE("buildFilter fail");
    }

}

int VideoSpeed::buildInitInput() {
    int ret;
    inputFormatCtx = NULL;
    ret = avformat_open_input(&inputFormatCtx, inputPath, NULL, NULL);
    if (ret < 0) {
        LOGE("avformat_open_input fail");
        return -1;
    }
    inputCodecCtxV = NULL;
    inputStreamIndexV = getVideoDecodeContext(inputFormatCtx, &inputCodecCtxV);
    if (inputStreamIndexV < 0 || inputCodecCtxV == NULL) {
        LOGE("getVideoDecodeContext fail");
        return -1;
    }
    inputCodecCtxA = NULL;
    inputStreamIndexA = getAudioDecodeContext(inputFormatCtx, &inputCodecCtxA);
    if (inputStreamIndexA < 0) {
        LOGE("getAudioDecodeContext fail");
        return -1;
    }
    return 0;
}

/**
 * 默认补偿写入头文件
 * @return
 */
int VideoSpeed::buildOutput() {
    outputForamtCtx = NULL;
    int ret = avformat_init_output(outputForamtCtx, NULL);
    if (ret < 0) {
        LOGE("avformat_init_output fail");
        return -1;
    }
    outputStreamIndexV = addOutputVideoStream(outputForamtCtx, &outputCodecCtxV,
                                              *inputFormatCtx->streams[inputStreamIndexV]->codecpar);
    if (outputStreamIndexV < 0) {
        LOGE("addOutputVideoStream fail");
        return -1;
    }
    outputStreamIndexA = addOutputAudioStream(outputForamtCtx, &outputCodecCtxA,
                                              *inputFormatCtx->streams[inputStreamIndexA]->codecpar);
    if (outputStreamIndexA < 0) {
        LOGE("addOutputAudioStream");
        return -1;
    }
    //写入头文件检测
    ret = writeOutoutHeader(outputForamtCtx, outputPath);
    if (ret < 0) {
        LOGE("writeOutoutHeader fail");
        return -1;
    }
    return 0;
}

int VideoSpeed::buildFilter() {
    int ret = initFilter("setpts=PTS/2", getVideoStreamIndex(inputFormatCtx),
                         inputFormatCtx->streams[videoStreamIndex]->time_base, inputCodecCtxV);
    if (ret < 0) {
        LOGE("initFilter fail");
    }
    return 0;
}
