//
// Created by DELLQ on 24/6/2024.
//

#ifndef MEDIAPACKAGE_PLAYER_JAVA_CALL_H
#define MEDIAPACKAGE_PLAYER_JAVA_CALL_H

#define MAIN_THREAD 0
#define CHILD_THREAD 1

#include <jni.h>

class PlayerJavaCall {
    JavaVM *javaVm = NULL;
    JNIEnv *jniEnv = NULL;
    jobject jobj = NULL;

    jmethodID jmid_prepared = NULL;
    jmethodID jmid_progress = NULL;

public:
    PlayerJavaCall(JavaVM *vm, JNIEnv *env, jobject object);

    ~PlayerJavaCall();

    void onCallJavaPrepared(int type);

    void onCallJavaProgress(int type,int progress, int duration);
};


#endif //MEDIAPACKAGE_PLAYER_JAVA_CALL_H
