//
// Created by Administrator on 2024/7/25.
//

#include "base_interface.h"

int BaseInterface::open_input_file(const char *filename, AVFormatContext **avformatCtx) {
    int ret = avformat_open_input(avformatCtx, filename, NULL, NULL);
    if (ret < 0) {
        LOGE("avformat_open_input fail:%s", filename);
        return ret;
    }
    if (avformatCtx == NULL) {
        LOGE("alloc AVFormatContext fail");
        return -1;
    }
    //寻找编码
    ret = avformat_find_stream_info(*avformatCtx, NULL);
    if (ret < 0) {
        LOGE("avformat_find_stream_info fail:%s", filename);
        return ret;
    }
    return 1;
}

int BaseInterface::getVideoStreamIndex(AVFormatContext *fmt_ctx) {
    if (videoStreamIndex == -1) {
        videoStreamIndex = av_find_best_stream(fmt_ctx, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1,
                                               NULL,
                                               0);
    }
    return videoStreamIndex;
}

int BaseInterface::getAudioStreamIndex(AVFormatContext *fmt_ctx) {
    if (audioStreamIndex == -1) {
        audioStreamIndex = av_find_best_stream(fmt_ctx, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1,
                                               NULL, 0);
    }
    return audioStreamIndex;
}

/**
 * 对codecContex赋值
 * @param avFormatContext
 * @param dec_ctx
 * @return
 */
int BaseInterface::getVideoDecodeContext(AVFormatContext *avFormatContext,
                                         AVCodecContext **codecContext) {
    //传统做法遍历streamindex获得videostreamindex
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;

        } else if (avFormatContext->streams[i]->codecpar->codec_type ==
                   AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
        }
    }
    //直接通过bestIndex方法直接获取index
//    videoStreamIndex = av_find_best_stream(avFormatContext, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1,
//                                           NULL, 0);
//    audioStreamIndex = av_find_best_stream(avFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1,
//                                           NULL, 0);

    AVCodec *avCodec;
    *codecContext = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(*codecContext,
                                  avFormatContext->streams[videoStreamIndex]->codecpar);
    int ret = avcodec_open2(*codecContext, avCodec, NULL);
    if (ret < 0) {
        LOGE("avcodec_open2 fail");
        return -1;
    }

    return audioStreamIndex;
}

