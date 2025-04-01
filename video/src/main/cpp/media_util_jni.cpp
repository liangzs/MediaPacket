//
// Created by DELLQ on 21/10/2024.
//
#include <string>
#include <jni.h>
#include <time.h>
#include <android/log.h>
#include <setjmp.h>
#include "ffmpeg/tool/include/video_trim.h"
#include "include/video_clip.h"
#include "ffmpeg/tool/include/media_util_jni.h"

JNIEnv *m_env = NULL;
jclass tagetClass;
jobject tagetObject;
VideoTrim *videoTrim;
//VideoClip *videoTrim;


// 定义代码跳转锚点
sigjmp_buf JUMP_ANCHOR;
volatile sig_atomic_t error_cnt = 0;

void exception_handler(int signal) {
    callBackError();
    error_cnt += 1;
    LOGE("catch Exception gifloader!");
    siglongjmp(JUMP_ANCHOR, 1);
}

//    sleep(2);
void registerSingleTrycath() {
    // 注册要捕捉的系统信号量
    struct sigaction sigact;
    struct sigaction old_action;
    sigaction(SIGABRT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) {
        sigset_t block_mask;
        sigemptyset(&block_mask);
        sigaddset(&block_mask, SIGABRT); // handler处理捕捉到的信号量时，需要阻塞的信号
        sigaddset(&block_mask, SIGSEGV); // handler处理捕捉到的信号量时，需要阻塞的信号

        sigemptyset(&sigact.sa_mask);
        sigact.sa_flags = 0;
        sigact.sa_mask = block_mask;
        sigact.sa_handler = exception_handler;
        sigaction(SIGABRT, &sigact, NULL); // 注册要捕捉的信号
        sigaction(SIGSEGV, &sigact, NULL); // 注册要捕捉的信号
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ijoysoft_mediasdk_module_ffmpeg_FFmpegMediaTool_videoTrimNative(JNIEnv *env, jobject thiz,
                                                                         jstring input_path,
                                                                         jstring output_path,
                                                                         jint start,
                                                                         jint end) {
//    registerSingleTrycath();
    tagetClass = env->GetObjectClass(thiz);
    tagetObject = thiz;
    m_env = env;
    const char *inputPath = const_cast<char *>(env->GetStringUTFChars(input_path, 0));
    const char *outPath = const_cast<char *>(env->GetStringUTFChars(output_path, 0));
//    videoTrim = new VideoClip(inputPath, outPath, start, end);
    videoTrim = new VideoTrim(inputPath, outPath, start, end);
    videoTrim->trimImpl();

    env->ReleaseStringUTFChars(input_path, inputPath);
    env->ReleaseStringUTFChars(output_path, outPath);
    delete videoTrim;
    videoTrim = NULL;
    m_env->DeleteLocalRef(tagetClass);

}

void callProgress(int progress) {
    LOGI("media util progress:%d", progress);
//    jclass javaClass = m_env->FindClass("com/ijoysoft/mediasdk/module/ffmpeg/FFmpegMediaTool");
    if (tagetClass == NULL) {
        LOGE("callProgress clazz isNULL");
        return;
    }

    jmethodID methodID = m_env->GetMethodID(tagetClass, "progress", "(I)V");
    if (methodID == NULL) {
        LOGE("callProgress methodID isNULL");
        return;
    }
    //调用该java方法
    m_env->CallVoidMethod(tagetObject, methodID, progress);
}

void callBackError() {
    LOGI("media util error callBackError");
//    jclass javaClass = m_env->FindClass("com/ijoysoft/mediasdk/module/ffmpeg/FFmpegMediaTool");
    if (tagetClass == NULL) {
        LOGE("callProgress clazz isNULL");
        return;
    }

    jmethodID methodID = m_env->GetMethodID(tagetClass, "error", "()V");
    if (methodID == NULL) {
        LOGE("callProgress methodID isNULL");
        return;
    }
    //调用该java方法
    m_env->CallVoidMethod(tagetObject, methodID);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ijoysoft_mediasdk_module_ffmpeg_FFmpegMediaTool_cancelNative(JNIEnv *env, jobject thiz) {
    if (videoTrim != NULL) {
        videoTrim->cancel();
    }

}