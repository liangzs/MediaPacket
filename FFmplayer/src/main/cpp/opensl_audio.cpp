//
// Created by DELLQ on 24/6/2024.
//

#include "opensl_audio.h"

int OpenSlAudio::getCurrentSampleRateForOpensles(int sample_rate) {
    int rate = 0;
    switch (sample_rate) {
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate = SL_SAMPLINGRATE_44_1;
    }
    return rate;
}

/**
 * ffmpeg解码音频，然后把解码后的frame对应的buffer，传到opensl中的queue中，opensl会遍历这个queue
 * @param bufQueueItf
 * @param context
 */
void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bufQueueItf, void *context) {

    OpenSlAudio *audioPlayer = (OpenSlAudio *) context;
    if (audioPlayer == NULL && audioPlayer->status->isPlaying()) {
        int bufferSize = audioPlayer->decodePacket();
        if (bufferSize > 0) {
            //这里是真正的传送数据进行播放，把swr_cevert后的outbuffer数据推送到audioPlayer->queue中
            //使用opensl 对象的时候，每次都需要强转成*指针对象才能用
            (*audioPlayer->pcmplayer_bufferqueue)->Enqueue(audioPlayer->pcmplayer_bufferqueue,
                                                           audioPlayer->outBuffer, bufferSize);
            LOGI("audioPlayer->clock = %f", audioPlayer->clock);
            //设置回调的频率
            if (audioPlayer->clock - audioPlayer->last_time > 1000) {
                audioPlayer->last_time = audioPlayer->clock;
                //进行java层的回调
                audioPlayer->playerJavaCall->onProgress(audioPlayer->clock);
            }

        }
        return;
    }
}


void OpenSlAudio::initOpenSl() {
    /** 初始化引擎，三步骤
    创建引擎对象：首先通过 slCreateEngine 创建引擎对象。
    实现引擎对象：然后通过 Realize 函数将引擎对象从未实现状态转换为已实实例化状态。
    获取引擎接口：最后，通过 GetInterface 函数获取引擎接口，以便调用引擎的功能。
     *
     */

    SLresult sLresult;
    sLresult = slCreateEngine(&engine_object, 0, 0, 0, 0, 0);
    (*engine_object)->Realize(engine_object, SL_BOOLEAN_FALSE);
    (*engine_object)->GetInterface(engine_object, SL_IID_ENGINE, &engine_engine);

    //利用引擎初始化混音器，多数组和长度要对应
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    (*engine_engine)->CreateOutputMix(engine_engine, &mix_object, 1, mids, mreq);
    //实例化成实实例状态
    (*mix_object)->Realize(mix_object, SL_BOOLEAN_FALSE);
    //获取接口
    sLresult = (*mix_object)->GetInterface(mix_object, SL_IID_ENVIRONMENTALREVERB,
                                           &slEnvironmentalReverbItf);
    if (sLresult == SL_RESULT_SUCCESS) {
        (*slEnvironmentalReverbItf)->SetEnvironmentalReverbProperties(slEnvironmentalReverbItf,
                                                                      &slEnvironmentalReverbSettings);
    }

    // 第三步，配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};

    /**
     * 通过这个设置可以设置左声道和右声道的播放
     */
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            2,//2个声道（立体声）
            static_cast<SLuint32>(getCurrentSampleRateForOpensles(sample_rate)),//44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    //数据源
    SLDataSource slDataSource = {&android_queue, &pcm};

    //
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mix_object};
    SLDataSink slDataSink = {&outputMix, 0};

    //涉及到的接口
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_MUTESOLO};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    //创建播放器,利用混音器，创建播放器
    (*engine_engine)->CreateAudioPlayer(engine_engine, &pcmplayer_object, &slDataSource,
                                        &slDataSink, 3, ids, req);
    //初始化
    (*pcmplayer_object)->Realize(pcmplayer_object, SL_BOOLEAN_FALSE);

    //获取播放接口接口
    (*pcmplayer_object)->GetInterface(pcmplayer_object, SL_IID_PLAY, &pcmplayer_player);
    //声道控制接口
    (*pcmplayer_object)->GetInterface(pcmplayer_object, SL_IID_MUTESOLO, &pcmplayer_mutesolo);
    //音量控制接口
    (*pcmplayer_object)->GetInterface(pcmplayer_object, SL_IID_VOLUME, &pcmplayer_volume);
    //缓存队列
    (*pcmplayer_object)->GetInterface(pcmplayer_object, SL_IID_BUFFERQUEUE, &pcmplayer_bufferqueue);
    //设置队列回调
    (*pcmplayer_bufferqueue)->RegisterCallback(pcmplayer_bufferqueue, pcmBufferCallBack, this);

    //设置正在播放状态
    (*pcmplayer_player)->SetPlayState(pcmplayer_player, SL_PLAYSTATE_PLAYING);

    //刷新第一帧
    pcmBufferCallBack(pcmplayer_bufferqueue, this);

}


