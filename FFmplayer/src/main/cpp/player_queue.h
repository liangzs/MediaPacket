//
// Created by DELLQ on 24/6/2024.
//

#ifndef MEDIAPACKAGE_PLAYER_QUEUE_H
#define MEDIAPACKAGE_PLAYER_QUEUE_H

#include <queue>
#include "player_status.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};

/**
 * 缓存队列
 */
class PlayerQueue {

public:
    PlayerStatus *status=NULL;
    std::queue<AVPacket *> packetQueue; // 缓存队列
    //put和get要进行动作
    pthread_mutex_t packetQueueMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t packetQueueCond = PTHREAD_COND_INITIALIZER;

public:
    PlayerQueue();

    ~PlayerQueue();

    int putPacket(AVPacket *packet);

    int getPacket(AVPacket *packet);

    int getQueueSize();

    void clearQueue();
};


#endif //MEDIAPACKAGE_PLAYER_QUEUE_H
