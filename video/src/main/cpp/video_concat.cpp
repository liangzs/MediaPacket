//
// Created by DELLQ on 16/7/2024.
//

#include "video_concat.h"


VideoConcat::VideoConcat(std::vector<char *> inputPaths, char *outputPath, int width, int height) {
    this->outWidth = width;
    this->outHeight = height;
    this->outPath = static_cast<char *>(malloc(strlen(outputPath) + 1));
    strcpy(this->outPath, outputPath);
    this->inputPaths = inputPaths;

    //音频参数初始化
    avSampleFormat = AVSampleFormat::AV_SAMPLE_FMT_S16;
    sampleRate = 44100;
    channelLayout = AV_CH_LAYOUT_STEREO;
    channels = av_get_channel_layout_nb_channels(channelLayout);
}

VideoConcat::~VideoConcat() {
    free(this->outPath);
    for (int i = 0; i < this->inputPaths.size(); i++) {
        free(this->inputPaths[i]);
    }
    inputPaths.clear();
}

int VideoConcat::startConcat() {
    this->start();//执行run，再run中进行解码
    if (initOuput() < 0) {
        LOGE("initOuput error");
        return -1;
    }

    return 1;
}

/**
 * 要初始化sws和swr
 * @param inputPath
 * @return
 */
int VideoConcat::initInput(char *inputPath) {
    int ret = avformat_open_input(&inFormatContext, inputPath, NULL, NULL);
    if (ret < 0) {
        LOGE("avformat_open_input error");
        return -1;
    }
    ret = avformat_find_stream_info(inFormatContext, NULL);
    if (ret < 0) {
        LOGE("avformat_find_stream_info error");
        return -1;
    }
    //查询信道
    for (int i = 0; i < inFormatContext->nb_streams; ++i) {
        if (inFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            inVideoStreamIndex = i;
            //宽高等其他信息赋值
            inWidth = inFormatContext->streams[i]->codecpar->width;
            inHeight = inFormatContext->streams[i]->codecpar->height;
            inVideoCodec = avcodec_find_decoder(inFormatContext->streams[i]->codecpar->codec_id);
        } else if (inFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            inAudioStreamIndex = i;
            inAudioCodec = avcodec_find_decoder(inFormatContext->streams[i]->codecpar->codec_id);

        }
    }
    //video
    inVideoCodecContext = avcodec_alloc_context3(inVideoCodec);
    ret = avcodec_parameters_to_context(inVideoCodecContext,
                                        inFormatContext->streams[inVideoStreamIndex]->codecpar);
    if (ret < 0) {
        LOGE("avcodec_parameters_to_context error");
        return -1;
    }
    //打开解码器
    ret = avcodec_open2(inVideoCodecContext, inVideoCodec, NULL);
    if (ret < 0) {
        LOGE("avcodec_open2 error");
        return -1;
    }
    //video sws
    releaseSwsContext();
    if (initSwsContext() < 0) {
        LOGE("initSwsContext error");
        return -1;
    }

    //audio
    inAudioCodecContext = avcodec_alloc_context3(inAudioCodec);
    if (inAudioCodecContext == NULL) {
        LOGE("avcodec_alloc_context3 error");
        return -1;
    }
    ret = avcodec_parameters_to_context(inAudioCodecContext,
                                        inFormatContext->streams[inAudioStreamIndex]->codecpar);
    if (ret < 0) {
        LOGE("inAudioCodecContext->avcodec_parameters_to_context error");
        return -1;
    }
    ret = avcodec_open2(inAudioCodecContext, inAudioCodec, NULL);
    if (ret < 0) {
        LOGE("inAudioCodecContext->avcodec_open2 error");
        return -1;
    }
    //audio swr
    releaseSwrContext();
    if (initSwrContext() < 0) {
        LOGE("initSwrContext error");
        return -1;
    }
    return 0;
}

/**
 * 这里再decode里执行，循环inputPath,每个文件都需要进行检测
 * @return
 */
int VideoConcat::initOuput() {
    //检测文件，写入头文件
    int ret = avformat_alloc_output_context2(&outFormatContext, NULL, NULL, outPath);
    if (ret < 0) {
        LOGE("avformat_alloc_output_context2 error");
        return -1;
    }
    //adVideoStream,addAudioStream
    if (addVideoStream() < 0) {
        LOGE("addVideoStream error");
        return -1;
    }
    if (addAudioStream() < 0) {
        LOGE("addAudioStream error");
        return -1;
    }
//检测文件写入是否成功
    if (!(oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outFormatContext->pb, outPath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("avio_open error");
            return -1;
        }
    }
//写入头文件
    ret = avformat_write_header(outFormatContext, NULL);
    if (ret < 0) {
        LOGE("avformat_write_header error");
        return -1;
    }

    return 0;
}

void VideoConcat::run() {
    for (int i = 0; i < inputPaths.size(); ++i) {
        char *inputPath = inputPaths[i];
        //先释放，再创建
        releaseInput();
        initInput(inputPath);

        //开始解码
        startDecode();

    }
}

