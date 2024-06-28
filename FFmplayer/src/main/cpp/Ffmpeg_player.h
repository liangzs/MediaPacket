//
// Created by DELLQ on 24/6/2024.
//

#ifndef MEDIAPACKAGE_FFMPEG_PLAYER_H
#define MEDIAPACKAGE_FFMPEG_PLAYER_H

#include "player_status.h"
#include "android_log.h"
#include "player_java_call.h"
#include "opensl_audio.h"

extern "C" {
#include "pthread.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
};


/**
 * 在析构函数中清理构造参数对象
 */
class FfmpegPlayer {

public:
    OpenSlAudio *audioPlayer = NULL;
    PlayerJavaCall *java_call = NULL;
    const char *char_path;

    pthread_t pthread_decode;

public:
    FfmpegPlayer(PlayerJavaCall *javaCall, const char *path);

    ~FfmpegPlayer();

    /**
     * 需要在子线程中执行
     */
    void prepare();

    /**
     * 线程中执行解码过程
     */
    void decodeThread();

    void start();

    void pause();

    void stop();

    void release();

    void setMute(int mute);

    void resume();
};


#endif //MEDIAPACKAGE_FFMPEG_PLAYER_H
