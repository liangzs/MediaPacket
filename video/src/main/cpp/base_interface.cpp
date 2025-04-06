


#include "base_interface.h"
#include "libavutil/avassert.h"

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
            avStreamVideoIn = avFormatContext->streams[i];

        } else if (avFormatContext->streams[i]->codecpar->codec_type ==
                   AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            avStreamAudioIn = avFormatContext->streams[i];
        }
    }

    const AVCodec *avCodec = avcodec_find_decoder(
            avFormatContext->streams[videoStreamIndex]->codecpar->codec_id);
    *codecContext = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(*codecContext,
                                  avFormatContext->streams[videoStreamIndex]->codecpar);
    int ret = avcodec_open2(*codecContext, avCodec, NULL);
    if (ret < 0) {
        LOGE("avcodec_open2 fail");
        return -1;
    }
    return 0;
}

int BaseInterface::getAudioDecodeContext(AVFormatContext *ps, AVCodecContext **dec_ctx) {
    int ret;
    audioStreamIndex = av_find_best_stream(ps, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

    //对引入入参进行赋值
    const AVCodec *avCodec = avcodec_find_decoder(
            ps->streams[audioStreamIndex]->codecpar->codec_id);
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
    videoPtsIndex = 0;
    audioPtsIndex = 0;
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

int BaseInterface::addOutputVideoStreamCopy(AVFormatContext *afc_output, AVCodecContext **vCtxE,
                                            AVFormatContext *afc_input) {
    avStreamVideoOut = avformat_new_stream(afc_output, NULL);
    //创建codecCxt，给codecParameter进行赋值
    AVOutputFormat *oformat = const_cast<AVOutputFormat *>(afc_output->oformat);

    if (oformat->video_codec == AVCodecID::AV_CODEC_ID_NONE) {
        LOGE("afc_output->oformat is AV_CODEC_ID_NONE");
        return -1;
    }
    const AVCodec *codec = avcodec_find_encoder(oformat->video_codec);
    if (codec == NULL) {
        LOGE("avcodec_find_encoder fail");
        return -1;
    }
    *vCtxE = avcodec_alloc_context3(codec);
    //给outVideoStreamIndex赋值
    int ret = avcodec_parameters_to_context(*vCtxE, afc_input->streams[videoStreamIndex]->codecpar);
    if (ret < 0) {
        LOGE("avcodec_parameters_to_context fail:%s", av_err2str(ret));
        return -1;
    }

//    (*vCtxE)->bit_rate = afc_input->bit_rate;
    (*vCtxE)->framerate = afc_input->streams[videoStreamIndex]->r_frame_rate;
    (*vCtxE)->time_base = afc_input->streams[videoStreamIndex]->time_base;
//    (*vCtxE)->pix_fmt = AV_PIX_FMT_YUV420P;
//    (*vCtxE)->codec_type = AVMEDIA_TYPE_VIDEO;
//    (*vCtxE)->width = codecpar.width;
//    (*vCtxE)->height = codecpar.height;
//    if ((*vCtxE)->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
//        (*vCtxE)->max_b_frames = 2;
//    }
//    if ((*vCtxE)->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
//        /* Needed to avoid using macroblocks in which some coeffs overflow.
//         * This does not happen with normal video, it just happens here as
//         * the motion of the chroma plane does not match the luma plane. */
//        (*vCtxE)->mb_decision = 2;
//    }
//    if ((*vCtxE)->codec_id == AV_CODEC_ID_H264)
//        av_opt_set((*vCtxE)->priv_data, "preset", "slow", 0);
    ret = avcodec_parameters_from_context(avStreamVideoOut->codecpar, *vCtxE);
    if (ret < 0) {
        LOGE("avcodec_parameters_from_context fail");
        return -1;
    }
    ret = avcodec_open2(*vCtxE, codec, NULL);
    if (ret < 0) {
        LOGE("addOutputVideoStream->avcodec_open2 fail");
        return -1;
    }
    LOGE(" init addOutputVideoStream success!");
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
    avStreamVideoOut = avformat_new_stream(afc_output, NULL);
    //创建codecCxt，给codecParameter进行赋值
    AVOutputFormat *oformat = const_cast<AVOutputFormat *>(afc_output->oformat);

    if (oformat->video_codec == AVCodecID::AV_CODEC_ID_NONE) {
        LOGE("afc_output->oformat is AV_CODEC_ID_NONE");
        return -1;
    }
    const AVCodec *codec = avcodec_find_encoder(oformat->video_codec);
    if (codec == NULL) {
        LOGE("avcodec_find_encoder fail");
        return -1;
    }
    //给outVideoStreamIndex赋值
    videoOutputStreamIndex = avStreamVideoOut->index;
    *vCtxE = avcodec_alloc_context3(codec);
    (*vCtxE)->bit_rate = 400000;
    (*vCtxE)->framerate = AVRational{outFrameRate, 1};
    (*vCtxE)->time_base = AVRational{1, outFrameRate};
    (*vCtxE)->gop_size = 100;
//    vCtxE->max_b_frames = 1;
    (*vCtxE)->pix_fmt = AV_PIX_FMT_YUV420P;
    (*vCtxE)->codec_type = AVMEDIA_TYPE_VIDEO;
    (*vCtxE)->width = codecpar.width;
    (*vCtxE)->height = codecpar.height;
    if ((*vCtxE)->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
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
    int ret = avcodec_parameters_from_context(avStreamVideoOut->codecpar, *vCtxE);
    if (ret < 0) {
        LOGE("avcodec_parameters_from_context fail");
        return -1;
    }
    ret = avcodec_open2(*vCtxE, codec, NULL);
    if (ret < 0) {
        LOGE("addOutputVideoStream->avcodec_open2 fail");
        return -1;
    }
    LOGE(" init addOutputVideoStream success!");
    return 0;
}

int BaseInterface::addOutputAudioStream(AVFormatContext *afc_output, AVCodecContext **aCtxE,
                                        AVCodecParameters codecpar) {
    AVStream *avStream = avformat_new_stream(afc_output, NULL);

    AVOutputFormat *oformat = const_cast<AVOutputFormat *>(afc_output->oformat);
    if (oformat->audio_codec == AVCodecID::AV_CODEC_ID_NONE) {
        LOGE("audio safc_output->oformat is AV_CODEC_ID_NONE");
        return -1;
    }
    AVCodec *avCodec = const_cast<AVCodec *>(avcodec_find_decoder(oformat->audio_codec));
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
    return 0;
}

int BaseInterface::writeOutoutHeader(AVFormatContext *avFormatContextOut, const char *outPath) {
    int ret = 0;
    //检测打开输出文件，然后检测输入头文件，然后输入文件，最后输入尾文件
    if (!(avFormatContextOut->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&avFormatContextOut->pb, outPath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("avio_open fail %s", av_err2str(ret));
            return -1;
        }
    }
    //写入文件头
    ret = avformat_write_header(avFormatContextOut, NULL);
    if (ret < 0) {
        LOGE("avformat_write_header fail  %s", av_err2str(ret));
        return -1;
    }
    return ret;
}

int BaseInterface::getVideoOutputStreamIndex() {
    return videoOutputStreamIndex;
}

int BaseInterface::getAudioOutputStreamIndex() {
    return audioOutputStreamIndex;
}

int BaseInterface::writeTrail(AVFormatContext *afc_output) {
    //写入尾部
    return av_write_trailer(afc_output);
}

AVFrame *BaseInterface::decodePacket(AVCodecContext *decode, AVPacket *packet) {
    int ret = avcodec_send_packet(decode, packet);
    if (ret < 0) {
        LOGE("avcodec_send_packet fail:%s", av_err2str(ret));
        return NULL;
    }
    AVFrame *avFrame = av_frame_alloc();
//    while (1) {
//        ret = avcodec_receive_frame(decode, avFrame);
//        if (ret == AVERROR(EAGAIN)) {
//            av_assert0(packet); // should never happen during flushing
//            return avFrame;
//        } else if (ret == AVERROR_EOF) {
//            return avFrame;
//        } else if (ret < 0) {
//            LOGE("Decoding error: %s\n", av_err2str(ret));
//            continue;
//        }
//    }
    ret = avcodec_receive_frame(decode, avFrame);
    if (ret < 0) {
        av_frame_free(&avFrame);
        LOGE("avcodec_receive_frame error: %s", av_err2str(ret));
        return NULL;
    }
    return avFrame;
}

AVPacket *BaseInterface::encodeFrame(AVFrame *frame, AVCodecContext *codecContext) {
    int ret = avcodec_send_frame(codecContext, frame);
    if (ret < 0) {
        LOGE("avcodec_send_frame fail:%s", av_err2str(ret));
        return NULL;
    }
    AVPacket *avPacket = av_packet_alloc();
    ret = avcodec_receive_packet(codecContext, avPacket);
    if (ret < 0) {
        av_packet_free(&avPacket);
        LOGE("avcodec_receive_packet fail:%s", av_err2str(ret));
        return NULL;
    }
    return avPacket;
}

int BaseInterface::getVideoOutFrameRate() {
    return 0;
}




