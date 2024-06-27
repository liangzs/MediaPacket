//
// Created by DELLQ on 24/6/2024.
//

#include "player_java_call.h"
#include "android_log.h"

PlayerJavaCall::PlayerJavaCall(JavaVM *vm, JNIEnv *env, jobject object) {
    this->javaVm = vm;
    this->jniEnv = env;
    this->jobj = env->NewGlobalRef(object);
    jclass jclazz = env->GetObjectClass(object);
    this->jmid_prepared = env->GetMethodID(jclazz, "onCallJavaPrepared", "()V");
    this->jmid_progress = env->GetMethodID(jclazz, "onCallJavaProgress", "(II)V");
}

PlayerJavaCall::~PlayerJavaCall() {

}

/**
 * //需要检测是否在当前线程调用还是在子线程进行调用
 */
void PlayerJavaCall::onCallJavaPrepared(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_prepared);
        return;
    }
    //子线程
    //重新获取java_vm中的jnienv对象
    JNIEnv *jniEnv1;
    if (javaVm->AttachCurrentThread(&jniEnv1, 0) != JNI_OK) {
        LOGE("jniEnv1 AttachCurrentThread error");
        return;
    }
    jniEnv1->CallVoidMethod(jobj, jmid_prepared);
    javaVm->DetachCurrentThread();
}

/**
 * //需要检测是否在当前线程调用还是在子线程进行调用
 * @param progress
 * @param duration
 */
void PlayerJavaCall::onCallJavaProgress(int type, int progress, int duration) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_progress, progress, duration);
        return;
    }
    //子线程
    JNIEnv *jniEnv1;
    if (javaVm->AttachCurrentThread(&jniEnv1, 0) != JNI_OK) {
        LOGE("jniEnv1 AttachCurrentThread error");
        return;
    }
    jniEnv1->CallVoidMethod(jobj, jmid_progress, progress, duration);
    javaVm->DetachCurrentThread();
}
