//
// Created by DELLQ on 24/6/2024.
//

#ifndef MEDIAPACKAGE_OPENSL_AUDIO_H
#define MEDIAPACKAGE_OPENSL_AUDIO_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "pthread.h"
#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"
};

/**
 * 这里创建opensl的播放，先用ffmpeg进行解包，然后把数据丢到opensl中
 */

class OpenSlAudio {
public:
    int audio_strem_index = -1;
    AVCodecContext *avCodecContext;
    AVCodecParameters *avCodecPar;
    pthread_t pthread_docode;

    //opensl
    //引擎
    SLObjectItf engine_object;
    SLEngineItf engine_sl;
    //混音器
    SLObjectItf mix_object;
    //环境混响设置，比如山谷，房间等模拟环境
    SLEnvironmentalReverbItf slEnvironmentalReverbItf;
    SLEnvironmentalReverbSettings slEnvironmentalReverbSettings;

    //播放器
    SLObjectItf player_object;
    SLPlayItf player_sl;
    SLVolumeItf sl_volume;


public:

    void initOpenSl();

    void release();

    void start();

};


#endif //MEDIAPACKAGE_OPENSL_AUDIO_H
