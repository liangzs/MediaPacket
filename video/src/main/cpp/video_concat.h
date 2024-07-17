//
// Created by DELLQ on 16/7/2024.
//

#ifndef MEDIAPACKAGE_VIDEO_CONCAT_H
#define MEDIAPACKAGE_VIDEO_CONCAT_H

#include "android_log.h"
#include <string>
#include <pthread.h>
#include <vector>
#include <queue>
#include "mythread.h"

extern "C" {
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#include "include/libavutil/frame.h"
#include "include/libswresample/swresample.h"
#include "include/libswscale/swscale.h"
};

/**
 * 因为是不同的文件，所以视频和音频都需要重接编解码，重采样的动作
 * 视频用sws，音频用swr
 */
class VideoConcat : public Mythread {
private:
    std::vector<char *> inputPaths;
    std::queue<AVPacket *> queueVideo;
    std::queue<AVPacket *> queueAudio;


//input
    AVFormatContext *inFormatContext;
    AVCodecContext *inVideoCodecContext;
    AVCodecContext *inAudioCodecContext;
    AVCodec *inVideoCodec;
    AVCodec *inAudioCodec;
    AVPacket *inPacket;
    AVFrame *inFrame;
    int inVideoStreamIndex;
    int inAudioStreamIndex;

    AVStream *inVideoStream;
    AVStream *inAudioStream;

    SwsContext *inSwsContext;
    SwrContext *inSwrContext;

    int inWidth;
    int inHeight;

//output
    AVFormatContext *outFormatContext;
    AVStream *outVideoStream;
    AVStream *outAudioStream;
    AVOutputFormat *oformat;

    AVCodecContext *outVideoCodecContext;
    AVCodecContext *outAudioCodecContext;
    AVCodec *outVideoCodec;
    AVCodec *outAudioCodec;
    char *outPath;
    int outWidth;
    int outHeight;
    //audio output
    AVSampleFormat avSampleFormat;
    int sampleRate;
    uint64_t channelLayout;
    int channels;

private:
    int addAudioStream();

    int addVideoStream();

    int initSwsContext();

    int initSwrContext();

    void releaseSwsContext();

    void releaseSwrContext();

public:
    VideoConcat(std::vector<char *> inputPaths, char *outputPath, int width, int height);

    ~VideoConcat();

    int startConcat();

    int initInput(char *inputPath);

    /**
     * 先清理输入参数
     */
    void releaseInput();

    int initOuput();

    void run() override;


};


#endif //MEDIAPACKAGE_VIDEO_CONCAT_H
