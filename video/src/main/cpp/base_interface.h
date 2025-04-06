
#ifndef MEDIAPACKAGE_BASE_INTERFACE_H
#define MEDIAPACKAGE_BASE_INTERFACE_H

#include "android_log.h"
#include <stdio.h>
#include <vector>

extern "C" {
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libavfilter/avfiltergraph.h"
#include "include/libavfilter/buffersink.h"
#include "include/libavfilter/buffersrc.h"
#include "include/libavutil/opt.h"
};

class BaseInterface {
protected:
    //输入
    int videoStreamIndex;
    int audioStreamIndex;
    //进度
    int progress;
    AVRational timeBaseFFmpeg;

    AVStream *avStreamVideoIn;
    AVStream *avStreamVideoOut;

    AVStream *avStreamAudioIn;
    AVStream *avStreamAudioOut;

    //时间戳索引计算
    int videoPtsIndex;
    int audioPtsIndex;
    int64_t *startPts;//int 数组,记录音、视频
    int64_t *startDts;//int 数组


    int open_input_file(const char *filename, AVFormatContext **ps);

    int getVideoDecodeContext(AVFormatContext *ps,
                              AVCodecContext **dec_ctx); //获取视频解码器，并返回videoStreamIndex;
    int getAudioDecodeContext(AVFormatContext *ps,
                              AVCodecContext **dec_ctx); //获取音频解码器，并返回audioStreamIndex;
    AVFrame *decodePacket(AVCodecContext *decode, AVPacket *packet);

    AVPacket *encodeFrame(AVFrame *frame, AVCodecContext *codecContext);

    int getVideoStreamIndex(AVFormatContext *fmt_ctx);

    int getAudioStreamIndex(AVFormatContext *fmt_ctx);

    //输出
    int videoOutputStreamIndex;
    int audioOutputStreamIndex;

    int initOutput(const char *ouput, AVFormatContext **ctx);

    int outFrameRate;

    int initOutput(const char *ouput, const char *format, AVFormatContext **ctx);

    int addOutputVideoStream(AVFormatContext *afc_output, AVCodecContext **vCtxE,
                             AVCodecParameters codecpar);

    int addOutputVideoStreamCopy(AVFormatContext *afc_output, AVCodecContext **vCtxE,
                                 AVFormatContext *afc_input);

    int addOutputAudioStream(AVFormatContext *afc_output, AVCodecContext **aCtxE,
                             AVCodecParameters codecpar);

    int getVideoOutFrameRate();

    int writeOutoutHeader(AVFormatContext *afc_output, const char *outputPath);

    int parseVideoParams(int *params, int size, AVCodecParameters *codecpar);

    int getVideoOutputStreamIndex();

    int getAudioOutputStreamIndex();

    int writeTrail(AVFormatContext *afc_output);


public:
    BaseInterface();

    ~BaseInterface();
};


#endif //MEDIAPACKAGE_BASE_INTERFACE_H