int BaseInterface::getAudioDecodeContext(AVFormatContext *ps, AVCodecContext **dec_ctx) {
    int ret;
    audioStreamIndex = av_find_best_stream(ps, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

    //对引入入参进行赋值
    AVCodec *avCodec = NULL;
    *dec_ctx = avcodec_alloc_context3(avCodec);

    avcodec_parameters_to_context(*dec_ctx, ps->streams[audioStreamIndex]->codecpar);

    //打开编码
    ret = avcodec_open2(*dec_ctx, avCodec, NULL);
    if (ret < 0) {
        LOGE("avcodec_open2 fail");
        return -1;
    }

    return audioStreamIndex;
}

BaseInterface::BaseInterface() {
    videoStreamIndex = -1;
    audioStreamIndex = -1;
    videoOutputStreamIndex = -1;
    audioOutputStreamIndex = -1;
    progress = 0;
    outFrameRate = 25;
    timeBaseFFmpeg = (AVRational) {1, AV_TIME_BASE};
}

BaseInterface::~BaseInterface() {

}

int BaseInterface::initOutput(const char *ouput, AVFormatContext **ctx) {
    int ret = avformat_alloc_output_context2(ctx, NULL, NULL, ouput);
    if (ret < 0) {
        LOGE("avformat_alloc_output_context2 fail");
        return -1;
    }
    return 0;
}

int BaseInterface::initOutput(const char *ouput, const char *format, AVFormatContext **ctx) {
    int ret = avformat_alloc_output_context2(ctx, NULL, format, ouput);
    if (ret < 0) {
        LOGE("avformat_alloc_output_context2 fail");
        return -1;
    }
    return 0;
}

/**
 * 添加视频的信道信息
 * @param afc_output
 * @param vCtxE
 * @param codecpar
 * @return
 */
int BaseInterface::addOutputVideoStream(AVFormatContext *afc_output, AVCodecContext **vCtxE,
                                        AVCodecParameters codecpar) {
    AVStream *avStream = avformat_new_stream(afc_output, NULL);
    //创建codecCxt，给codecParameter进行赋值
    AVOutputFormat *oformat = afc_output->oformat;

    if (oformat->video_codec == AVCodecID::AV_CODEC_ID_NONE) {
        LOGE("afc_output->oformat is AV_CODEC_ID_NONE");
        return -1;
    }
    AVCodec *codec = avcodec_find_encoder(oformat->video_codec);
    if (codec == NULL) {
        LOGE("avcodec_find_encoder fail");
        return -1;
    }
    //给outVideoStreamIndex赋值
    videoOutputStreamIndex = avStream->index;
    *vCtxE = avcodec_alloc_context3(codec);
    (*vCtxE)->bit_rate = outFrameRate * codecpar.width * codecpar.height * 3 / 2;
    (*vCtxE)->framerate = AVRational{outFrameRate, 1};
    (*vCtxE)->time_base = AVRational{1, outFrameRate};
    (*vCtxE)->gop_size = 100;
//    vCtxE->max_b_frames = 1;
    (*vCtxE)->pix_fmt = (AVPixelFormat) codecpar.format;
    (*vCtxE)->codec_type = AVMEDIA_TYPE_VIDEO;
    (*vCtxE)->width = codecpar.width;
    (*vCtxE)->height = codecpar.height;
    if ((*vCtxE)->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        (*vCtxE)->max_b_frames = 2;
    }
    if ((*vCtxE)->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        (*vCtxE)->mb_decision = 2;
    }
    if ((*vCtxE)->codec_id == AV_CODEC_ID_H264)
        av_opt_set((*vCtxE)->priv_data, "preset", "slow", 0);

    int ret = avcodec_parameters_from_context(avStream->codecpar, *vCtxE);
    if (ret < 0) {
        LOGE("avcodec_parameters_from_context fail");
        return -1;
    }
    ret = avcodec_open2(*vCtxE, codec, NULL);
    if (ret < 0) {
        LOGE("avcodec_open2 fail");
        return -1;
    }
    LOGE(" init addOutputVideoStream success!");
    return videoOutputStreamIndex;
}

int BaseInterface::addOutputAudioStream(AVFormatContext *afc_output, AVCodecContext **aCtxE,
                                        AVCodecParameters codecpar) {
    AVStream *avStream = avformat_new_stream(afc_output, NULL);

    AVOutputFormat *oformat = afc_output->oformat;
    if (oformat->audio_codec == AVCodecID::AV_CODEC_ID_NONE) {
        LOGE("audio safc_output->oformat is AV_CODEC_ID_NONE");
        return -1;
    }
    AVCodec *avCodec = avcodec_find_decoder(oformat->audio_codec);
    if (avCodec == NULL) {
        LOGE("audio avcodec_find_decoder fail ");
        return -1;
    }
    *aCtxE = avcodec_alloc_context3(avCodec);
    //直接复用 输入的AVCodecParameters 参数
    (*aCtxE)->bit_rate = 64000;
    (*aCtxE)->sample_fmt = (AVSampleFormat) codecpar.format;
    (*aCtxE)->sample_rate = codecpar.sample_rate;
    (*aCtxE)->channel_layout = codecpar.channel_layout;
    (*aCtxE)->channels = codecpar.channels;
    (*aCtxE)->time_base = (AVRational) {1, codecpar.sample_rate};
    (*aCtxE)->codec_type = AVMEDIA_TYPE_AUDIO;
    avStream->time_base = (AVRational) {1, codecpar.sample_rate};

    int ret = avcodec_parameters_from_context(avStream->codecpar, *aCtxE);
    if (ret < 0) {
        LOGE(" avcodec_parameters_from_context FAILD ! ");
        return -1;
    }
    ret = avcodec_open2(*aCtxE, avCodec, NULL);
    if (ret < 0) {
        LOGE(" audio Could not open codec %s ", av_err2str(ret));
        return -1;
    }

    LOGE(" init output success audio!");
    return audioOutputStreamIndex;
}

int BaseInterface::getVideoOutFrameRate() {
    return outFrameRate;
}

/**
 * 编码
 * @param frame
 * @param codecContext
 * @return
 */
AVPacket *BaseInterface::encodeFrame(AVFrame *frame, AVCodecContext *codecContext) {
    int ret = avcodec_send_frame(codecContext, frame);
    if (ret < 0) {
        LOGE("encodeFrame->avcodec_send_frame fail");
        return NULL;
    }
    AVPacket *avPacket = av_packet_alloc();
    ret = avcodec_receive_packet(codecContext, avPacket);
    if (ret < 0) {
        LOGE("encodeFrame->avcodec_receive_packet fail");
        return NULL;
    }
    return avPacket;
}

/**
 * 解码，得到avframe的yuv数据
 * @param decode
 * @param packet
 * @return
 */
AVFrame *BaseInterface::decodePacket(AVCodecContext *decode, AVPacket *packet) {
    AVFrame *frame = av_frame_alloc();
    int ret = avcodec_send_packet(decode, packet);
    if (ret < 0) {
        LOGE("decodePacket->avcodec_send_packet fail");
        return NULL;
    }
    ret = avcodec_receive_frame(decode, frame);
    if (ret < 0) {
        LOGE("decodePacket->avcodec_receive_frame fail");
        return NULL;
    }
    return frame;
}

