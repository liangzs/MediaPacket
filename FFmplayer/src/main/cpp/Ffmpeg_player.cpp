//
// Created by DELLQ on 24/6/2024.
//

#include "Ffmpeg_player.h"

FfmpegPlayer::FfmpegPlayer(PlayerJavaCall *javaCall, const char *path) {
    this->java_call = javaCall;
    this->char_path = path;
    if (audioPlayer == NULL) {
        audioPlayer = new OpenSlAudio(this->java_call);
    }
}

FfmpegPlayer::~FfmpegPlayer() {
    delete java_call;
    delete audioPlayer;
}

void *extendCDecode(void *data) {
    FfmpegPlayer *player = static_cast<FfmpegPlayer *>(data);
    LOGI("prepare decode");
    player->decodeThread();
    //销毁线程
    pthread_exit(&player->pthread_decode);
}

void FfmpegPlayer::prepare() {
    pthread_create(&pthread_decode, NULL, extendCDecode, this);
}


void FfmpegPlayer::decodeThread() {
    //1.注册组件
    av_register_all();
    //2.封装格式上下文
    avformat_network_init();
    //3.打开输入视频文件
    AVFormatContext *avFormatContext = avformat_alloc_context();
    if (avformat_open_input(&avFormatContext, this->char_path, NULL, NULL) != 0) {
        LOGE("打开视频文件失败");
        return;
    }

    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        LOGE("获取视频流信息失败");
        return;
    }
    LOGI("avformat_find_stream_info success");
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audioPlayer->audio_strem_index = i;
            LOGI("audio_strem_index success");
            audioPlayer->avCodecPar = avFormatContext->streams[i]->codecpar;
            LOGI("avCodecPar success");
            audioPlayer->sample_rate = avFormatContext->streams[i]->codecpar->sample_rate;
            LOGI("sample_rate success");
            audioPlayer->time_base = avFormatContext->streams[i]->time_base;
            LOGI("time_base success");
            audioPlayer->duration = avFormatContext->duration;
            LOGI("duration success");
            audioPlayer->avFormatContext = avFormatContext;
            LOGI("avFormatContext success");
            break;
        }
    }
    if (audioPlayer->audio_strem_index == -1) {
        LOGE("没有找到音频流");
        return;
    }
    LOGI("audio_strem_index:%d", audioPlayer->audio_strem_index);

    AVCodec *codec = avcodec_find_decoder(
            avFormatContext->streams[audioPlayer->audio_strem_index]->codecpar->codec_id);
    if (codec == NULL) {
        LOGE("没有找到解码器");
        return;
    }
    //换另外一种方式初始化avCodecContxt
    audioPlayer->avCodecContext = avcodec_alloc_context3(codec);
    if (audioPlayer->avCodecContext == NULL) {
        LOGE("can not fill decodecctx");
        return;
    }
    LOGI("avCodecContext success");

    //给avCodecContext赋值
    if (avcodec_parameters_to_context(audioPlayer->avCodecContext, audioPlayer->avCodecPar) < 0) {
        LOGE("could not fill avCodecContext");
        return;
    }
    LOGI("avcodec_parameters_to_context success");

    if (avcodec_open2(audioPlayer->avCodecContext, codec, NULL) < 0) {
        LOGE("could not open codec");
        return;
    }
    LOGI("avcodec_open2 success");
    java_call->onCallJavaPrepared(CHILD_THREAD);
    LOGI("onCallJavaPrepared success");
}

/**
 * 进行播放,然后这里是解码数据放到队列中
 */
void FfmpegPlayer::start() {
    audioPlayer->start();

}

void FfmpegPlayer::pause() {
    audioPlayer->pause();
}

void FfmpegPlayer::stop() {
//    audioPlayer->stop();
}

void FfmpegPlayer::release() {
    audioPlayer->release();
}

void FfmpegPlayer::setMute(int mute) {
    audioPlayer->setMute(mute);
}

void FfmpegPlayer::resume() {
    audioPlayer->resume();
}