void OpenSlAudio::release() {

}

/**
 * cpp中的c方法，然后通过*data把this传递进去引用cpp的本身方法
 * @param data
 * @return
 */
void *openslDecode(void *data) {
    OpenSlAudio *audioPlayer = static_cast<OpenSlAudio *>(data);

    //是时候opengles
    audioPlayer->initOpenSl();

    pthread_exit(&audioPlayer->pthread_docode);
}

/**
 * 创建子线程进行解码
 */
void OpenSlAudio::start() {
    pthread_create(&pthread_docode, NULL, openslDecode, this);
}


OpenSlAudio::~OpenSlAudio() {

}

OpenSlAudio::OpenSlAudio(PlayerStatus *status, PlayerJavaCall *playerJavaCall1) {
    this->status = status;
    this->playerJavaCall = playerJavaCall1;
    this->queue = new PlayerQueue();
    outBuffer = static_cast<uint8_t *>(av_malloc(SAMPLE_RATE * 2 * 2));
}

void recyclePacket(AVPacket *packet) {
//    av_free_packet(packet);
    av_free(packet);
    packet = NULL;
}

void recycleFrame(AVFrame *frame) {
    av_frame_free(&frame);
    av_free(frame);
    frame = NULL;
}

/**
 * 循环遍历
 * @return
 */
int OpenSlAudio::decodePacket() {
    int bufferSize;
    while (status->isPlaying() && queue->getQueueSize() > 0) {
        avPacket = av_packet_alloc();
        queue->getPacket(avPacket);
        int ret = avcodec_send_packet(avCodecContext, avPacket);
        if (ret != 0) {
            recyclePacket(avPacket);
            continue;
        }
        avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret != 0) {
            recyclePacket(avPacket);
            recycleFrame(avFrame);
            continue;
        }
        //out_sample_rate=44100*AV_SAMPLE_FMT_S16*AV_CH_LAYOUT_STEREO====44100 * 2 * 2
        const int size = 176400;
        if (swrContext == NULL) {
            swrContext = swr_alloc();
            //定义输出配置
            int out_ch_layout = AV_CH_LAYOUT_STEREO;//立体声
            AVSampleFormat out_format = AVSampleFormat::AV_SAMPLE_FMT_S16;//16位
            int out_sample_rate = 44100;//采样率
            swr_alloc_set_opts(swrContext, out_ch_layout, out_format, out_sample_rate,
                               avFrame->channel_layout,
                               static_cast<AVSampleFormat>(avFrame->format), avFrame->sample_rate,
                               0, 0);

            //配置好参数之后需要初始化
            swr_init(swrContext);
        }
        //这里返回的是重采样后的样本数，即具体是数据
        bufferSize = swr_convert(swrContext, &outBuffer, size,
                                 reinterpret_cast<const uint8_t **>(&avFrame->data),
                                 avFrame->nb_samples);
        //时间的计算是time_base * pts，因为数据传输时为了精简，时间戳不会传很长
        time_now = avFrame->pts * av_q2d(time_base);
        if (time_now < clock) {
            time_now = clock;
        }
        clock = time_now;
        recycleFrame(avFrame);
        recyclePacket(avPacket);
    }
    return bufferSize;
}





