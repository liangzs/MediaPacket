//
// Created by DELLQ on 24/6/2024.
//

#include "player_queue.h"

PlayerQueue::PlayerQueue() {

}

int PlayerQueue::putPacket(AVPacket *packet) {
    pthread_mutex_lock(&this->packetQueueMutex);

    //通知消费者
    pthread_cond_signal(&packetQueueCond);
    pthread_mutex_unlock(&this->packetQueueMutex);
    return 0;
}

PlayerQueue::~PlayerQueue() {

}

int PlayerQueue::getPacket(AVPacket *packet) {
    pthread_mutex_lock(&this->packetQueueMutex);

    //消费者等待数据上来
    pthread_cond_wait(&packetQueueCond,&packetQueueMutex);
    pthread_mutex_unlock(&this->packetQueueMutex);
    return 0;
}

int PlayerQueue::getQueueSize() {
    return 0;
}

void PlayerQueue::clearQueue() {

}
