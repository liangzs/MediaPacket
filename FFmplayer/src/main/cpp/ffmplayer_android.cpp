#include <jni.h>
#include <string>
#include "ffmpeg_player.h"

FfmpegPlayer *player = NULL;
PlayerStatus *playerStatus;


extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_ffmplayer_FFmPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path) {
    char *char_path = const_cast<char *>(env->GetStringUTFChars(path, 0));
    if (player == NULL) {
        playerStatus = new PlayerStatus();
        player = new FfmpegPlayer(playerStatus, char_path);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_ffmplayer_FFmPlayer_prepare(JNIEnv *env, jobject thiz) {
}
extern "C"
JNIEXPORT void JNICALL
Java_com_liangzs_ffmplayer_FFmPlayer_start(JNIEnv *env, jobject thiz) {
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