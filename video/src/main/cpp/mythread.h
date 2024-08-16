//
// Created by DELLQ on 16/7/2024.
//

#ifndef MEDIAPACKAGE_MYTHREAD_H
#define MEDIAPACKAGE_MYTHREAD_H

#include "pthread.h"
#include "stdlib.h"
#include "string"
#include "android_log.h"
#include <time.h>

extern "C" {
#include "libavutil/time.h"
};

/**
 * 定义一个thread，可以实现类似java调用thread的习惯
 */
class Mythread {
private:
    pthread_t thread;
    char *threadName;

    static void *startThread(void *);

public:
    bool isExist;//退出线程
    bool isPause;//等待线程

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


    Mythread();

    Mythread(char *threadName);

    ~Mythread();

    void start();

    void pause();

    void resume();

    void join();

    void stop();

    void sleep(int ms);


    virtual void run() = 0;

};


#endif //MEDIAPACKAGE_MYTHREAD_H