int VideoConcat::addVideoStream() {
    outVideoStream = avformat_new_stream(outFormatContext, NULL);
    if (outVideoStream == NULL) {
        LOGE("avformat_new_stream error");
        return -1;
    }
    oformat = outFormatContext->oformat;
    if (oformat->video_codec == AV_CODEC_ID_NONE) {
        LOGE("oformat->video_codec==AV_CODEC_ID_NONE");
        return -1;
    }
    outVideoCodec = avcodec_find_encoder(oformat->video_codec);
    if (outVideoCodec == NULL) {
        LOGE("avcodec_find_encoder error");
        return -1;
    }
    outVideoCodecContext = avcodec_alloc_context3(outVideoCodec);
    if (outVideoCodecContext == NULL) {
        LOGE("avcodec_alloc_context3 error");
        return -1;
    }
    outVideoCodecContext->width = outWidth;
    outVideoCodecContext->height = outHeight;
    outVideoCodecContext->framerate = AVRational{25, 1};
    outVideoCodecContext->time_base = AVRational{1, 25};
    outVideoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    if (avcodec_parameters_from_context(outVideoStream->codecpar, outVideoCodecContext) < 0) {
        LOGE("addVideoStream->avcodec_parameters_from_context error");
        return -1;
    }
    //打开编码
    if (avcodec_open2(outVideoCodecContext, outVideoCodec, NULL) < 0) {
        LOGE("addVideoStream->avcodec_open2 error");
        return -1;
    }
    return 0;
}

int VideoConcat::addAudioStream() {
    outAudioStream = avformat_new_stream(outFormatContext, NULL);
    if (outAudioStream == NULL) {
        LOGE("avformat_new_stream error");
        return -1;
    }
    if (oformat->audio_codec == AV_CODEC_ID_NONE) {
        LOGE("oforamt->audio_codec==AV_CODEC_ID_NONE");
        return -1;
    }
    outAudioCodec = avcodec_find_decoder(oformat->audio_codec);
    if (outAudioCodec == NULL) {
        LOGE("avcodec_find_decoder error");
        return -1;
    }
    outAudioCodecContext = avcodec_alloc_context3(outAudioCodec);
    if (outAudioCodecContext == NULL) {
        LOGE("avcodec_alloc_context3 error");
        return -1;
    }
    outAudioCodecContext->sample_fmt = avSampleFormat;
    outAudioCodecContext->sample_rate = sampleRate;
    outAudioCodecContext->time_base = AVRational{1, sampleRate};
    outAudioCodecContext->channel_layout = channelLayout;
    //av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO)其实就是2
    outAudioCodecContext->channels = channels;
    outAudioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
    if (avcodec_parameters_from_context(outAudioStream->codecpar, outAudioCodecContext) < 0) {
        LOGE("addAudioStream->avcodec_parameters_from_context error");
        return -1;
    }
    if (avcodec_open2(outAudioCodecContext, outAudioCodec, NULL) < 0) {
        LOGE("addAudioStream->avcodec_open2 error");
        return -1;
    }

    return 0;
}

void VideoConcat::releaseInput() {
    if (inFormatContext != NULL) {
        avformat_free_context(inFormatContext);
        inFormatContext = NULL;
    }
    if (inVideoCodecContext != NULL) {
        avcodec_free_context(&inVideoCodecContext);
        inVideoCodecContext = NULL;
    }
    if (inAudioCodecContext != NULL) {
        avcodec_free_context(&inAudioCodecContext);
    }

}

/**
 * video sws,如果是要再android屏幕显示，则需要重现转成rgb格式，然后用新的buffer取承载数据
 * eg: uint8_t *outBuffer = static_cast<uint8_t *>(av_malloc(sizeof(uint8_t) * numbytes));
 * av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, outBuffer,
 *                           AVPixelFormat::AV_PIX_FMT_RGBA, avCodecContext->width,
 *                            avCodecContext->height, 1);
 * @return
 */
int VideoConcat::initSwrContext() {

    inSwsContext = sws_getContext(inWidth, inHeight, inVideoCodecContext->pix_fmt, outWidth,
                                  outHeight, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL,
                                  NULL);
    if (inSwsContext == NULL) {
        LOGE("sws_getContext NULL");
        return -1;
    }
    return 0;
}

/**
 * audio的swr过程为alloc，set_option init ,最后使用convert
 * @return
 */
int VideoConcat::initSwsContext() {
    inSwrContext = swr_alloc();
    if (inSwrContext == NULL) {
        LOGE("swr_alloc error");
        return -1;
    }
    AVCodecParameters *parameters = inFormatContext->streams[inAudioStreamIndex]->codecpar;
    inSwrContext = swr_alloc_set_opts(inSwrContext,
                                      av_get_default_channel_layout(channels), avSampleFormat,
                                      sampleRate,
                                      av_get_default_channel_layout(parameters->channels),
                                      static_cast<AVSampleFormat>(parameters->format),
                                      parameters->sample_rate, 0,
                                      0);
    if (inSwrContext == NULL) {
        LOGE("swr_alloc_set_opts error");
        return -1;
    }
    if (swr_init(inSwrContext) < 0) {
        LOGE("swr_init error");
        return -1;
    }
    return 0;
}

void VideoConcat::releaseSwsContext() {
    if (inSwsContext != NULL) {
        sws_freeContext(inSwsContext);
        inSwsContext = NULL;
    }
}

void VideoConcat::releaseSwrContext() {
    swr_free(&inSwrContext);
    inSwrContext = NULL;
}

int VideoConcat::startDecode() {

}
