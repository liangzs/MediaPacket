//
// Created by Administrator on 2024/10/27.
//

#include "opencv_jni.h"
#include <jni.h>
#include "opencv2/opencv.hpp"

using namespace cv;
extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_opencv_OpencvJni_init(JNIEnv *env, jobject thiz, jstring path) {
    const char *pPath = env->GetStringUTFChars(path, 0);
    Ptr<CascadeClassifier> classifier = makePtr<CascadeClassifier>(pPath);
    LOGI("init success");

    env->ReleaseStringUTFChars(path, pPath);
}