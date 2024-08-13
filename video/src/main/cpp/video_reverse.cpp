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
    yuvSize = inWdith * inHeight * 3 / 2;
    ySize = inWdith * inHeight;
    readBuffer = static_cast<char *>(malloc(yuvSize));
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
    return 1;
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
    addOutputAudioStream(outFormatCtx, &outACodecCtx,
                         *inFormatCtx->streams[audioStreamIndex]->codecpar);
    return 1;
}

const char *tempYuv = "sdcard/FFmpeg/temp.yuv";

/**
 * 这里进行编解码放到缓存区
 */
void VideoReverse::startReverse() {

    this->run();
    //打开文件
    fCache = fopen(tempYuv, "wb+");
    //遍历解码出所有的关键帧时间戳
    int ret = 0;
    while (!isExist) {
        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(inFormatCtx, packet);
        if (ret < 0) {
            av_packet_free(&packet);
            break;
        }
        if (packet->stream_index == inputVideoStreamIndex) {
            if (packet->flags & AV_PKT_FLAG_KEY) {
                keyFrameTime.push_back(packet->pts);
            }
            av_packet_free(&packet);
        } else if (packet->stream_index == inputAudioStreamIndex) {
            //音频不做处理，直接缓存输出，需要转换一下package的时间戳
            av_packet_rescale_ts(packet, inFormatCtx->streams[audioStreamIndex]->time_base,
                                 outFormatCtx->streams[audioOutputStreamIndex]->time_base);
            audioPackages.push(packet);
        }
    }

    //从最后一帧开始解析，先seek到最后一帧的关键帧，然后得到gop，然后反向写入gop序列
    nowKeyFrame = keyFrameTime.size() - 1;
    av_seek_frame(inFormatCtx, videoStreamIndex, keyFrameTime.at(nowKeyFrame),
                  AVSEEK_FLAG_BACKWARD);


}

/**
 * 这里是遍历缓存进行读取数据
 */
void VideoReverse::run() {

}
