//
// Created by DELLQ on 24/6/2024.
//

#ifndef MEDIAPACKAGE_ANDROID_LOG_H
#define MEDIAPACKAGE_ANDROID_LOG_H

#include <android/log.h>

#define LOG_ENABLE 1
#define LOG_TAG "FFmpegPlayer"
#ifdef LOG_ENABLE
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...)
#define LOGE(...)
#endif

#endif //MEDIAPACKAGE_ANDROID_LOG_H
