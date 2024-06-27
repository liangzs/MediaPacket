//
// Created by DELLQ on 24/6/2024.
//

#ifndef MEDIAPACKAGE_OPENSL_AUDIO_H
#define MEDIAPACKAGE_OPENSL_AUDIO_H

#include "player_status.h"
#include "player_queue.h"
#include "player_java_call.h"
#include "android_log.h"
#include <string>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "pthread.h"
#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"
#include "libswresample/swresample.h"
#include "libswresample/swresample.h"
};

/**
 * 这里创建opensl的播放，先用ffmpeg进行解包，然后把数据丢到opensl中
 */

class OpenSlAudio {
public:
    int audio_strem_index = -1;
    AVCodecContext *avCodecContext;
    AVFormatContext *avFormatContext;
    AVCodecParameters *avCodecPar;
    pthread_t pthread_docode;
    pthread_t pthread_push_packet;
    Status status=IDLE;
    PlayerQueue *queue = NULL;
    PlayerJavaCall *playerJavaCall;
    uint8_t *outBuffer;//要计算这个值，所以得传一个sample_rate进来
    const int SAMPLE_RATE = 44100;//或者强制写死也行


    AVPacket *avPacket;
    AVFrame *avFrame;
    SwrContext *swrContext = NULL;

    AVRational time_base;
    int64_t duration;
    //opensl
    int sample_rate;
    //引擎
    SLObjectItf engine_object;
    SLEngineItf engine_engine;
    //混音器
    SLObjectItf mix_object;
    //环境混响设置，比如山谷，房间等模拟环境
    SLEnvironmentalReverbItf slEnvironmentalReverbItf;
    SLEnvironmentalReverbSettings slEnvironmentalReverbSettings;

    //播放器
    SLObjectItf pcmplayer_object;
    //播放控制
    SLPlayItf pcmplayer_player;
    //音量控制
    SLVolumeItf pcmplayer_volume;
    //声道播放控制
    SLMuteSoloItf pcmplayer_mutesolo;
    //缓冲队列
    SLAndroidSimpleBufferQueueItf pcmplayer_bufferqueue;

    //因为声音比较敏感，解码时间不一定是正常的时间所以要定义两个世界，一个是解码时间，另外一个是当前的时间
    //当然还涉及到seeek的时候，时间是不一致的
    double clock;
    double time_now;
    //定义一个刷新时间
    double last_time;


public:
    OpenSlAudio(PlayerJavaCall *playerJavaCall1);

    ~OpenSlAudio();

    /**
     *  SL_IID_ENGINE: 引擎接口，用于创建和管理其他 OpenSL ES 对象。
        SL_IID_PLAY: 播放接口，用于控制音频播放（播放、暂停、停止等）。
        SL_IID_BUFFERQUEUE: 缓冲队列接口，用于管理音频数据缓冲区的队列。
        SL_IID_VOLUME: 音量接口，用于控制音量、增益和静音。
        SL_IID_MUTESOLO: 静音与独奏接口，用于设置音频通道的静音和独奏。声道控制
        SL_IID_SEEK: 寻找接口，用于控制音频流的播放位置（快进、倒退）。
        SL_IID_PREFETCHSTATUS: 预取状态接口，用于获取音频数据预取的状态。
        SL_IID_RECORD: 录音接口，用于控制音频录制。
        SL_IID_ANDROIDSIMPLEBUFFERQUEUE: Android 简单缓冲队列接口，类似于 SL_IID_BUFFERQUEUE，但专为 Android 设计。
        SL_IID_ENVIRONMENTALREVERB: 环境混响接口，用于设置和控制环境混响效果。
        SL_IID_EFFECTSEND: 效果发送接口，用于控制音频效果发送。
        SL_IID_3DLOCATION: 3D 位置接口，用于控制和获取 3D 空间中的音频源位置。
        SL_IID_3DDOPPLER: 3D 多普勒接口，用于设置和控制多普勒效应。
        SL_IID_3DGROUPING: 3D 分组接口，用于控制 3D 音频源的分组。
        SL_IID_3DCOMMIT: 3D 提交接口，用于提交 3D 音频源的设置更改。
        SL_IID_3DLOCATION: 3D 位置接口，用于获取和设置 3D 音频源的位置。
        SL_IID_DYNAMICSOURCE: 动态音源接口，用于管理动态音频源。
        SL_IID_OUTPUTMIX: 输出混音器接口，用于管理和控制输出混音器。
        SL_IID_METADATAEXTRACTION: 元数据提取接口，用于提取和获取音频流的元数据。
     */
    void initOpenSl();

    void release();

    void start();

    int getCurrentSampleRateForOpensles(int sample_rate);

    int decodePacket();

    void pushPacket();

    bool checkError(SLresult result, std::string msg);

    bool isPlaying();

};


#endif //MEDIAPACKAGE_OPENSL_AUDIO_H
