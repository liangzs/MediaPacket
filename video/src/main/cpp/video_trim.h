//
// Created by DELLQ on 11/7/2024.
//

#ifndef MEDIAPACKAGE_VIDEO_TRIM_H
#define MEDIAPACKAGE_VIDEO_TRIM_H

#include "android_log.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "pthread.h"
};

/**
 * 视频裁剪,其实就是seek到具体位置，把trim内的packet写入到文件
 */
class VideoTrim {
public:
    //放到子线程取处理
    pthread_t trimThread;
private:
    int startTime = 0;
    int endTime = 0;
    char *inputPath;
    char *outputPath;

    //ffmpeg
    AVFormatContext *inputFormatContext;
    AVFormatContext *outputFormatContext;
    AVPacket *avPacket;
    int videoStreamIndex = -1;
    int audioStreamIndex = -1;

    AVCodecContext *avCtxD;
    AVCodecContext *avCtxE;
    AVCodec *avCodecD;
    AVCodec *avCodecE;

    int width;
    int height;

    AVStream *videoStream;
    AVStream *audioStream;
    //外写文件
    AVOutputFormat *avOutputFormat;
    AVStream *outputVideoStream;
    AVStream *outputAudioStream;

public:
    VideoTrim(char *inputPath, char *outputpath, long startTime, long endTime);

    ~VideoTrim();

    void startTrim();


private:
    int initInput();

    int initOutput();

    int decodeEncode();

    AVFrame *decodePackage();

    AVPacket *encodePackage(AVFrame *frame);

    void trimImpl();

    /**
     * 修改广告库，需要
     */
    void writePacket(AVPacket *packet);

    int addVideoStream(int width, int height);

    int addAudioStream();
};


#endif //MEDIAPACKAGE_VIDEO_TRIM_H
