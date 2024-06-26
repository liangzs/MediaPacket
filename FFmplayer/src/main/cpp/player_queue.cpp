//
// Created by DELLQ on 24/6/2024.
//

#include "player_queue.h"

PlayerQueue::PlayerQueue() {

}

int PlayerQueue::putPacket(AVPacket *packet) {
    pthread_mutex_lock(&this->packetQueueMutex);
    this->packetQueue.push(packet);
    //通知消费者
    pthread_cond_signal(&packetQueueCond);
    pthread_mutex_unlock(&this->packetQueueMutex);
    return 0;
}

PlayerQueue::~PlayerQueue() {
    getQueueSize();
}

int PlayerQueue::getPacket(AVPacket *packet) {
    pthread_mutex_lock(&this->packetQueueMutex);

    if (status != NULL && status->isPlaying()) {
        if (packetQueue.empty()) {
            pthread_cond_wait(&packetQueueCond, &packetQueueMutex);
            return 0;
        }
        AVPacket *currentPacket = packetQueue.front();
        if (av_packet_ref(packet, currentPacket) == 0) {
            packetQueue.pop();
        }
        //使用后即释放
//        av_free_packet(currentPacket);
        av_free(currentPacket);
        currentPacket = NULL;
    }
    //消费者等待数据上来

    pthread_mutex_unlock(&this->packetQueueMutex);
    return 0;
}

int PlayerQueue::getQueueSize() {
    return this->packetQueue.size();
}

void PlayerQueue::clearQueue() {
    pthread_cond_signal(&packetQueueCond);
    pthread_mutex_lock(&this->packetQueueMutex);
    while (!packetQueue.empty()) {
        AVPacket *packet = packetQueue.front();
        packetQueue.pop();
//        av_free_packet(packet);
        av_free(packet);
        packet = NULL;
    }

    pthread_mutex_unlock(&packetQueueMutex);
}
