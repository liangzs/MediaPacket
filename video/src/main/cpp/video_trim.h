//
// Created by DELLQ on 28/8/2024.
//

#ifndef VIDEOMAKERTAB_VIDEO_TRIM_H
#define VIDEOMAKERTAB_VIDEO_TRIM_H

#include "base_interface.h"
#include "thread"
#include "media_util_jni.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
};

/**
 * 针对单个文件的精准trim
 */
class VideoTrim : BaseInterface {
public:
    pthread_t pthread;
private:
    bool isCancel = false;
    AVRational curRational;

//    in
    AVFormatContext *avFormatContextIn = NULL;
    AVCodecContext *avCtxVideoIn = NULL;
    AVCodec *avCodecVideoIn = NULL;
    char *inputPath;
    int startTime;
    int endTime;
    int offsetTime = 0;//时间戳起始时间

//    out
    char *outPath;
    AVFormatContext *avFormatContextOut = NULL;
    AVCodecContext *avCtxVideoOut = NULL;
    AVPacket *avPacket;
    AVOutputFormat *afot = NULL;
    AVCodec *avCodecVideoOut = NULL;
    bool hadDrawKeyFrame;


private:
    int buildInput();

    int buildOutput();

    void startDecode();

    int writePackPage(AVFormatContext *avFormatContext, AVStream *inStream, AVStream *outStream,
                      AVPacket *avPacket);
    void copyContext(AVCodecContext *vCtxIn,AVCodecContext **vCtxE);

public:
    VideoTrim(const char *inputPath, const char *output, long start, long end);

    ~VideoTrim();

    void trimImpl();

    void writeData();

    void readData();

    void cancel();
};


#endif //VIDEOMAKERTAB_VIDEO_TRIM_H
