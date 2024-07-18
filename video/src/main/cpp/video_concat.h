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
#include "include/libavutil/fifo.h"
#include "include/libavutil/audio_fifo.h"
#include "include/libavutil/time.h"
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
    int QUEUE_MAX_SIZE = 100;

//input
    AVFormatContext *inFormatContext;
    AVCodecContext *inVideoCodecContext;
    AVCodecContext *inAudioCodecContext;
    AVCodec *inVideoCodec;
    AVCodec *inAudioCodec;
    AVPacket *inPacket;
    int inVideoStreamIndex;
    int inAudioStreamIndex;

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

    AVFrame *outVideoFrame;
    AVFrame *outAudioFrame;

    //重新穿甲缓冲区接受pcm数据(swr后的数据),长度根据采用率*通道数*位数
    uint8_t *pcmOutBuffer;
    //利用audiofifo
    AVAudioFifo *audioFifo;

    //因为是视频的拼接，所以时间戳要重新计算,根据index进行计算，视频就是videoFrameCount*timebase,
    // 音频是波，则是采用率累加，所以两者需要分开计算，然后参数不一致
    int videoFrameCount;
    int videoTimebase;

    int audioFrameCount;
    int audioTimebase;

    char *outPath;
    int outWidth;
    int outHeight;
    //audio output
    AVSampleFormat avSampleFormat;
    int sampleRate;
    uint64_t channelLayout;
    int channels;
    int frameRate;

private:
    int addAudioStream();

    int addVideoStream();

    int initSwsContext();

    int initSwrContext();

    void releaseSwsContext();

    void releaseSwrContext();

    //开始解码
    int startDecode();


    void clearQueue();

    /**
     * 解码包
     * @return
     */
    AVFrame *decodePacket(AVCodecContext *avCodecContext);

    /**
     * 编码包
     * @return
     */
    AVPacket *encodePacket(AVCodecContext *avCodecContext, AVFrame *avFrame);

    /**
     * 单独创建音频avFrame
     * @param size
     * @return
     */
    AVFrame *createAudioFrame(int size);

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
