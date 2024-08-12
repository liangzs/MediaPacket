//
// Created by Administrator on 2024/7/25.
//

#include "video_reverse.h"

VideoReverse::VideoReverse(char *inputPath, char *outputPath) {
    this->inputPath = static_cast<char *>(malloc(strlen(inputPath) + 1));
    strcpy(this->inputPath, inputPath);
    this->outputPath = static_cast<char *>(malloc(strlen(outputPath) + 1));
    strcpy(this->outputPath, outputPath);
    //设置变量的初始值
    initValue();

    buildInput();
    buildOutput();
}

VideoReverse::~VideoReverse() {

}

void VideoReverse::initValue() {


}

/**
 * 解封装，得到input数据
 * @return
 */
int VideoReverse::buildInput() {
    int ret = open_input_file(this->inputPath, &inFormatCtx);
    if (ret < 0) {
        LOGE("open_input_file fail");
        return -1;
    }
    videoStreamIndex = getVideoDecodeContext(inFormatCtx, &inVCodecCtx);
    audioStreamIndex = getAudioDecodeContext(inFormatCtx, &inACodecCtx);
    //检测是否初始化成功
    if (inVCodecCtx == NULL || videoStreamIndex == -1) {
        LOGE("getVideoDecodeContext fail");
        return -1;
    }
    if (inACodecCtx == NULL || audioStreamIndex == -1) {
        LOGE("getAudioDecodeContext fail");
        return -1;
    }
    inWdith = inFormatCtx->streams[videoStreamIndex]->codecpar->width;
    inHeight = inFormatCtx->streams[videoStreamIndex]->codecpar->height;
    //获取时长
    int64_t duration = inFormatCtx->duration * av_q2d(timeBaseFFmpeg);
    return 0;
}


/**
 * 初始化输出配置
 * @return
 */
int VideoReverse::buildOutput() {
    initOutput(outputPath, &outFormatCtx);
    //添加视频信道,传入inpu的codecParameter
    addOutputVideoStream(outFormatCtx, &outVCodecCtx,
                         *inFormatCtx->streams[videoStreamIndex]->codecpar);

    //添加音频信道
    return 0;
    addOutputAudioStream(outFormatCtx, &outACodecCtx,
                         *inFormatCtx->streams[audioStreamIndex]->codecpar);
}
