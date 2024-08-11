//
// Created by Administrator on 2024/7/25.
//

#ifndef MEDIAPACKAGE_VIDEO_REVERSE_H
#define MEDIAPACKAGE_VIDEO_REVERSE_H

#include <stdio.h>
#include "mythread.h"
#include "base_interface.h"

extern "C" {
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
};
using namespace std;

/**
 * 倒序只针对视频，不针对音频
 * 倒序功能，读取packet时候，如果是关键帧的时候做一个分割，这期间所有的packet作为一个gop进行存储
 * 把gop存储到tempfile文件从，然后从文件倒序读取进行编码输出
 *
 * 存入和读取应该都是按照原始数据进行处理，即通过yuv的数据格式进行固定存读取
 */
class VideoReverse : Mythread, BaseInterface {

private:
    //input
    char *inputPath;
    int inputVideoStreamIndex;
    int inputAudioStreamIndex;
    AVFormatContext *inFormatCtx;
    AVCodecContext *inVCodecCtx;
    AVCodecContext *inACodecCtx;

    //output
    char *outputPath;
    AVFormatContext *outForamtCtx;
    AVCodecContext *outVCodecCtx;
    AVCodecContext *outACodecCtx;
    int inWdith;
    int inHeight;
    //YUV数据
    FILE *fCache;

    //编解码
    vector<int64_t> keyFrameTimeStamps;//解码前遍历所以的关键帧并保存，后续拿这个做seek操作来读取一个gop


public:
    VideoReverse(char *inputPath, char *outputPath);

    ~VideoReverse();

    void initValue();

    int buildInput();

    int buildOutput();

};


#endif //MEDIAPACKAGE_VIDEO_REVERSE_H
