//
// Created by DELLQ on 24/6/2024.
//

#include "Ffmpeg_player.h"

FfmpegPlayer::FfmpegPlayer(PlayerStatus *status, const char *path) {
    this->status = status;
    this->java_call = new PlayerJavaCall();
    this->char_path = path;
    if (audioPlayer == NULL) {
        audioPlayer = new OpenSlAudio();
    }
}

FfmpegPlayer::~FfmpegPlayer() {
    delete java_call;
    delete audioPlayer;
}

void *extendCDecode(void *data) {
    FfmpegPlayer *player = static_cast<FfmpegPlayer *>(data);
    player->decodeThread();
    //销毁线程
    pthread_exit(&player->pthread_decode);
    return 0;
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
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audioPlayer->audio_strem_index = i;
            audioPlayer->avCodecPar = avFormatContext->streams[i]->codecpar;
            avFormatContext->streams[i]->codec;
            break;
        }
    }
    if (audioPlayer->audio_strem_index == -1) {
        LOGE("没有找到音频流");
        return;
    }
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
    //给avCodecContext赋值
    if(avcodec_parameters_to_context(audioPlayer->avCodecContext, audioPlayer->avCodecPar)<0){
        LOGE("could not fill avCodecContext");
        return;
    }

    if(avcodec_open2(audioPlayer->avCodecContext,codec,NULL)<0){
        LOGE("could not open codec");
        return;
    }
    java_call->onPrepared();
}

/**
 * 进行播放
 */
void FfmpegPlayer::start() {
        audioPlayer.sta
}




