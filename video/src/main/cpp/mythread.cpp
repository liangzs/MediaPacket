//
// Created by DELLQ on 16/7/2024.
//

#include "mythread.h"


void Mythread::start() {
    isPause = false;
    pthread_create(&thread, NULL, startThread, this);
}

Mythread::Mythread() {

}

Mythread::Mythread(char *name) {
    threadName = static_cast<char *>(malloc(strlen(name) + 1));
    strcpy(threadName, name);
}

Mythread::~Mythread() {
    pthread_mutex_destroy(&mutex);
    free(threadName);
    pthread_exit(&thread);
}

void Mythread::pause() {
    isPause = true;
}

void Mythread::join() {
    if (threadName != NULL) {
        LOGE(" thread_join  %s ", threadName);
    }
    if (thread == NULL) {
        LOGE("join thread faild ! may be thread not started !");
        return;
    }
    void *t;
    pthread_join(thread, &t);
}

void *Mythread::startThread(void *data) {
    Mythread *mythread = static_cast<Mythread *>(data);
    mythread->run();
}

void Mythread::stop() {
    isExist = true;
}

void Mythread::resume() {
    isPause = false;
    isExist = false;
}
