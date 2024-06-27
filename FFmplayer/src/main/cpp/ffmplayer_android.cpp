#include <jni.h>
#include <string>
#include "ffmpeg_player.h"
#include "player_java_call.h"

FfmpegPlayer *player = NULL;

JavaVM *javaVm;
PlayerJavaCall *javaCall;


/**
 * onloadLibray时候会自动调用这个方法
 * @param vm
 * @param reserved
 * @return
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVm = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_ffmplayer_FFmPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path) {
    char *char_path = const_cast<char *>(env->GetStringUTFChars(path, 0));
    LOGI("setDataSource: %s", char_path);
    if (javaCall == NULL) {
        javaCall = new PlayerJavaCall(javaVm, env, thiz);
    }
    if (player == NULL) {
        player = new FfmpegPlayer(javaCall, char_path);
    }
    player->prepare();
    LOGI("prepare success");

}

extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_ffmplayer_FFmPlayer_prepare(JNIEnv *env, jobject thiz) {
}
extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_ffmplayer_FFmPlayer_start(JNIEnv *env, jobject thiz) {
    if (player != NULL) {
        player->start();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_ffmplayer_FFmPlayer_pause(JNIEnv *env, jobject thiz) {
}
extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_ffmplayer_FFmPlayer_stop(JNIEnv *env, jobject thiz) {
}
extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_ffmplayer_FFmPlayer_release(JNIEnv *env, jobject thiz) {
}